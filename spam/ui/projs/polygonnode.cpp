#include "polygonnode.h"
#include <ui/evts.h>
#include <helper/h5db.h>
#include <helper/commondef.h>
#pragma warning( push )
#pragma warning( disable : 4819 4003 )
#include <2geom/2geom.h>
#include <2geom/path-sink.h>
#include <2geom/path-intersection.h>
#include <2geom/svg-path-parser.h>
#pragma warning( pop )
#include <algorithm>

PolygonNode::PolygonNode(const SPModelNode &parent, const wxString &title)
    : GeomNode(parent, title)
{
    Geom::Affine ide = Geom::Affine::identity();
    for (int i = 0; i < data_.transform.size(); ++i)
    {
        data_.transform[i] = ide[i];
    }
}

PolygonNode::~PolygonNode()
{
}

bool PolygonNode::IsTypeOf(const SpamEntityType t) const
{
    switch (t)
    {
    case SpamEntityType::kET_GEOM:
    case SpamEntityType::kET_GEOM_POLYGON:
        return true;

    default: return false;
    }
}

bool PolygonNode::IsLegalHit(const SpamEntityOperation entityOp) const
{
    switch (entityOp)
    {
    case SpamEntityOperation::kEO_GENERAL:
    case SpamEntityOperation::kEO_GEOM_CREATE:
    case SpamEntityOperation::kEO_GEOM_TRANSFORM:
    case SpamEntityOperation::kEO_VERTEX_MOVE:
        return true;

    case SpamEntityOperation::kEO_VERTEX_ADD:
        if (hlData_.hls == HighlightState::kHlEdge)
        {
            return true;
        }
        else
        {
            return false;
        }

    case SpamEntityOperation::kEO_VERTEX_DELETE:
        if (hlData_.hls == HighlightState::kHlNode &&
            hlData_.subid == 0 && GetNumCorners()>3)
        {
            return true;
        }
        else
        {
            return false;
        }

    default:
        return false;
    }
}

void PolygonNode::BuildPath(Geom::PathVector &pv) const
{
    Geom::PathBuilder pb(pv);
    if (GetNumCorners()>0)
    {
        pb.moveTo(Geom::Point(data_.points.front()[0], data_.points.front()[1]));
        for (int c = 1; c<GetNumCorners(); ++c)
        {
            pb.lineTo(Geom::Point(data_.points[c][0], data_.points[c][1]));
        }

        if (GetNumCorners()>2)
        {
            pb.closePath();
        }
        else
        {
            pb.flush();
        }
    }
}

void PolygonNode::BuildNode(Geom::PathVector &pv, NodeIdVector &ids, const double sx, const double sy) const
{
    if (selData_.ss == SelectionState::kSelNodeEdit)
    {
        for (int c = 0; c<GetNumCorners(); ++c)
        {
            Geom::Point pt{ data_.points[c][0], data_.points[c][1] };
            Geom::Point a = pt + Geom::Point(-3*sx, -3*sx);
            Geom::Point b = pt + Geom::Point(3*sx, 3*sx);
            pv.push_back(Geom::Path(Geom::Rect(a, b)));
            ids.push_back({ c, 0 });
        }
    }
    pv *= Geom::Translate(0.5, 0.5);
}

void PolygonNode::BuildEdge(CurveVector &pth, NodeIdVector &ids) const
{
    if (selData_.ss == SelectionState::kSelNodeEdit)
    {
        int numEdges = GetNumCorners();
        for (int e = 0; e<numEdges; ++e)
        {
            Geom::Point pts{ data_.points[e][0], data_.points[e][1] };
            Geom::Point pte{ data_.points[(e+1) % numEdges][0], data_.points[(e + 1) % numEdges][1] };

            pth.push_back(std::make_unique<Geom::LineSegment>(pts, pte));
            ids.push_back({ e, 0 });
        }
    }
    DrawableNode::BuildEdge(pth, ids);
}

SelectionData PolygonNode::HitTest(const Geom::Point &pt) const
{
    SelectionData sd{ selData_.ss , HitState ::kHsNone, -1, -1};
    Geom::PathVector pv;
    BuildPath(pv);

    if (Geom::contains(pv.front(), pt))
    {
        sd.hs    = HitState::kHsFace;
        sd.id    = 0;
        sd.subid = 0;
        sd.master = 0;
    }

    return sd;
}

SelectionData PolygonNode::HitTest(const Geom::Point &pt, const double sx, const double sy) const
{
    return DrawableNode::HitTest(pt, sx, sy);
}

bool PolygonNode::IsIntersection(const Geom::Rect &box) const
{
    Geom::PathVector pv;
    BuildPath(pv);

    Geom::OptRect rect = pv.boundsFast();
    if (rect)
    {
        return box.contains(rect);
    }

    return false;
}

void PolygonNode::StartTransform()
{
    base_ = data_;
    DrawableNode::StartTransform();
}

void PolygonNode::EndTransform()
{
    base_.points.clear();
    Geom::Affine ide = Geom::Affine::identity();
    for (int i = 0; i < data_.transform.size(); ++i)
    {
        base_.transform[i] = ide[i];
    }

    DrawableNode::EndTransform();
}

void PolygonNode::ResetTransform()
{
    data_ = base_;
    PolygonNode::EndTransform();
}

void PolygonNode::NodeEdit(const Geom::Point &anchorPt, const Geom::Point &freePt, const Geom::Point &lastPt)
{
    Geom::Coord dx = freePt.x() - lastPt.x();
    Geom::Coord dy = freePt.y() - lastPt.y();
    if (HitState::kHsNode == selData_.hs)
    {
        if (selData_.id>-1 && selData_.id<GetNumCorners())
        {
            data_.points[selData_.id][0] += dx;
            data_.points[selData_.id][1] += dy;
        }
    }
    else if (HitState::kHsEdge == selData_.hs)
    {
        int numCorners = GetNumCorners();
        if (selData_.id>-1 && selData_.id<numCorners)
        {
            data_.points[selData_.id][0] += dx;
            data_.points[selData_.id][1] += dy;

            int idx = (selData_.id + 1) % numCorners;
            data_.points[idx][0] += dx;
            data_.points[idx][1] += dy;
        }
    }
    else
    {
        Geom::Affine aff = Geom::Affine::identity();
        aff *= Geom::Translate(dx, dy);
        PolygonNode::DoTransform(aff, dx, dy);
    }
}

void PolygonNode::ResetNodeEdit()
{
}

SpamResult PolygonNode::Modify(const Geom::Point &pt, const int editMode, const SelectionData &sd)
{
    if (SelectionState::kSelNone == selData_.ss)
    {
        return SpamResult::kSR_SUCCESS_NOOP;
    }

    if (kSpamID_TOOLBOX_NODE_ADD == editMode && HitState::kHsEdge==sd.hs)
    {
        int numEdges = GetNumCorners();
        if (sd.id<numEdges && sd.id>=0)
        {
            Geom::Point pts{ data_.points[sd.id][0], data_.points[sd.id][1] };
            Geom::Point pte{ data_.points[(sd.id + 1) % numEdges][0], data_.points[(sd.id + 1) % numEdges][1] };

            Geom::LineSegment edge(pts, pte);
            Geom::Coord t = edge.nearestTime(pt);
            Geom::Point nPt = edge.pointAt(t);
            std::array<double, 2> pt{ nPt.x(), nPt.y()};
            data_.points.insert(data_.points.begin()+sd.id+1, pt);

            return SpamResult::kSR_SUCCESS;
        }
    }
    else if (kSpamID_TOOLBOX_NODE_DELETE == editMode && HitState::kHsNode == sd.hs)
    {
        int numEdges = GetNumCorners();
        if (sd.id < numEdges && sd.id >= 0 && numEdges>3)
        {
            data_.points.erase(data_.points.begin() + sd.id);
            return SpamResult::kSR_SUCCESS;
        }
    }
    else
    {
        return SpamResult::kSR_FAILURE;
    }

    return SpamResult::kSR_FAILURE;
}

boost::any PolygonNode::CreateMemento() const
{
    auto mem = std::make_shared<Memento>();
    mem->style   = drawStyle_;
    mem->data    = data_;
    mem->rank    = rank_;
    mem->visible = visible_;
    mem->locked  = locked_;

    return mem;
}

bool PolygonNode::RestoreFromMemento(const boost::any &memento)
{
    try
    {
        auto mem = boost::any_cast<std::shared_ptr<Memento>>(memento);

        if (mem)
        {
            drawStyle_ = mem->style;
            data_      = mem->data;
            rank_      = mem->rank;
            visible_   = mem->visible;
            locked_    = mem->locked;
        }

        return true;
    }
    catch (const boost::bad_any_cast &)
    {
        return false;
    }
}

void PolygonNode::AddCorner(const Geom::Point &pt)
{
    data_.points.push_back({ pt.x(), pt.y() });
}

void PolygonNode::PopCorner()
{
    data_.points.pop_back();
}

void PolygonNode::GetCorner(int pos, Geom::Point &corner) const
{
    corner.x() = data_.points[pos][0];
    corner.y() = data_.points[pos][1];
}

void PolygonNode::BuildOpenPath(Geom::PathVector &pv)
{
    Geom::PathBuilder pb(pv);
    if (GetNumCorners()>0)
    {
        pb.moveTo(Geom::Point(data_.points.front()[0], data_.points.front()[1]));
        for (int c = 1; c<GetNumCorners(); ++c)
        {
            pb.lineTo(Geom::Point(data_.points[c][0], data_.points[c][1]));
        }

        pb.flush();
    }
}

void PolygonNode::Save(const H5::Group &g) const
{
    std::string utf8Title(title_.ToUTF8().data());
    if (g.nameExists(utf8Title))
    {
        H5Ldelete(g.getId(), utf8Title.data(), H5P_DEFAULT);
    }

    if (!g.nameExists(utf8Title))
    {
        H5::LinkCreatPropList lcpl;
        lcpl.setCharEncoding(H5T_CSET_UTF8);
        H5::Group cwg = g.createGroup(title_, lcpl);
        H5DB::SetAttribute(cwg, CommonDef::GetSpamDBNodeTypeAttrName(), GetTypeName());

        H5DB::Save(cwg, std::string("Points"),    data_.points);
        H5DB::Save(cwg, std::string("Transform"), data_.transform);

        ModelNode::Save(cwg);
    }
}

void PolygonNode::Load(const H5::Group &g, const NodeFactory &nf, const SPModelNode &me)
{
    H5DB::Load(g, std::string("Points"),    data_.points);
    H5DB::Load(g, std::string("Transform"), data_.transform);

    ModelNode::Load(g, nf, me);
}

void PolygonNode::DoTransform(const Geom::Affine &aff, const double dx, const double dy)
{
    if (HitState::kHsFace == selData_.hs)
    {
        for (auto &pt : data_.points)
        {
            pt[0] += dx;
            pt[1] += dy;
        }
    }
    else
    {
        if (!aff.isIdentity())
        {
            for (int i = 0; i<static_cast<int>(base_.points.size()); ++i)
            {
                Geom::Point pt2{ base_.points[i][0], base_.points[i][1] };
                pt2 *= aff;
                data_.points[i][0] = pt2.x();
                data_.points[i][1] = pt2.y();
            }
        }
    }
}