#ifndef __OPENCV_MVLAB_CONTOUR_IMPL_HPP__
#define __OPENCV_MVLAB_CONTOUR_IMPL_HPP__

#include <opencv2/mvlab/contour.hpp>
#include <boost/optional.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <tbb/scalable_allocator.h>
#include <2geom/2geom.h>

namespace cv {
namespace mvlab {

class ContourImpl : public Contour
{
public:
    ContourImpl() {}
    ContourImpl(const std::vector<Point2f> &vertexes, const bool closed);
    ContourImpl(Point2fSequence *vertexes, const bool closed);
    ContourImpl(ScalablePoint2fSequenceSequence *curves, UScalableBoolSequence *closed);

public:
    int Draw(Mat &img, const Scalar& color, const float thickness, const int style) const CV_OVERRIDE;
    int Draw(InputOutputArray img, const Scalar& color, float thickness, const int style) const CV_OVERRIDE;

public:
    bool Empty() const CV_OVERRIDE;
    int Count() const CV_OVERRIDE;
    void CountPoints(std::vector<int> &cPoints) const CV_OVERRIDE;
    void GetArea(std::vector<double> &areas) const CV_OVERRIDE;
    void GetLength(std::vector<double> &lengthes) const CV_OVERRIDE;
    void GetCentroid(std::vector<cv::Point2f> &centroids) const CV_OVERRIDE;
    void GetBoundingBox(std::vector<cv::Rect> &boundingBoxes) const CV_OVERRIDE;
    Ptr<Contour> Simplify(const float tolerance) const CV_OVERRIDE;
    //Access
    int GetPoints(std::vector<Point2f> &vertexes) const CV_OVERRIDE;
    //Geometric Transformations
    cv::Ptr<Contour> Move(const cv::Point2f &delta) const CV_OVERRIDE;
    cv::Ptr<Contour> Zoom(const cv::Size2f &scale) const CV_OVERRIDE;
    //Features
    void TestClosed(std::vector<int> &isClosed) const CV_OVERRIDE;
    void TestPoint(const cv::Point2f &point, std::vector<int> &isInside) const CV_OVERRIDE;
    void TestSelfIntersection(const cv::String &closeContour, std::vector<int> &doesIntersect) const CV_OVERRIDE;

public:
    void Feed(Cairo::RefPtr<Cairo::Context> &cr) const;
    const Point2fSequence &GetVertexes() const { return curves_.front(); };

private:
    void DrawVerified(Mat &img, const Scalar& color, const float thickness, const int style) const;
    void AreaCenter() const;
    void ChangedCoordinatesToFixed() const;

private:
    const UScalableBoolSequence is_closed_;
    const ScalablePoint2fSequenceSequence curves_;
    mutable UScalableIntSequence x_fixed_;
    mutable UScalableIntSequence y_fixed_;
    mutable UScalableDoubleSequence length_;
    mutable UScalableDoubleSequence area_;
    mutable UScalablePoint2fSequence centroid_;
    mutable UScalableRectSequence bbox_;
};

}
}

#endif
