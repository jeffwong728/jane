#ifndef SPAM_UI_TOP_LEVEL_ROOT_FRAME_H
#define SPAM_UI_TOP_LEVEL_ROOT_FRAME_H
#include <wx/wxprec.h>
#include <wx/nativewin.h>
#include <wx/aui/aui.h>
#include <wx/dataview.h>
#include <wx/filehistory.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/popupwin.h>
#include <ui/cmndef.h>
#include <ui/projs/modelfwd.h>
#include <string>
#include <vector>
#include <map>
class ProjPanel;
class ModelEvent;
class ProjTreeModel;
class DropImageEvent;
class CVImagePanel;
class CairoCanvas;
namespace cv
{
    class Mat;
}

class RootFrame : public wxFrame
{
    enum {
        kSpam_TOOLBOX_PROBE,
        kSpam_TOOLBOX_GEOM,
        kSpam_TOOLBOX_MATCH,
        kSpam_TOOLBOX_STYLE,

        kSpam_TOOLBOX_GUARD
    };
    friend struct Spamer;
public:
    RootFrame();
    ~RootFrame();

private:
    void CreateMenu();
    void CreateAuiPanes();

public:
    ProjPanel *GetProjPanel();
    wxAuiNotebook *GetStationNotebook() const;
    ProjTreeModel *GetProjTreeModel();
    void SyncScale(double scale, wxAuiNotebook *nb, wxWindow *page);

private:
    void OnHello(wxCommandEvent& e);
    void OnExit(wxCommandEvent& e);
    void OnClose(wxCloseEvent& e);
    void OnLoadImage(wxCommandEvent& e);
    void OnAbout(wxCommandEvent& e);
    void OnSize(wxSizeEvent& e);
    void OnAddStations(const SPModelNodeVector &stations);
    void OnDeleteStations(const SPModelNodeVector &stations);
    void OnAddGeoms(const SPModelNodeVector &geoms);
    void OnDeleteGeoms(const SPModelNodeVector &geoms);
    void OnGlowGeom(const SPModelNode &geom);
    void OnDimGeom(const SPModelNode &geom);
    void OnNewProject(ModelEvent& e);
    void OnLoadProject(ModelEvent& e);
    void OnViewMainTool(wxCommandEvent& e);
    void OnViewImage(wxCommandEvent& e);
    void OnViewProject(wxCommandEvent& e);
    void OnViewLog(wxCommandEvent& e);
    void OnViewToolboxBar(wxCommandEvent& e);
    void OnViewDefaultLayout(wxCommandEvent& e);
    void OnUpdateUI(wxUpdateUIEvent& e);
    void OnStationActivated(wxDataViewEvent &e);
    void OnSetTileLayout(wxCommandEvent& e);
    void OnTileLayout(wxCommandEvent& e);
    void OnStackLayout(wxCommandEvent& e);
    void OnDropImage(DropImageEvent& e);
    void OnTabExtension(wxAuiNotebookEvent& e);
    void OnPageSelect(wxAuiNotebookEvent& e);
    void OnAuiPageClosed(wxAuiManagerEvent& e);
    void OnZoomIn(wxCommandEvent &cmd);
    void OnZoomOut(wxCommandEvent &cmd);
    void OnZoomExtent(wxCommandEvent &cmd);
    void OnZoomOriginal(wxCommandEvent &cmd);
    void OnZoomHalf(wxCommandEvent &cmd);
    void OnZoomDouble(wxCommandEvent &cmd);
    void OnEnterScale(wxCommandEvent& e);
    void OnSelectScale(wxCommandEvent &e);
    void OnToolboxInfo(wxCommandEvent& e);
    void OnToolboxGeom(wxCommandEvent& e);
    void OnToolboxMatch(wxCommandEvent& e);
    void OnToolboxStyle(wxCommandEvent& e);

private:
    wxAuiNotebook *CreateStationNotebook();
    std::string GetNextCVStationWinName();
    std::vector<std::string> GetAllTabPaneNames(const std::string &perspective);
    wxToolBar *MakeStationToolBar(wxWindow *parent);
    void Zoom(double (CVImagePanel::*zoomFun)(bool), wxCommandEvent &cmd);
    void UpdateGeoms(void (CairoCanvas::*updateFun)(const SPDrawableNodeVector &), const SPModelNodeVector &geoms);
    void SyncScale(wxControl *extCtrl);
    void EnablePageImageTool(wxControl *extCtrl, bool bEnable);
    void SetStationImage(const wxString &uuidStation, const cv::Mat &img);
    wxPoint GetToolboxPopupPosition(const wxSize &popUpSize);
    void ToggleToolboxPane(const int boxShow);
    void ConnectCanvas(CVImagePanel *imgPanel);
    void ClickToolbox(wxCommandEvent& e, const int toolIndex);
    CVImagePanel *FindImagePanelByStation(const SPStationNode &station) const;

private:
    mutable wxAuiManager wxAuiMgr_;
    const wxString mainToolPanelName_;
    const wxString stationNotebookName_;
    const wxString projPanelName_;
    const wxString logPanelName_;
    const wxString toolBoxBarName_;
    wxFileHistory  imageFileHistory_;
    int cCVStation_{ 0 };
    double scale_{ 100.0 };
    wxString initialPerspective_;
    const wxString toolBoxLabels[kSpam_TOOLBOX_GUARD];
    std::unique_ptr<Spamer> spamer_;
};
#endif //SPAM_UI_TOP_LEVEL_ROOT_FRAME_H