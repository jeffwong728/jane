#include "helper.h"
#include <cstdlib>
#include <ui/proc/rgn.h>
#include <opencv2/highgui.hpp>
#include <2geom/cairo-path-sink.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>

std::map<std::string, cv::Mat> UnitTestHelper::s_images_cache_;
std::map<std::string, std::tuple<cv::Mat, cv::Mat>> UnitTestHelper::s_gray_images_cache_;

void UnitTestHelper::ClearImagesCache()
{
    s_images_cache_.swap(std::map<std::string, cv::Mat>());
    s_gray_images_cache_.swap(std::map<std::string, std::tuple<cv::Mat, cv::Mat>>());
}

cv::Mat UnitTestHelper::GetImage(const std::experimental::filesystem::path &rPath)
{
    auto fIt = s_images_cache_.find(rPath.generic_string());
    if (fIt != s_images_cache_.end())
    {
        return fIt->second;
    }
    else
    {
        std::experimental::filesystem::path utRootDir = std::getenv("SPAM_UNITTEST_ROOT");
        utRootDir.append("idata");
        utRootDir.append(rPath);
        cv::Mat img = cv::imread(cv::String(utRootDir.string().c_str()), cv::IMREAD_UNCHANGED);
        s_images_cache_[rPath.generic_string()] = img;

        return img;
    }
}

void UnitTestHelper::WriteImage(const cv::Mat &img, const std::experimental::filesystem::path &rPath)
{
    std::experimental::filesystem::path utRootDir = std::getenv("SPAM_UNITTEST_ROOT");
    utRootDir.append("rdata");
    utRootDir.append(rPath);
    cv::imwrite(cv::String(utRootDir.string().c_str()), img);
}

std::tuple<cv::Mat, cv::Mat> UnitTestHelper::GetGrayScaleImage(const std::experimental::filesystem::path &rPath)
{
    auto fIt = s_gray_images_cache_.find(rPath.generic_string());
    if (fIt != s_gray_images_cache_.end())
    {
        return fIt->second;
    }
    else
    {
        std::experimental::filesystem::path utRootDir = std::getenv("SPAM_UNITTEST_ROOT");
        utRootDir.append("idata");
        utRootDir.append(rPath);
        cv::Mat img = cv::imread(cv::String(utRootDir.string().c_str()), cv::IMREAD_UNCHANGED);

        int dph = img.depth();
        int cnl = img.channels();

        cv::Mat colorImg;
        cv::Mat grayImg;
        if (CV_8U == dph && (1 == cnl || 3 == cnl || 4 == cnl))
        {
            if (1 == cnl)
            {
                grayImg = img;
                cv::cvtColor(img, colorImg, cv::COLOR_GRAY2BGRA);
            }
            else if (3 == cnl)
            {
                cv::cvtColor(img, colorImg, cv::COLOR_BGR2BGRA);
                cv::cvtColor(img, grayImg, cv::COLOR_BGR2GRAY);
            }
            else
            {
                colorImg = img;
                cv::cvtColor(img, grayImg, cv::COLOR_BGRA2GRAY);
            }
        }

        s_gray_images_cache_[rPath.generic_string()] = std::make_tuple(grayImg, colorImg);
        return { grayImg , colorImg };
    }
}

void UnitTestHelper::DrawPathToImage(const Geom::PathVector &pth, const Color& color, cv::Mat &img)
{
    auto data = img.ptr(0, 0);
    auto imgSurf = Cairo::ImageSurface::create(data, Cairo::Format::FORMAT_RGB24, img.cols, img.rows, static_cast<int>(img.step1()));
    auto cr = Cairo::Context::create(imgSurf);

    Geom::CairoPathSink cairoPathSink(cr->cobj());
    cairoPathSink.feed(pth);
    cr->set_source_rgba(color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, color[3] / 255.0);
    cr->set_line_width(1);
    cr->stroke();
}