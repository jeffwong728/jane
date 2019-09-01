#ifndef SPAM_UI_PROC_PIXEL_TEMPLATE_H
#define SPAM_UI_PROC_PIXEL_TEMPLATE_H
#include "rgn.h"
#include <ui/errdef.h>
#include <array>
#include <vector>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/imgproc.hpp>
#include <boost/optional.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant.hpp>
#include <tbb/scalable_allocator.h>
#pragma warning( push )
#pragma warning( disable : 4819 4003 4267 4244)
#include <2geom/path.h>
#include <2geom/pathvector.h>
#pragma warning( pop )

using PixelValueSequence = std::vector<int16_t>;
using NCCValueSequence = std::vector<uint8_t>;
using Point3iSet = std::vector<cv::Point3i>;
using NCCValuePairSequence = std::vector<std::pair<uint8_t, uint8_t>>;
using PointPairSet = std::vector<std::pair<cv::Point, cv::Point>>;
using RegularNCCValues = std::vector<uint32_t, tbb::scalable_allocator<uint32_t>>;

enum TemplPart
{
    kTP_WaveMiddle = 0,
    kTP_WaveBack = 1,
    kTP_WaveFront = 2,
    kTP_WaveGuard = 3
};

struct PixelTemplData
{
    PixelTemplData(const float a, const float s) : angle(a), scale(s) {}
    PixelValueSequence pixlVals;
    PointSet           pixlLocs;
    std::vector<int>   mindices;
    cv::Point          minPoint;
    cv::Point          maxPoint;
    float              angle;
    float              scale;
};

struct NCCTemplData
{
    NCCTemplData(const float a, const float s)
        : angle(a)
        , scale(s)
        , partASum(0)
        , partBSum(0)
        , partASqrSum(0)
        , partBSqrSum(0)
        , cPartABoundaries(0)
        , betaz(0)
        , norm(0) {}

    Point3iSet           partALocs;
    NCCValueSequence     partAVals;
    Point3iSet           partBLocs;
    NCCValueSequence     partBVals;
    PointPairSet         partBBoundaries;
    std::vector<int>     mindices;
    cv::Point            minPoint;
    cv::Point            maxPoint;
    int64_t              partASum;
    int64_t              partBSum;
    int64_t              partASqrSum;
    int64_t              partBSqrSum;
    double               betaz;
    double               norm;
    int                  cPartABoundaries;
    float                angle;
    float                scale;
};

struct BruteForceNCCTemplData
{
    BruteForceNCCTemplData(const float a, const float s)
        : angle(a)
        , scale(s)
        , sum(0)
        , sqrSum(0)
        , norm(0) {}

    PointSet         locs;
    NCCValueSequence vals;
    RegularNCCValues regVals;
    std::vector<int> mindices;
    cv::Point        minPoint;
    cv::Point        maxPoint;
    int64_t          sum;
    int64_t          sqrSum;
    double           norm;
    float            angle;
    float            scale;
};

using PixelTemplDatas = std::vector<PixelTemplData>;
using NCCTemplDatas = std::vector<NCCTemplData>;
using BFNCCTemplDatas = std::vector<BruteForceNCCTemplData>;
using TemplDataSequence = boost::variant<PixelTemplDatas, NCCTemplDatas, BFNCCTemplDatas>;
struct LayerTemplData
{
    LayerTemplData(const float as, const float ss) : angleStep(as), scaleStep(ss) {}
    float angleStep;
    float scaleStep;
    TemplDataSequence tmplDatas;
};

struct PixelTmplCreateData
{
    const cv::Mat &srcImg;
    const Geom::PathVector &tmplRgn;
    const Geom::PathVector &roi;
    const int angleStart;
    const int angleExtent;
    const int pyramidLevel;
    const cv::TemplateMatchModes matchMode;
};

template<typename TAngle>
struct AngleRange
{
    AngleRange(const TAngle s, const TAngle e)
        : start(normalize(s)), end(normalize(e))
    {
    }

    TAngle normalize(const TAngle a)
    {
        TAngle angle = a;
        while (angle < -180) angle += 360;
        while (angle > 180) angle -= 360;
        return angle;
    }

    bool contains(const TAngle a)
    {
        TAngle na = normalize(a);
        if (start < end) {
            return !(na > end || na < start);
        } else {
            return !(na > end && na < start);
        }
    }

    bool between(const TAngle a)
    {
        TAngle na = normalize(a);
        if (start < end) {
            return na < end && na > start;
        } else {
            return na < end || na > start;
        }
    }

    TAngle start;
    TAngle end;
};

class PixelTemplate
{
public:
    class Candidate
    {
    public:
        Candidate() : row(0), col(0), score(0.f), mindex(-1) {}
        Candidate(const int r, const int c) : row(r), col(c), score(0.f), mindex(-1), label(0) {}
        Candidate(const int r, const int c, const float s) : row(r), col(c), score(s), mindex(-1), label(0) {}
        Candidate(const int r, const int c, const int m, const float s) : row(r), col(c), mindex(m), score(s), label(0) {}
        Candidate(const int r, const int c, const int m, const int l) : row(r), col(c), mindex(m), score(0.f), label(l) {}
        int row;
        int col;
        int mindex;
        int label;
        float score;
    };

    class CandidateGroup;
    using CandidateList = std::vector<Candidate, tbb::scalable_allocator<Candidate>>;
    using CandidateGroupList = std::vector<CandidateGroup>;

    class CandidateRun
    {
    public:
        CandidateRun() : row(0), colb(0), cole(0) {}
        CandidateRun(const int r, const int cb, const int ce) : row(r), colb(cb), cole(ce) {}
        bool IsColumnIntersection(const CandidateRun &r) const { return !(cole < r.colb || r.cole < colb); }
        int row;
        int colb;
        int cole;
        Candidate best;
    };

    class CandidateGroup : public std::vector<CandidateRun>
    {
    public:
        CandidateGroup() {}
        CandidateGroup(CandidateList &candidates);
        void RLEncodeCandidates(CandidateList &candidates);
        void Connect(CandidateGroupList &candidateGroups);
        RowRangeList row_ranges;
        AdjacencyList adjacency_list;
        std::vector<int> run_stack;
        std::vector<std::vector<int>> rgn_idxs;
    };

    friend struct SADTopLayerScaner;
    friend struct SADCandidateScaner;
    friend struct NCCTopLayerScaner;
    template<bool TouchBorder> friend struct BFNCCTopLayerScaner;
    template<bool TouchBorder> friend struct BFNCCCandidateScaner;

public:
    PixelTemplate();
    ~PixelTemplate();

public:
    SpamResult matchPixelTemplate(const cv::Mat &img, const int sad, cv::Point2f &pos, float &angle);
    SpamResult matchNCCTemplate(const cv::Mat &img, const float minScore, cv::Point2f &pos, float &angle, float &score);
    SpamResult CreateTemplate(const PixelTmplCreateData &createData);
    const std::vector<LayerTemplData> &GetTmplDatas() const { return pyramid_tmpl_datas_; }
    cv::Mat GetTopScoreMat() const;
    cv::Point2f GetCenter() const { return cfs_.front(); }

private:
    void destroyData();
    void clearCacheMatchData();
    SpamResult verifyCreateData(const PixelTmplCreateData &createData);
    SpamResult calcCreateTemplate(const PixelTmplCreateData &createData);
    SpamResult fastCreateTemplate(const PixelTmplCreateData &createData);
    void linkTemplatesBetweenLayers();
    static uint8_t getMinMaxGrayScale(const cv::Mat &img, const PointSet &maskPoints, const cv::Point &point);
    void supressNoneMaximum();
    SpamResult changeToNCCTemplate();
    SpamResult changeToBruteForceNCCTemplate();
    void processToplayerSearchROI(const PixelTmplCreateData &createData);
    static double calcValue1(const int64_t T, const int64_t S, const int64_t n);
    static double calcValue2(const int64_t T, const int64_t S, const int64_t Sp, const int64_t n, const int64_t np);

private:
    int pyramid_level_;
    cv::TemplateMatchModes match_mode_;
    std::vector<cv::Point2f> cfs_; // centre of referance
    std::vector<SpamRgn>     tmpl_rgns_;
    std::vector<LayerTemplData> pyramid_tmpl_datas_;
    std::vector<cv::Mat> pyrs_;
    std::vector<const uint8_t *> row_ptrs_;
    SpamRgn          top_layer_full_domain_;
    SpamRgn          top_layer_search_roi_;
    Geom::PathVector top_layer_search_roi_g_;
    CandidateList candidates_;
    CandidateList top_candidates_;
    CandidateList final_candidates_;
    CandidateGroup candidate_runs_;
    CandidateGroupList candidate_groups_;
};

inline double PixelTemplate::calcValue1(const int64_t T, const int64_t S, const int64_t n)
{
    int64_t A = n * T;
    int64_t B = S * S;
    if (A <= B)
    {
        return -1.0;
    }
    else
    {
        return std::sqrt(static_cast<double>(A - B)) / std::sqrt(static_cast<double>(n));
    }
}

inline double PixelTemplate::calcValue2(const int64_t Tp, const int64_t S, const int64_t Sp, const int64_t n, const int64_t np)
{
    int64_t A = n * n * Tp;
    int64_t B = np * S * S;
    int64_t C = 2 * n * S * Sp;
    int64_t D = A + B - C;
    if (D <= 0)
    {
        return -1.0;
    }
    else
    {
        return std::sqrt(D) / static_cast<double>(n);
    }
}
#endif //SPAM_UI_PROC_PIXEL_TEMPLATE_H