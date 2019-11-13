#include "precomp.hpp"
#include "region_bool.hpp"

namespace cv {
namespace mvlab {

void RegionBoolOp::GetRows(const RunSequence &srcRuns, const RowBeginSequence &rowBegs, UScalableIntSequence &rows)
{
    UScalableIntSequence::pointer pRow = rows.data();
    const RowBeginSequence::const_pointer pRowBegEnd = rowBegs.data() + rowBegs.size() - 1;
    for (RowBeginSequence::const_pointer pRowBeg = rowBegs.data(); pRowBeg != pRowBegEnd; ++pRowBeg, ++pRow)
    {
        *pRow = srcRuns[*pRowBeg].row;
    }
}

int RegionBoolOp::IntersectRows(UScalableIntSequence &rows1, UScalableIntSequence &rows2)
{
    UScalableIntSequence::const_pointer pRow1 = rows1.begin();
    UScalableIntSequence::const_pointer pRow2 = rows2.begin();
    const UScalableIntSequence::const_pointer pRowEnd1 = rows1.end();
    const UScalableIntSequence::const_pointer pRowEnd2 = rows2.end();
    UScalableIntSequence::pointer pResRow1 = rows1.begin();
    UScalableIntSequence::pointer pResRow2 = rows2.begin();

    int row1 = 0, row2 = 0;
    while (pRow1 != pRowEnd1 && pRow2 != pRowEnd2)
    {
        if (*pRow1 < *pRow2)
        {
            ++pRow1;
            ++row1;
        }
        else if (*pRow2 < *pRow1)
        {
            ++pRow2;
            ++row2;
        }
        else
        {
            *pResRow1 = row1;
            *pResRow2 = row2;
            ++pRow1;
            ++pRow2;
            ++row1;
            ++row2;
            ++pResRow1;
            ++pResRow2;
        }
    }

    return static_cast<int>(std::distance(rows1.begin(), pResRow1));
}

RunSequence RegionComplementOp::Do(const RunSequence &srcRuns, const RowBeginSequence &rowBegs, const cv::Rect &rcUniverse)
{
    const int numLines = rcUniverse.height + 1;
    const int numRows = static_cast<int>(rowBegs.size()) - 1;

    int numDstRuns = numLines - numRows;
    const RowBeginSequence::const_pointer pRowBegEnd = rowBegs.data() + rowBegs.size();
    for (RowBeginSequence::const_pointer pRowBeg = rowBegs.data()+1; pRowBeg != pRowBegEnd; ++pRowBeg)
    {
        numDstRuns += *pRowBeg - *(pRowBeg-1) + 1;
    }

    RunSequence dstRuns(numDstRuns);
    RunSequence::pointer pDstRun = dstRuns.data();

    int rIdx = 0;
    RowBeginSequence::const_pointer pRowBeg = rowBegs.data();
    const int lBeg = rcUniverse.y;
    const int lEnd = rcUniverse.y + rcUniverse.height;
    const RunSequence::const_pointer pRunBeg = srcRuns.data();
    const int colMin = rcUniverse.x;
    const int colMax = rcUniverse.x + rcUniverse.width + 1;
    for (int l = lBeg; l <= lEnd; ++l)
    {
        if (rIdx < numRows)
        {
            const RunSequence::const_pointer pRowRunBeg = pRunBeg + *pRowBeg;
            if (l == pRowRunBeg->row)
            {
                const RunSequence::const_pointer pRowRunEnd = pRunBeg + *(pRowBeg+1);
                int colb = colMin;
                for (RunSequence::const_pointer pRun = pRowRunBeg; pRun != pRowRunEnd; ++pRun)
                {
                    pDstRun->row = l;
                    pDstRun->colb = colb;
                    pDstRun->cole = pRun->colb;
                    pDstRun->label = 0;
                    colb = pRun->cole;

                    pDstRun += 1;
                }

                pDstRun->row = l;
                pDstRun->colb = colb;
                pDstRun->cole = colMax;
                pDstRun->label = 0;
                pDstRun += 1;

                rIdx += 1;
                pRowBeg += 1;
            }
            else
            {
                pDstRun->row = l;
                pDstRun->colb = colMin;
                pDstRun->cole = colMax;
                pDstRun->label = 0;
                pDstRun += 1;
            }
        }
        else
        {
            pDstRun->row = l;
            pDstRun->colb = colMin;
            pDstRun->cole = colMax;
            pDstRun->label = 0;
            pDstRun += 1;
        }
    }

    return dstRuns;
}

RunSequence RegionIntersectionOp::Do(const RunSequence &srcRuns1,
    const RowBeginSequence &rowBegs1,
    const RunSequence &srcRuns2,
    const RowBeginSequence &rowBegs2)
{
    UScalableIntSequence rows1(rowBegs1.size() - 1);
    UScalableIntSequence rows2(rowBegs2.size() - 1);
    GetRows(srcRuns1, rowBegs1, rows1);
    GetRows(srcRuns2, rowBegs2, rows2);

    int numResRuns = 0;
    int numInters = IntersectRows(rows1, rows2);
    for (int i = 0; i < numInters; ++i)
    {
        const int interRowIdx1 = rows1[i];
        const int interRowIdx2 = rows2[i];
        numResRuns += (rowBegs1[interRowIdx1 + 1] - rowBegs1[interRowIdx1]) + (rowBegs2[interRowIdx2 + 1] - rowBegs2[interRowIdx2]) - 1;
    }

    if (0 == numResRuns)
    {
        return RunSequence();
    }

    RunSequence dstRuns(numResRuns);
    RunSequence::const_pointer cur1 = srcRuns1.data() + rowBegs1[rows1[0]];
    RunSequence::const_pointer cur2 = srcRuns2.data() + rowBegs2[rows2[0]];
    const RunSequence::const_pointer end1 = srcRuns1.data() + rowBegs1[rows1[numInters - 1] + 1];
    const RunSequence::const_pointer end2 = srcRuns2.data() + rowBegs1[rows1[numInters - 1] + 1];

    RunSequence::pointer pResRun = dstRuns.data();
    while (cur1 != end1 && cur2 != end2)
    {
        if (cur1->row < cur2->row || (cur1->row == cur2->row && cur1->cole <= cur2->colb))
        {
            ++cur1;
        }
        else if (cur2->row < cur1->row || (cur1->row == cur2->row && cur2->cole <= cur1->colb))
        {
            ++cur2;
        }
        else
        {
            pResRun->row = cur1->row;
            pResRun->colb = std::max(cur1->colb, cur2->colb);
            pResRun->cole = std::min(cur1->cole, cur2->cole);
            pResRun->label = 0;
            ++pResRun;
            if (cur1->cole < cur2->cole)
            {
                ++cur1;
            }
            else
            {
                ++cur2;
            }
        }
    }

    numResRuns = static_cast<int>(std::distance(dstRuns.data(), pResRun));
    assert(std::distance(dstRuns.data(), pResRun) <= dstRuns.size());
    dstRuns.resize(numResRuns);

    return dstRuns;
}

RunSequence RegionIntersectionOp::Do2(const RunSequence &srcRuns1, const RunSequence &srcRuns2)
{
    if (srcRuns1.empty() || srcRuns2.empty())
    {
        return RunSequence();
    }

    RunSequence dstRuns(srcRuns1.size() + srcRuns2.size());
    RunSequence::const_pointer cur1 = srcRuns1.data();
    RunSequence::const_pointer cur2 = srcRuns2.data();
    const RunSequence::const_pointer end1 = srcRuns1.data() + srcRuns1.size();
    const RunSequence::const_pointer end2 = srcRuns2.data() + srcRuns2.size();

    RunSequence::pointer pResRun = dstRuns.data();
    while (cur1 != end1 && cur2 != end2)
    {
        if (cur1->row < cur2->row || (cur1->row == cur2->row && cur1->cole <= cur2->colb))
        {
            ++cur1;
        }
        else if (cur2->row < cur1->row || (cur1->row == cur2->row && cur2->cole <= cur1->colb))
        {
            ++cur2;
        }
        else
        {
            pResRun->row = cur1->row;
            pResRun->colb = std::max(cur1->colb, cur2->colb);
            pResRun->cole = std::min(cur1->cole, cur2->cole);
            pResRun->label = 0;
            ++pResRun;
            if (cur1->cole < cur2->cole)
            {
                ++cur1;
            }
            else
            {
                ++cur2;
            }
        }
    }

    assert(std::distance(dstRuns.data(), pResRun) <= dstRuns.size());
    dstRuns.resize(std::distance(dstRuns.data(), pResRun));

    return dstRuns;
}

RunSequence RegionUnion2Op::Do(const RunSequence &srcRuns1, const RunSequence &srcRuns2)
{
    if (srcRuns1.empty())
    {
        return srcRuns2;
    }

    if (srcRuns2.empty())
    {
        return srcRuns1;
    }

    RunSequence dstRuns(srcRuns1.size()+ srcRuns2.size());
    RunSequence::const_pointer cur1 = srcRuns1.data();
    RunSequence::const_pointer cur2 = srcRuns2.data();
    const RunSequence::const_pointer end1 = srcRuns1.data() + srcRuns1.size();
    const RunSequence::const_pointer end2 = srcRuns2.data() + srcRuns2.size();

    RunSequence::pointer pRevRun = dstRuns.data();
    RunSequence::pointer pResRun = dstRuns.data();
    if (cur1->row < cur2->row)
    {
        pResRun->row = cur1->row; pResRun->colb = cur1->colb; pResRun->cole = cur1->cole; pResRun->label = 0;
        ++pResRun;
        ++cur1;
    }
    else if (cur2->row < cur1->row)
    {
        pResRun->row  = cur2->row; pResRun->colb = cur2->colb; pResRun->cole = cur2->cole; pResRun->label = 0;
        ++pResRun;
        ++cur2;
    }
    else
    {
        if (cur1->colb < cur2->colb)
        {
            pResRun->row = cur1->row; pResRun->colb = cur1->colb; pResRun->cole = cur1->cole; pResRun->label = 0;
            ++pResRun;
            ++cur1;
        }
        else if(cur2->colb < cur2->colb)
        {
            pResRun->row = cur2->row; pResRun->colb = cur2->colb; pResRun->cole = cur2->cole; pResRun->label = 0;
            ++pResRun;
            ++cur2;
        }
        else
        {
            if (cur1->cole < cur2->cole)
            {
                pResRun->row = cur1->row; pResRun->colb = cur1->colb; pResRun->cole = cur1->cole; pResRun->label = 0;
                ++pResRun;
                ++cur1;
            }
            else
            {
                pResRun->row = cur2->row; pResRun->colb = cur2->colb; pResRun->cole = cur2->cole; pResRun->label = 0;
                ++pResRun;
                ++cur2;
            }
        }
    }

    while (cur1 != end1 && cur2 != end2)
    {
        RunSequence::const_pointer pNewRun = cur1;
        if (cur1->row < cur2->row)
        {
            ++cur1;
        }
        else if (cur2->row < cur1->row)
        {
            pNewRun = cur2;
            ++cur2;
        }
        else if (cur1->colb < cur2->colb)
        {
            ++cur1;
        }
        else if (cur2->colb < cur1->colb)
        {
            pNewRun = cur2;
            ++cur2;
        }
        else
        {
            ++cur1;
        }

        if (pNewRun->row == pRevRun->row && pNewRun->colb <= pRevRun->cole)
        {
            pRevRun->cole = std::max(pNewRun->cole, pRevRun->cole);
        }
        else
        {
            pResRun->row = pNewRun->row; pResRun->colb = pNewRun->colb; pResRun->cole = pNewRun->cole; pResRun->label = 0;
            pRevRun = pResRun;
            ++pResRun;
        }
    }

    for (; cur1 != end1; ++cur1, ++pResRun)
    {
        pResRun->row = cur1->row; pResRun->colb = cur1->colb; pResRun->cole = cur1->cole; pResRun->label = 0;
    }

    for (; cur2 != end2; ++cur2, ++pResRun)
    {
        pResRun->row = cur2->row; pResRun->colb = cur2->colb; pResRun->cole = cur2->cole; pResRun->label = 0;
    }

    assert(std::distance(dstRuns.data(), pResRun) <= dstRuns.size());
    dstRuns.resize(std::distance(dstRuns.data(), pResRun));

    return dstRuns;
}
}
}