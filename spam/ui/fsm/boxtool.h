#ifndef SPAM_UI_FSM_BOX_TOOL_H
#define SPAM_UI_FSM_BOX_TOOL_H
#include "spamer.h"

struct EntitySelection
{
    SPDrawableNodeVector ents;
    SPDrawableNodeVector delayDelEnts;
    SpamMany  mementos;
    std::vector<SelectionData> states;
};

using Selections = std::unordered_map<std::string, EntitySelection>;

struct BoxToolImpl
{
    BoxToolImpl(const int tid) : toolId(tid) {}
    ~BoxToolImpl();

    void StartBoxing(const EvLMouseDown &e);
    void ContinueBoxing(const EvMouseMove &e);
    void EndBoxing(const EvLMouseUp &e);

    void ResetBoxing(const EvReset &e);

    void EnterCanvas(const EvCanvasEnter &e);
    void LeaveCanvas(const EvCanvasLeave &e);

    void Safari(const EvMouseMove &e);
    void QuitApp(const EvAppQuit &e);
    void QuitTool(const EvToolQuit &e);
    void DeleteDrawable(const EvDrawableDelete &e);
    void SelectDrawable(const EvDrawableSelect &e);
    void ClearSelection(const std::string &uuid);
    void ClearHighlightData();

    virtual void FireDeselectEntity(const SPDrawableNodeVector &ents) const = 0;
    virtual void FireSelectEntity(const SPDrawableNodeVector &ents) const = 0;
    virtual void FireDimEntity(const SPDrawableNode &ent) const = 0;
    virtual void FireGlowEntity(const SPDrawableNode &ent) const = 0;

    Geom::Point    anchor;
    Geom::OptRect  rect;
    SPDrawableNode highlight;
    HighlightData  hlData;
    Selections     selData;
    const int toolId;
};

template<typename ToolT, int ToolId>
struct BoxTool : public BoxToolImpl
{
    BoxTool(ToolT &t) : BoxToolImpl(ToolId), tool(t){}
    ~BoxTool() { BoxToolImpl::QuitTool(EvToolQuit(ToolId)); }

    void FireDeselectEntity(const SPDrawableNodeVector &ents) const override { tool.context<Spamer>().sig_EntityDesel(ents); }
    void FireSelectEntity(const SPDrawableNodeVector &ents) const override { tool.context<Spamer>().sig_EntitySel(ents); }
    void FireDimEntity(const SPDrawableNode &ent) const override { tool.context<Spamer>().sig_EntityDim(ent); }
    void FireGlowEntity(const SPDrawableNode &ent) const override { tool.context<Spamer>().sig_EntityGlow(ent); }

    ToolT &tool;
};

#endif //SPAM_UI_FSM_BOX_TOOL_H