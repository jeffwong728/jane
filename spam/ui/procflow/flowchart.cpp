#include "flowchart.h"
#include "stepbase.h"
#include <wx/artprov.h>
#include <wx/display.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/dnd.h>
#include <wx/log.h>
#include <algorithm>
#pragma warning( push )
#pragma warning( disable : 4819 4003 )
#include <2geom/2geom.h>
#pragma warning( pop )

#include "initstep.h"
#include "endstep.h"
#include "cvtstep.h"
#include "threshstep.h"

class DnDText : public wxTextDropTarget
{
public:
    DnDText(FlowChart *pOwner) { m_pOwner = pOwner; }

    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text) wxOVERRIDE;

private:
    FlowChart *m_pOwner;
};

bool DnDText::OnDropText(wxCoord x, wxCoord y, const wxString& text)
{
    m_pOwner->AppendStep(x, y, text);
    return true;
}

struct FlowChartState
{
    FlowChartState(FlowChart *const chartCtrl) : flowChart(chartCtrl) {}
    virtual ~FlowChartState() {}
    virtual void OnLeftMouseDown(wxMouseEvent &e) {}
    virtual void OnMouseMove(wxMouseEvent &e) {}
    virtual void OnLeftMouseUp(wxMouseEvent &e) {}
    virtual void Enter() {}
    virtual void Exit() {}
    FlowChart *const flowChart;
};

struct FlowChartContext
{
    FlowChartContext(FlowChart *const chartCtrl) : flowChart(chartCtrl) {}
    virtual ~FlowChartContext() {}
    FlowChart *const flowChart;
};

struct FreeStateContext : public FlowChartContext
{
    FreeStateContext(FlowChart *const chartCtrl) : FlowChartContext(chartCtrl) {}

    void StartDraging(wxMouseEvent &e)
    {
        anchorPos = e.GetPosition();
        lastPos = e.GetPosition();
    }

    void Draging(wxMouseEvent &e)
    {
        const wxPoint thisPos = e.GetPosition();
        const wxPoint oldMinPos(std::min(anchorPos.x, lastPos.x), std::min(anchorPos.y, lastPos.y));
        const wxPoint oldMaxPos(std::max(anchorPos.x, lastPos.x), std::max(anchorPos.y, lastPos.y));
        const wxPoint newMinPos(std::min(anchorPos.x, thisPos.x), std::min(anchorPos.y, thisPos.y));
        const wxPoint newMaxPos(std::max(anchorPos.x, thisPos.x), std::max(anchorPos.y, thisPos.y));
        const wxRect  oldRect(oldMinPos, oldMaxPos);
        const wxRect  newRect(newMinPos, newMaxPos);
        flowChart->DrawRubberBand(oldRect, newRect);
        lastPos = thisPos;
    }

    void EndDraging(wxMouseEvent &e)
    {
        const wxPoint thisPos = e.GetPosition();
        const wxPoint oldMinPos(std::min(anchorPos.x, lastPos.x), std::min(anchorPos.y, lastPos.y));
        const wxPoint oldMaxPos(std::max(anchorPos.x, lastPos.x), std::max(anchorPos.y, lastPos.y));
        const wxRect  oldRect(oldMinPos, oldMaxPos);
        flowChart->DrawRubberBand(oldRect, wxRect());
        lastPos = wxPoint();
        anchorPos = wxPoint();
    }

    wxPoint anchorPos;
    wxPoint lastPos;
};

struct FreeIdleState : public FlowChartState
{
    FreeIdleState(FlowChart *const chartCtrl, FreeStateContext *const ctx) : FlowChartState(chartCtrl), context(ctx)
    {
    }

    void Enter() wxOVERRIDE { wxLogMessage(wxT("FreeIdleState Enter.")); }
    void Exit() wxOVERRIDE { wxLogMessage(wxT("FreeIdleState Quit.")); }

    void OnLeftMouseDown(wxMouseEvent &e) wxOVERRIDE
    {
        flowChart->SwitchState(FlowChart::kStateFreeDraging);
        context->StartDraging(e);
    }

    FreeStateContext *const context;
};

struct FreeDragingState : public FlowChartState
{
    FreeDragingState(FlowChart *const chartCtrl, FreeStateContext *const ctx) : FlowChartState(chartCtrl), context(ctx)
    {}

    void Enter() wxOVERRIDE { wxLogMessage(wxT("FreeDragingState Enter.")); }
    void Exit() wxOVERRIDE { wxLogMessage(wxT("FreeDragingState Quit.")); }

    void OnMouseMove(wxMouseEvent &e) wxOVERRIDE
    {
        context->Draging(e);
    }

    void OnLeftMouseUp(wxMouseEvent &e) wxOVERRIDE
    {
        context->EndDraging(e);
        flowChart->SwitchState(FlowChart::kStateFreeIdle);
    }

    FreeStateContext *const context;
};

FlowChart::FlowChart(wxWindow* parent)
{
    wxWindow::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_RAISED);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    InheritAttributes();

    Bind(wxEVT_ENTER_WINDOW, &FlowChart::OnEnterWindow,     this, wxID_ANY);
    Bind(wxEVT_LEAVE_WINDOW, &FlowChart::OnLeaveWindow,     this, wxID_ANY);
    Bind(wxEVT_LEFT_DOWN,    &FlowChart::OnLeftMouseDown,   this, wxID_ANY);
    Bind(wxEVT_LEFT_UP,      &FlowChart::OnLeftMouseUp,     this, wxID_ANY);
    Bind(wxEVT_RIGHT_DOWN,   &FlowChart::OnRightMouseDown,  this, wxID_ANY);
    Bind(wxEVT_RIGHT_UP,     &FlowChart::OnRightMouseUp,    this, wxID_ANY);
    Bind(wxEVT_MOTION,       &FlowChart::OnMouseMotion,     this, wxID_ANY);
    Bind(wxEVT_MOUSEWHEEL,   &FlowChart::OnMouseWheel,      this, wxID_ANY);
    Bind(wxEVT_PAINT,        &FlowChart::OnPaint,           this, wxID_ANY);

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create());
    if (gc)
    {
        wxDouble width{0};
        wxDouble height{0};
        gc->SetFont(*wxNORMAL_FONT, *wxCYAN);
        gc->GetTextExtent(wxT("123"), &width, &height);
        gapX_ = wxRound(height);
        gapY_ = wxRound(height);
    }

    allStates_.resize(kStateGuard);
    allContexts_.resize(kStateContextGuard);
    allContexts_[kStateContextFree] = std::make_unique<FreeStateContext>(this);
    allStates_[kStateFreeIdle] = std::make_unique<FreeIdleState>(this, dynamic_cast<FreeStateContext*>(allContexts_[kStateContextFree].get()));
    allStates_[kStateFreeDraging] = std::make_unique<FreeDragingState>(this, dynamic_cast<FreeStateContext*>(allContexts_[kStateContextFree].get()));
    currentState_ = allStates_[kStateFreeIdle].get();
    currentState_->Enter();

    SetDropTarget(new DnDText(this));
}

void FlowChart::AppendStep(wxCoord x, wxCoord y, const wxString& stepType)
{
    bool needRefresh = true;
    if (wxT("initstep") == stepType)
    {
        steps_.push_back(std::make_shared<InitStep>());
    }
    else if (wxT("endstep") == stepType)
    {
        steps_.push_back(std::make_shared<EndStep>());
    }
    else if (wxT("cvtstep") == stepType)
    {
        steps_.push_back(std::make_shared<CvtStep>());
    }
    else if (wxT("threshstep") == stepType)
    {
        steps_.push_back(std::make_shared<ThreshStep>());
    }
    else
    {
        needRefresh = false;
    }

    if (needRefresh)
    {
        wxAffineMatrix2D invMat = affMat_; invMat.Invert();
        wxRect bbox = steps_.back()->GetBoundingBox();
        const wxPoint2DDouble srcPoint(x - bbox.GetWidth()/2, y - bbox.GetHeight()/2);
        const wxPoint2DDouble dstPoint = invMat.TransformPoint(srcPoint);
        x = wxRound(dstPoint.m_x);
        y = wxRound(dstPoint.m_y);
        steps_.back()->SetRect(wxRect(x, y, -1, -1));
        Refresh(false);
    }
}

void FlowChart::SwitchState(const int newState)
{
    if (0 <= newState && newState < kStateGuard)
    {
        currentState_->Exit();
        currentState_ = allStates_[newState].get();
        currentState_->Enter();
    }
}

void FlowChart::DrawRubberBand(const wxRect &oldRect, const wxRect &newRect)
{
    wxRect rect = oldRect.Union(newRect);

    if (rect.IsEmpty()) {
        return;
    }

    rubberBandBox_ = newRect;
    const int iTol = 1;
    wxRect irects[4];
    if (!oldRect.IsEmpty())
    {
        const int t = oldRect.GetTop();
        const int b = oldRect.GetBottom();
        const int l = oldRect.GetLeft();
        const int r = oldRect.GetRight();

        irects[0] = wxRect(wxPoint(l - iTol, t - iTol), wxPoint(r + iTol, t + iTol));
        irects[1] = wxRect(wxPoint(l - iTol, b - iTol), wxPoint(r + iTol, b + iTol));
        irects[2] = wxRect(wxPoint(l - iTol, t + iTol), wxPoint(l + iTol, b - iTol));
        irects[3] = wxRect(wxPoint(r - iTol, t + iTol), wxPoint(r + iTol, b - iTol));
    }

    if (!newRect.IsEmpty())
    {
        const int t = newRect.GetTop();
        const int b = newRect.GetBottom();
        const int l = newRect.GetLeft();
        const int r = newRect.GetRight();

        irects[0].Union(wxRect(wxPoint(l - iTol, t - iTol), wxPoint(r + iTol, t + iTol)));
        irects[1].Union(wxRect(wxPoint(l - iTol, b - iTol), wxPoint(r + iTol, b + iTol)));
        irects[2].Union(wxRect(wxPoint(l - iTol, t + iTol), wxPoint(l + iTol, b - iTol)));
        irects[3].Union(wxRect(wxPoint(r - iTol, t + iTol), wxPoint(r + iTol, b - iTol)));
    }

    for (const auto &iRect : irects)
    {
        Refresh(false, &iRect);
    }
}

void FlowChart::PointSelect(const wxPoint &pos)
{

}

void FlowChart::BoxSelect(const wxPoint &minPos, const wxPoint &maxPos)
{

}

void FlowChart::OnEnterWindow(wxMouseEvent &e)
{
}

void FlowChart::OnLeaveWindow(wxMouseEvent &e)
{
}

void FlowChart::OnLeftMouseDown(wxMouseEvent &e)
{
    if (e.LeftIsDown() && e.RightIsDown())
    {
        affMat_ = wxAffineMatrix2D();
        Refresh(false);
    }

    currentState_->OnLeftMouseDown(e);
}

void FlowChart::OnRightMouseDown(wxMouseEvent &e)
{
    lastPos_ = e.GetPosition();
    if (e.LeftIsDown() && e.RightIsDown())
    {
        affMat_ = wxAffineMatrix2D();
        Refresh(false);
    }
}

void FlowChart::OnRightMouseUp(wxMouseEvent &e)
{

}

void FlowChart::OnLeftMouseUp(wxMouseEvent &e)
{
    currentState_->OnLeftMouseUp(e);
}

void FlowChart::OnMouseMotion(wxMouseEvent &e)
{
    if (e.Dragging() && e.RightIsDown())
    {
        wxAffineMatrix2D invMat = affMat_; invMat.Invert();
        const auto deltaPos = invMat.TransformDistance(e.GetPosition() - lastPos_);
        affMat_.Translate(deltaPos.m_x, deltaPos.m_y);
        Refresh(false);
    }
    lastPos_ = e.GetPosition();
    currentState_->OnMouseMove(e);
}

void FlowChart::OnMouseWheel(wxMouseEvent &e)
{
    const auto tPt = affMat_.TransformPoint(wxPoint2DDouble(e.GetPosition()));
    affMat_.Translate(tPt.m_x, tPt.m_y);
    const double s = e.GetWheelRotation() > 0 ? 1.1 : 0.9;
    affMat_.Scale(s, s);
    affMat_.Translate(-tPt.m_x, -tPt.m_y);
    Refresh(false);
}

void FlowChart::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);
    wxGCDC gcdc(dc);

    dc.SetBrush(wxBrush(wxColour(55, 56, 58)));
    dc.SetPen(wxNullPen);

    wxRegionIterator upd(GetUpdateRegion());
    while (upd)
    {
        int vX = upd.GetX();
        int vY = upd.GetY();
        int vW = upd.GetW();
        int vH = upd.GetH();

        wxRect cRect{ vX , vY , vW, vH };
        dc.DrawRectangle(cRect);

        upd++;
    }

    gcdc.SetTransformMatrix(affMat_);
    for (SPStepBase &step : steps_)
    {
        step->Draw(gcdc);
    }

    wxAffineMatrix2D idenMat;
    gcdc.SetTransformMatrix(idenMat);
    if (!rubberBandBox_.IsEmpty())
    {
        wxPen ruberPen(*wxLIGHT_GREY, 1, wxSOLID);
        gcdc.SetPen(ruberPen);
        gcdc.SetBrush(wxNullBrush);
        gcdc.DrawRectangle(rubberBandBox_);
    }
}

void FlowChart::DrawBackground() const
{
    wxGCDC dc;
    wxColour backgroundColour = GetBackgroundColour();
    dc.SetBrush(wxBrush(backgroundColour));
    dc.SetPen(wxNullPen);
    wxRect cRect = GetClientRect();
    dc.DrawRectangle(cRect);
}
