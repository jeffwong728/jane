#ifndef __OPENCV_MVLAB_REGION_BOOL_HPP__
#define __OPENCV_MVLAB_REGION_BOOL_HPP__

#include "region_impl.hpp"

namespace cv {
namespace mvlab {
struct RegionBoolOp
{
    static void GetRows(const RunSequence &srcRuns, const RowBeginSequence &rowBegs, UScalableIntSequence &rows);
    static int  IntersectRows(UScalableIntSequence &rows1, UScalableIntSequence &rows2);
};

struct RegionComplementOp : public RegionBoolOp
{
    RunSequence Do(const RunSequence &srcRuns, const RowBeginSequence &rowBegs, const cv::Rect &rcUniverse);
};

struct RegionDifferenceOp : public RegionBoolOp
{
};

struct RegionIntersectionOp : public RegionBoolOp
{
    RunSequence Do(const RunSequence &srcRuns1, const RowBeginSequence &rowBegs1, const RunSequence &srcRuns2, const RowBeginSequence &rowBegs2);
    RunSequence Do2(const RunSequence &srcRuns1, const RunSequence &srcRuns2);
};

struct RegionSymmDifferenceOp : public RegionBoolOp
{
};

struct RegionUnion2Op : public RegionBoolOp
{
    RunSequence Do(const RunSequence &srcRuns1, const RunSequence &srcRuns2);
};

}
}

#endif //__OPENCV_MVLAB_REGION_BOOL_HPP__