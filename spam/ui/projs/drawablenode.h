#ifndef SPAM_UI_PROJS_DRAWABLE_NODE_H
#define SPAM_UI_PROJS_DRAWABLE_NODE_H
#include "modelfwd.h"
#include "modelnode.h"
#include <cairomm/cairomm.h>
#pragma warning( push )
#pragma warning( disable : 4819 4003 )
#include <2geom/path.h>
#pragma warning( pop )

class DrawableNode : public ModelNode
{
public:
    DrawableNode() : ModelNode()
    {
        ClearSelection();
        ClearHighlight();
    }
    DrawableNode(const SPModelNode &parent) : ModelNode(parent) 
    {
        ClearSelection();
        ClearHighlight();
    }
    DrawableNode(const SPModelNode &parent, const wxString &title);
    ~DrawableNode();

public:
    void ChangeColorToSelected();
    void RestoreColor();
    bool IsSelected() const { return SelectionState::kSelNone!=selData_.ss; }
    bool IsContainer() const override { return false; }
    virtual bool IsTypeOf(const SpamEntityType t) const = 0;
    virtual bool IsLegalHit(const SpamEntityOperation entityOp) const = 0;
    virtual void BuildPath(Geom::PathVector &pv) const = 0;
    virtual void BuildNode(Geom::PathVector &pv, NodeIdVector &ids, const double sx, const double sy) const = 0;
    virtual void BuildEdge(CurveVector &pth, NodeIdVector &ids) const;
    virtual void BuildHandle(Geom::PathVector &hpv) const {}
    virtual void BuildScaleHandle(const Geom::Point(&corners)[4], const double sx, const double sy, Geom::PathVector &hpv) const;
    virtual void BuildSkewHandle(const Geom::Point(&corners)[4], const double sx, const double sy, Geom::PathVector &hpv) const;
    virtual void BuildRotateHandle(const Geom::Point(&corners)[4], const double sx, const double sy, Geom::PathVector &hpv) const;
    virtual void BuildBox(const Geom::Point(&corners)[4], Geom::PathVector &bpv) const;
    virtual void BuildCorners(const Geom::PathVector &pv, Geom::Point(&corners)[4]) const;
    virtual SelectionData HitTest(const Geom::Point &pt) const = 0;
    virtual SelectionData HitTest(const Geom::Point &pt, const double sx, const double sy) const;
    virtual bool IsHitFace(const Geom::Point &pt, const Geom::PathVector &pv) const;
    virtual bool IsIntersection(const Geom::Rect &box) const = 0;
    virtual Geom::OptRect GetBoundingBox() const;
    virtual Geom::OptRect GetBoundingBox(const Geom::PathVector &pv) const;

    void StartEdit(const int toolId);
    void Edit(const int toolId, const Geom::Point &anchorPt, const Geom::Point &freePt, const Geom::Point &lastPt);
    void EndEdit(const int toolId);
    void ResetEdit(const int toolId);

    // Transform
    virtual void StartTransform();
    virtual void Transform(const Geom::Point &anchorPt, const Geom::Point &freePt, const Geom::Point &lastPt);
    virtual void EndTransform();
    virtual void ResetTransform() = 0;

    // Edit node
    virtual void StartNodeEdit();
    virtual void NodeEdit(const Geom::Point &anchorPt, const Geom::Point &freePt, const Geom::Point &lastPt);
    virtual void EndNodeEdit();
    virtual void ResetNodeEdit() = 0;

    // Modify geometry
    virtual SpamResult Modify(const Geom::Point &pt, const int editMode, const SelectionData &sd) { return SpamResult::kSR_SUCCESS_NOOP; }

    virtual boost::any CreateMemento() const = 0;
    virtual bool RestoreFromMemento(const boost::any &memento) = 0;
    void Draw(Cairo::RefPtr<Cairo::Context> &cr) const;
    void DrawHighlight(Cairo::RefPtr<Cairo::Context> &cr) const;
    void SetHighlightData(const HighlightData &hd) { hlData_ = hd; }
    HighlightData GetHighlightData() const { return hlData_; }
    void ClearSelection();
    void ClearHighlight();
    void HighlightFace();
    void Select(int toolId);
    void SwitchSelectionState(int toolId);
    void SetSelectionData(const SelectionData &selData);
    void SetColor(const wxColour &color) { drawStyle_.strokeColor_ = color; }
    wxColour GetColor() const { return drawStyle_.strokeColor_; }
    void SetFillColor(const wxColour &color) { drawStyle_.fillColor_ = color; }
    wxColour GetFillColor() const { return drawStyle_.fillColor_; }
    void SetLineWidth(const double lineWidth) { drawStyle_.strokeWidth_ = lineWidth; }
    double GetLineWidth() const { return drawStyle_.strokeWidth_; }
    cv::Ptr<cv::mvlab::Region> ToRegion() const;
    const SelectionData &GetSelectionData() const { return selData_; }
    static HighlightData MapSelectionToHighlight(const SelectionData &sd);
    static bool IsHighlightChanged(const HighlightData &hdl, const HighlightData &hdr);
    static SelectionState GetInitialSelectState(const int toolId);

protected:
    virtual void DoTransform(const Geom::Affine &aff, const double dx, const double dy) = 0;

private:
    void DrawHighlightFace(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &fpv, const double ux, const double ax) const;
    void DrawHighlightScaleHandle(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &spv, const double ux, const double ax) const;
    void DrawHighlightSkewHandle(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &spv, const double ux, const double ax) const;
    void DrawHighlightRotateHandle(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &rpv, const double ux, const double ax) const;
    void DrawHighlightNode(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &npv, const NodeIdVector &ids, const double ux, const double ax) const;
    void DrawHighlightHandle(Cairo::RefPtr<Cairo::Context> &cr, const Geom::PathVector &rpv, const HighlightState hs, const double ux, const double ax) const;
    void DrawHighlightEdge(Cairo::RefPtr<Cairo::Context> &cr, const CurveVector &pth, const double ux, const double ax) const;
    void BuildRotateMat(const Geom::Point &anchorPt, const Geom::Point &freePt, Geom::Affine &aff);
    void BuildScaleMat(const Geom::Point &anchorPt, const Geom::Point &freePt, Geom::Affine &aff);
    void BuildSkewMat(const Geom::Point &anchorPt, const Geom::Point &freePt, Geom::Affine &aff);
    void SwitchTransformState();
    void SwitchNodeEditState();

protected:
    static bool IsNeedRefresh(const Geom::Rect &bbox, const std::vector<Geom::Rect> &invalidRects)
    {
        for (const Geom::Rect &invalidRect : invalidRects)
        {
            if (invalidRect.intersects(bbox))
            {
                return true;
            }
        }

        return false;
    }

protected:
    SelectionData selData_;
    HighlightData hlData_;
    Geom::Rect  baseRect_;
    DrawStyle   baseStyle_;
};

#endif //SPAM_UI_PROJS_DRAWABLE_NODE_H