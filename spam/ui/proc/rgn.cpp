#include "rgn.h"
#include "basic.h"
#include <limits>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#ifdef free
#undef free
#endif
#include <tbb/tbb.h>
#include <tbb/scalable_allocator.h>
#include <stack>
#pragma warning( push )
#pragma warning( disable : 4819 4003 4267)
#include <2geom/path-sink.h>
#include <2geom/path-intersection.h>
#pragma warning( pop )

namespace
{
    constexpr int g_numRgnColors = 12;
    static int g_color_index_s = 0;
    static const uint32_t g_rgnColors[g_numRgnColors] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF,
        0xFF800000, 0xFFFFFF00, 0xFF808000, 0xFF008000, 0xFF00FFFF, 0xFF008080, 0xFF000080, 0xFFFF00FF, 0xFF800080 };
};

class AdjacencyBuilderTBB
{
public:
    AdjacencyBuilderTBB(const SpamRunList &allRuns, const RowRangeList &rowRanges, AdjacencyList &adjacencyList)
        : allRuns_(allRuns), rowRanges_(rowRanges), adjacencyList_(adjacencyList) {}

public:
    void operator()(const tbb::blocked_range<int> &r) const
    {
        int numRows = r.end() - r.begin();
        if (1 < numRows)
        {
            const int safeBeg = r.begin();
            const int safeEnd = r.end() - 1;
            for (auto row = safeBeg; row != safeEnd; ++row)
            {
                const RowRange &rowRange = rowRanges_[row];
                const RowRange &nextRowRange = rowRanges_[row + 1];

                if ((rowRange.row + 1) == nextRowRange.row)
                {
                    int nextBegIdx = nextRowRange.beg;
                    const int nextEndIdx = nextRowRange.end;
                    for (int runIdx = rowRange.beg; runIdx != rowRange.end; ++runIdx)
                    {
                        bool metInter = false;
                        for (int nextRunIdx = nextBegIdx; nextRunIdx != nextEndIdx; ++nextRunIdx)
                        {
                            if (IsRunColumnIntersection(allRuns_[runIdx], allRuns_[nextRunIdx]))
                            {
                                metInter = true;
                                nextBegIdx = nextRunIdx;
                                adjacencyList_[runIdx].push_back(nextRunIdx);
                                adjacencyList_[nextRunIdx].push_back(runIdx);
                            }
                            else if (metInter)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (numRows)
        {
            processFirstRow(r.begin());
            processLastRow(r.end() - 1);
        }
    }

private:
    void processFirstRow(const int firstRow) const
    {
        const RowRange &rowRange = rowRanges_[firstRow];
        const RowRange &prevRowRange = rowRanges_[firstRow - 1];
        if ((rowRange.row - 1) == prevRowRange.row)
        {
            int prevBegIdx = prevRowRange.beg;
            const int prevEndIdx = prevRowRange.end;
            for (int runIdx = rowRange.beg; runIdx != rowRange.end; ++runIdx)
            {
                bool metInter = false;
                for (int prevRunIdx = prevBegIdx; prevRunIdx != prevEndIdx; ++prevRunIdx)
                {
                    if (IsRunColumnIntersection(allRuns_[runIdx], allRuns_[prevRunIdx]))
                    {
                        metInter = true;
                        prevBegIdx = prevRunIdx;
                        adjacencyList_[runIdx].push_back(prevRunIdx);
                    }
                    else if (metInter)
                    {
                        break;
                    }
                }
            }
        }
    }

    void processLastRow(const int lastRow) const
    {
        const RowRange &rowRange = rowRanges_[lastRow];
        const RowRange &nextRowRange = rowRanges_[lastRow + 1];
        if ((rowRange.row + 1) == nextRowRange.row)
        {
            int nextBegIdx = nextRowRange.beg;
            const int nextEndIdx = nextRowRange.end;
            for (int runIdx = rowRange.beg; runIdx != rowRange.end; ++runIdx)
            {
                bool metInter = false;
                for (int nextRunIdx = nextBegIdx; nextRunIdx != nextEndIdx; ++nextRunIdx)
                {
                    if (IsRunColumnIntersection(allRuns_[runIdx], allRuns_[nextRunIdx]))
                    {
                        metInter = true;
                        nextBegIdx = nextRunIdx;
                        adjacencyList_[runIdx].push_back(nextRunIdx);
                    }
                    else if (metInter)
                    {
                        break;
                    }
                }
            }
        }
    }

private:
    const SpamRunList &allRuns_;
    const RowRangeList &rowRanges_;
    AdjacencyList &adjacencyList_;
};

class RegionConnectorPI
{
public:
    RegionConnectorPI(){}

private:

};

class RunLengthEncoder
{
public:
    RunLengthEncoder(const cv::Mat &binaryImage)
        : firstPixel_(binaryImage.data)
        , stride_(binaryImage.step1())
        , width_(binaryImage.cols)
    {}

    RunLengthEncoder(RunLengthEncoder& x, tbb::split)
        : firstPixel_(x.firstPixel_)
        , stride_(x.stride_)
        , width_(x.width_)
    {}

    void operator()(const tbb::blocked_range<int16_t>& br)
    {
        constexpr int16_t left = 0;
        const int16_t right = width_;
        const int16_t end = br.end();
        for (int16_t r = br.begin(); r != end; ++r)
        {
            int16_t cb = -1;
            const uchar* pRow = firstPixel_ + r * stride_;
            for (int16_t c = left; c < right; ++c)
            {
                if (pRow[c])
                {
                    if (cb < 0)
                    {
                        cb = c;
                    }
                }
                else
                {
                    if (cb > -1)
                    {
                        runs_.emplace_back(r, cb, c );
                        cb = -1;
                    }
                }
            }

            if (cb > -1)
            {
                runs_.emplace_back(r, cb, right);
            }
        }
    }

    void join(const RunLengthEncoder& y) { runs_.insert(runs_.begin(), y.runs_.cbegin(), y.runs_.cend()); }
    SpamRunList &Runs() { return runs_; }

private:
    const uchar* const firstPixel_;
    const size_t stride_;
    const int16_t width_;
    SpamRunList runs_;
};

SpamRgn::SpamRgn()
{
    if (g_color_index_s > g_numRgnColors - 1)
    {
        g_color_index_s = 0;
    }
    color_ = g_rgnColors[g_color_index_s++];
}

SpamRgn::SpamRgn(const Geom::PathVector &pv)
    : SpamRgn()
{
    AddRun(pv);
}

SpamRgn::~SpamRgn()
{ 
    BasicImgProc::ReturnRegion(std::move(data_));
}

void SpamRgn::AddRun(const cv::Mat &binaryImage)
{
    int dph = binaryImage.depth();
    int cnl = binaryImage.channels();
    if (CV_8U == dph && 1 == cnl)
    {
        constexpr int16_t top = 0;
        const int16_t bot = binaryImage.rows;
        constexpr int16_t left = 0;
        const int16_t right = binaryImage.cols;
        for (int16_t r = top; r<bot; ++r)
        {
            int16_t cb = -1;
            const uchar* pRow = binaryImage.data + r * binaryImage.step1();
            for (int16_t c = left; c < right; ++c)
            {
                if (pRow[c])
                {
                    if (cb < 0)
                    {
                        cb = c;
                    }
                }
                else
                {
                    if (cb > -1)
                    {
                        data_.emplace_back(r, cb, c );
                        cb = -1;
                    }
                }
            }

            if (cb > -1)
            {
                data_.emplace_back(r, cb, right );
            }
        }
    }

    ClearCacheData();
}

void SpamRgn::SetRun(const Geom::PathVector &pv)
{
    clear();
    Geom::OptRect bbox = pv.boundsFast();
    if (bbox)
    {
        int t = cvFloor(bbox.get().top());
        int b = cvCeil(bbox.get().bottom()) + 1;
        int l = cvFloor(bbox.get().left());
        int r = cvCeil(bbox.get().right()) + 1;
        cv::Rect rect(cv::Point(l-3, t-3), cv::Point(r+3, b+3));
        cv::Mat mask = BasicImgProc::PathToMask(pv*Geom::Translate(-rect.x, -rect.y), rect.size());
        AddRun(mask);

        for (SpamRun &run : data_)
        {
            run.colb += rect.x;
            run.cole += rect.x;
            run.row  += rect.y;
        }
    }

    ClearCacheData();
}

void SpamRgn::AddRun(const Geom::PathVector &pv)
{
    clear();
    constexpr int negInf = std::numeric_limits<int>::min();
    Geom::OptRect bbox = pv.boundsFast();
    if (bbox)
    {
        int t = cvFloor(bbox.get().top());
        int b = cvCeil(bbox.get().bottom()) + 1;
        int l = cvFloor(bbox.get().left());
        int r = cvCeil(bbox.get().right()) + 1;
        for (int row = t; row < b; ++row)
        {
            int cb = negInf;
            for (int c = l; c < r; ++c)
            {
                if (IsPointInside(pv, Geom::Point(c, row)))
                {
                    if (cb == negInf)
                    {
                        cb = c;
                    }
                }
                else
                {
                    if (cb != negInf)
                    {
                        data_.emplace_back(row, cb, c);
                        cb = negInf;
                    }
                }
            }

            if (cb != negInf)
            {
                data_.emplace_back(row, cb, r);
            }
        }
    }
}

void SpamRgn::AddRunParallel(const cv::Mat &binaryImage)
{
    int dph = binaryImage.depth();
    int cnl = binaryImage.channels();
    if (CV_8U == dph && 1 == cnl)
    {
        RunLengthEncoder enc(binaryImage);
        tbb::parallel_reduce(tbb::blocked_range<int16_t>(0, binaryImage.rows), enc);
        data_.swap(enc.Runs());
    }

    ClearCacheData();
}

void SpamRgn::Draw(const cv::Mat &dstImage, const double sx, const double sy) const
{
    int dph = dstImage.depth();
    int cnl = dstImage.channels();
    if (CV_8U == dph && 4 == cnl)
    {
        cv::Rect bbox = BoundingBox();
        int minx = cvFloor(sx * bbox.x);
        int miny = cvFloor(sy * bbox.y);
        int maxx = cvCeil(sx * (bbox.x + bbox.width + 1));
        int maxy = cvCeil(sy * (bbox.y + bbox.height + 1));
        for (int y = miny; y < maxy; ++y)
        {
            int16_t or = static_cast<int>(cvRound(y / sy - 0.5));
            auto lb = std::lower_bound(data_.cbegin(), data_.cend(), or, [](const SpamRun &run, const int16_t val) { return  run.row < val; });
            if (lb != data_.cend() && or == lb->row)
            {
                int r = y;
                if (r >= 0 && r<dstImage.rows)
                {
                    auto pRow = dstImage.data + r * dstImage.step1();
                    for (int x = minx; x<maxx; ++x)
                    {
                        int16_t oc = static_cast<int>(cvRound(x / sx - 0.5));
                        auto itRun = lb;
                        bool cInside = false;
                        while (itRun != data_.cend() && or == itRun->row)
                        {
                            if (oc < itRun->cole && oc >= itRun->colb)
                            {
                                cInside = true;
                                break;
                            }

                            itRun += 1;
                        }

                        if (cInside)
                        {
                            int c = x;
                            if (c >= 0 && c<dstImage.cols)
                            {
                                auto pPixel = reinterpret_cast<uint32_t *>(pRow + c * 4);
                                *pPixel = 0xFF0000;
                            }
                        }
                    }
                }
            }
        }
    }
}

double SpamRgn::Area() const
{
    if (area_ == boost::none)
    {
        double a = 0;
        double x = 0;
        double y = 0;
        for (const SpamRun &r : data_)
        {
            const auto n = r.cole - r.colb;
            a += n;
            x += (r.cole - 1 + r.colb) * n / 2.0;
            y += r.row * n;
        }

        area_ = a;
        if (a>0)
        {
            centroid_.emplace(x / a, y / a);
        }
        else
        {
            centroid_.emplace(0, 0);
        }
    }

    return *area_;
}

cv::Point2d SpamRgn::Centroid() const
{
    if (centroid_ == boost::none)
    {
        Area();
    }

    return *centroid_;
}

Geom::Circle SpamRgn::MinCircle() const
{
    if (minCircle_ == boost::none)
    {
        float radius = 0;
        cv::Point2f center;
        std::vector<cv::Point> points;
        points.reserve(data_.size()*2);

        for (const SpamRun &r : data_)
        {
            const auto n = r.cole - r.colb;
            if (1==n)
            {
                points.emplace_back(r.colb, r.row);
            }
            else
            {
                points.emplace_back(r.colb, r.row);
                points.emplace_back(r.cole-1, r.row);
            }
        }

        if (points.empty())
        {
            minCircle_.emplace(0.0, 0.0, 0.0);
        }
        else
        {
            cv::minEnclosingCircle(points, center, radius);
            minCircle_.emplace(center.x, center.y, radius);
        }
    }

    return *minCircle_;
}

int SpamRgn::NumHoles() const
{
    if (contours_ == boost::none)
    {
        contours_.emplace();
        contours_.emplace();
        RunTypeDirectionEncoder encoder(*const_cast<SpamRgn *>(this));
        encoder.track(*contours_);
    }

    return static_cast<int>(contours_->holes.size());
}

SPSpamRgnVector SpamRgn::Connect() const
{
    const AdjacencyList &al = GetAdjacencyList();
    std::vector<uint8_t> met(al.size());
    std::vector<int> runStack;
    std::vector<std::vector<int>> rgnIdxs;
    const int numRuns = static_cast<int>(al.size());
    for (int n=0; n<numRuns; ++n)
    {
        if (!met[n])
        {
            rgnIdxs.emplace_back();
            runStack.push_back(n);
            while (!runStack.empty())
            {
                const int seedRun = runStack.back();
                runStack.pop_back();
                if (!met[seedRun])
                {
                    met[seedRun] = 1;
                    rgnIdxs.back().push_back(seedRun);
                    for (const int a : al[seedRun])
                    {
                        runStack.push_back(a);
                    }
                }
            }
        }
    }

    tbb::parallel_for(size_t(0), rgnIdxs.size(), [&rgnIdxs](size_t i) { std::sort(rgnIdxs[i].begin(), rgnIdxs[i].end()); });

    SPSpamRgnVector rgs = std::make_shared<SpamRgnVector>();
    rgs->resize(rgnIdxs.size());
    const int numRgns = static_cast<int>(rgnIdxs.size());
    for (int r = 0; r < numRgns; ++r)
    {
        SpamRgn &rgn = (*rgs)[r];
        const std::vector<int> &rgnIdx = rgnIdxs[r];
        const int numRgnRuns = static_cast<int>(rgnIdx.size());
        rgn.data_.resize(numRgnRuns);
        for (int rr = 0; rr < numRgnRuns; ++rr)
        {
            rgn.data_[rr] = data_[rgnIdx[rr]];
        }
    }

    return rgs;
}

cv::Rect SpamRgn::BoundingBox() const
{
    if (bbox_ == boost::none)
    {
        cv::Point minPoint{ std::numeric_limits<int16_t>::max(), std::numeric_limits<int16_t>::max() };
        cv::Point maxPoint{ std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::min() };

        for (const SpamRun &r : data_)
        {
            if (r.row < minPoint.y) {
                minPoint.y = r.row;
            }

            if (r.row > maxPoint.y) {
                maxPoint.y = r.row;
            }

            if (r.colb < minPoint.x) {
                minPoint.x = r.colb;
            }

            if (r.cole > maxPoint.x) {
                maxPoint.x = r.cole;
            }
        }

        if (data_.empty()) {
            bbox_ = cv::Rect();
        } else {
            bbox_ = cv::Rect(minPoint, maxPoint);
        }
    }

    return *bbox_;
}

bool SpamRgn::Contain(const int16_t r, const int16_t c) const
{
    auto lb = std::lower_bound(data_.cbegin(), data_.cend(), r, [](const SpamRun &run, const int16_t val) { return val < run.row; });
    while (lb != data_.cend() && r == lb->row)
    {
        if (c<lb->cole && c>=lb->colb)
        {
            return true;
        }

        lb += 1;
    }

    return false;
}

const AdjacencyList &SpamRgn::GetAdjacencyList() const
{
    if (adjacencyList_==boost::none)
    {
        adjacencyList_.emplace(data_.size());
        if (!data_.empty())
        {
            const RowRangeList &rowRanges = GetRowRanges();
            AdjacencyBuilderTBB adjBuilder(data_, rowRanges, *adjacencyList_);
            tbb::parallel_for(tbb::blocked_range<int>(1, static_cast<int>(rowRanges.size() - 1)), adjBuilder);
        }
    }

    return *adjacencyList_;
}

const Geom::PathVector &SpamRgn::GetPath() const
{
    if (path_==boost::none)
    {
        path_.emplace();
        RunTypeDirectionEncoder encoder(*const_cast<SpamRgn *>(this));
        encoder.track(*path_);
    }

    return *path_;
}

const RegionContourCollection &SpamRgn::GetContours() const
{
    if (contours_ == boost::none)
    {
        contours_.emplace();
        RunTypeDirectionEncoder encoder(*const_cast<SpamRgn *>(this));
        encoder.track(*contours_);
    }

    return *contours_;
}

const RowRangeList &SpamRgn::GetRowRanges() const
{
    if (rowRanges_== boost::none)
    {
        rowRanges_.emplace();
        if (!data_.empty())
        {
            constexpr int Infinity = std::numeric_limits<int>::max();
            constexpr int NegInfinity = std::numeric_limits<int>::min();
            rowRanges_->reserve(data_.size() + 2);
            rowRanges_->emplace_back(NegInfinity, Infinity, Infinity);

            int begIdx = 0;
            int16_t currentRow = data_.front().row;

            const int numRuns = static_cast<int>(data_.size());
            for (int run = 0; run < numRuns; ++run)
            {
                if (data_[run].row != currentRow)
                {
                    rowRanges_->emplace_back(currentRow, begIdx, run);
                    begIdx = run;
                    currentRow = data_[run].row;
                }
            }

            rowRanges_->emplace_back(currentRow, begIdx, numRuns);
            rowRanges_->emplace_back(Infinity, Infinity, Infinity);
        }
    }

    return *rowRanges_;
}

void SpamRgn::ClearCacheData()
{
    area_           = boost::none;
    path_           = boost::none;
    bbox_           = boost::none;
    contours_       = boost::none;
    contours_       = boost::none;
    rowRanges_      = boost::none;
    adjacencyList_  = boost::none;
    centroid_       = boost::none;
    minBox_         = boost::none;
    minCircle_      = boost::none;
}

SPSpamRgnVector SpamRgn::ConnectMT() const
{
    const RowRangeList &rowRanges = GetRowRanges();
    const int numTasks = std::min(10, static_cast<int>(BasicImgProc::GetNumWorkers()));
    const int averageRunsPerTask = GetNumRuns() / numTasks;
    boost::container::static_vector<std::pair<int, int>, 10> taskRuns;

    int cRuns = 0;
    int lastRun = 0;
    const int vlaidRowIndexEnd = static_cast<int>(rowRanges.size())-1;
    for (int rowIndex=1; rowIndex<vlaidRowIndexEnd; ++rowIndex)
    {
        const auto &rowRange = rowRanges[rowIndex];
        cRuns += (rowRange.end - rowRange.beg);
        if (cRuns > averageRunsPerTask)
        {
            taskRuns.emplace_back(lastRun, rowRange.end);
            lastRun = rowRange.end;
        }
    }
    taskRuns.emplace_back(lastRun, GetNumRuns());

    return std::make_shared<SpamRgnVector>();
}

bool SpamRgn::IsPointInside(const Geom::PathVector &pv, const Geom::Point &pt)
{
    if (!pv.empty())
    {
        int numWinding = 0;
        for (const Geom::Path &pth : pv)
        {
            if (Geom::contains(pth, pt))
            {
                numWinding += 1;
            }
        }

        if (numWinding & 1)
        {
            return true;
        }
    }

    return false;
}

PointSet::PointSet(const SpamRgn &rgn)
{
    for (const SpamRun &run : rgn.GetData())
    {
        for (int16_t col = run.colb; col < run.cole; ++col)
        {
            emplace_back(col, run.row);
        }
    }
}

PointSet::PointSet(const SpamRgn &rgn, const cv::Point &offset)
{
    for (const SpamRun &run : rgn.GetData())
    {
        for (int16_t col = run.colb; col < run.cole; ++col)
        {
            emplace_back(col + offset.x, run.row + offset.y);
        }
    }
}

std::pair<cv::Point, cv::Point> PointSet::MinMax() const
{
    cv::Point minPoint{ std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
    cv::Point maxPoint{ std::numeric_limits<int>::min(), std::numeric_limits<int>::min() };

    for (const cv::Point &pt : *this)
    {
        if (pt.y < minPoint.y) {
            minPoint.y = pt.y;
        }

        if (pt.y > maxPoint.y) {
            maxPoint.y = pt.y;
        }

        if (pt.x < minPoint.x) {
            minPoint.x = pt.x;
        }

        if (pt.x > maxPoint.x) {
            maxPoint.x = pt.x;
        }
    }

    if (empty()) {
        return std::make_pair(cv::Point(), cv::Point());
    } else {
        return std::make_pair(minPoint, maxPoint);
    }
}

bool PointSet::IsInsideImage(const cv::Size &imgSize) const
{
    for (const cv::Point &point : *this)
    {
        if (point.x<0 || point.x>=imgSize.width || point.y < 0 || point.y >= imgSize.height)
        {
            return false;
        }
    }

    return true;
}

RD_LIST RunTypeDirectionEncoder::encode() const
{
    RD_LIST rd_list;
    rd_list.reserve(rgn_.data_.size() * 2 + 1);
    rd_list.emplace_back(0, 0, 0, 0);
    rd_list.front().FLAG = 1;

    if (rgn_.data_.empty())
    {
        return rd_list;
    }

    std::vector<int16_t> P_BUFFER;
    std::vector<int16_t> C_BUFFER;
    constexpr int16_t Infinity = std::numeric_limits<int16_t>::max();
    P_BUFFER.push_back(Infinity);

    bool extended = false;
    SpamRunList runData;
    const SpamRunList *pRunData = nullptr;
    if (rgn_.data_.capacity() >= rgn_.data_.size()+2)
    {
        extended = true;
        const int16_t lastRow = rgn_.data_.back().row;
        rgn_.data_.emplace_back(lastRow + 1, Infinity, Infinity);
        rgn_.data_.emplace_back(lastRow + 2, Infinity, Infinity);
        pRunData = &rgn_.data_;
    }
    else
    {
        runData.resize(rgn_.data_.size() + 2);
        ::memcpy(&runData[0], &rgn_.data_[0], sizeof(runData[0])*rgn_.data_.size());

        const int16_t lastRow = rgn_.data_.back().row;
        runData[rgn_.data_.size()]   = SpamRun(lastRow + 1, Infinity, Infinity);
        runData[rgn_.data_.size()+1] = SpamRun(lastRow + 2, Infinity, Infinity);
        pRunData = &runData;
    }

    int P3 = 0;
    int P4 = 1;
    int P5 = 0;

    int16_t l = (*pRunData)[0].row;
    const int numRuns = static_cast<int>(pRunData->size());
    for (int n=0; n<numRuns; ++n)
    {
        const SpamRun &r = (*pRunData)[n];
        if (r.row == l)
        {
            C_BUFFER.push_back(r.colb);
            C_BUFFER.push_back(r.cole);
        }
        else
        {
            C_BUFFER.push_back(Infinity);

            int P1 = 0;
            int P2 = 0;
            int State = 0;
            int16_t X1 = P_BUFFER[P1];
            int16_t X2 = C_BUFFER[P2];
            int16_t X  = X2;

            bool stay = true;
            while (stay)
            {
                int RD_CODE = 0;
                switch (State)
                {
                case 0:
                    if (X1>X2) {
                        State = 2; X = X2; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1<X2) {
                        State = 1; P1 += 1; X1 = P_BUFFER[P1];
                    } else if (X1 < Infinity) {
                        State = 3; RD_CODE = 2; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    } else {
                        stay = false;
                    }
                    break;

                case 1:
                    if (X1 > X2) {
                        State = 3; X = X2; RD_CODE = 4; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1 < X2) {
                        State = 0; X = X1; RD_CODE = 5; P1 += 1; X1 = P_BUFFER[P1];
                    } else {
                        State = 4; X = X1; RD_CODE = 4; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    }
                    break;

                case 2:
                    if (X1 > X2) {
                        State = 0; RD_CODE = 1; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1 < X2) {
                        State = 3; RD_CODE = 3; P1 += 1; X1 = P_BUFFER[P1];
                    } else {
                        State = 5; RD_CODE = 3; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    }
                    break;

                case 3:
                    if (X1 > X2) {
                        State = 5; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1 < X2) {
                        State = 4; X = X1; P1 += 1; X1 = P_BUFFER[P1];
                    } else {
                        State = 0; RD_CODE = 6; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    }
                    break;

                case 4:
                    if (X1 > X2) {
                        State = 0; RD_CODE = 8; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1 < X2) {
                        State = 3; RD_CODE = 10; P1 += 1; X1 = P_BUFFER[P1];
                    } else {
                        State = 5; RD_CODE = 10; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    }
                    break;

                case 5:
                    if (X1 > X2) {
                        State = 3; X = X2; RD_CODE = 9; P2 += 1; X2 = C_BUFFER[P2];
                    } else if (X1 < X2) {
                        State = 0; X = X1; RD_CODE = 7; P1 += 1; X1 = P_BUFFER[P1];
                    } else {
                        State = 4; X = X1; RD_CODE = 9; P1 += 1; X1 = P_BUFFER[P1]; P2 += 1; X2 = C_BUFFER[P2];
                    }
                    break;

                default:
                    break;
                }

                if (RD_CODE)
                {
                    P3 += 1;
                    const int QI = qis_[RD_CODE];
                    rd_list.emplace_back(X, l, RD_CODE, QI);

                    if (QI)
                    {
                        rd_list[P5].W_LINK = P3;
                        P5 = P3;
                    }
                    else
                    {
                        rd_list[P5].W_LINK = P3 + 1;
                    }

                    if (5 == RD_CODE)
                    {
                        if (downLink_[RD_CODE][rd_list[P4].CODE])
                        {
                            rd_list[P4].LINK = P3;
                            rd_list[P4].QI -= 1;
                            if (1 > rd_list[P4].QI)
                            {
                                P4 = rd_list[P4].W_LINK;
                            }
                        }

                        if (upLink_[RD_CODE][rd_list[P4].CODE])
                        {
                            rd_list[P3].LINK = P4;
                            rd_list[P4].QI -= 1;
                            if (1 > rd_list[P4].QI)
                            {
                                P4 = rd_list[P4].W_LINK;
                            }
                        }
                    }
                    else
                    {
                        if (upLink_[RD_CODE][rd_list[P4].CODE])
                        {
                            rd_list[P3].LINK = P4;
                            rd_list[P4].QI -= 1;
                            if (1 > rd_list[P4].QI)
                            {
                                P4 = rd_list[P4].W_LINK;
                            }
                        }

                        if (downLink_[RD_CODE][rd_list[P4].CODE])
                        {
                            rd_list[P4].LINK = P3;
                            rd_list[P4].QI -= 1;
                            if (1 > rd_list[P4].QI)
                            {
                                P4 = rd_list[P4].W_LINK;
                            }
                        }
                    }
                }
            }

            P_BUFFER.swap(C_BUFFER);
            C_BUFFER.resize(0);
            n -= 1;
            l += 1;
        }
    }

    if (extended)
    {
        rgn_.data_.pop_back();
        rgn_.data_.pop_back();
    }

    return rd_list;
}

void RunTypeDirectionEncoder::track(RD_CONTOUR_LIST &contours, RD_CONTOUR_LIST &holes) const
{
    RD_LIST rd_list = encode();
    for (RD_LIST_ENTRY &e : rd_list)
    {
        if (!e.FLAG)
        {
            e.FLAG = 1;
            if (1==e.CODE)
            {
                RD_CONTOUR outerContour;
                outerContour.emplace_back(e.X, e.Y);
                int nextLink = e.LINK;
                while (!rd_list[nextLink].FLAG)
                {
                    rd_list[nextLink].FLAG = 1;
                    if (count_[rd_list[nextLink].CODE])
                    {
                        outerContour.emplace_back(outerContour.back().x, rd_list[nextLink].Y);
                        outerContour.emplace_back(rd_list[nextLink].X, rd_list[nextLink].Y);
                    }
                    nextLink = rd_list[nextLink].LINK;
                }

                if (!outerContour.empty())
                {
                    outerContour.emplace_back(outerContour.back().x, outerContour.front().y);
                    contours.push_back(std::move(outerContour));
                }
            }
            else
            {
                if (9 == e.CODE)
                {
                    RD_CONTOUR hole;
                    hole.emplace_back(e.X, e.Y);
                    int nextLink = e.LINK;
                    while (!rd_list[nextLink].FLAG)
                    {
                        rd_list[nextLink].FLAG = 1;
                        if (count_[rd_list[nextLink].CODE])
                        {
                            hole.emplace_back(hole.back().x, rd_list[nextLink].Y);
                            hole.emplace_back(rd_list[nextLink].X, rd_list[nextLink].Y);
                        }
                        nextLink = rd_list[nextLink].LINK;
                    }

                    if (!hole.empty())
                    {
                        hole.emplace_back(hole.back().x, hole.front().y);
                        holes.push_back(std::move(hole));
                    }
                }
            }
        }
    }
}

void RunTypeDirectionEncoder::track(Geom::PathVector &pv) const
{
    Geom::PathBuilder pb(pv);
    RD_LIST rd_list = encode();
    for (RD_LIST_ENTRY &e : rd_list)
    {
        if (!e.FLAG)
        {
            e.FLAG = 1;
            if (1 == e.CODE || 9 == e.CODE)
            {
                double lastX  = e.X;
                double firstY = e.Y;
                pb.moveTo(Geom::Point(lastX, firstY));
                int nextLink = e.LINK;
                while (!rd_list[nextLink].FLAG)
                {
                    rd_list[nextLink].FLAG = 1;
                    if (count_[rd_list[nextLink].CODE])
                    {
                        pb.lineTo(Geom::Point(lastX, rd_list[nextLink].Y));
                        pb.lineTo(Geom::Point(rd_list[nextLink].X, rd_list[nextLink].Y));
                        lastX = rd_list[nextLink].X;
                    }
                    nextLink = rd_list[nextLink].LINK;
                }

                pb.lineTo(Geom::Point(lastX, firstY));
                pb.closePath();
            }
        }
    }
}

void RunTypeDirectionEncoder::track(RegionContourCollection &rcc) const
{
    RD_LIST rd_list = encode();
    for (RD_LIST_ENTRY &e : rd_list)
    {
        if (!e.FLAG)
        {
            e.FLAG = 1;
            if (1 == e.CODE || 9 == e.CODE)
            {
                int lastX  = e.X;
                int firstY = e.Y;
                SpamContour *outerContour = (1 == e.CODE) ? &rcc.outers.emplace_back() : &rcc.holes.emplace_back();
                outerContour->moveTo(lastX, firstY);
                int nextLink = e.LINK;
                while (!rd_list[nextLink].FLAG)
                {
                    rd_list[nextLink].FLAG = 1;
                    if (count_[rd_list[nextLink].CODE])
                    {
                        const auto X = rd_list[nextLink].X;
                        const auto Y = rd_list[nextLink].Y;
                        outerContour->lineTo(lastX, Y);
                        outerContour->lineTo(X, Y);
                        lastX = X;
                    }
                    nextLink = rd_list[nextLink].LINK;
                }

                outerContour->lineTo(lastX, firstY);
            }
        }
    }
}