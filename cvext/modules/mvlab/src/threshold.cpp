#include "precomp.hpp"
#include <utility.hpp>
#include <region/region_impl.hpp>

namespace cv {
namespace mvlab {

class RunsPerRowCounter
{
public:
    RunsPerRowCounter(const cv::Mat *const imgMat, UScalableIntSequence *const numRunsPerRow, const uint32_t minGray, const uint32_t maxGray)
        : img_mat_(imgMat)
        , num_runs_per_row_(numRunsPerRow)
        , min_gray(minGray)
        , max_gray(maxGray)
    {}

    void operator()(const tbb::blocked_range<int>& br) const;

private:
    const cv::Mat *const img_mat_;
    UScalableIntSequence *const num_runs_per_row_;
    const uint32_t min_gray;
    const uint32_t max_gray;
};

void RunsPerRowCounter::operator()(const tbb::blocked_range<int>& br) const
{
    constexpr int32_t simdSize = 32;
    const int32_t width = img_mat_->cols;
    const auto stride = img_mat_->step1();
    const int32_t regularWidth = width & (-simdSize);
    const int32_t binWidth = regularWidth + simdSize;
    std::vector<int8_t, MyAlloc<int8_t>> binVec(binWidth + 1);

    vcl::Vec32uc lowThresh(min_gray);
    vcl::Vec32uc highThresh(max_gray);

    uchar* pRow = img_mat_->data + br.begin() * stride;
    int *pNumRuns = num_runs_per_row_->data() + br.begin();

    for (int row = br.begin(); row < br.end(); ++row)
    {
        int col = 0;
        int8_t *pBin = binVec.data() + 1;
        const uchar* pCol = pRow;
        for (; col < regularWidth; col += simdSize)
        {
            vcl::Vec32uc pixelVals;
            pixelVals.load(pCol);

            vcl::Vec32c binVals = (pixelVals >= lowThresh) & (pixelVals <= highThresh);
            binVals.store(pBin);

            pBin += simdSize;
            pCol += simdSize;
        }

        if (col < width)
        {
            vcl::Vec32uc pixelVals;
            pixelVals.load_partial(width - col, pCol);

            vcl::Vec32c binVals = (pixelVals >= lowThresh) & (pixelVals <= highThresh);
            binVals.store_partial(width - col, pBin);
        }

        pBin = binVec.data();
        for (int n = 0; n < binWidth; n += simdSize)
        {
            vcl::Vec32c preVals, curVals;
            preVals.load(pBin);
            curVals.load(pBin + 1);
            *pNumRuns -= vcl::horizontal_add(preVals ^ curVals);

            pBin += simdSize;
        }

        *pNumRuns >>= 1;
        pRow += stride;
        pNumRuns += 1;
    }
}

class Thresholder
{
public:
    Thresholder(const cv::Mat *const imgMat, const UScalableIntSequence *const numRunsPerRow, RunSequence *const allRuns, const uint32_t minGray, const uint32_t maxGray)
        : img_mat_(imgMat)
        , num_runs_per_row_(numRunsPerRow)
        , all_runs_(allRuns)
        , min_gray(minGray)
        , max_gray(maxGray)
    {}

    void operator()(const tbb::blocked_range<int>& br) const;

private:
    const cv::Mat *const img_mat_;
    const UScalableIntSequence *const num_runs_per_row_;
    RunSequence *const all_runs_;
    const uint32_t min_gray;
    const uint32_t max_gray;
};

void Thresholder::operator()(const tbb::blocked_range<int>& br) const
{
    constexpr int32_t simdSize = 32;
    const int32_t width = img_mat_->cols;
    const auto stride = img_mat_->step1();
    const int32_t regularWidth = width & (-simdSize);
    const int32_t binWidth = regularWidth + simdSize;
    std::array<int8_t, simdSize> traVec;
    std::vector<int8_t, MyAlloc<int8_t>> binVec(binWidth + 1);

    vcl::Vec32uc lowThresh(min_gray);
    vcl::Vec32uc highThresh(max_gray);

    uchar* pRow = img_mat_->data + br.begin() * stride;
    const int *pNumRuns = num_runs_per_row_->data() + br.begin();
    int preNumRuns = (br.begin() > 0) ? pNumRuns[-1] : 0;
    RunLength *pRun = all_runs_->data() + preNumRuns;

    for (int row = br.begin(); row < br.end(); ++row)
    {
        int col = 0;
        int8_t *pBin = binVec.data() + 1;
        const uchar* pCol = pRow;
        for (; col < regularWidth; col += simdSize)
        {
            vcl::Vec32uc pixelVals;
            pixelVals.load(pCol);

            vcl::Vec32c binVals = (pixelVals >= lowThresh) & (pixelVals <= highThresh);
            binVals.store(pBin);

            pBin += simdSize;
            pCol += simdSize;
        }

        if (col < width)
        {
            vcl::Vec32uc pixelVals;
            pixelVals.load_partial(width - col, pCol);

            vcl::Vec32c binVals = (pixelVals >= lowThresh) & (pixelVals <= highThresh);
            binVals.store_partial(width - col, pBin);
        }

        bool b = true;
        pBin = binVec.data();
        for (int n = 0; n < binWidth; n += simdSize)
        {
            vcl::Vec32c preVals, curVals;
            preVals.load(pBin);
            curVals.load(pBin + 1);
            vcl::Vec32c changeVals = preVals ^ curVals;
            if (vcl::horizontal_add(changeVals))
            {
                changeVals.store(traVec.data());
                for (int i = 0; i < simdSize; ++i)
                {
                    if (traVec[i])
                    {
                        if (b)
                        {
                            pRun->row = row;
                            pRun->colb = i + n;
                            pRun->label = 0;
                            b = false;
                        }
                        else
                        {
                            pRun->cole = i + n;
                            pRun += 1;
                            b = true;
                        }
                    }
                }
            }

            pBin += simdSize;
        }

        preNumRuns = *pNumRuns;
        pRow += stride;
        pNumRuns += 1;
    }
}

extern RunSequence RunLengthEncode(const cv::Mat &imgMat, const int minGray, const int maxGray);
RunSequence RunLengthEncode(const cv::Mat &imgMat, const int minGray, const int maxGray)
{
    const int nThreads = static_cast<int>(tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism));
    const int numRows = imgMat.rows;

    const bool is_parallel = nThreads > 1 && (numRows / nThreads) >= 2;
    if (is_parallel)
    {
        UScalableIntSequence numRunsPerRow(imgMat.rows);
        std::memset(numRunsPerRow.data(), 0, numRunsPerRow.size() * sizeof(UScalableIntSequence::value_type));
        RunsPerRowCounter numRunsCounter(&imgMat, &numRunsPerRow, minGray, maxGray);
        tbb::parallel_for(tbb::blocked_range<int>(0, imgMat.rows), numRunsCounter);

        for (int n = 1; n < imgMat.rows; ++n)
        {
            numRunsPerRow[n] += numRunsPerRow[n - 1];
        }

        RunSequence allRuns(numRunsPerRow.back());
        Thresholder thresher(&imgMat, &numRunsPerRow, &allRuns, minGray, maxGray);
        tbb::parallel_for(tbb::blocked_range<int>(0, imgMat.rows), thresher);

        return allRuns;
    }
    else
    {
        UScalableIntSequence numRunsPerRow(imgMat.rows);
        std::memset(numRunsPerRow.data(), 0, numRunsPerRow.size() * sizeof(UScalableIntSequence::value_type));
        RunsPerRowCounter numRunsCounter(&imgMat, &numRunsPerRow, minGray, maxGray);
        numRunsCounter(tbb::blocked_range<int>(0, imgMat.rows));

        for (int n = 1; n < imgMat.rows; ++n)
        {
            numRunsPerRow[n] += numRunsPerRow[n - 1];
        }

        RunSequence allRuns(numRunsPerRow.back());
        Thresholder thresher(&imgMat, &numRunsPerRow, &allRuns, minGray, maxGray);
        thresher(tbb::blocked_range<int>(0, imgMat.rows));

        return allRuns;
    }
}

cv::Ptr<Region> Threshold(cv::InputArray src, const int minGray, const int maxGray)
{
    cv::Mat imgMat = src.getMat();
    if (imgMat.empty())
    {
        return cv::makePtr<RegionImpl>();
    }

    if (minGray < 0 || minGray > 255)
    {
        return cv::makePtr<RegionImpl>();
    }

    if (maxGray < 0 || maxGray > 255)
    {
        return cv::makePtr<RegionImpl>();
    }

    if (minGray >= maxGray)
    {
        return cv::makePtr<RegionImpl>();
    }

    int dph = imgMat.depth();
    int cnl = imgMat.channels();
    if (CV_8U != dph || 1 != cnl)
    {
        return cv::makePtr<RegionImpl>();
    }

    RunSequence allRuns = RunLengthEncode(imgMat, minGray, maxGray);
    return cv::makePtr<RegionImpl>(&allRuns);
}

}
}
