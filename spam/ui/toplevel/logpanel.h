#ifndef SPAM_UI_TOP_LEVEL_LOG_PANEL_H
#define SPAM_UI_TOP_LEVEL_LOG_PANEL_H
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <ui/cmndef.h>

class LogPanel : public wxPanel
{
public:
    LogPanel(wxWindow* parent);
    ~LogPanel();

private:
    wxToolBar *MakeToolBar();

private:
    void OnClear(wxCommandEvent &cmd);
    void OnSave(wxCommandEvent &cmd);
};
#endif //SPAM_UI_TOP_LEVEL_LOG_PANEL_H