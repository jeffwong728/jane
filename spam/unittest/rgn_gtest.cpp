#include "helper.h"
#include <cassert>
#include <opencv2/highgui.hpp>
#include <gtest/gtest.h>
#include <2geom/rect.h>
#include <2geom/circle.h>
#include <2geom/path-intersection.h>
#include <ui/proc/rgn.h>
#include <ui/proc/basic.h>
#include <tbb/tbb.h>

namespace
{
TEST(RgnAreaTest, Empty)
{
    SpamRgn rgn;
    EXPECT_DOUBLE_EQ(rgn.Area(), 0.0);
}

TEST(RgnAreaTest, SingleRun)
{
    SpamRgn rgn;
    rgn.AddRun(1, 2, 5);
    EXPECT_DOUBLE_EQ(rgn.Area(), 3.0);
}

TEST(RgnCentroidTest, Empty)
{
    SpamRgn rgn;
    EXPECT_DOUBLE_EQ(rgn.Centroid().x, 0.0);
    EXPECT_DOUBLE_EQ(rgn.Centroid().y, 0.0);
}

TEST(RgnCentroidTest, SingleRun)
{
    SpamRgn rgn;
    rgn.AddRun(1, 2, 5);
    EXPECT_DOUBLE_EQ(rgn.Centroid().x, 3.0);
    EXPECT_DOUBLE_EQ(rgn.Centroid().y, 1.0);
}

TEST(RgnCentroidTest, TwoRuns)
{
    SpamRgn rgn;
    rgn.AddRun(0, 0, 2);
    rgn.AddRun(1, 0, 2);
    EXPECT_DOUBLE_EQ(rgn.Centroid().x, 0.5);
    EXPECT_DOUBLE_EQ(rgn.Centroid().y, 0.5);
}

TEST(RgnCentroidTest, MultipleRuns)
{
    SpamRgn rgn;
    rgn.AddRun(1, 2, 3);
    rgn.AddRun(2, 1, 4);
    rgn.AddRun(3, 0, 5);
    EXPECT_DOUBLE_EQ(rgn.Centroid().x, 2.0);
    EXPECT_DOUBLE_EQ(rgn.Centroid().y, 22.0/9);
}

TEST(PointInCircleTest, Positive)
{
    Geom::Path pth(Geom::Circle(Geom::Point(2, 2), 1));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(2, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(1, 2)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(2, 2)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(3, 2)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(2, 3)));
}

TEST(PointInCircleTest, Odd)
{
    Geom::Path pth(Geom::Circle(Geom::Point(3, 3), 1));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(3, 2)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(2, 3)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(3, 3)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(4, 3)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(3, 4)));
}

TEST(PointInCircleTest, Zero)
{
    Geom::Path pth(Geom::Circle(Geom::Point(0, 0), 1));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(0, -1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(-1, 0)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(0, 0)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(1, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(0, 1)));
}

TEST(PointInRectangleTest, Zero)
{
    Geom::Path pth(Geom::Rect(Geom::Point(0, 0), Geom::Point(1, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(0, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(0, 1)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, 1)));
}

TEST(PointInRectangleTest, Positive)
{
    Geom::Path pth(Geom::Rect(Geom::Point(1, 1), Geom::Point(3, 3)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(1, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(1, 2)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, 3)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(2, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(2, 2)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(2, 3)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(3, 1)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(3, 2)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(3, 3)));
}

TEST(PointInRectangleTest, Negative)
{
    Geom::Path pth(Geom::Rect(Geom::Point(-1, -1), Geom::Point(1, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(-1, -1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(-1, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(-1, 1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(0, -1)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(0, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(0, 1)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, -1)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, 0)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(1, 1)));
}

TEST(PointInRectangleTest, Big)
{
    Geom::Path pth(Geom::Rect(Geom::Point(-1, -1), Geom::Point(60, 57)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(60, -1)));
    EXPECT_FALSE(Geom::contains(pth, Geom::Point(60, 0)));
    EXPECT_TRUE(Geom::contains(pth, Geom::Point(60, 1)));
}

TEST(RgnCentroidTest, Circle)
{
    SpamRgn rgn0;
    rgn0.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(0, 0), 50))));
    EXPECT_NEAR(rgn0.Centroid().x, 0.0, 0.01);
    EXPECT_NEAR(rgn0.Centroid().y, 0.0, 0.01);

    SpamRgn rgn1;
    rgn1.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(2, 2), 50))));
    EXPECT_NEAR(rgn1.Centroid().x, 2.0, 0.01);
    EXPECT_NEAR(rgn1.Centroid().y, 2.0, 0.01);

    SpamRgn rgn2;
    rgn2.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(-2, -2), 50))));
    EXPECT_NEAR(rgn2.Centroid().x, -2.0, 0.01);
    EXPECT_NEAR(rgn2.Centroid().y, -2.0, 0.01);

    SpamRgn rgn3;
    rgn3.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(10, 10), 50))));
    EXPECT_NEAR(rgn3.Centroid().x, 10.0, 0.01);
    EXPECT_NEAR(rgn3.Centroid().y, 10.0, 0.01);
}

TEST(RgnCentroidTest, Rectangle)
{
    SpamRgn rgn0;
    rgn0.SetRegion(Geom::PathVector(Geom::Path(Geom::Rect(Geom::Point(0, 0), Geom::Point(1, 1)))));
    EXPECT_DOUBLE_EQ(rgn0.Centroid().x, 0.0);
    EXPECT_DOUBLE_EQ(rgn0.Centroid().y, 0.0);

    SpamRgn rgn1;
    rgn1.SetRegion(Geom::PathVector(Geom::Path(Geom::Rect(Geom::Point(1, 1), Geom::Point(3, 3)))));
    EXPECT_DOUBLE_EQ(rgn1.Centroid().x, 1.5);
    EXPECT_DOUBLE_EQ(rgn1.Centroid().y, 1.5);

    SpamRgn rgn2;
    rgn2.SetRegion(Geom::PathVector(Geom::Path(Geom::Rect(Geom::Point(-1, -1), Geom::Point(1, 1)))));
    EXPECT_DOUBLE_EQ(rgn2.Centroid().x, -0.5);
    EXPECT_DOUBLE_EQ(rgn2.Centroid().y, -0.5);

    SpamRgn rgn3;
    rgn3.SetRegion(Geom::PathVector(Geom::Path(Geom::Rect(Geom::Point(-1, -2), Geom::Point(20, 10)))));
    EXPECT_DOUBLE_EQ(rgn3.Centroid().x, 9);
    EXPECT_DOUBLE_EQ(rgn3.Centroid().y, 3.5);
}

TEST(RgnMinCircleTest, Empty)
{
    SpamRgn rgn;
    Geom::Circle c = rgn.MinCircle();
    EXPECT_TRUE(c.isDegenerate());
}

TEST(RgnMinCircleTest, Single)
{
    SpamRgn rgn;
    rgn.AddRun(1, 1, 2);
    Geom::Circle c = rgn.MinCircle();
    EXPECT_DOUBLE_EQ(c.center(Geom::X), 1);
    EXPECT_DOUBLE_EQ(c.center(Geom::Y), 1);
}

TEST(RgnMinCircleTest, Circle)
{
    SpamRgn rgn;
    rgn.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(10, 20), 50))));

    Geom::Circle c = rgn.MinCircle();
    EXPECT_NEAR(c.radius(), 50, 1e-3);
    EXPECT_DOUBLE_EQ(c.center(Geom::X), 10);
    EXPECT_DOUBLE_EQ(c.center(Geom::Y), 20);
}

TEST(RgnMinCircleTest, Rectangle)
{
    SpamRgn rgn;
    for (int8_t row = 1; row < 100; ++row)
    {
        rgn.AddRun(row, 10, 21);
    }

    Geom::Rect rect(Geom::Point(10, 1), Geom::Point(20, 99));
    Geom::Circle c = rgn.MinCircle();
    EXPECT_NEAR(c.radius(), rect.diameter() / 2, 1e-3);
    EXPECT_DOUBLE_EQ(c.center(Geom::X), 15);
    EXPECT_DOUBLE_EQ(c.center(Geom::Y), 50);
}

TEST(RgnMinCircleTest, RotatedRect)
{
    Geom::Rect rect(Geom::Point(-1, -2), Geom::Point(20, 10));
    Geom::Path pth(rect);
    Geom::PathVector pv(pth);
    pv *= Geom::Translate(-9.5, -4) * Geom::Rotate::from_degrees(60) * Geom::Translate(9.5, 4);

    SpamRgn rgn;
    rgn.SetRegion(pv);

    Geom::Circle c = rgn.MinCircle();
    EXPECT_NEAR(c.radius(), rect.diameter()/2, 1);
    EXPECT_DOUBLE_EQ(c.center(Geom::X), 9.5);
    EXPECT_DOUBLE_EQ(c.center(Geom::Y), 4);
}

TEST(RgnPyramidTest, Circle)
{
    Geom::PathVector pv(Geom::Path(Geom::Circle(Geom::Point(100, 200), 50)));
    SpamRgn rgn0(pv*Geom::Scale(0.5, 0.5));

    cv::Point2d cg = SpamRgn(pv).Centroid();
    SpamRgn rgn1(pv * Geom::Translate(-cg.x, -cg.y)*Geom::Scale(0.5, 0.5)*Geom::Translate(cg.x*0.5, cg.y*0.5));

    EXPECT_DOUBLE_EQ(rgn0.Centroid().x, rgn1.Centroid().x);
    EXPECT_DOUBLE_EQ(rgn0.Centroid().y, rgn1.Centroid().y);
}

TEST(RgnPyramidTest, Rectangle)
{
    Geom::PathVector pv(Geom::Path(Geom::Rect(Geom::Point(-10, 18), Geom::Point(100, 90))));
    SpamRgn rgn0(pv*Geom::Scale(0.5, 0.5));

    cv::Point2d cg = SpamRgn(pv).Centroid();
    SpamRgn rgn1(pv * Geom::Translate(-cg.x, -cg.y)*Geom::Scale(0.5, 0.5)*Geom::Translate(cg.x*0.5, cg.y*0.5));

    EXPECT_DOUBLE_EQ(rgn0.Centroid().x, rgn1.Centroid().x);
    EXPECT_DOUBLE_EQ(rgn0.Centroid().y, rgn1.Centroid().y);
}

TEST(RgnPyramidTest, Polygon)
{
    Geom::PathVector pv;
    Geom::PathBuilder pb(pv);
    pb.moveTo(Geom::Point(10, 10));
    pb.lineTo(Geom::Point(5, 20));
    pb.lineTo(Geom::Point(30, 50));
    pb.lineTo(Geom::Point(60, 20));
    pb.lineTo(Geom::Point(50, 5));
    pb.closePath();

    SpamRgn rgn0(pv*Geom::Scale(0.5, 0.5));

    cv::Point2d cg = SpamRgn(pv).Centroid();
    SpamRgn rgn1(pv * Geom::Translate(-cg.x, -cg.y)*Geom::Scale(0.5, 0.5)*Geom::Translate(cg.x*0.5, cg.y*0.5));

    EXPECT_DOUBLE_EQ(rgn0.Centroid().x, rgn1.Centroid().x);
    EXPECT_DOUBLE_EQ(rgn0.Centroid().y, rgn1.Centroid().y);
}

TEST(RgnPyramidTest, Disjoint)
{
    Geom::PathVector pv;
    Geom::PathBuilder pb(pv);
    pb.append(Geom::Path(Geom::Circle(Geom::Point(100, 200), 30)));
    pb.closePath();
    pb.append(Geom::Path(Geom::Rect(Geom::Point(-10, 18), Geom::Point(30, 30))));
    pb.closePath();

    SpamRgn rgn0(pv*Geom::Scale(0.5, 0.5));

    cv::Point2d cg = SpamRgn(pv).Centroid();
    SpamRgn rgn1(pv * Geom::Translate(-cg.x, -cg.y)*Geom::Scale(0.5, 0.5)*Geom::Translate(cg.x*0.5, cg.y*0.5));

    EXPECT_DOUBLE_EQ(rgn0.Centroid().x, rgn1.Centroid().x);
    EXPECT_DOUBLE_EQ(rgn0.Centroid().y, rgn1.Centroid().y);
}

TEST(PointSetTest, Circle)
{
    SpamRgn maskRgn(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(32, 24), 20))));
    PointSet maskPoints(maskRgn);

    cv::Mat bkImg(48, 64, CV_8UC1, cv::Scalar());
    for (const cv::Point &pt : maskPoints)
    {
        bkImg.at<uint8_t>(pt) = 0xFF;
    }

    SpamRgn rgn;
    rgn.SetRegion(Geom::PathVector(Geom::Path(Geom::Circle(Geom::Point(32, 24), 20))), std::vector<uint8_t>());
    EXPECT_DOUBLE_EQ(rgn.Centroid().x, 32.0);
    EXPECT_DOUBLE_EQ(rgn.Centroid().y, 24.0);
}

TEST(BasicImgProcTest, Transform)
{
    cv::Mat img(480, 640, CV_8UC1, cv::Scalar());
    for (int row = 0; row < 480; row += 30)
    {
        for (int col = 0; col < 640;col += 30)
        {
            cv::drawMarker(img, cv::Point(col, row), CV_RGB(255, 255, 255), cv::MARKER_CROSS, 10, 1);
        }
    }

    cv::Mat invMat;
    cv::Point2f pivPt{ 240, 320 };
    cv::Mat rotMat = cv::getRotationMatrix2D(pivPt, 30, 1.0);
    cv::invertAffineTransform(rotMat, invMat);

    cv::Mat dst;
    BasicImgProc::Transform(img, dst, invMat, cv::Rect(0, 0, 640, 480));
    UnitTestHelper::WriteImage(img, "transform_mask_src.png");
    UnitTestHelper::WriteImage(dst, "transform_mask_dst.png");

    cv::warpAffine(img, dst, rotMat, cv::Size(dst.cols, dst.rows), cv::INTER_LINEAR);
    UnitTestHelper::WriteImage(dst, "transform_cv_dst.png");
}

}