#ifndef SPAM_UI_PROC_REGION_H
#define SPAM_UI_PROC_REGION_H
#include <array>
#include <vector>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/imgproc.hpp>
#include <boost/optional.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/scalable_allocator.h>
#pragma warning( push )
#pragma warning( disable : 4819 4003 4267 4244)
#include <2geom/circle.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>
#pragma warning( pop )

struct SpamRun
{
    SpamRun() : row(0), colb(0), cole(0), label(0) {}
    SpamRun(const SpamRun &r) : row(r.row), colb(r.colb), cole(r.cole), label(r.label) {}
    SpamRun(const int16_t ll, const int16_t bb, const int16_t ee) : row(ll), colb(bb), cole(ee), label(0) {}
    SpamRun(const int16_t ll, const int16_t bb, const int16_t ee, const uint16_t lab) : row(ll), colb(bb), cole(ee), label(lab) {}
    int row;  // line number (row) of run
    int colb; // column index of beginning(include) of run
    int cole; // column index of ending(exclude) of run
    int label;
};

struct RD_LIST_ENTRY
{
    RD_LIST_ENTRY(const int x, const int y, const int code, const int qi) : X(x), Y(y), CODE(code), LINK(0), W_LINK(0), QI(qi), FLAG(0) {}
    int X;
    int Y;
    int CODE;
    int LINK;
    int W_LINK;
    int QI;
    int FLAG;
};

class SpamRgn;
using VertexList = boost::container::small_vector<int, 5>;
using AdjacencyList = std::vector<VertexList>;
using SpamRgnVector = std::vector<SpamRgn>;
using SPSpamRgn = std::shared_ptr<SpamRgn>;
using SPSpamRgnVector = std::shared_ptr<SpamRgnVector>;
using RgnBufferZone = std::unordered_map<std::string, SPSpamRgnVector>;
using RD_LIST = std::vector<RD_LIST_ENTRY>;
using RD_CONTOUR = std::vector<cv::Point, tbb::scalable_allocator<cv::Point>>;
using RD_CONTOUR_LIST = std::vector<RD_CONTOUR, tbb::scalable_allocator<RD_CONTOUR>>;
using SpamRunList = std::vector<SpamRun, tbb::scalable_allocator<SpamRun>>;
using RowRunStartList = std::vector<int>;
using LabelT = int;

struct SpamContour
{
    void moveTo(const int x, const int y) { start.x = x; start.y = y; }
    void lineTo(const int x, const int y) { points.emplace_back(x, y); }
    cv::Point start;
    RD_CONTOUR points;
};

using ContourVector = std::vector<SpamContour, tbb::scalable_allocator<SpamContour>>;
struct RegionContourCollection
{
    ContourVector holes;
    ContourVector outers;
};

class SpamRgn
{
    friend class BasicImgProc;
    friend class RunTypeDirectionEncoder;

public:
    SpamRgn();
    SpamRgn(const Geom::PathVector &pv);
    SpamRgn(const Geom::PathVector &pv, std::vector<uint8_t> &buf);
    ~SpamRgn();

public:
    bool isEmpty() const {};
    void swap(SpamRgn &o) { data_.swap(o.data_); ClearCacheData(); }
    void clear() { data_.resize(0); ClearCacheData(); }

public:
    void AddRun(const int16_t l, const int16_t cb, const int16_t ce) { data_.push_back({ l, cb, ce }); ClearCacheData(); }
    void AddRun(const cv::Mat &binaryImage);
    void SetRegion(const cv::Rect &rect);
    void SetRegion(const Geom::PathVector &pv);
    void SetRegion(const Geom::PathVector &pv, std::vector<uint8_t> &buf);
    void AddRunParallel(const cv::Mat &binaryImage);
    void Draw(const cv::Mat &dstImage, const double sx, const double sy) const;
    int GetNumRuns() const { return static_cast<int>(data_.size()); }

public:
    double Area() const;
    cv::Point2d Centroid() const;
    Geom::Circle MinCircle() const;
    int NumHoles() const;
    SPSpamRgnVector Connect();
    SPSpamRgnVector ConnectMT() const;
    cv::Rect BoundingBox() const;
    bool Contain(const int16_t r, const int16_t c) const;
    const Geom::PathVector &GetPath() const;
    const RegionContourCollection &GetContours() const;
    int GetRowRanges(RowRunStartList &rBegs) const;
    uint32_t GetColor() const { return color_; }
    uint8_t  GetRed() const { return static_cast<uint8_t>(0xFF & color_); }
    uint8_t  GetGreen() const { return static_cast<uint8_t>(0xFF & color_ >> 8); }
    uint8_t  GetBlue() const { return static_cast<uint8_t>(0xFF & color_ >> 16); }
    uint8_t  GetAlpha() const { return static_cast<uint8_t>(0xFF & color_ >> 24); }
    SpamRunList &GetData() { return data_; }
    const SpamRunList &GetData() const { return data_; }

private:
    void ClearCacheData();
    static bool IsPointInside(const Geom::PathVector &pv, const Geom::Point &pt);

public:
    mutable SpamRunList data_;
    uint32_t             color_;
    mutable boost::optional<double>                   area_;
    mutable boost::optional<cv::Point2d>              centroid_;
    mutable boost::optional<cv::Rect>                 bbox_;
    mutable boost::optional<cv::RotatedRect>          minBox_;
    mutable boost::optional<Geom::Circle>             minCircle_;
    mutable boost::optional<Geom::PathVector>         path_;
    mutable boost::optional<RegionContourCollection>  contours_;
    mutable boost::optional<RowRunStartList>          rowRanges_;
};

class PointSet : public std::vector<cv::Point>
{
public:
    PointSet() {}
    PointSet(const SpamRgn &rgn);
    PointSet(const SpamRgn &rgn, const cv::Point &offset);
    std::pair<cv::Point, cv::Point> MinMax() const;

public:
    bool IsInsideImage(const cv::Size &imgSize) const;
};

class RunTypeDirectionEncoder
{
public:
    RunTypeDirectionEncoder(SpamRgn &rgn) : rgn_(rgn) {}

public:
    RD_LIST encode() const;
    void track(RD_CONTOUR_LIST &contours, RD_CONTOUR_LIST &holes) const;
    void track(Geom::PathVector &pv) const;
    void track(RegionContourCollection &rcc) const;

private:
    SpamRgn &rgn_;
    const int qis_[11]{ 0, 2, 1, 1, 1, 0, 1, 1, 1, 2, 0 };
    const int count_[11]{1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1};
    const int downLink_[11][11]{ {0}, {0}, {0, 1, 1, 1, 1, 0, 0, 0, 0, 1}, {0, 1, 1, 1, 1, 0, 0, 0, 0, 1}, {0, 1, 1, 1, 1, 0, 0, 0, 0, 1}, {0, 1, 1, 1, 1, 0, 0, 0, 0, 1}, {0}, {0}, {0}, {0}, {0, 1, 1, 1, 1, 0, 0, 0, 0, 1} };
    const int upLink_[11][11]{ {0}, {0}, {0}, {0}, {0}, {0, 1, 0, 0, 0, 0, 1, 1, 1, 1}, {0, 1, 0, 0, 0, 0, 1, 1, 1, 1}, {0, 1, 0, 0, 0, 0, 1, 1, 1, 1}, {0, 1, 0, 0, 0, 0, 1, 1, 1, 1}, {0}, {0, 1, 0, 0, 0, 0, 1, 1, 1, 1} };
};

class ConnectWuParallel
{
    class FirstScan8Connectivity
    {
        LabelT *P_;
        int *chunksSizeAndLabels_;
        const int maxCol_;
        const RowRunStartList &rowRunBegs_;
        SpamRunList &data_;

    public:
        FirstScan8Connectivity(LabelT *P, int *chunksSizeAndLabels, const int maxCol, const RowRunStartList &rowRunBegs, SpamRunList &data);
        void operator()(const tbb::blocked_range<int>& br) const;
    };

public:
    ConnectWuParallel() : maxCol(-1) {}

public:
    SPSpamRgnVector operator() (SpamRgn &rgn, int connectivity);
    void mergeLabels8Connectivity(SpamRgn &rgn, LabelT *P, const int *chunksSizeAndLabels);
    void mergeLabels4Connectivity(SpamRgn &rgn, LabelT *P, const int *chunksSizeAndLabels);

private:
    RowRunStartList rowRunBegs;
    int maxCol;
};

inline bool IsRunColumnIntersection(const SpamRun &r1, const SpamRun &r2)
{
    return !(r1.cole < r2.colb || r2.cole < r1.colb);
}

#endif //SPAM_UI_PROC_REGION_H