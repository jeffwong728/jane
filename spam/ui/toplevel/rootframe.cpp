﻿// wxWidgets "Hello World" Program
// For compilers that support precompilation, includes "wx/wx.h".
#include "rootframe.h"
#include "maintoolpane.h"
#include "projpanel.h"
#include "logpanel.h"
#include "mainstatus.h"
#include <ui/spam.h>
#include <ui/evts.h>
#include <ui/fsm/spamer.h>
#include <ui/cv/cvimagepanel.h>
#include <ui/cv/cairocanvas.h>
#include <ui/projs/stationnode.h>
#include <ui/projs/drawablenode.h>
#include <ui/projs/projnode.h>
#include <ui/projs/projtreemodel.h>
#include <ui/toolbox/stylebox.h>
#include <ui/toolbox/probebox.h>
#include <ui/toolbox/matchbox.h>
#include <ui/toolbox/geombox.h>
#include <opencv2/imgproc.hpp>
#include <wx/artprov.h>
#include <wx/statline.h>
#pragma warning( push )
#pragma warning( disable : 4003 )
#include <2geom/bezier.h>
#include <2geom/path.h>
#include <2geom/pathvector.h>
#include <2geom/path-intersection.h>
#include <2geom/svg-path-parser.h>
#include <2geom/svg-path-writer.h>
#include <2geom/cairo-path-sink.h>
#pragma warning( pop )
#include <set>
#include <vector>
#include <functional>
#include <cairo.h>
#include <helper/splivarot.h>
#include <helper/commondef.h>
#include <boost/algorithm/string.hpp>
#pragma warning( push )
#pragma warning( disable : 5033 )
#ifdef pid_t
#undef pid_t
#endif
#ifdef HAVE_SSIZE_T
#undef HAVE_SSIZE_T
#endif
#include <boost/python.hpp>
#pragma warning( pop )
#include <ui/misc/percentvalidator.h>

RootFrame::RootFrame()
    : wxFrame(NULL, wxID_ANY, wxT("Spam"))
    , mainToolPanelName_(wxT("maintool"))
    , stationNotebookName_(wxT("station"))
    , projPanelName_(wxT("proj"))
    , logPanelName_(wxT("log"))
    , toolBoxBarName_(wxT("toolBoxBar"))
    , toolBoxLabels{wxT("toolBoxInfo"), wxT("toolBoxGeom"), wxT("toolBoxMatch"), wxT("toolBoxStyle") }
    , imageFileHistory_(9, spamID_BASE_IMAGE_FILE_HISTORY)
    , spamer_(std::make_unique<Spamer>())
{
    //SetIcon(wxIcon(wxT("res\\target_128.png"), wxBITMAP_TYPE_PNG));
    CreateMenu();
    //CreateStatusBar();
    SetStatusBar(new MainStatus(this));
    SetStatusText("Welcome to wxWidgets!");

    Bind(wxEVT_SIZE, &RootFrame::OnSize, this, wxID_ANY);
    Bind(wxEVT_CLOSE_WINDOW, &RootFrame::OnClose, this, wxID_ANY);
    Bind(wxEVT_UPDATE_UI, &RootFrame::OnUpdateUI, this, wxID_ANY);
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &RootFrame::OnStationActivated, this, wxID_ANY);

    wxTheApp->Bind(spamEVT_PROJECT_NEW,     &RootFrame::OnNewProject,    this, wxID_ANY);
    wxTheApp->Bind(spamEVT_PROJECT_LOADED,  &RootFrame::OnLoadProject,   this, wxID_ANY);
    wxTheApp->Bind(spamEVT_DROP_IMAGE,      &RootFrame::OnDropImage,     this, wxID_ANY);

    CreateAuiPanes();
    SetSize(wxSize(800, 600));

    spamer_->initiate();

    auto m = GetProjTreeModel();
    if (m)
    {
        m->sig_StationAdd.connect(std::bind(&RootFrame::OnAddStations, this, std::placeholders::_1));
        m->sig_StationDelete.connect(std::bind(&RootFrame::OnDeleteStations, this, std::placeholders::_1));
        m->sig_GeomAdd.connect(std::bind(&RootFrame::OnAddGeoms, this, std::placeholders::_1));
        m->sig_GeomDelete.connect(std::bind(&RootFrame::OnDeleteGeoms, this, std::placeholders::_1));
    }

    auto p = GetProjPanel();
    if (p)
    {
        p->sig_EntityGlow.connect(std::bind(&RootFrame::OnGlowGeom, this, std::placeholders::_1));
        p->sig_EntityDim.connect(std::bind(&RootFrame::OnDimGeom, this, std::placeholders::_1));

        spamer_->sig_EntityDim.connect(std::bind(&ProjPanel::DimEntity, p, std::placeholders::_1));
        spamer_->sig_EntityGlow.connect(std::bind(&ProjPanel::GlowEntity, p, std::placeholders::_1));
    }
}

RootFrame::~RootFrame()
{
    wxAuiMgr_.UnInit();
}

void RootFrame::CreateMenu()
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&Hello...\tCtrl-H", "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    menuFile->Append(spamID_LOAD_IMAGE, wxT("Load Image"), wxT("Load a image file"), wxITEM_NORMAL);

    wxMenu *menuView = new wxMenu;
    menuView->Append(spamID_VIEW_MAIN_TOOL, wxT("Main Tool Panel"), wxT("Show main tool panel"), wxITEM_CHECK);
    menuView->Append(spamID_VIEW_IMAGE, wxT("Image Panel"), wxT("Show image panel"), wxITEM_CHECK);
    menuView->Append(spamID_VIEW_PROJECT, wxT("Project Panel"), wxT("Show project panel"), wxITEM_CHECK);
    menuView->Append(spamID_VIEW_LOG, wxT("Log Panel"), wxT("Show log panel"), wxITEM_CHECK);
    menuView->Append(spamID_VIEW_TOOLBOX_BAR, wxT("Toolbox Bar"), wxT("Show toolbox bar"), wxITEM_CHECK);
    menuView->AppendSeparator();
    menuView->Append(spamID_VIEW_DEFAULT_LAYOUT, wxT("Default Layout"), wxT("Show default layout"), wxITEM_NORMAL);
    menuView->AppendSeparator();
    menuView->Append(spamID_VIEW_SET_TILE_LAYOUT, wxT("Set tile layout"), wxT("Set current station view as tile layout"), wxITEM_NORMAL);
    menuView->AppendSeparator();
    menuView->Append(spamID_VIEW_TILE_LAYOUT, wxT("Tile layout"), wxT("Tile station view"), wxITEM_NORMAL);
    menuView->Append(spamID_VIEW_STACK_LAYOUT, wxT("Stack layout"), wxT("Stack station view"), wxITEM_NORMAL);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuView, "&View");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &RootFrame::OnHello, this, ID_Hello);
    Bind(wxEVT_MENU, &RootFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &RootFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &RootFrame::OnLoadImage, this, spamID_LOAD_IMAGE);
    Bind(wxEVT_MENU, &RootFrame::OnViewMainTool, this, spamID_VIEW_MAIN_TOOL);
    Bind(wxEVT_MENU, &RootFrame::OnViewImage, this, spamID_VIEW_IMAGE);
    Bind(wxEVT_MENU, &RootFrame::OnViewProject, this, spamID_VIEW_PROJECT);
    Bind(wxEVT_MENU, &RootFrame::OnViewLog, this, spamID_VIEW_LOG);
    Bind(wxEVT_MENU, &RootFrame::OnViewToolboxBar, this, spamID_VIEW_TOOLBOX_BAR);
    Bind(wxEVT_MENU, &RootFrame::OnViewDefaultLayout, this, spamID_VIEW_DEFAULT_LAYOUT);
    Bind(wxEVT_MENU, &RootFrame::OnSetTileLayout, this, spamID_VIEW_SET_TILE_LAYOUT);
    Bind(wxEVT_MENU, &RootFrame::OnTileLayout, this, spamID_VIEW_TILE_LAYOUT);
    Bind(wxEVT_MENU, &RootFrame::OnStackLayout, this, spamID_VIEW_STACK_LAYOUT);
}

void RootFrame::CreateAuiPanes()
{
    wxAuiMgr_.SetManagedWindow(this);
    Bind(wxEVT_AUI_PANE_CLOSE, &RootFrame::OnAuiPageClosed, this);

    wxAuiToolBar* tbBar = new wxAuiToolBar(this, kSpamToolboxBar, wxDefaultPosition, wxDefaultSize, wxAUI_TB_VERTICAL);
    tbBar->SetToolBitmapSize(wxSize(24, 24));
    tbBar->AddTool(kSpamID_TOOLBOX_PROBE, toolBoxLabels[kSpam_TOOLBOX_PROBE], wxArtProvider::GetBitmap(wxART_NEW), wxT("Image Infomation"), wxITEM_CHECK);
    tbBar->AddTool(kSpamID_TOOLBOX_GEOM, toolBoxLabels[kSpam_TOOLBOX_GEOM], wxArtProvider::GetBitmap(wxART_NEW), wxT("Geometry Tool"), wxITEM_CHECK);
    tbBar->AddTool(kSpamID_TOOLBOX_MATCH, toolBoxLabels[kSpam_TOOLBOX_MATCH], wxArtProvider::GetBitmap(wxART_NEW), wxT("Pattern Match"), wxITEM_CHECK);
    tbBar->AddSeparator();
    tbBar->AddTool(kSpamID_TOOLBOX_STYLE, toolBoxLabels[kSpam_TOOLBOX_STYLE], wxArtProvider::GetBitmap(wxART_NEW), wxT("Geometry Style"), wxITEM_CHECK);
    tbBar->Realize();

    tbBar->Bind(wxEVT_TOOL, &RootFrame::OnToolboxInfo, this, kSpamID_TOOLBOX_PROBE);
    tbBar->Bind(wxEVT_TOOL, &RootFrame::OnToolboxGeom, this, kSpamID_TOOLBOX_GEOM);
    tbBar->Bind(wxEVT_TOOL, &RootFrame::OnToolboxMatch, this, kSpamID_TOOLBOX_MATCH);
    tbBar->Bind(wxEVT_TOOL, &RootFrame::OnToolboxStyle, this, kSpamID_TOOLBOX_STYLE);

    auto mainToolPane = new MainToolPane(this, imageFileHistory_);
    auto &mainToolPaneInfo = wxAuiPaneInfo();
    mainToolPaneInfo.Name(mainToolPanelName_);
    mainToolPaneInfo.Top();
    mainToolPaneInfo.Gripper(false);
    mainToolPaneInfo.CloseButton(false);
    mainToolPaneInfo.CaptionVisible(false);
    mainToolPaneInfo.PaneBorder(false);
    mainToolPaneInfo.MinSize(mainToolPane->GetSizer()->GetMinSize());
    mainToolPaneInfo.DockFixed();
    wxAuiMgr_.AddPane(mainToolPane, mainToolPaneInfo.Layer(999));
    wxAuiMgr_.AddPane(CreateStationNotebook(), wxAuiPaneInfo().Name(stationNotebookName_).Center().PaneBorder(false).CloseButton(false).CaptionVisible(false));
    wxAuiMgr_.AddPane(new ProjPanel(this), wxAuiPaneInfo().Name(projPanelName_).Left().Caption(wxT("Project Explorer")));
    wxAuiMgr_.AddPane(new LogPanel(this), wxAuiPaneInfo().Name(logPanelName_).Left().Bottom().Caption("Log"));

    auto infoBox = new ProbeBox(this);
    auto geomBox = new GeomBox(this);
    auto matchBox = new MatchBox(this);
    auto styleBox = new StyleBox(this);

    infoBox->sig_ToolEnter.connect(std::bind(&Spamer::OnToolEnter, spamer_.get(), std::placeholders::_1));
    infoBox->sig_ToolQuit.connect(std::bind(&Spamer::OnToolQuit, spamer_.get(), std::placeholders::_1));
    geomBox->sig_ToolEnter.connect(std::bind(&Spamer::OnToolEnter, spamer_.get(), std::placeholders::_1));
    geomBox->sig_ToolQuit.connect(std::bind(&Spamer::OnToolQuit, spamer_.get(), std::placeholders::_1));
    matchBox->sig_ToolEnter.connect(std::bind(&Spamer::OnToolEnter, spamer_.get(), std::placeholders::_1));
    matchBox->sig_ToolQuit.connect(std::bind(&Spamer::OnToolQuit, spamer_.get(), std::placeholders::_1));

    wxAuiMgr_.AddPane(infoBox, wxAuiPaneInfo().Name(toolBoxLabels[kSpam_TOOLBOX_PROBE]).Right().Caption("Probe").Show(false));
    wxAuiMgr_.AddPane(geomBox, wxAuiPaneInfo().Name(toolBoxLabels[kSpam_TOOLBOX_GEOM]).Right().Caption("Geometry Tool").Show(false));
    wxAuiMgr_.AddPane(matchBox, wxAuiPaneInfo().Name(toolBoxLabels[kSpam_TOOLBOX_MATCH]).Right().Caption("Template Matching").Show(false));
    wxAuiMgr_.AddPane(styleBox, wxAuiPaneInfo().Name(toolBoxLabels[kSpam_TOOLBOX_STYLE]).Right().Caption("Style").Show(false));

    auto &toolBoxBarPaneInfo = wxAuiPaneInfo();
    toolBoxBarPaneInfo.Name(toolBoxBarName_).Caption(wxT("Toolbox Bar")).ToolbarPane().Right().Gripper(false);
    wxAuiMgr_.AddPane(tbBar, toolBoxBarPaneInfo);

    auto mainToolPanePers = wxAuiMgr_.SavePaneInfo(mainToolPaneInfo);
    auto toolBoxBarPanePers = wxAuiMgr_.SavePaneInfo(toolBoxBarPaneInfo);
    initialPerspective_ = wxAuiMgr_.SavePerspective();
    const auto &projPerspective = SpamConfig::Get<wxString>(CommonDef::GetProjPanelCfgPath());
    if (!projPerspective.empty())
    {
        wxAuiMgr_.LoadPerspective(projPerspective);
        wxAuiMgr_.LoadPaneInfo(mainToolPanePers, mainToolPaneInfo);
        wxAuiMgr_.LoadPaneInfo(toolBoxBarPanePers, toolBoxBarPaneInfo);
    }

    wxAuiMgr_.GetPane(toolBoxLabels[kSpam_TOOLBOX_PROBE]).MinSize(infoBox->GetSizer()->GetMinSize());
    wxAuiMgr_.GetPane(toolBoxLabels[kSpam_TOOLBOX_GEOM]).MinSize(geomBox->GetSizer()->GetMinSize());
    wxAuiMgr_.GetPane(toolBoxLabels[kSpam_TOOLBOX_MATCH]).MinSize(matchBox->GetSizer()->GetMinSize());
    wxAuiMgr_.GetPane(toolBoxLabels[kSpam_TOOLBOX_STYLE]).MinSize(styleBox->GetSizer()->GetMinSize());


    ToggleToolboxPane(kSpam_TOOLBOX_GUARD);
    wxAuiMgr_.GetArtProvider()->SetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE, 0);
    wxAuiMgr_.Update();
}

ProjPanel *RootFrame::GetProjPanel()
{
    return dynamic_cast<ProjPanel *>(wxAuiMgr_.GetPane(projPanelName_).window);
}

wxAuiNotebook *RootFrame::GetStationNotebook() const
{
    return dynamic_cast<wxAuiNotebook *>(wxAuiMgr_.GetPane(stationNotebookName_).window);
}

ProjTreeModel *RootFrame::GetProjTreeModel()
{
    auto projPanel = GetProjPanel();
    if (projPanel)
    {
        return projPanel->GetProjTreeModel();
    }

    return nullptr;
}

void RootFrame::SyncScale(double scale, wxAuiNotebook *nb, wxWindow *page)
{
    if (nb && page)
    {
        scale_ = scale;
        if (page->IsShown())
        {
            SyncScale(nb->FindExtensionCtrlByPage(page));
        }
    }
}

void RootFrame::OnExit(wxCommandEvent& e)
{
    Close(false);
}

void RootFrame::OnClose(wxCloseEvent& e)
{
    SpamConfig::Set(CommonDef::GetProjPanelCfgPath(), wxAuiMgr_.SavePerspective());
    auto projPanel = GetProjPanel();
    if (projPanel && projPanel->IsProjectModified())
    {
        wxMessageDialog dialog(this, wxT("Project have modified. Do you want to save first?"),
            wxT("Project Modified"), wxCENTER | wxNO_DEFAULT | wxYES_NO | wxCANCEL | wxICON_WARNING);
        dialog.SetYesNoCancelLabels(wxT("&Yes"), wxT("&Discard changes"), wxT("&Cancel"));
        if (wxID_NO == dialog.ShowModal())
        {
            e.Skip();
        }
        else
        {
            e.Veto();
        }
    }
    else
    {
        e.Skip();
    }
}

void RootFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets Hello World example",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}

void RootFrame::OnHello(wxCommandEvent& event)
{
    //wxLogMessage("Hello world from wxWidgets!");
    wxString wildCard{ "Python files (*.py)|*.py" };
    wxFileDialog openFileDialog(this, wxT("Open Python file"), "", "", wildCard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() != wxID_CANCEL)
    {
        auto fullPath = openFileDialog.GetPath();
        try
        {
            boost::python::object mainModule = boost::python::import("__main__");
            boost::python::object mainNamespace = mainModule.attr("__dict__");
            Spam::ClearPyOutput();
            boost::python::exec_file(fullPath.ToStdString().c_str(), mainNamespace);
            Spam::LogPyOutput();
        }
        catch (const boost::python::error_already_set&)
        {
            Spam::PopupPyError();
        }
    }
}

void RootFrame::OnLoadImage(wxCommandEvent& event)
{
    wxString wildCard{"PNG files (*.png)|*.png"};
    wildCard.Append("|JPEG files (*.jpg;*.jpeg)|*.jpg;*.jpeg");
    wildCard.Append("|BMP files (*.bmp)|*.bmp");
    wildCard.Append("|GIF files (*.gif)|*.gif");
    wildCard.Append("|GIF files (*.tif)|*.tif");
    wildCard.Append("|All files (*.*)|*.*");

    wxFileDialog openFileDialog(this, wxT("Open image file"), "", "", wildCard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() != wxID_CANCEL)
    {
        auto fullPath = openFileDialog.GetPath();

        auto stationNB = GetStationNotebook();
        if (stationNB)
        {
            auto sel = stationNB->GetSelection();
            if (wxNOT_FOUND != sel)
            {
                auto imgPanelPage = dynamic_cast<CVImagePanel *>(stationNB->GetPage(sel));
                if (imgPanelPage)
                {
                    scale_ = imgPanelPage->LoadImageFromFile(fullPath);
                    auto extCtrl = stationNB->FindExtensionCtrlByPage(imgPanelPage);
                    if (imgPanelPage->HasImage())
                    {
                        SetStationImage(imgPanelPage->GetName(), imgPanelPage->GetImage());
                        EnablePageImageTool(extCtrl, true);
                        SyncScale(extCtrl);
                    }
                    else
                    {
                        EnablePageImageTool(extCtrl, false);
                    }

                    imageFileHistory_.AddFileToHistory(fullPath);
                }
            }
        }
#if 0
        auto src = cv::imread(cv::String(fullPath.c_str()), cv::IMREAD_COLOR);

        cv::Mat disSrc;
        cv::cvtColor(src, disSrc, cv::COLOR_BGR2BGRA);

        const auto tilePanel = dynamic_cast<const CVImagePanel *>(wxAuiMgr_.GetPane(wxT("cv1")).window);
        tilePanel->AdjustImgWndSize(nCVWndId % 2, wxSize(disSrc.cols, disSrc.rows));

        const auto &curImgWnd = cvWndNames[nCVWndId % 2];
        const cv::Rect imgWndSize = cv::getWindowImageRect(curImgWnd);

        Geom::PathVector pv = Geom::parse_svg_path("m 28.969534,110.28325 c -1.796599,4.42678 -3.748208,10.61397 -6.128371,19.63472 -8.420958,31.9152 -6.755309,21.93975 -8.597107,54.44133 h 16.705972 c -0.963418,-7.97698 -1.687496,-16.75675 -1.044576,-26.77407 2.11937,-33.0219 2.119647,-12.49504 22.606885,-40.16203 2.097235,-2.83221 3.939323,-5.18938 5.599081,-7.13995 z m 56.585786,0 c 4.527431,2.5729 10.232013,5.43577 17.82154,8.03252 33.90992,11.60229 14.12926,16.95702 45.91981,54.44133 4.25123,5.01264 7.36171,8.75065 9.61416,11.6022 h 17.35164 c -4.15814,-9.44396 -9.55048,-21.56978 -12.83674,-37.48431 -4.59873,-22.27048 -12.31725,-30.62014 -22.92706,-36.59174 z");
        Geom::Affine a1 = Geom::Scale(0.99, 0.99);
        Geom::Affine a2 = Geom::Translate(disSrc.cols / 2, disSrc.rows / 2);
        pv = pv*(a1*a2);

        Geom::PathVector pv1 = Geom::parse_svg_path("M 145.14285,117.83929 A 60.098213,34.773811 0 0 1 85.04464,152.6131 60.098213,34.773811 0 0 1 24.946426,117.83929 60.098213,34.773811 0 0 1 85.04464,83.065475 60.098213,34.773811 0 0 1 145.14285,117.83929 Z");
        Geom::PathVector pv3 = Geom::parse_svg_path("M 60.47619,188.14285 C 68.791666,122.375 55.184523,1.422619 84.666666,50.559522 114.14881,99.696427 168.57738,179.07143 114.14881,183.60714 c -54.428572,4.53571 -53.67262,4.53571 -53.67262,4.53571 z");
        auto pvr = sp_pathvector_boolop(pv3, pv1, bool_op_union, fill_nonZero, fill_nonZero)*Geom::Scale(2, 2);

        auto imgSurf = cairo_image_surface_create_for_data(disSrc.data, CAIRO_FORMAT_RGB24, disSrc.cols, disSrc.rows, disSrc.step1());
        auto cr = cairo_create(imgSurf);

        Geom::Point ctrlPts[4] = { Geom::Point(200, 200), Geom::Point(100, 200), Geom::Point(100, 300), Geom::Point(100, 400) };
        Geom::CubicBezier cb(ctrlPts[0], ctrlPts[1], ctrlPts[2], ctrlPts[3]);
        Geom::Path pv2;
        pv2.append(cb);
        pv2.close();

        Geom::CairoPathSink cairoPathSink(cr);
        cairo_set_line_width(cr, 3);
        cairo_set_source_rgba(cr, 0.0, 0.8, 0.8, 0.2);
        cairoPathSink.feed(pv);
        cairoPathSink.feed(pvr);
        //cairo_stroke(cr);
        cairo_fill(cr);
        cairo_destroy(cr);
        cairo_surface_destroy(imgSurf);
        //cv::imwrite(cv::String("C:\\Users\\wwang\\Desktop\\dest.png"), disSrc);

        //cv::drawMarker(disSrc, cv::Point(200, 200), cv::Scalar(0, 0, 255), cv::MARKER_DIAMOND, 10, 3);
        //cv::drawMarker(disSrc, cv::Point(100, 200), cv::Scalar(0, 0, 255), cv::MARKER_DIAMOND, 10, 3);
        //cv::drawMarker(disSrc, cv::Point(100, 300), cv::Scalar(0, 0, 255), cv::MARKER_DIAMOND, 10, 3);
        //cv::drawMarker(disSrc, cv::Point(100, 400), cv::Scalar(0, 0, 255), cv::MARKER_DIAMOND, 10, 3);

        cv::imshow(curImgWnd, disSrc);
        nCVWndId += 1;
#endif
    }
}

void RootFrame::OnSize(wxSizeEvent& event)
{
}

void RootFrame::OnAddStations(const SPModelNodeVector &stations)
{
    auto stationNB = GetStationNotebook();
    if (stationNB && !stations.empty())
    {
        stationNB->Freeze();
        for (const auto &s : stations)
        {
            auto station = std::dynamic_pointer_cast<StationNode>(s);
            if (station)
            {
                wxString uuidName = station->GetUUIDTag();
                wxSize size = wxDefaultSize;
                auto initImg = station->GetImage();
                if (!initImg.empty())
                {
                    size = wxSize(initImg.cols, initImg.rows);
                }
                auto imgPanel = new CVImagePanel(stationNB, GetNextCVStationWinName(), uuidName, wxDefaultSize);
                imgPanel->SetImage(initImg);
                ConnectCanvas(imgPanel);
                stationNB->AddPage(imgPanel, station->GetTitle(), true);
            }
        }
        stationNB->Thaw();
    }
}

void RootFrame::OnDeleteStations(const SPModelNodeVector &stations)
{
    auto stationNB = GetStationNotebook();
    if (!stations.empty() && stationNB)
    {
        stationNB->Freeze();
        for (const auto &station : stations)
        {
            wxString uuidName = station->GetUUIDTag();
            auto cPages = stationNB->GetPageCount();
            wxWindow *stationWnd = nullptr;
            decltype(cPages) stationPageIdx = 0;
            for (decltype(cPages) idx = 0; idx<cPages; ++idx)
            {
                auto wnd = stationNB->GetPage(idx);
                if (wnd && uuidName == wnd->GetName())
                {
                    stationWnd = wnd;
                    stationPageIdx = idx;
                    break;
                }
            }

            if (stationWnd)
            {
                stationNB->DeletePage(stationPageIdx);
            }
        }
        stationNB->Thaw();
    }
}

void RootFrame::OnAddGeoms(const SPModelNodeVector &geoms)
{
    UpdateGeoms(&CairoCanvas::DrawDrawables, geoms);
}

void RootFrame::OnDeleteGeoms(const SPModelNodeVector &geoms)
{
    UpdateGeoms(&CairoCanvas::EraseDrawables, geoms);
}

void RootFrame::OnGlowGeom(const SPModelNode &geom)
{
    auto drawable = std::dynamic_pointer_cast<DrawableNode>(geom);
    auto station  = std::dynamic_pointer_cast<StationNode>(geom->GetParent());
    if (drawable && station)
    {
        auto imgPanel = FindImagePanelByStation(station);
        if (imgPanel)
        {
            imgPanel->GetCanvas()->HighlightDrawable(drawable);
        }
    }
}

void RootFrame::OnDimGeom(const SPModelNode &geom)
{
    auto drawable = std::dynamic_pointer_cast<DrawableNode>(geom);
    auto station = std::dynamic_pointer_cast<StationNode>(geom->GetParent());
    if (drawable && station)
    {
        auto imgPanel = FindImagePanelByStation(station);
        if (imgPanel)
        {
            imgPanel->GetCanvas()->DimDrawable(drawable);
        }
    }
}

void RootFrame::OnNewProject(ModelEvent& e)
{
    auto projModel = dynamic_cast<const ProjTreeModel *>(e.GetModel());
    if (projModel)
    {
        auto stationNB = GetStationNotebook();
        if (stationNB)
        {
            stationNB->Freeze();
            stationNB->DeleteAllPages();
            stationNB->Thaw();
        }
    }
    e.Skip();
}

void RootFrame::OnLoadProject(ModelEvent& e)
{
    auto projModel = dynamic_cast<const ProjTreeModel *>(e.GetModel());
    if (projModel)
    {
        auto stationNB = GetStationNotebook();
        if (stationNB)
        {
            stationNB->Freeze();
            stationNB->DeleteAllPages();
            const auto &stations = projModel->GetAllStations();
            for (const auto &station : stations)
            {
                if (station)
                {
                    wxString uuidName = station->GetUUIDTag();
                    wxSize size = wxDefaultSize;
                    auto initImg = station->GetImage();
                    if (!initImg.empty())
                    {
                        size = wxSize(initImg.cols, initImg.rows);
                    }
                    auto imgPanel = new CVImagePanel(stationNB, GetNextCVStationWinName(), uuidName, size);
                    ConnectCanvas(imgPanel);
                    imgPanel->SetImage(initImg);
                    stationNB->AddPage(imgPanel, station->GetTitle(), true);
                }
            }
            stationNB->Thaw();
        }
    }
    e.Skip();
}

void RootFrame::OnViewMainTool(wxCommandEvent& e)
{
    auto &pane = wxAuiMgr_.GetPane(mainToolPanelName_);
    bool v = pane.IsShown();
    pane.Show(!v);
    wxAuiMgr_.Update();
}

void RootFrame::OnViewImage(wxCommandEvent& e)
{
    auto &pane = wxAuiMgr_.GetPane(stationNotebookName_);
    bool v = pane.IsShown();
    pane.Show(!v);
    wxAuiMgr_.Update();
}

void RootFrame::OnViewProject(wxCommandEvent& e)
{
    auto &pane = wxAuiMgr_.GetPane(projPanelName_);
    bool v = pane.IsShown();
    pane.Show(!v);
    wxAuiMgr_.Update();
}

void RootFrame::OnViewLog(wxCommandEvent& e)
{
    auto &pane = wxAuiMgr_.GetPane(logPanelName_);
    bool v = pane.IsShown();
    pane.Show(!v);
    wxAuiMgr_.Update();
}

void RootFrame::OnViewToolboxBar(wxCommandEvent& e)
{
    auto &pane = wxAuiMgr_.GetPane(toolBoxBarName_);
    bool v = pane.IsShown();
    pane.Show(!v);
    wxAuiMgr_.Update();
}

void RootFrame::OnViewDefaultLayout(wxCommandEvent& e)
{
    wxAuiMgr_.LoadPerspective(initialPerspective_);
    wxAuiMgr_.Update();
}

void RootFrame::OnUpdateUI(wxUpdateUIEvent& e)
{
    switch (e.GetId())
    {
    case spamID_VIEW_MAIN_TOOL:   e.Check(wxAuiMgr_.GetPane(mainToolPanelName_).IsShown());   break;
    case spamID_VIEW_IMAGE:       e.Check(wxAuiMgr_.GetPane(stationNotebookName_).IsShown()); break;
    case spamID_VIEW_PROJECT:     e.Check(wxAuiMgr_.GetPane(projPanelName_).IsShown());       break;
    case spamID_VIEW_LOG:         e.Check(wxAuiMgr_.GetPane(logPanelName_).IsShown());        break;
    case spamID_VIEW_TOOLBOX_BAR: e.Check(wxAuiMgr_.GetPane(toolBoxBarName_).IsShown());      break;
    case wxID_UNDO:               e.Enable(SpamUndoRedo::IsUndoable());                       break;
    case wxID_REDO:               e.Enable(SpamUndoRedo::IsRedoable());                       break;
    default: break;
    }
}

void RootFrame::OnStationActivated(wxDataViewEvent &e)
{
    auto node = static_cast<ModelNode*>(e.GetItem().GetID());
    auto sNode = dynamic_cast<const StationNode*>(node);
    auto stationNB = GetStationNotebook();
    if (sNode && stationNB)
    {
        auto cPages = stationNB->GetPageCount();
        std::vector<wxString> pageUUIDs(cPages);

        std::size_t nPageIndex = 0;
        std::generate(pageUUIDs.begin(), pageUUIDs.end(), [&]() { return stationNB->GetPage(nPageIndex++)->GetName(); });
        auto fIt = std::find(pageUUIDs.begin(), pageUUIDs.end(), sNode->GetUUIDTag());
        if (fIt != pageUUIDs.end())
        {
            auto stationIndex = std::distance(pageUUIDs.begin(), fIt);
            auto currSel = stationNB->GetSelection();
            if (currSel != stationIndex)
            {
                stationNB->SetSelection(std::distance(pageUUIDs.begin(), fIt));
            } 
        }
        else
        {
            stationNB->Freeze();
            wxSize size = wxDefaultSize;
            auto initImg = sNode->GetImage();
            if (!initImg.empty())
            {
                size = wxSize(initImg.cols, initImg.rows);
            }
            wxString uuidName = sNode->GetUUIDTag();
            auto imgPanel = new CVImagePanel(stationNB, GetNextCVStationWinName(), uuidName, wxDefaultSize);
            imgPanel->SetImage(initImg);
            ConnectCanvas(imgPanel);
            stationNB->AddPage(imgPanel, sNode->GetTitle(), true);
            stationNB->Thaw();
        }
    }
}

void RootFrame::OnSetTileLayout(wxCommandEvent& e)
{
    auto stationNB = GetStationNotebook();
    if (stationNB)
    {
        StringDict layout;
        wxString   perspective;
        stationNB->SavePerspective(layout, perspective);
        auto m = GetProjTreeModel();
        if (m)
        {
            auto prj = m->GetProject();
            if (prj)
            {
                prj->SetPerspective(perspective);
                m->SetModified(true);
            }

            for (const auto &l : layout)
            {
                auto s = m->FindStationByUUID(l.first);
                if (s)
                {
                    s->SetTaberName(l.second);
                    m->SetModified(true);
                }
            }
        }
    }
}

void RootFrame::OnTileLayout(wxCommandEvent& e)
{
    auto stationNB = GetStationNotebook();
    if (stationNB)
    {
        StringDict  layout;
        std::string perspective;
        std::set<std::string> allTaberNames;

        auto m = GetProjTreeModel();
        if (m)
        {
            auto prj = m->GetProject();
            if (prj)
            {
                perspective = prj->GetPerspective();
            }

            auto ss = m->GetAllStations();

            for (const auto &s : ss)
            {
                wxString uuidTag   = s->GetUUIDTag();
                std::string taberName = s->GetTaberName();
                if (!taberName.empty())
                {
                    allTaberNames.insert(taberName);
                    layout[uuidTag] = taberName;
                }
            }
        }

        if (!perspective.empty() && !layout.empty())
        {
            auto allPaneNames = GetAllTabPaneNames(perspective);
            auto pred = [&allTaberNames](const std::string &n) { return allTaberNames.cend() != allTaberNames.find(n); };
            if (std::all_of(allPaneNames.cbegin(), allPaneNames.cend(), pred))
            {
                stationNB->Freeze();
                stationNB->LoadPerspective(layout, perspective);
                stationNB->Thaw();
            }
        }
    }
}

void RootFrame::OnStackLayout(wxCommandEvent& e)
{
    auto stationNB = GetStationNotebook();
    if (stationNB)
    {
        stationNB->Freeze();
        stationNB->StackAllPages();
        stationNB->Thaw();
    }
}

void RootFrame::OnDropImage(DropImageEvent& e)
{
    imageFileHistory_.AddFileToHistory(e.GetImageFilePath());
    auto imgPanel = dynamic_cast<CVImagePanel *>(e.GetDropTarget());
    if (imgPanel)
    {
        scale_ = imgPanel->LoadImageFromFile(e.GetImageFilePath());
        auto stationNB = GetStationNotebook();
        if (stationNB)
        {
            auto extCtrl = stationNB->FindExtensionCtrlByPage(imgPanel);
            if (imgPanel->HasImage())
            {
                SetStationImage(imgPanel->GetName(), imgPanel->GetImage());
                EnablePageImageTool(extCtrl, true);
                SyncScale(extCtrl);
            }
            else
            {
                EnablePageImageTool(extCtrl, false);
            }
        }
    }
    e.Skip();
}

void RootFrame::OnTabExtension(wxAuiNotebookEvent& e)
{
    auto extCtrl = dynamic_cast<wxControl *>(e.GetEventObject());
    if (extCtrl)
    {
        auto tbStation = MakeStationToolBar(extCtrl);
        wxSizer * const sizerRoot = new wxBoxSizer(wxHORIZONTAL);
        sizerRoot->Add(tbStation, wxSizerFlags().Border(wxALL, 0).CenterVertical().Proportion(1))->SetId(kSpamImageToolBar);
        sizerRoot->Add(new wxStaticLine(extCtrl, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), wxSizerFlags().Border(wxALL, 0).Expand());
        extCtrl->SetSize(tbStation->GetBestSize());
        extCtrl->SetSizer(sizerRoot);
        extCtrl->GetSizer()->SetSizeHints(extCtrl);
    }
}

void RootFrame::OnPageSelect(wxAuiNotebookEvent& e)
{
    wxAuiNotebook* nb = dynamic_cast<wxAuiNotebook *>(e.GetEventObject());
    if (nb)
    {
        auto sel = e.GetSelection();
        if (wxNOT_FOUND != sel)
        {
            auto page = nb->GetPage(sel);
            auto extCtrl = nb->FindExtensionCtrlByPage(page);
            CVImagePanel *imgPage = dynamic_cast<CVImagePanel *>(page);
            if (imgPage && imgPage->HasImage())
            {
                scale_ = imgPage->GetScale();
                EnablePageImageTool(extCtrl, true);
                SyncScale(extCtrl);
            }
            else
            {
                EnablePageImageTool(extCtrl, false);
            }
        }
    }
}

void RootFrame::OnAuiPageClosed(wxAuiManagerEvent& e)
{
    auto paneInfo = e.GetPane();
    if (paneInfo && paneInfo->window)
    {
        int toolPageId = paneInfo->window->GetId();
        if (toolPageId<kSpamID_TOOLPAGE_GUARD && toolPageId>=kSpamID_TOOLPAGE_PROBE)
        {
            wxAuiToolBar* tbBar = dynamic_cast<wxAuiToolBar*>(wxAuiMgr_.GetPane(toolBoxBarName_).window);
            if (tbBar)
            {
                auto numTools = static_cast<int>(tbBar->GetToolCount());
                for (int t = 0; t<numTools; ++t)
                {
                    auto toolItem = tbBar->FindToolByIndex(t);
                    toolItem->SetState(0);
                }
            }
        }

        ToolBox *toolBox = dynamic_cast<ToolBox *>(paneInfo->window);
        if (toolBox)
        {
            toolBox->QuitToolbox();
        }
    }
}

void RootFrame::OnZoomIn(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomIn, cmd);
}

void RootFrame::OnZoomOut(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomOut, cmd);
}

void RootFrame::OnZoomExtent(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomExtent, cmd);
}

void RootFrame::OnZoomOriginal(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomOriginal, cmd);
}

void RootFrame::OnZoomHalf(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomHalf, cmd);
}

void RootFrame::OnZoomDouble(wxCommandEvent &cmd)
{
    Zoom(&CVImagePanel::ZoomDouble, cmd);
}

void RootFrame::OnEnterScale(wxCommandEvent& e)
{
    wxVariant *var = dynamic_cast<wxVariant *>(e.GetEventUserData());
    if (var)
    {
        wxControl *extCtrl = dynamic_cast<wxControl *>(var->GetWxObjectPtr());
        wxAuiNotebook *nb = GetStationNotebook();
        if (extCtrl && nb)
        {
            wxSizerItem* sizerItem = extCtrl->GetSizer()->GetItemById(kSpamImageToolBar);
            if (sizerItem)
            {
                auto tb = dynamic_cast<wxToolBar *>(sizerItem->GetWindow());
                if (tb)
                {
                    if (!tb->TransferDataFromWindow())
                    {
                        tb->TransferDataToWindow();
                    }
                }
            }

            wxWindow* page = nb->FindPageByExtensionCtrl(extCtrl);
            CVImagePanel *imgPanelPage = dynamic_cast<CVImagePanel *>(page);
            if (imgPanelPage)
            {
                sizerItem = imgPanelPage->GetSizer()->GetItemById(kSpamImageCanvas);
                if (sizerItem)
                {
                    auto cvWidget = dynamic_cast<CairoCanvas *>(sizerItem->GetWindow());
                    if (cvWidget)
                    {
                        if (!cvWidget->HasFocus())
                        {
                            cvWidget->SetFocus();
                        }

                        cvWidget->ScaleImage(scale_ / 100);
                    }
                }
            }
        }
    }
}

void RootFrame::OnSelectScale(wxCommandEvent &e)
{
    OnEnterScale(e);
}

void RootFrame::OnToolboxInfo(wxCommandEvent& e)
{
    ClickToolbox(e, kSpam_TOOLBOX_PROBE);
}

void RootFrame::OnToolboxGeom(wxCommandEvent& e)
{
    ClickToolbox(e, kSpam_TOOLBOX_GEOM);
}

void RootFrame::OnToolboxMatch(wxCommandEvent& e)
{
    ClickToolbox(e, kSpam_TOOLBOX_MATCH);
}

void RootFrame::OnToolboxStyle(wxCommandEvent& e)
{
    ClickToolbox(e, kSpam_TOOLBOX_STYLE);
}

wxAuiNotebook *RootFrame::CreateStationNotebook()
{
    long style = wxAUI_NB_BOTTOM | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_WINDOWLIST_BUTTON | wxNO_BORDER;
    wxAuiNotebook* nb = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, style);
    nb->Bind(wxEVT_AUINOTEBOOK_TAB_EXTENSION, &RootFrame::OnTabExtension, this, wxID_ANY);
    nb->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &RootFrame::OnPageSelect, this, wxID_ANY);
    return nb;
}

std::string RootFrame::GetNextCVStationWinName()
{
    auto cStr = std::to_string(cCVStation_++);
    return std::string("cvStationWin") + cStr;
}

std::vector<std::string> RootFrame::GetAllTabPaneNames(const std::string &perspective)
{
    std::vector<std::string> paneNames;
    std::vector<std::string> strPanes;
    boost::split(strPanes, perspective, boost::is_any_of("|"), boost::token_compress_on);
    for (const auto &strPane : strPanes)
    {
        std::vector<std::string> strAttrs;
        boost::split(strAttrs, strPane, boost::is_any_of(";"), boost::token_compress_on);
        for (const auto &strAttr : strAttrs)
        {
            std::vector<std::string> strValPairs;
            boost::split(strValPairs, strAttr, boost::is_any_of("="), boost::token_compress_on);
            if (2== strValPairs.size() && ("name"==strValPairs[0]))
            {
                if ("dummy" != strValPairs[1])
                {
                    paneNames.push_back(strValPairs[1]);
                }
            }
        }
    }

    return paneNames;
}

wxToolBar *RootFrame::MakeStationToolBar(wxWindow *parent)
{
    constexpr int sz = 16;
    wxToolBar *tb = new wxToolBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
    tb->SetMargins(wxSize(0, 0));
    tb->SetToolBitmapSize(wxSize(sz, sz));

    wxString choices[] =
    {
        wxT("3.125 %"),
        wxT("6.25 %"),
        wxT("12.5 %"),
        wxT("25 %"),
        wxT("50 %"),
        wxT("100 %"),
        wxT("150 %"),
        wxT("200 %"),
        wxT("400 %"),
        wxT("800 %")
    };

    PercentValidator<double> val(3, &scale_, wxNUM_VAL_NO_TRAILING_ZEROES);
    val.SetRange(0.01, 10000);

    auto choice = new wxComboBox(tb, kSpamID_SCALE_CHOICE, wxEmptyString, wxDefaultPosition, wxSize(72, 20), 10, choices, wxCB_DROPDOWN | wxTE_PROCESS_ENTER, val);
    choice->SetMargins(0, 0);
    choice->SetSelection(5);

    tb->AddControl(choice, wxT("Scale Factor"));
    tb->AddSeparator();
    tb->AddTool(kSpamID_ZOOM_OUT, wxT("Zoom Out"), wxBitmap(wxT("res/zoom_out_16.png"), wxBITMAP_TYPE_PNG), wxNullBitmap, wxITEM_DROPDOWN);

    wxMenu* menu = new wxMenu;
    wxBitmap zoomInBM(wxT("res/zoom_in_16.png"), wxBITMAP_TYPE_PNG);
    wxBitmap zoomExBM(wxT("res/zoom_extent_16.png"), wxBITMAP_TYPE_PNG);
    wxBitmap zoom11BM(wxT("res/zoom_original_16.png"), wxBITMAP_TYPE_PNG);
    wxBitmap zoom12BM(wxT("res/zoom_half_16.png"), wxBITMAP_TYPE_PNG);
    wxBitmap zoom21BM(wxT("res/zoom_double_16.png"), wxBITMAP_TYPE_PNG);
    menu->AppendCheckItem(kSpamID_ZOOM_IN, wxT("Zoom In"))->SetBitmaps(zoomInBM, zoomInBM);
    menu->AppendCheckItem(kSpamID_ZOOM_EXTENT, wxT("Zoom Extent"))->SetBitmaps(zoomExBM, zoomExBM);
    menu->AppendSeparator();
    menu->AppendCheckItem(kSpamID_ZOOM_ORIGINAL, wxT("Zoom 1:1"))->SetBitmaps(zoom11BM, zoom11BM);
    menu->AppendCheckItem(kSpamID_ZOOM_HALF, wxT("Zoom Half"))->SetBitmaps(zoom12BM, zoom12BM);
    menu->AppendCheckItem(kSpamID_ZOOM_DOUBLE, wxT("Zoom Double"))->SetBitmaps(zoom21BM, zoom21BM);
    tb->SetDropdownMenu(kSpamID_ZOOM_OUT, menu);
    tb->Realize();

    tb->Bind(wxEVT_TOOL,           &RootFrame::OnZoomOut,      this, kSpamID_ZOOM_OUT,      wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    menu->Bind(wxEVT_TOOL,         &RootFrame::OnZoomIn,       this, kSpamID_ZOOM_IN,       wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    menu->Bind(wxEVT_TOOL,         &RootFrame::OnZoomExtent,   this, kSpamID_ZOOM_EXTENT,   wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    menu->Bind(wxEVT_TOOL,         &RootFrame::OnZoomOriginal, this, kSpamID_ZOOM_ORIGINAL, wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    menu->Bind(wxEVT_TOOL,         &RootFrame::OnZoomHalf,     this, kSpamID_ZOOM_HALF,     wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    menu->Bind(wxEVT_TOOL,         &RootFrame::OnZoomDouble,   this, kSpamID_ZOOM_DOUBLE,   wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    choice->Bind(wxEVT_TEXT_ENTER, &RootFrame::OnEnterScale,   this, kSpamID_SCALE_CHOICE,  wxID_ANY, new wxVariant(parent, wxT("extCtrl")));
    choice->Bind(wxEVT_COMBOBOX,   &RootFrame::OnSelectScale,  this, kSpamID_SCALE_CHOICE,  wxID_ANY, new wxVariant(parent, wxT("extCtrl")));

    tb->EnableTool(kSpamID_SCALE_CHOICE, false);
    tb->EnableTool(kSpamID_ZOOM_OUT, false);

    return tb;
}

void RootFrame::Zoom(double (CVImagePanel::*zoomFun)(bool), wxCommandEvent &cmd)
{
    wxVariant *var = dynamic_cast<wxVariant *>(cmd.GetEventUserData());
    if (var)
    {
        wxControl *extCtrl = dynamic_cast<wxControl *>(var->GetWxObjectPtr());
        wxAuiNotebook *nb = GetStationNotebook();
        if (extCtrl && nb)
        {
            wxWindow* page = nb->FindPageByExtensionCtrl(extCtrl);
            CVImagePanel *imgPanelPage = dynamic_cast<CVImagePanel *>(page);
            if (imgPanelPage)
            {
                scale_ = (imgPanelPage->*zoomFun)(false);
                SyncScale(extCtrl);
            }
        }
    }
}

void RootFrame::UpdateGeoms(void (CairoCanvas::*updateFun)(const SPDrawableNodeVector &), const SPModelNodeVector &geoms)
{
    auto stationNB = GetStationNotebook();
    std::map<wxString, std::pair<CairoCanvas *, SPDrawableNodeVector>> canvs;

    for (const auto &geom : geoms)
    {
        if (geom && stationNB)
        {
            auto station = geom->GetParent();
            if (station)
            {
                wxString uuidName = station->GetUUIDTag();
                auto cPages = stationNB->GetPageCount();
                wxWindow *stationWnd = nullptr;
                decltype(cPages) stationPageIdx = 0;
                for (decltype(cPages) idx = 0; idx<cPages; ++idx)
                {
                    auto wnd = stationNB->GetPage(idx);
                    if (wnd && uuidName == wnd->GetName())
                    {
                        stationWnd = wnd;
                        stationPageIdx = idx;
                        break;
                    }
                }

                if (stationWnd)
                {
                    CVImagePanel *imgPanel = dynamic_cast<CVImagePanel *>(stationWnd);
                    if (imgPanel && imgPanel->GetCanvas())
                    {
                        auto &canv = canvs[uuidName];
                        canv.first = imgPanel->GetCanvas();
                        canv.second.push_back(std::dynamic_pointer_cast<DrawableNode>(geom));
                    }
                }

            }
        }
    }

    for (auto &canv : canvs)
    {
        (canv.second.first->*updateFun)(canv.second.second);
    }
}

void RootFrame::SyncScale(wxControl *extCtrl)
{
    if (extCtrl)
    {
        wxSizerItem* sizerItem = extCtrl->GetSizer()->GetItemById(kSpamImageToolBar);
        if (sizerItem)
        {
            auto tb = dynamic_cast<wxToolBar *>(sizerItem->GetWindow());
            if (tb)
            {
                tb->TransferDataToWindow();
            }
        }
    }
}

void RootFrame::EnablePageImageTool(wxControl *extCtrl, bool bEnable)
{
    if (extCtrl)
    {
        wxSizerItem* sizerItem = extCtrl->GetSizer()->GetItemById(kSpamImageToolBar);
        if (sizerItem)
        {
            auto tb = dynamic_cast<wxToolBar *>(sizerItem->GetWindow());
            if (tb)
            {
                tb->EnableTool(kSpamID_SCALE_CHOICE, bEnable);
                tb->EnableTool(kSpamID_ZOOM_OUT, bEnable);
            }
        }
    }
}

void RootFrame::SetStationImage(const wxString &uuidStation, const cv::Mat &img)
{
    auto m = GetProjTreeModel();
    if (m)
    {
        auto s = m->FindStationByUUID(uuidStation);
        if (s)
        {
            s->SetImage(img);
            m->SetModified(true);
        }
    }
}

wxPoint RootFrame::GetToolboxPopupPosition(const wxSize &popUpSize)
{
    wxPoint pos(0, 0);
    auto nb = GetStationNotebook();
    if (nb)
    {
        auto nbRect = nb->GetScreenRect();
        auto borderSize = nb->GetWindowBorderSize();
        nbRect = nbRect.Deflate(borderSize);

        pos.y = nbRect.GetTop() + 3;
        pos.x = nbRect.GetRight() - 3 - popUpSize.GetWidth();
    }

    return pos;
}

void RootFrame::ToggleToolboxPane(const int boxShow)
{
    for (const auto &label : toolBoxLabels)
    {
        auto &tbPanelInfo = wxAuiMgr_.GetPane(label);
        if (tbPanelInfo.IsShown())
        {
            tbPanelInfo.Show(false);

            ToolBox *toolBox = dynamic_cast<ToolBox *>(tbPanelInfo.window);
            if (toolBox)
            {
                toolBox->QuitToolbox();
            }
        }
    }

    if (boxShow<kSpam_TOOLBOX_GUARD && boxShow>= kSpam_TOOLBOX_PROBE)
    {
        auto &tbPanelInfo = wxAuiMgr_.GetPane(toolBoxLabels[boxShow]);
        if (!tbPanelInfo.IsShown())
        {
            tbPanelInfo.Show(true);

            ToolBox *toolBox = dynamic_cast<ToolBox *>(tbPanelInfo.window);
            if (toolBox)
            {
                toolBox->OpenToolbox();
            }
        }
    }

    wxAuiMgr_.Update();
}

void RootFrame::ConnectCanvas(CVImagePanel *imgPanel)
{
    if (imgPanel)
    {
        CairoCanvas *canv = imgPanel->GetCanvas();
        if (canv)
        {
            canv->sig_Char.connect(std::bind(&Spamer::OnCanvasChar, spamer_.get(), std::placeholders::_1));
            canv->sig_KeyUp.connect(std::bind(&Spamer::OnCanvasKeyUp, spamer_.get(), std::placeholders::_1));
            canv->sig_KeyDown.connect(std::bind(&Spamer::OnCanvasKeyDown, spamer_.get(), std::placeholders::_1));
            canv->sig_MouseMotion.connect(std::bind(&Spamer::OnCanvasMouseMotion, spamer_.get(), std::placeholders::_1));
            canv->sig_LeftMouseUp.connect(std::bind(&Spamer::OnCanvasLeftMouseUp, spamer_.get(), std::placeholders::_1));
            canv->sig_LeftMouseDown.connect(std::bind(&Spamer::OnCanvasLeftMouseDown, spamer_.get(), std::placeholders::_1));
            canv->sig_LeftDClick.connect(std::bind(&Spamer::OnCanvasLeftDClick, spamer_.get(), std::placeholders::_1));
            canv->sig_MiddleDown.connect(std::bind(&Spamer::OnCanvasMiddleDown, spamer_.get(), std::placeholders::_1));
            canv->sig_EnterWindow.connect(std::bind(&Spamer::OnCanvasEnter, spamer_.get(), std::placeholders::_1));
            canv->sig_LeaveWindow.connect(std::bind(&Spamer::OnCanvasLeave, spamer_.get(), std::placeholders::_1));
        }
    }
}

void RootFrame::ClickToolbox(wxCommandEvent& e, const int toolIndex)
{
    auto eo = e.GetEventObject();
    wxAuiToolBar* tbBar = dynamic_cast<wxAuiToolBar*>(eo);
    if (tbBar)
    {
        bool isChecked = e.IsChecked();
        ToggleToolboxPane(isChecked ? toolIndex : kSpam_TOOLBOX_GUARD);

        auto thisIdx = tbBar->GetToolIndex(e.GetId());
        auto numTools = static_cast<int>(tbBar->GetToolCount());
        for (int t = 0; t<numTools; ++t)
        {
            if (thisIdx != t)
            {
                auto toolItem = tbBar->FindToolByIndex(t);
                toolItem->SetState(0);
            }
        }
    }
}

CVImagePanel *RootFrame::FindImagePanelByStation(const SPStationNode &station) const
{
    auto stationNB = GetStationNotebook();
    if (stationNB && station)
    {
        wxString uuidName = station->GetUUIDTag();
        auto cPages = stationNB->GetPageCount();
        wxWindow *stationWnd = nullptr;
        for (decltype(cPages) idx = 0; idx<cPages; ++idx)
        {
            auto wnd = stationNB->GetPage(idx);
            if (wnd && uuidName == wnd->GetName())
            {
                stationWnd = wnd;
                break;
            }
        }

        if (stationWnd)
        {
            return dynamic_cast<CVImagePanel *>(stationWnd);
        }
    }

    return nullptr;
}