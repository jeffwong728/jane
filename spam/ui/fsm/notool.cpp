#include "notool.h"
#include "probetool.h"
#include "profiletool.h"
#include "proctool.h"
#include "pyramidtool.h"
#include "recttool.h"
#include "linetool.h"
#include "ellipsetool.h"
#include "transformtool.h"
#include "nodeedittool.h"
#include "polygontool.h"
#include "beziergontool.h"
#include "booltool.h"
#include <wx/log.h>
#include <ui/spam.h>
#include <ui/cv/cairocanvas.h>
#include <ui/projs/geomnode.h>
#include <ui/projs/drawablenode.h>
#include <ui/toplevel/rootframe.h>

NoTool::NoTool()
    : NoBoxTool(*this)
{
    wxLogMessage(wxT("NoTool Enter."));
}

NoTool::~NoTool()
{
    wxLogMessage(wxT("NoTool Quit."));
}

NoToolIdle::NoToolIdle()
{
    wxLogMessage(wxT("NoToolIdle Enter."));
}

NoToolIdle::~NoToolIdle()
{
    wxLogMessage(wxT("NoToolIdle Quit."));
}

sc::result NoToolIdle::react(const EvToolEnter &e)
{
    const int toolId = e.toolId;
    if (kSpamID_TOOLBOX_PROBE_SELECT == toolId)
    {
        return transit<ProbeTool>();
    }
    else if (kSpamID_TOOLBOX_PROBE_REGION == toolId)
    {
        return transit<RegionTool>();
    }
    else if (kSpamID_TOOLBOX_PROBE_HISTOGRAM == toolId)
    {
        return transit<HistogramTool>();
    }
    else if (kSpamID_TOOLBOX_PROBE_PROFILE == toolId)
    {
        return transit<ProfileTool>();
    }
    else if (kSpamID_TOOLBOX_PROC_THRESHOLD == toolId)
    {
        return transit<ThresholdTool>();
    }
    else if (kSpamID_TOOLBOX_PROC_PYRAMID == toolId)
    {
        return transit<PyramidTool>();
    }
    else if (kSpamID_TOOLBOX_PROC_FILTER == toolId)
    {
        return transit<FilterTool>();
    }
    else if (kSpamID_TOOLBOX_PROC_EDGE == toolId)
    {
        return transit<EdgeTool>();
    }
    else if (kSpamID_TOOLBOX_PROC_CONVERT == toolId)
    {
        return transit<ColorConvertTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_RECT == toolId)
    {
        return transit<RectTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_ELLIPSE == toolId)
    {
        return transit<EllipseTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_POLYGON == toolId)
    {
        return transit<PolygonTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_BEZIERGON == toolId)
    {
        return transit<BeziergonTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_LINE == toolId)
    {
        return transit<LineTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_TRANSFORM == toolId)
    {
        return transit<TransformTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_EDIT == toolId)
    {
        return transit<NodeEditTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_UNION == toolId)
    {
        return transit<UnionTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_INTERS == toolId)
    {
        return transit<IntersectionTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_DIFF == toolId)
    {
        return transit<DiffTool>();
    }
    else if (kSpamID_TOOLBOX_GEOM_SYMDIFF == toolId)
    {
        return transit<XORTool>();
    }
    else
    {
        return forward_event();
    }
}

NoToolDraging::NoToolDraging()
{
    wxLogMessage(wxT("NoToolDraging Enter."));
}

NoToolDraging::~NoToolDraging()
{
    wxLogMessage(wxT("NoToolDraging Quit."));
}