#ifndef SPAM_UI_FSM_EVENTS_H
#define SPAM_UI_FSM_EVENTS_H
#include <wx/event.h>
#include <boost/statechart/event.hpp>
#include <ui/cmndef.h>
#include <ui/projs/modelfwd.h>

struct EvLMouseDown : boost::statechart::event<EvLMouseDown>
{
    EvLMouseDown(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvLMouseUp : boost::statechart::event<EvLMouseUp>
{
    EvLMouseUp(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvMouseMove : boost::statechart::event<EvMouseMove>
{
    EvMouseMove(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvLMouseDClick : boost::statechart::event<EvLMouseDClick>
{
    EvLMouseDClick(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvMMouseDown : boost::statechart::event<EvMMouseDown>
{
    EvMMouseDown(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvCanvasEnter : boost::statechart::event<EvCanvasEnter>
{
    EvCanvasEnter(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvCanvasLeave : boost::statechart::event<EvCanvasLeave>
{
    EvCanvasLeave(const wxMouseEvent &e) : evData(e) {}
    wxMouseEvent evData;
};

struct EvReset : boost::statechart::event<EvReset> 
{
    EvReset(const wxKeyEvent &e) : evData(e) {}
    wxKeyEvent evData;
};

struct EvAppQuit : boost::statechart::event<EvAppQuit> {};

struct EvToolEnter : boost::statechart::event<EvToolEnter>
{
    EvToolEnter(const int id) : toolId(id) {}
    const int toolId;
};

struct EvToolQuit : boost::statechart::event<EvToolQuit>
{
    EvToolQuit(const int id) : toolId(id) {}
    const int toolId;
};

struct EvToolOption : boost::statechart::event<EvToolOption>
{
    EvToolOption(const ToolOptions &tos) : toolOptions(tos) {}
    const ToolOptions toolOptions;
};

struct EvDrawableDelete : boost::statechart::event<EvDrawableDelete>
{
    EvDrawableDelete(const SPDrawableNodeVector &dras) : drawables(dras) {}
    SPDrawableNodeVector drawables;
};

struct EvDrawableSelect : boost::statechart::event<EvDrawableSelect>
{
    EvDrawableSelect(const SPDrawableNodeVector &dras) : drawables(dras) {}
    SPDrawableNodeVector drawables;
};

#endif //SPAM_UI_FSM_EVENTS_H