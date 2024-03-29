#ifndef SPAM_UI_PROC_SHAPE_TEMPLATE_H
#define SPAM_UI_PROC_SHAPE_TEMPLATE_H
#include "rgn.h"
#include "basetmpl.h"

struct ShapeClusterData
{
    ShapeClusterData() : label(0) {}
    int label;
    cv::Point2f center;
    cv::Point2f direction;
};

using GradientSequence = std::vector<float>;
using ClusterSequence = std::vector<ShapeClusterData>;

struct ShapeTemplData
{
    ShapeTemplData(const float a, const float s) : angle(a), scale(s) {}
    GradientSequence gNXVals;
    GradientSequence gNYVals;
    PointSet         edgeLocs;
    ClusterSequence  clusters;
    std::vector<int> mindices;
    cv::Point        minPoint;
    cv::Point        maxPoint;
    float            angle;
    float            scale;
};

struct ShapeTmplCreateData
{
    BaseTmplCreateData baseData;
    const int lowContrast;
    const int highContrast;
};

struct ShapeTmplMatchOption
{
    const float minScore;
    const float greediness;
    const int minContrast;
    const bool touchBorder;
};

struct LayerShapeData
{
    LayerShapeData(const float as, const float ss) : angleStep(as), scaleStep(ss) {}
    std::vector<ShapeTemplData> tmplDatas;
    float angleStep;
    float scaleStep;
};

class ShapeTemplate : public BaseTemplate
{
public:
    template<bool TouchBorder> friend struct ShapeTopLayerScaner;
    template<bool TouchBorder> friend struct ShapeCandidateScaner;
    template<bool TouchBorder> friend struct DeformCandidateScaner;

public:
    ShapeTemplate();
    ~ShapeTemplate();

public:
    SpamResult matchShapeTemplate(const cv::Mat &img, const ShapeTmplMatchOption &smo, cv::Point2f &pos, float &angle, float &score);
    SpamResult CreateTemplate(const ShapeTmplCreateData &createData);
    const std::vector<LayerShapeData> &GetTmplDatas() const { return pyramid_tmpl_datas_; }
    cv::Mat GetTopScoreMat() const;
    void DumpTemplate(std::ostream &oss) const;
    void DrawTemplate(cv::Mat &img, const cv::Point2f &pos, const float angle) const;

private:
    void destroyData();
    SpamResult createShapeTemplate(const ShapeTmplCreateData &createData);
    void linkTemplatesBetweenLayers();
    float estimateSubPixelPose(const Candidate &bestCandidate, const float minScore, const int minContrast, const float greediness, cv::Point2f &pos, float &angle);
    void groupEdgePoints(ShapeTemplData &shptd) const;

private:
    std::vector<LayerShapeData> pyramid_tmpl_datas_;
    cv::Mat top_layer_dx_;
    cv::Mat top_layer_dy_;
    std::vector<const float *> dx_row_ptrs_;
    std::vector<const float *> dy_row_ptrs_;
};
#endif //SPAM_UI_PROC_SHAPE_TEMPLATE_H