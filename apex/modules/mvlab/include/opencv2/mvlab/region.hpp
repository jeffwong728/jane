#ifndef __OPENCV_MVLAB_REGION_HPP__
#define __OPENCV_MVLAB_REGION_HPP__

#include "forward.hpp"
#include <opencv2/core.hpp>

namespace cv {
namespace mvlab {

class CV_EXPORTS_W Region {

public:
    Region() {}
    virtual ~Region() {}

    CV_WRAP static Ptr<Region> CreateEmpty();
    CV_WRAP static Ptr<Region> CreateRectangle(const Rect2f &rect);
    CV_WRAP static Ptr<Region> CreateRotatedRectangle(const RotatedRect &rotatedRect);
    CV_WRAP static Ptr<Region> CreateCircle(const Point2f &center, const float radius);
    CV_WRAP static Ptr<Region> CreateEllipse(const Point2f &center, const Size2f &size);
    CV_WRAP static Ptr<Region> CreateRotatedEllipse(const Point2f &center, const Size2f &size, const float angle);

public:
    virtual int Draw(Mat &img, const Scalar& fillColor, const Scalar& borderColor, const float borderThickness = 1, const int borderStyle = 0) const = 0;

public:
    CV_WRAP virtual bool Empty() const = 0;
    CV_WRAP virtual double Area() const = 0;
    CV_WRAP virtual cv::Point2d Centroid() const = 0;
    CV_WRAP virtual cv::Rect BoundingBox() const = 0;
    CV_WRAP virtual int Count() const = 0;
    CV_WRAP virtual int CountRow() const = 0;
    CV_WRAP virtual cv::Ptr<RegionCollection> Connect(const int connectivity) const = 0;
    CV_WRAP virtual int Connect2(const int connectivity, CV_OUT std::vector<Ptr<Region>> &regions) const = 0;
    CV_WRAP virtual int Draw(InputOutputArray img, const Scalar& fillColor, const Scalar& borderColor, const float borderThickness = 1, const int borderStyle = 0) const = 0;
};

}
}

#endif
