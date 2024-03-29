#ifndef SPAM_UI_FSM_PROC_TOOL_H
#define SPAM_UI_FSM_PROC_TOOL_H
#include "spamer.h"
#include "notool.h"
#include "boxtool.h"
#include <set>
#pragma warning( push )
#pragma warning( disable : 4819 4003 )
#include <2geom/path.h>
#pragma warning( pop )

struct FilterTool;
struct FilterIdle;
struct FilterDraging;
using  FilterBoxTool = BoxTool<FilterTool, kSpamID_TOOLBOX_PROC_FILTER>;

struct FilterTool : boost::statechart::simple_state<FilterTool, Spamer, FilterIdle>, FilterBoxTool
{
    using BoxToolT = BoxToolImpl;
    FilterTool() : FilterBoxTool(*this) {}
    ~FilterTool() {}

    void OnOptionChanged(const EvToolOption &e);
    void OnBoxingEnded(const EvBoxingEnded &e);
    void OnImageClicked(const EvImageClicked &e);
    void OnEntityClicked(const EvEntityClicked &e);
    sc::result react(const EvToolQuit &e);
    void BuildParameters(std::map<std::string, int> &iParams, std::map<std::string, double> &fParams);

    typedef boost::mpl::list<
        boost::statechart::transition<EvReset, FilterTool>,
        boost::statechart::in_state_reaction<EvAppQuit, BoxToolT, &BoxToolT::QuitApp>,
        boost::statechart::in_state_reaction<EvBoxingEnded, FilterTool, &FilterTool::OnBoxingEnded>,
        boost::statechart::in_state_reaction<EvImageClicked, FilterTool, &FilterTool::OnImageClicked>,
        boost::statechart::in_state_reaction<EvEntityClicked, FilterTool, &FilterTool::OnEntityClicked>,
        boost::statechart::in_state_reaction<EvDrawableDelete, BoxToolT, &BoxToolT::DeleteDrawable>,
        boost::statechart::in_state_reaction<EvDrawableSelect, BoxToolT, &BoxToolT::SelectDrawable>,
        boost::statechart::custom_reaction<EvToolQuit>> reactions;

    ToolOptions toolOptions;
    std::set<std::string> uuids;
};

struct FilterIdle : boost::statechart::simple_state<FilterIdle, FilterTool>
{
    FilterIdle() {}
    ~FilterIdle() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseDown, FilterDraging, FilterTool::BoxToolT, &FilterTool::BoxToolT::StartBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, FilterTool::BoxToolT, &FilterTool::BoxToolT::Safari>,
        boost::statechart::in_state_reaction<EvToolOption, FilterTool, &FilterTool::OnOptionChanged>,
        boost::statechart::in_state_reaction<EvCanvasLeave, FilterTool::BoxToolT, &FilterTool::BoxToolT::LeaveCanvas>> reactions;
};

struct FilterDraging : boost::statechart::simple_state<FilterDraging, FilterTool>
{
    FilterDraging() {}
    ~FilterDraging() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseUp, FilterIdle, FilterTool::BoxToolT, &FilterTool::BoxToolT::EndBoxing>,
        boost::statechart::transition<EvReset, FilterIdle, FilterTool::BoxToolT, &FilterTool::BoxToolT::ResetBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, FilterTool::BoxToolT, &FilterTool::BoxToolT::ContinueBoxing>> reactions;
};

struct ThresholdTool;
struct ThresholdIdle;
struct ThresholdDraging;
using  ThresholdBoxTool = BoxTool<ThresholdTool, kSpamID_TOOLBOX_PROC_THRESHOLD>;

struct ThresholdTool : boost::statechart::simple_state<ThresholdTool, Spamer, ThresholdIdle>, ThresholdBoxTool
{
    using BoxToolT = BoxToolImpl;
    ThresholdTool() : ThresholdBoxTool(*this) {}
    ~ThresholdTool() {}

    void OnOptionChanged(const EvToolOption &e);
    void OnBoxingEnded(const EvBoxingEnded &e);
    void OnImageClicked(const EvImageClicked &e);
    void OnEntityClicked(const EvEntityClicked &e);
    sc::result react(const EvToolQuit &e);

    typedef boost::mpl::list<
        boost::statechart::transition<EvReset, ThresholdTool>,
        boost::statechart::in_state_reaction<EvAppQuit, BoxToolT, &BoxToolT::QuitApp>,
        boost::statechart::in_state_reaction<EvBoxingEnded, ThresholdTool, &ThresholdTool::OnBoxingEnded>,
        boost::statechart::in_state_reaction<EvImageClicked, ThresholdTool, &ThresholdTool::OnImageClicked>,
        boost::statechart::in_state_reaction<EvEntityClicked, ThresholdTool, &ThresholdTool::OnEntityClicked>,
        boost::statechart::in_state_reaction<EvDrawableDelete, BoxToolT, &BoxToolT::DeleteDrawable>,
        boost::statechart::in_state_reaction<EvDrawableSelect, BoxToolT, &BoxToolT::SelectDrawable>,
        boost::statechart::custom_reaction<EvToolQuit>> reactions;

    ToolOptions toolOptions;
    std::set<std::string> uuids;
};

struct ThresholdIdle : boost::statechart::simple_state<ThresholdIdle, ThresholdTool>
{
    ThresholdIdle() {}
    ~ThresholdIdle() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseDown, ThresholdDraging, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::StartBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::Safari>,
        boost::statechart::in_state_reaction<EvToolOption, ThresholdTool, &ThresholdTool::OnOptionChanged>,
        boost::statechart::in_state_reaction<EvCanvasLeave, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::LeaveCanvas>> reactions;
};

struct ThresholdDraging : boost::statechart::simple_state<ThresholdDraging, ThresholdTool>
{
    ThresholdDraging() {}
    ~ThresholdDraging() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseUp, ThresholdIdle, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::EndBoxing>,
        boost::statechart::transition<EvReset, ThresholdIdle, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::ResetBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, ThresholdTool::BoxToolT, &ThresholdTool::BoxToolT::ContinueBoxing>> reactions;
};

struct EdgeTool;
struct EdgeIdle;
struct EdgeDraging;
using  EdgeBoxTool = BoxTool<EdgeTool, kSpamID_TOOLBOX_PROC_EDGE>;

struct EdgeTool : boost::statechart::simple_state<EdgeTool, Spamer, EdgeIdle>, EdgeBoxTool
{
    using BoxToolT = BoxToolImpl;
    EdgeTool() : EdgeBoxTool(*this) {}
    ~EdgeTool() {}

    void OnOptionChanged(const EvToolOption &e);
    void OnBoxingEnded(const EvBoxingEnded &e);
    void OnImageClicked(const EvImageClicked &e);
    void OnEntityClicked(const EvEntityClicked &e);
    sc::result react(const EvToolQuit &e);
    void BuildParameters(std::map<std::string, int> &iParams, std::map<std::string, double> &fParams);

    typedef boost::mpl::list<
        boost::statechart::transition<EvReset, EdgeTool>,
        boost::statechart::in_state_reaction<EvAppQuit, BoxToolT, &BoxToolT::QuitApp>,
        boost::statechart::in_state_reaction<EvBoxingEnded, EdgeTool, &EdgeTool::OnBoxingEnded>,
        boost::statechart::in_state_reaction<EvImageClicked, EdgeTool, &EdgeTool::OnImageClicked>,
        boost::statechart::in_state_reaction<EvEntityClicked, EdgeTool, &EdgeTool::OnEntityClicked>,
        boost::statechart::in_state_reaction<EvDrawableDelete, BoxToolT, &BoxToolT::DeleteDrawable>,
        boost::statechart::in_state_reaction<EvDrawableSelect, BoxToolT, &BoxToolT::SelectDrawable>,
        boost::statechart::custom_reaction<EvToolQuit>> reactions;

    ToolOptions toolOptions;
    std::set<std::string> uuids;
};

struct EdgeIdle : boost::statechart::simple_state<EdgeIdle, EdgeTool>
{
    EdgeIdle() {}
    ~EdgeIdle() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseDown, EdgeDraging, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::StartBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::Safari>,
        boost::statechart::in_state_reaction<EvToolOption, EdgeTool, &EdgeTool::OnOptionChanged>,
        boost::statechart::in_state_reaction<EvCanvasLeave, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::LeaveCanvas>> reactions;
};

struct EdgeDraging : boost::statechart::simple_state<EdgeDraging, EdgeTool>
{
    EdgeDraging() {}
    ~EdgeDraging() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseUp, EdgeIdle, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::EndBoxing>,
        boost::statechart::transition<EvReset, EdgeIdle, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::ResetBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, EdgeTool::BoxToolT, &EdgeTool::BoxToolT::ContinueBoxing>> reactions;
};

struct ColorConvertTool;
struct ColorConvertIdle;
struct ColorConvertDraging;
using  ColorConvertBoxTool = BoxTool<ColorConvertTool, kSpamID_TOOLBOX_PROC_CONVERT>;

struct ColorConvertTool : boost::statechart::simple_state<ColorConvertTool, Spamer, ColorConvertIdle>, ColorConvertBoxTool
{
    using BoxToolT = BoxToolImpl;
    ColorConvertTool() : ColorConvertBoxTool(*this) {}
    ~ColorConvertTool() {}

    void OnOptionChanged(const EvToolOption &e);
    void OnBoxingEnded(const EvBoxingEnded &e);
    void OnImageClicked(const EvImageClicked &e);
    void OnEntityClicked(const EvEntityClicked &e);
    sc::result react(const EvToolQuit &e);
    void BuildParameters(std::map<std::string, int> &iParams, std::map<std::string, double> &fParams);

    typedef boost::mpl::list<
        boost::statechart::transition<EvReset, ColorConvertTool>,
        boost::statechart::in_state_reaction<EvAppQuit, BoxToolT, &BoxToolT::QuitApp>,
        boost::statechart::in_state_reaction<EvBoxingEnded, ColorConvertTool, &ColorConvertTool::OnBoxingEnded>,
        boost::statechart::in_state_reaction<EvImageClicked, ColorConvertTool, &ColorConvertTool::OnImageClicked>,
        boost::statechart::in_state_reaction<EvEntityClicked, ColorConvertTool, &ColorConvertTool::OnEntityClicked>,
        boost::statechart::in_state_reaction<EvDrawableDelete, BoxToolT, &BoxToolT::DeleteDrawable>,
        boost::statechart::in_state_reaction<EvDrawableSelect, BoxToolT, &BoxToolT::SelectDrawable>,
        boost::statechart::custom_reaction<EvToolQuit>> reactions;

    ToolOptions toolOptions;
    std::set<std::string> uuids;
};

struct ColorConvertIdle : boost::statechart::simple_state<ColorConvertIdle, ColorConvertTool>
{
    ColorConvertIdle() {}
    ~ColorConvertIdle() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseDown, ColorConvertDraging, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::StartBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::Safari>,
        boost::statechart::in_state_reaction<EvToolOption, ColorConvertTool, &ColorConvertTool::OnOptionChanged>,
        boost::statechart::in_state_reaction<EvCanvasLeave, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::LeaveCanvas>> reactions;
};

struct ColorConvertDraging : boost::statechart::simple_state<ColorConvertDraging, ColorConvertTool>
{
    ColorConvertDraging() {}
    ~ColorConvertDraging() {}

    typedef boost::mpl::list<
        boost::statechart::transition<EvLMouseUp, ColorConvertIdle, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::EndBoxing>,
        boost::statechart::transition<EvReset, ColorConvertIdle, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::ResetBoxing>,
        boost::statechart::in_state_reaction<EvMouseMove, ColorConvertTool::BoxToolT, &ColorConvertTool::BoxToolT::ContinueBoxing>> reactions;
};

#endif //SPAM_UI_FSM_PROC_TOOL_H