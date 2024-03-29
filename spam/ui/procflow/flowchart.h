#ifndef SPAM_UI_PROCFLOW_FLOWCHART_WIDGET_H
#define SPAM_UI_PROCFLOW_FLOWCHART_WIDGET_H
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/dcgraph.h>
#include <boost/signals2.hpp>
#include "stepfwd.h"

class StepBase;
struct FlowChartState;
struct FlowChartContext;

class FlowChart : public wxControl
{
public:
    enum
    {
        kStateFreeIdle,
        kStateFreeDraging,
        kStateFreeEditing,
        kStateConnectIdle,
        kStateConnectConnecting,
        kStateGuard
    };

    enum
    {
        kStateContextFree,
        kStateContextConnect,
        kStateContextGuard
    };

public:
    FlowChart(wxWindow* parent);

public:
    void AppendStep(wxCoord x, wxCoord y, const wxString& stepType);
    void SwitchState(const int newState);
    void DrawRubberBand(const wxRect &oldRect, const wxRect &newRect);

public:
    void AlignLeft();
    void AlignVCenter();

public:
    SPStepBase GetSelect(const wxPoint &pos);
    bool AccumulatePointSelect(const wxPoint &pos);
    SPStepBase XORPointSelect(const wxPoint &pos);
    void ExclusivePointSelect(const wxPoint &pos);
    void TogglePointSelect(const wxPoint &pos);
    void ExclusiveBoxSelect(const wxRect &rcBox);
    void ToggleBoxSelect(const wxRect &rcBox);
    void HighlightTest(const wxPoint &pos);
    void PortHighlightTest(const wxPoint &pos, const bool expectInPort);
    void ClearStatus();
    void SetConnectionMarks();
    void DoEditing(SPStepBase &step, const wxPoint &apos, const wxPoint &lpos, const wxPoint &cpos);

protected:
    void OnEnterWindow(wxMouseEvent &e);
    void OnLeaveWindow(wxMouseEvent &e);
    void OnLeftMouseDown(wxMouseEvent &e);
    void OnLeftMouseUp(wxMouseEvent &e);
    void OnRightMouseDown(wxMouseEvent &e);
    void OnRightMouseUp(wxMouseEvent &e);
    void OnMouseMotion(wxMouseEvent &e);
    void OnMouseWheel(wxMouseEvent &e);
    void OnPaint(wxPaintEvent &e);

protected:
    wxSize DoGetBestSize() const wxOVERRIDE { return wxSize(300, 300); }
    bool InheritsBackgroundColour() const { return true; }

private:
    void DrawBackground() const;

private:
    int gapX_{5};
    int gapY_{5};
    std::vector<SPStepBase> steps_;
    wxAffineMatrix2D affMat_;
    wxPoint lastPos_;
    FlowChartState *currentState_ = nullptr;
    std::vector<std::unique_ptr<FlowChartState>> allStates_;
    std::vector<std::unique_ptr<FlowChartContext>> allContexts_;
    wxRect rubberBandBox_;

    wxDECLARE_NO_COPY_CLASS(FlowChart);
};
#endif //SPAM_UI_PROCFLOW_FLOWCHART_WIDGET_H
