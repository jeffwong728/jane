#include "cairocanvas.h"
#include <ui/cmndef.h>
#include <ui/spam.h>
#include <ui/projs/stationnode.h>
#include <ui/projs/geomnode.h>
#include <ui/projs/drawablenode.h>
#include <ui/projs/projtreemodel.h>
#include <ui/projs/fixednode.h>
#include <ui/projs/profilenode.h>
#include <ui/cmds/geomcmd.h>
#include <ui/cmds/transcmd.h>
#include <ui/imgproc/basic.h>
#include <wx/graphics.h>
#include <wx/popupwin.h>
#include <wx/artprov.h>
#include <algorithm>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <ui/misc/scopedtimer.h>
#include <ui/evts.h>
#include <2geom/cairo-path-sink.h>
#include <tbb/tick_count.h>

CairoCanvas::CairoCanvas(wxWindow* parent, const std::string &cvWinName, const wxString &uuidStation, const wxSize& size)
: wxScrolledCanvas(parent, wxID_ANY, wxPoint(0, 0), size)
, cvWndName_(cvWinName.c_str())
, stationUUID_(uuidStation)
, anchorX_(0)
, anchorY_(0)
{
    SetScrollRate(6, 6);
    Bind(wxEVT_SIZE,             &CairoCanvas::OnSize,            this, wxID_ANY);
    Bind(wxEVT_CHAR,             &CairoCanvas::OnChar,            this, wxID_ANY);
    Bind(wxEVT_ENTER_WINDOW,     &CairoCanvas::OnEnterWindow,     this, wxID_ANY);
    Bind(wxEVT_LEAVE_WINDOW,     &CairoCanvas::OnLeaveWindow,     this, wxID_ANY);
    Bind(wxEVT_LEFT_DOWN,        &CairoCanvas::OnLeftMouseDown,   this, wxID_ANY);
    Bind(wxEVT_LEFT_UP,          &CairoCanvas::OnLeftMouseUp,     this, wxID_ANY);
    Bind(wxEVT_MOTION,           &CairoCanvas::OnMouseMotion,     this, wxID_ANY);
    Bind(wxEVT_LEFT_DCLICK,      &CairoCanvas::OnLeftDClick,      this, wxID_ANY);
    Bind(wxEVT_MIDDLE_DOWN,      &CairoCanvas::OnMiddleDown,      this, wxID_ANY);
    Bind(wxEVT_PAINT,            &CairoCanvas::OnPaint,           this, wxID_ANY);
    Bind(wxEVT_ERASE_BACKGROUND, &CairoCanvas::OnEraseBackground, this, wxID_ANY);
    Bind(wxEVT_CONTEXT_MENU,     &CairoCanvas::OnContextMenu,     this, wxID_ANY);
    Bind(wxEVT_MENU,             &CairoCanvas::OnDeleteEntities,  this, kSpamID_DELETE_ENTITIES);
    Bind(wxEVT_MENU,             &CairoCanvas::OnPushToBack,      this, kSpamID_PUSH_TO_BACK);
    Bind(wxEVT_MENU,             &CairoCanvas::OnBringToFront,    this, kSpamID_BRING_TO_FRONT);
    Bind(wxEVT_TIMER,            &CairoCanvas::OnTipTimer,        this, wxID_ANY);

    SetDropTarget(new DnDImageFile(parent));
}

CairoCanvas::~CairoCanvas()
{
}

void CairoCanvas::ShowImage(const cv::Mat &img)
{
    int dph = img.depth();
    int cnl = img.channels();
    srcImg_ = img;

    if (CV_8U == dph && (1==cnl || 3==cnl || 4==cnl))
    {
        if (1 == cnl)
        {
            cv::cvtColor(img, srcMat_, cv::COLOR_GRAY2BGRA);
        }
        else if (3 == cnl)
        {
            cv::cvtColor(img, srcMat_, cv::COLOR_BGR2BGRA);
        }
        else
        {
            srcMat_ = img;
        }

        wxSize sViewPort = GetClientSize();
        wxSize sToSize = GetDispMatSize(sViewPort, wxSize(srcMat_.cols, srcMat_.rows));
        SetVirtualSize(sToSize);
        MoveAnchor(sViewPort, sToSize);
        ScaleShowImage(sToSize);
    }
}

void CairoCanvas::SwitchImage(const std::string &iName)
{
    auto itF = imgBufferZone_.find(iName);
    if (itF != imgBufferZone_.end())
    {
        if (SpamEntityType::kET_IMAGE == itF->second.iType)
        {
            bool haveVisibleRgn = !rgnsVisiable_.empty();
            ClearVisiableRegions();
            if (itF->second.iSrcImage.ptr() != srcImg_.ptr())
            {
                ShowImage(itF->second.iSrcImage);
            }
            else
            {
                if (haveVisibleRgn)
                {
                    Refresh(false);
                }
            }
        }
        else
        {
            if (SpamEntityType::kET_REGION == itF->second.iType)
            {
                if (1!=rgnsVisiable_.size() || rgnsVisiable_[0]!=itF->second.iName.ToStdString())
                {
                    ClearVisiableRegions();
                    SetVisiableRegion(itF->second.iName.ToStdString());
                    Refresh(false);
                }
            }
        }
    }
}

void CairoCanvas::ExtentImage()
{
    if (HasImage())
    {
        wxSize sViewPort = GetSize();
        wxSize sToSize = GetFitSize(sViewPort, wxSize(srcMat_.cols, srcMat_.rows));
        SetVirtualSize(sToSize);
        MoveAnchor(sViewPort, sToSize);
        ScaleShowImage(sToSize);
    }
}

void CairoCanvas::ScaleImage(double scaleVal)
{
    if (HasImage() && scaleVal>0 && scaleVal<10.000001)
    {
        wxSize sToSize(wxRound(srcMat_.cols*scaleVal), wxRound(srcMat_.rows*scaleVal));
        if (sToSize.GetWidth()>3 && sToSize.GetHeight()>3)
        {
            wxSize sViewPort = GetClientSize();
            SetVirtualSize(sToSize);
            MoveAnchor(sViewPort, sToSize);
            ScaleShowImage(sToSize);
        }
    }
}

void CairoCanvas::ScaleUp(double val)
{
    if (HasImage() && val > 0)
    {
        double fval = GetMatScale();
        if (fval > 0)
        {
            double newScale = std::min(10.0, fval * (1 + val));
            ScaleImage(newScale);
        }
    }
}

void CairoCanvas::ScaleDown(double val)
{
    if (HasImage() && val > 0 && val < 1)
    {
        double fval = GetMatScale();
        if (fval > 0)
        {
            double newScale = std::max(0.01, fval * (1 - val));
            ScaleImage(newScale);
        }
    }
}

double CairoCanvas::GetMatScale() const
{ 
    if (srcMat_.empty() || disMat_.empty())
    {
        return 1;
    }
    else
    {
        return (disMat_.rows + 0.0) / srcMat_.rows;
    } 
}

bool CairoCanvas::IsInImageRect(const wxPoint &pt) const
{
    if (!srcImg_.empty())
    {
        int dph = srcImg_.depth();
        int cnl = srcImg_.channels();

        wxPoint imgPt = wxPoint(ScreenToImage(pt));
        wxRect imgRect(wxPoint(0, 0), wxPoint(srcImg_.cols - 1, srcImg_.rows - 1));
        if (CV_8U == dph && (1 == cnl || 3 == cnl || 4 == cnl) && imgRect.Contains(imgPt))
        {
            return true;
        }
    }

    return false;
}

wxPoint CairoCanvas::ScreenToDispImage(const wxPoint &pt) const
{
    if (srcMat_.empty() || disMat_.empty())
    {
        return wxPoint();
    }
    else
    {
        return CalcUnscrolledPosition(pt) - wxSize(anchorX_, anchorY_);
    }
}

wxPoint CairoCanvas::DispImageToScreen(const wxPoint &pt) const
{
    return CalcScrolledPosition(pt + wxSize(anchorX_, anchorY_));
}

wxPoint CairoCanvas::DispImageToDevice(const wxPoint &pt) const
{
    return pt + wxSize(anchorX_, anchorY_);
}

wxRect CairoCanvas::DispImageToScreen(const wxRect &rc) const
{
    wxPoint tl = DispImageToScreen(rc.GetTopLeft());
    wxPoint br = DispImageToScreen(rc.GetBottomRight());
    return wxRect(tl, br);
}

wxRect CairoCanvas::DispImageToDevice(const wxRect &rc) const
{
    wxPoint tl = DispImageToDevice(rc.GetTopLeft());
    wxPoint br = DispImageToDevice(rc.GetBottomRight());
    return wxRect(tl, br);
}

wxRealPoint CairoCanvas::ScreenToImage(const wxPoint &pt) const
{
    auto uspt = CalcUnscrolledPosition(pt) - wxSize(anchorX_, anchorY_);

    if (srcMat_.empty() || disMat_.empty())
    {
        return wxRealPoint(uspt.x, uspt.y);
    }
    else
    {
        auto sX = srcMat_.cols / (disMat_.cols + 0.0);
        auto sY = srcMat_.rows / (disMat_.rows + 0.0);

        return wxRealPoint((uspt.x + 0.5)*sX - 0.5, (uspt.y + 0.5)*sY - 0.5);
    }
}

wxRect CairoCanvas::ImageToScreen(const Geom::OptRect &rc) const
{
    if (rc)
    {
        int x = wxRound(rc->left()*GetMatScale());
        int y = wxRound(rc->top()*GetMatScale());
        int w = wxRound(rc->width()*GetMatScale());
        int h = wxRound(rc->height()*GetMatScale());
        return DispImageToScreen(wxRect(x, y, w, h));
    }
    else
    {
        return wxRect();
    }
}

wxRect CairoCanvas::ImageToDevice(const Geom::OptRect &rc) const
{
    if (rc)
    {
        int x = wxRound(rc->left()*GetMatScale());
        int y = wxRound(rc->top()*GetMatScale());
        int w = wxRound(rc->width()*GetMatScale());
        int h = wxRound(rc->height()*GetMatScale());
        return DispImageToDevice(wxRect(x, y, w, h));
    }
    else
    {
        return wxRect();
    }
}

void CairoCanvas::DrawDrawables(const SPDrawableNodeVector &des)
{
    InvalidateDrawable(des);
}

void CairoCanvas::EraseDrawables(const SPDrawableNodeVector &des)
{
    InvalidateDrawable(des);
}

void CairoCanvas::DrawRegion(const cv::Ptr<cv::mvlab::Region> &rgn)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (rgn && station)
        {
            std::vector<SPRegionNode> &rgnArr = rgns_[rgn.get()];
            if (rgnArr.empty())
            {
                cv::Rect tBBox;
                for (int rr = 0; rr < rgn->Count(); ++rr)
                {
                    cv::Ptr<cv::mvlab::Region> subRgn = rgn->SelectObj(rr);
                    SPRegionNode rgnNode = std::make_shared<RegionNode>(subRgn, station);

                    wxColour lineColor = station->GetColor();
                    wxColour fillColor = station->GetFillColor();
                    if (rgn->Count() > 1)
                    {
                        lineColor = station->GetNextColor();
                        fillColor = lineColor;
                    }

                    rgnNode->SetLineWidth(station->GetLineWidth());
                    rgnNode->SetDraw(std::string("fill") == station->GetDraw());
                    rgnNode->SetColor(lineColor);
                    rgnNode->SetFillColor(fillColor);

                    tBBox |= subRgn->BoundingBox();
                    rgnArr.push_back(rgnNode);
                }

                Geom::OptRect boundRect(tBBox.tl().x, tBBox.tl().y, tBBox.br().x, tBBox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(station->GetLineWidth()), cvRound(station->GetLineWidth()));
                    Refresh(false, &invalidRect);
                }
            }
        }
    }
}

void CairoCanvas::EraseRegion(const cv::Ptr<cv::mvlab::Region> &rgn)
{
    if (rgn)
    {
        auto it = rgns_.find(rgn.get());
        if (it != rgns_.cend())
        {
            for (const SPRegionNode &rgnNode : it->second)
            {
                cv::Rect bbox = rgnNode->BoundingBox();
                Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(rgnNode->GetLineWidth()), cvRound(rgnNode->GetLineWidth()));
                    Refresh(false, &invalidRect);
                }
            }
            rgns_.erase(it);
        }
    }
}

void CairoCanvas::DrawContour(const cv::Ptr<cv::mvlab::Contour> &contr)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (contr && station)
        {
            std::vector<DispContour> &contrArr = contrs_[contr.get()];
            if (contrArr.empty())
            {
                for (int cc = 0; cc < contr->Count(); ++cc)
                {
                    cv::Ptr<cv::mvlab::Contour> subContr = contr->SelectObj(cc);

                    contrArr.emplace_back();
                    DispContour &dContr = contrArr.back();
                    dContr.cvContr = subContr;

                    dContr.curves.reserve(subContr->CountCurves());
                    dContr.bboxs.reserve(subContr->CountCurves());
                    subContr->GetTestClosed(dContr.isClosed);

                    for (int c = 0; c < subContr->CountCurves(); ++c)
                    {
                        dContr.curves.emplace_back();
                        dContr.bboxs.emplace_back();
                        subContr->SelectPoints(c, dContr.curves.back());
                        dContr.bboxs.back() = cv::mvlab::BoundingBox(dContr.curves.back());
                    }

                    wxColour lineColor = station->GetColor();
                    wxColour fillColor = station->GetFillColor();
                    if (contr->Count() > 1)
                    {
                        lineColor = station->GetNextColor();
                        fillColor = lineColor;
                    }

                    dContr.lineWidth = station->GetLineWidth();
                    dContr.lineStyle = 0;
                    dContr.drawMode = std::string("fill") == station->GetDraw();
                    dContr.lineColor[0] = lineColor.Red() / 255.0;
                    dContr.lineColor[1] = lineColor.Green() / 255.0;
                    dContr.lineColor[2] = lineColor.Blue() / 255.0;
                    dContr.lineColor[3] = lineColor.Alpha() / 255.0;
                    dContr.fillColor[0] = fillColor.Red() / 255.0;
                    dContr.fillColor[1] = fillColor.Green() / 255.0;
                    dContr.fillColor[2] = fillColor.Blue() / 255.0;
                    dContr.fillColor[3] = fillColor.Alpha() / 255.0;

                    cv::Rect bbox = subContr->BoundingBox();
                    Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                    if (boundRect && boundRect->area() > 0.)
                    {
                        wxRect invalidRect = ImageToScreen(boundRect);
                        invalidRect.Inflate(cvRound(dContr.lineWidth), cvRound(dContr.lineWidth));
                        Refresh(false, &invalidRect);
                    }
                }
            }
        }
    }
}

void CairoCanvas::EraseContour(const cv::Ptr<cv::mvlab::Contour> &contr)
{
    if (contr)
    {
        auto it = contrs_.find(contr.get());
        if (it != contrs_.cend())
        {
            for (const DispContour &dContr : it->second)
            {
                cv::Rect bbox = dContr.cvContr->BoundingBox();
                Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(dContr.lineWidth), cvRound(dContr.lineWidth));
                    Refresh(false, &invalidRect);
                }
            }
            contrs_.erase(it);
        }
    }
}

void CairoCanvas::DrawMarker(const Geom::PathVector &marker)
{
    Geom::OptRect boundRect = marker.boundsFast();
    if (boundRect && boundRect->area() > 0.)
    {
        markers_.push_back(marker);
        wxRect invalidRect = ImageToScreen(boundRect).Inflate(32, 32);
        Refresh(false, &invalidRect);
    }
}

void CairoCanvas::RefreshDrawable(const SPDrawableNode &de)
{
    InvalidateDrawable(SPDrawableNodeVector(1, de));
}

void CairoCanvas::RefreshRect(const Geom::OptRect &invalidRect)
{
    if (invalidRect && invalidRect->area() > 0.)
    {
        wxRect iRect = ImageToScreen(invalidRect);
        ConpensateHandle(iRect);
        Refresh(false, &iRect);
    }
}

void CairoCanvas::RefreshRects(const std::vector<Geom::OptRect> &invalidRects)
{
    Geom::OptRect boundRect;
    for (const auto &irc : invalidRects)
    {
        boundRect.unionWith(irc);
    }

    if (boundRect && boundRect->area() > 0.)
    {
        wxRect invalidRect = ImageToScreen(boundRect);
        ConpensateHandle(invalidRect);
        Refresh(false, &invalidRect);
    }
}

void CairoCanvas::HighlightDrawable(const SPDrawableNode &de)
{
    InvalidateDrawable(SPDrawableNodeVector(1, de));
}

void CairoCanvas::DimDrawable(const SPDrawableNode &de)
{
    InvalidateDrawable(SPDrawableNodeVector(1, de));
}

void CairoCanvas::DrawPathVector(const Geom::PathVector &pth, const Geom::OptRect &rect)
{
    rubber_band_ = Geom::OptRect();
    path_vector_ = pth;

    if (rect) 
    {
        wxRect invalidRect = ImageToScreen(rect);
        ConpensateHandle(invalidRect);
        Refresh(false, &invalidRect);
    }
}

void CairoCanvas::DrawBox(const Geom::OptRect &oldRect, const Geom::OptRect &newRect)
{
    Geom::OptRect rect = oldRect;
    rect.unionWith(newRect);

    if (!rect) {
        return;
    }

    rubber_band_ = newRect;
    if (rubber_band_)
    {
        wxRect rcRubber = ImageToDevice(rubber_band_);
        rubber_band_->setTop(rcRubber.GetTop());
        rubber_band_->setBottom(rcRubber.GetBottom());
        rubber_band_->setLeft(rcRubber.GetLeft());
        rubber_band_->setRight(rcRubber.GetRight());
    }

    const double iTol = 1.0 / GetMatScale();
    Geom::OptRect irects[4];
    if (oldRect)
    {
        Geom::Coord t = oldRect->top();
        Geom::Coord b = oldRect->bottom();
        Geom::Coord l = oldRect->left();
        Geom::Coord r = oldRect->right();

        irects[0] = Geom::OptRect(l - iTol, t - iTol, r + iTol, t + iTol);
        irects[1] = Geom::OptRect(l - iTol, b - iTol, r + iTol, b + iTol);
        irects[2] = Geom::OptRect(l - iTol, t + iTol, l + iTol, b - iTol);
        irects[3] = Geom::OptRect(r - iTol, t + iTol, r + iTol, b - iTol);
    }

    if (newRect)
    {
        Geom::Coord t = newRect->top();
        Geom::Coord b = newRect->bottom();
        Geom::Coord l = newRect->left();
        Geom::Coord r = newRect->right();

        irects[0].unionWith(Geom::OptRect(l - iTol, t - iTol, r + iTol, t + iTol));
        irects[1].unionWith(Geom::OptRect(l - iTol, b - iTol, r + iTol, b + iTol));
        irects[2].unionWith(Geom::OptRect(l - iTol, t + iTol, l + iTol, b - iTol));
        irects[3].unionWith(Geom::OptRect(r - iTol, t + iTol, r + iTol, b - iTol));
    }

    for (const auto &iRect : irects)
    {
        wxRect invalidRect = ImageToScreen(iRect);
        Refresh(false, &invalidRect);
    }
}

WPGeomNode CairoCanvas::AddRect(const RectData &rd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("rect%d"), cRect_++);
            auto cmd = std::make_shared<CreateRectCmd>(model, station, title, rd);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());

            return cmd->GetGeom();
        }
    }

    return WPGeomNode();
}

WPGeomNode CairoCanvas::AddLine(const LineData &ld)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("line%d"), cLine_++);
            auto cmd = std::make_shared<CreateLineCmd>(model, station, title, ld);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
            return cmd->GetGeom();
        }
    }
    return WPGeomNode();
}

WPGeomNode CairoCanvas::AddEllipse(const GenericEllipseArcData &ed)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("ellipse%d"), cEllipse_++);
            auto cmd = std::make_shared<CreateEllipseCmd>(model, station, title, ed);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
            return cmd->GetGeom();
        }
    }
    return WPGeomNode();
}

void CairoCanvas::DoEdit(const int toolId, const SPDrawableNodeVector &selEnts, const SpamMany &mementos)
{
    switch (toolId)
    {
    case kSpamID_TOOLBOX_GEOM_TRANSFORM: DoTransform(selEnts, mementos); break;
    case kSpamID_TOOLBOX_GEOM_EDIT:      DoNodeEdit(selEnts, mementos);  break;
    default: break;
    }
}

WPGeomNode CairoCanvas::AddPolygon(const PolygonData &pd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("polygon%d"), cPolygon_++);
            auto cmd = std::make_shared<CreatePolygonCmd>(model, station, title, pd);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
            return cmd->GetGeom();
        }
    }
    return WPGeomNode();
}

WPGeomNode CairoCanvas::AddBeziergon(const BezierData &bd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("beziergon%d"), cBeziergon_++);
            auto cmd = std::make_shared<CreateBeziergonCmd>(model, station, title, bd);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
            return cmd->GetGeom();
        }
    }
    return WPGeomNode();
}

void CairoCanvas::DoTransform(const SPDrawableNodeVector &selEnts, const SpamMany &mementos)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            auto cmd = std::make_shared<TransformCmd>(model, station, selEnts, mementos);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

void CairoCanvas::DoNodeEdit(const SPDrawableNodeVector &selEnts, const SpamMany &mementos)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            auto cmd = std::make_shared<NodeEditCmd>(model, station, selEnts, mementos);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

void CairoCanvas::DoUnion(const SPDrawableNodeVector &selEnts)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("beziergon%d"), cBeziergon_++);
            SPGeomNodeVector geoms;
            for (const auto &drawable : selEnts)
            {
                auto g = std::dynamic_pointer_cast<GeomNode>(drawable);
                if (g)
                {
                    geoms.push_back(g);
                }
            }

            auto cmd = std::make_shared<UnionGeomsCmd>(model, geoms, title);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

void CairoCanvas::DoIntersection(const SPDrawableNodeVector &selEnts)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("beziergon%d"), cBeziergon_++);
            SPGeomNodeVector geoms;
            for (const auto &drawable : selEnts)
            {
                auto g = std::dynamic_pointer_cast<GeomNode>(drawable);
                if (g)
                {
                    geoms.push_back(g);
                }
            }

            auto cmd = std::make_shared<IntersectionGeomsCmd>(model, geoms, title);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

void CairoCanvas::DoXOR(const SPDrawableNode &dn1, const SPDrawableNode &dn2)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("beziergon%d"), cBeziergon_++);
            auto g1 = std::dynamic_pointer_cast<GeomNode>(dn1);
            auto g2 = std::dynamic_pointer_cast<GeomNode>(dn2);

            auto cmd = std::make_shared<XORGeomsCmd>(model, g1, g2, title);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

void CairoCanvas::DoDifference(const SPDrawableNode &dn1, const SPDrawableNode &dn2)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            wxString title = wxString::Format(wxT("beziergon%d"), cBeziergon_++);
            auto g1 = std::dynamic_pointer_cast<GeomNode>(dn1);
            auto g2 = std::dynamic_pointer_cast<GeomNode>(dn2);

            auto cmd = std::make_shared<DiffGeomsCmd>(model, g1, g2, title);
            cmd->Do();
            SpamUndoRedo::AddCommand(cmd);
            Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
        }
    }
}

SPStationNode CairoCanvas::GetStation() const
{
    auto model = Spam::GetModel();
    if (model)
    {
        return model->FindStationByUUID(stationUUID_);
    }

    return SPStationNode();
}

SPDrawableNode CairoCanvas::FindDrawable(const Geom::Point &pt)
{
    SPStationNode sn = GetStation();
    if (sn)
    {
        return sn->FindDrawable(pt);
    }

    return SPDrawableNode();
}

SPDrawableNode CairoCanvas::FindDrawable(const Geom::Point &pt, const double sx, const double sy, SelectionData &sd)
{
    sd.ss = SelectionState::kSelNone;
    sd.hs = HitState::kHsNone;
    sd.id = -1;
    sd.subid = -1;
    sd.master = 0;

    const SelectionFilter *sf = Spam::GetSelectionFilter();
    for (const auto &rgnsItem : rgns_)
    {
        for (const SPRegionNode &rgnNode : rgnsItem.second)
        {
            if (sf && sf->IsPass(rgnNode))
            {
                auto ht = rgnNode->HitTest(pt, sx, sy);
                if (ht.hs != HitState::kHsNone)
                {
                    sd = ht;
                    return rgnNode;
                }
            }
        }
    }

    SPStationNode sn = GetStation();
    if (sn)
    {
        return sn->FindDrawable(pt, sx, sy, sd);
    }

    return SPDrawableNode();
}

SPDrawableNodeVector CairoCanvas::GetAllFixed() const
{
    SPDrawableNodeVector allFixed;
    for (const auto &rgnsItem : rgns_)
    {
        for (const SPRegionNode &rgnNode : rgnsItem.second)
        {
            allFixed.push_back(rgnNode);
        }
    }
    return allFixed;
}

cv::Ptr<cv::mvlab::Region> CairoCanvas::FindRegion(const Geom::Point &pt)
{
    cv::Point point{cvRound(pt.x()), cvRound(pt.y())};
    for (const auto &rgnsItem : rgns_)
    {
        for (const auto &rgnItem : rgnsItem.second)
        {
            if (rgnItem->GetRegion()->TestPoint(point))
            {
                return rgnItem->GetRegion();
            }
        }
    }

    return nullptr;
}

cv::Ptr<cv::mvlab::Contour> CairoCanvas::FindContour(const Geom::Point &pt)
{
    return nullptr;
}

void CairoCanvas::SelectDrawable(const Geom::Rect &box, SPDrawableNodeVector &ents)
{
    SPStationNode sn = GetStation();
    if (sn)
    {
        sn->SelectDrawable(box, ents);
    }

    const SelectionFilter *sf = Spam::GetSelectionFilter();
    for (const auto &rgnsItem : rgns_)
    {
        for (const SPRegionNode &rgnNode : rgnsItem.second)
        {
            if (sf && sf->IsPass(rgnNode) && rgnNode->IsIntersection(box))
            {
                ents.push_back(rgnNode);
            }
        }
    }
}

void CairoCanvas::ModifyDrawable(const SPDrawableNode &ent, const Geom::Point &pt, const SelectionData &sd, const int editMode)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            if (ent)
            {
                boost::any memento = ent->CreateMemento();
                SpamResult sr = ent->Modify(pt, editMode, sd);
                if (SpamResult::kSR_SUCCESS == sr)
                {
                    auto cmd = std::make_shared<NodeModifyCmd>(model, station, ent, memento);
                    cmd->Do();
                    SpamUndoRedo::AddCommand(cmd);
                    Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
                }

                if (SpamResult::kSR_FAILURE == sr)
                {
                    Spam::SetStatus(StatusIconType::kSIT_ERROR, wxT("Modify geometry failed!"));
                }
            }
        }
    }
}

void CairoCanvas::DismissInstructionTip()
{
    if (tip_)
    {
        tip_->Show(false);
        tip_.reset();
    }

    if (tipTimer_)
    {
        tipTimer_->Stop();
        tipTimer_->StartOnce(500);
    }
    else
    {
        tipTimer_ = std::make_unique<wxTimer>(this, wxID_ANY);
        tipTimer_->StartOnce(500);
    }
}

void CairoCanvas::SetInstructionTip(std::vector<wxString> &&messages, const wxPoint &pos)
{
    tipPos_ = pos;

    if (!std::equal(tipMessages_.cbegin(), tipMessages_.cend(), messages.cbegin(), messages.cend()))
    {
        tipMessages_ = messages;
    }
}

void CairoCanvas::SetInstructionTip(const wxString &message, const std::string &icon, const wxPoint &pos)
{
    if (icon != tipIcon_) tipIcon_ = icon;
    tipPos_ = pos;

    if (1 == tipMessages_.size() && tipMessages_.front() == message)
    {
        return;
    }

    tipMessages_.clear();
    tipMessages_.push_back(message);
}

void CairoCanvas::StopInstructionTip()
{
    if (tip_)
    {
        tip_.reset();
    }

    if (tipTimer_)
    {
        tipTimer_.reset();
    }
}

void CairoCanvas::ShowPixelValue(const wxPoint &pos)
{
    if (!srcImg_.empty())
    {
        int dph = srcImg_.depth();
        int cnl = srcImg_.channels();

        wxPoint imgPt = wxPoint(ScreenToImage(pos));
        wxRect imgRect(wxPoint(0, 0), wxPoint(srcImg_.cols-1, srcImg_.rows-1));
        if (CV_8U == dph && (1 == cnl || 3 == cnl || 4 == cnl) && imgRect.Contains(imgPt))
        {
            std::vector<wxString> messages;
            if (1 == cnl)
            {
                auto pPixel = srcImg_.data + imgPt.y * srcImg_.step1() + imgPt.x;
                auto pixelVal = pPixel[0];
                messages.push_back(wxString::Format(wxT("Grayscale: %u"), pPixel[0]));
            }
            else if (3 == cnl)
            {
                auto pPixel = srcImg_.data + imgPt.y * srcImg_.step1() + imgPt.x * 3;
                messages.push_back(wxString::Format(wxT("<span foreground='red'>R: %u</span>"), pPixel[2]));
                messages.push_back(wxString::Format(wxT("<span foreground='green'>G: %u</span>"), pPixel[1]));
                messages.push_back(wxString::Format(wxT("<span foreground='blue'>B: %u</span>"), pPixel[0]));
            }
            else
            {
                auto pPixel = srcImg_.data + imgPt.y * srcImg_.step1() + imgPt.x * 4;
                messages.push_back(wxString::Format(wxT("<span foreground='red'>R: %u</span>"), pPixel[2]));
                messages.push_back(wxString::Format(wxT("<span foreground='green'>G: %u</span>"), pPixel[1]));
                messages.push_back(wxString::Format(wxT("<span foreground='blue'>B: %u</span>"), pPixel[0]));
                messages.push_back(wxString::Format(wxT("<span foreground='black'>Alpha: %u</span>"), pPixel[3]));
            }

            SetInstructionTip(std::move(messages), pos);
        }
    }
}

void CairoCanvas::PopupImageInfomation(const wxPoint &pos)
{
    if (!srcImg_.empty())
    {
        int dph = srcImg_.depth();
        int cnl = srcImg_.channels();
        wxString::const_pointer dphStr[CV_16F + 1] = { wxT("8U"), wxT("8S"), wxT("16U"), wxT("16S"), wxT("32S"), wxT("32F"), wxT("64F"), wxT("16F") };

        std::vector<wxString> messages;
        messages.push_back(wxString::Format(wxT("<b>Channels:</b> %d"),  cnl));
        messages.push_back(wxString::Format(wxT("<b>Depth:</b> %s"),  dphStr[dph]));
        messages.push_back(wxString::Format(wxT("<b>Width:</b> %d"),  srcImg_.cols));
        messages.push_back(wxString::Format(wxT("<b>Height:</b> %d"),  srcImg_.rows));
        messages.push_back(wxString::Format(wxT("<b>Pixel Size:</b> %zd"), srcImg_.elemSize()));
        messages.push_back(wxString::Format(wxT("<b>Pixel Size1:</b> %zd"), srcImg_.elemSize1()));
        messages.push_back(wxString::Format(wxT("<b>Stride:</b> %zd"), srcImg_.step1()));

        InformationTip *info = new InformationTip(this, messages, wxBitmap());
        info->Position(ClientToScreen(pos), wxSize(0, 5));
        info->Popup(this);
    }
}

void CairoCanvas::ClearSelectRegions()
{
    for (auto &rgnsItem : rgns_)
    {
        for (auto &rgnItem : rgnsItem.second)
        {
            if (rgnItem->IsSelected())
            {
                rgnItem->ClearSelection();
                cv::Rect bbox = rgnItem->GetRegion()->BoundingBox();
                Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(rgnItem->GetLineWidth()), cvRound(rgnItem->GetLineWidth()));
                    Refresh(false, &invalidRect);
                }
            }
        }
    }
}

void CairoCanvas::ClearSelectContours()
{
    for (auto &contrsItem : contrs_)
    {
        for (auto &contrItem : contrsItem.second)
        {
            if (contrItem.selected)
            {
                contrItem.selected = false;
                cv::Rect bbox = contrItem.cvContr->BoundingBox();
                Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(contrItem.lineWidth), cvRound(contrItem.lineWidth));
                    Refresh(false, &invalidRect);
                }
            }
        }
    }
}

void CairoCanvas::PopupRegionInfomation(const wxPoint &pos, const cv::Ptr<cv::mvlab::Region> &rgn)
{
    for (auto &rgnsItem : rgns_)
    {
        for (auto &rgnItem : rgnsItem.second)
        {
            if (rgnItem->IsSelected())
            {
                rgnItem->ClearSelection();
                cv::Rect bbox = rgnItem->BoundingBox();
                Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                if (boundRect && boundRect->area() > 0.)
                {
                    wxRect invalidRect = ImageToScreen(boundRect);
                    invalidRect.Inflate(cvRound(rgnItem->GetLineWidth()), cvRound(rgnItem->GetLineWidth()));
                    Refresh(false, &invalidRect);
                }
            }
            else
            {
                if (rgn == rgnItem->GetRegion())
                {
                    rgnItem->Select(0);
                    cv::Rect bbox = rgnItem->BoundingBox();
                    Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
                    if (boundRect && boundRect->area() > 0.)
                    {
                        wxRect invalidRect = ImageToScreen(boundRect);
                        invalidRect.Inflate(cvRound(rgnItem->GetLineWidth()), cvRound(rgnItem->GetLineWidth()));
                        Refresh(false, &invalidRect);
                    }
                }
            }
        }
    }

    std::vector<wxString> messages;
    messages.push_back(wxString::Format(wxT("<b>Area:</b> %.1f"), rgn->Area()));
    messages.push_back(wxString::Format(wxT("<b>Contour Length:</b> %.1f"), rgn->Contlength()));
    messages.push_back(wxString::Format(wxT("<b>Hole Area:</b> %.1f"), rgn->AreaHoles()));
    messages.push_back(wxString::Format(wxT("<b>Circularity:</b> %.3f"), rgn->Circularity()));
    messages.push_back(wxString::Format(wxT("<b>Compactness:</b> %.3f"), rgn->Compactness()));
    messages.push_back(wxString::Format(wxT("<b>Convexity:</b> %.3f"), rgn->Convexity()));
    messages.push_back(wxString::Format(wxT("<b>Connected Component:</b> %d"), rgn->CountConnect()));
    messages.push_back(wxString::Format(wxT("<b>Hole Number:</b> %d"), rgn->CountHoles()));
    messages.push_back(wxString::Format(wxT("<b>Orientation:</b> %.3f"), rgn->Orientation()));

    InformationTip *info = new InformationTip(this, messages, wxBitmap());
    info->Position(ClientToScreen(pos), wxSize(0, 5));
    info->Popup(this);
}

void CairoCanvas::PopupContourInfomation(const wxPoint &pos, const cv::Ptr<cv::mvlab::Contour> &contr)
{
}

void CairoCanvas::PushImageIntoBufferZone(const std::string &name)
{
    wxSize thumbnailSize(80, 80);
    double scaleX = (thumbnailSize.x + 0.0) / srcMat_.cols;
    double scaleY = (thumbnailSize.y + 0.0) / srcMat_.rows;

    cv::Mat thumbMat;
    if (scaleX < 1.0 || scaleY < 1.0)
    {
        cv::Size newSize{ thumbnailSize.x, thumbnailSize.y };
        if (scaleX < scaleY)
        {
            newSize.height = static_cast<int>(scaleX * srcMat_.rows);
        }
        else
        {
            newSize.width = static_cast<int>(scaleY * srcMat_.cols);
        }

        cv::resize(srcMat_, thumbMat, newSize, 0.0, 0.0, cv::INTER_AREA);
    }
    else
    {
        thumbMat = srcMat_;
    }

    wxImage thumbImg;
    thumbImg.Create(thumbMat.cols, thumbMat.rows, false);
    for (int r = 0; r<thumbMat.rows; ++r)
    {
        for (int c = 0; c<thumbMat.cols; ++c)
        {
            auto pPixel = thumbMat.data + r * thumbMat.step1() + c * 4;
            thumbImg.SetRGB(c, r, pPixel[2], pPixel[1], pPixel[0]);
        }
    }

    ImageBufferItem bufItem{ SpamEntityType::kET_IMAGE, name, stationUUID_, srcImg_, wxBitmap(thumbImg) };
    auto insResult = imgBufferZone_.insert(std::make_pair(name, bufItem));
    if (!insResult.second)
    {
        insResult.first->second.iName = bufItem.iName;
        insResult.first->second.iStationUUID = bufItem.iStationUUID;
        insResult.first->second.iSrcImage = bufItem.iSrcImage;
        insResult.first->second.iThumbnail = bufItem.iThumbnail;
        sig_ImageBufferItemUpdate(bufItem);
    }
    else
    {
        sig_ImageBufferItemAdd(bufItem);
    }
}

void CairoCanvas::PushRegionsIntoBufferZone(const std::string &name, const cv::Ptr<cv::mvlab::Region> &rgns)
{
    wxSize thumbnailSize(80, 80);
    double scaleX = (thumbnailSize.x + 0.0) / srcMat_.cols;
    double scaleY = (thumbnailSize.y + 0.0) / srcMat_.rows;

    cv::Mat thumbMat;
    if (scaleX < 1.0 || scaleY < 1.0)
    {
        cv::Size newSize{ thumbnailSize.x, thumbnailSize.y };
        if (scaleX < scaleY)
        {
            newSize.height = static_cast<int>(scaleX * srcMat_.rows);
        }
        else
        {
            newSize.width = static_cast<int>(scaleY * srcMat_.cols);
        }

        thumbMat = cv::Mat::zeros(newSize, CV_8UC4);
    }
    else
    {
        thumbMat = cv::Mat::zeros(srcMat_.rows, srcMat_.cols, CV_8UC4);
    }

    rgns->Draw(thumbMat, cv::Scalar());

    wxImage thumbImg;
    thumbImg.Create(thumbMat.cols, thumbMat.rows, false);
    for (int r = 0; r<thumbMat.rows; ++r)
    {
        for (int c = 0; c<thumbMat.cols; ++c)
        {
            auto pPixel = thumbMat.data + r * thumbMat.step1() + c * 4;
            thumbImg.SetRGB(c, r, pPixel[2], pPixel[1], pPixel[0]);
        }
    }

    ImageBufferItem bufItem{ SpamEntityType::kET_REGION, name, stationUUID_, srcImg_, wxBitmap(thumbImg) };
    auto insResult = rgnBufferZone_.insert(std::make_pair(name, rgns));
    if (!insResult.second)
    {
        insResult.first->second = rgns;
    }

    auto insImgResult = imgBufferZone_.insert(std::make_pair(name, bufItem));
    if (!insImgResult.second)
    {
        insImgResult.first->second.iName = bufItem.iName;
        insImgResult.first->second.iStationUUID = bufItem.iStationUUID;
        insImgResult.first->second.iSrcImage = bufItem.iSrcImage;
        insImgResult.first->second.iThumbnail = bufItem.iThumbnail;
        sig_ImageBufferItemUpdate(bufItem);
    }
    else
    {
        sig_ImageBufferItemAdd(bufItem);
    }
}

void CairoCanvas::EraseBoxArea(const Geom::Rect &boxArea)
{
    std::vector<Geom::PathVector> remainingMarkers;
    for (Geom::PathVector &marker : markers_)
    {
        const auto oRect = marker.boundsFast();
        if (oRect && boxArea.contains(oRect)) {
            continue;
        }

        remainingMarkers.push_back(std::move(marker));
    }

    markers_.swap(remainingMarkers);
    wxRect invalidRect = ImageToScreen(boxArea).Inflate(32, 32);
    Refresh(false, &invalidRect);
}

void CairoCanvas::EraseFullArea()
{
    Geom::Rect iRect;
    for (const Geom::PathVector &marker : markers_)
    {
        const auto oRect = marker.boundsFast();
        if (oRect)
        {
            iRect.unionWith(oRect);
        }
    }

    for (const auto &rgn : rgns_)
    {
        for (const auto &dRgn : rgn.second)
        {
            cv::Rect bbox = dRgn->BoundingBox();
            Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
            iRect.unionWith(boundRect);
        }
    }

    for (const auto &contr : contrs_)
    {
        for (const auto &dContr : contr.second)
        {
            cv::Rect bbox = dContr.cvContr->BoundingBox();
            Geom::OptRect boundRect(bbox.tl().x, bbox.tl().y, bbox.br().x, bbox.br().y);
            iRect.unionWith(boundRect);
        }
    }

    rgns_.clear();
    contrs_.clear();
    markers_.clear();

    if (iRect.area() > 0.)
    {
        wxRect invalidRect = ImageToScreen(iRect).Inflate(32, 32);
        Refresh(false, &invalidRect);
    }
}

void CairoCanvas::UpdateProfileNode(const Geom::Point &begPoint, const Geom::Point &endPoint)
{
    if (!profileNode_)
    {
        profileNode_ = std::make_shared<ProfileNode>(nullptr);
    }
    else
    {
        Geom::OptRect rcBox = profileNode_->GetBoundingBox();
        if (rcBox && rcBox->area() > 0.)
        {
            wxRect invalidRect = ImageToScreen(rcBox);
            ConpensateHandle(invalidRect);
            Refresh(false, &invalidRect);
        }
    }

    if (profileNode_)
    {
        profileNode_->SetData(begPoint, endPoint);
        Geom::OptRect rcBox = profileNode_->GetBoundingBox();
        if (rcBox && rcBox->area() > 0.)
        {
            wxRect invalidRect = ImageToScreen(rcBox);
            ConpensateHandle(invalidRect);
            Refresh(false, &invalidRect);
        }
    }
}

void CairoCanvas::RemoveProfileNode()
{
    if (profileNode_)
    {
        Geom::OptRect rcBox = profileNode_->GetBoundingBox();
        if (rcBox && rcBox->area() > 0.)
        {
            wxRect invalidRect = ImageToScreen(rcBox);
            ConpensateHandle(invalidRect);
            Refresh(false, &invalidRect);
        }
    }

    profileNode_ = nullptr;
}

void CairoCanvas::UpdatePyramid(const Geom::Rect &roiBox, const int pyraLevel)
{
    imgProc_.ipKind = kIPK_PYRAMID;
    imgProc_.roi = Geom::Path(roiBox);
    if (pyraLevel > 1 && !disMat_.empty())
    {
        imgProc_.disMats.clear();
        imgProc_.iParams[cp_ToolProcPyramidLevel] = pyraLevel;
        ImageProcessPyramid();
        Refresh(false);
    }
}

void CairoCanvas::UpdateBinary(const Geom::Rect &roiBox, const int minGray, const int maxGray, const int channel)
{
    imgProc_.ipKind = kIPK_BINARY;
    imgProc_.roi = Geom::Path(roiBox);
    if (!disMat_.empty() && channel>=0 && channel< disMat_.channels() && minGray < maxGray)
    {
        imgProc_.disMats.clear();
        imgProc_.iParams[cp_ToolProcThresholdMin] = minGray;
        imgProc_.iParams[cp_ToolProcThresholdMax] = maxGray;
        imgProc_.iParams[cp_ToolProcThresholdChannel] = channel;
        ImageProcessBinary();
        Refresh(false);
    }
}

void CairoCanvas::UpdateFilter(const Geom::Rect &roiBox, const std::map<std::string, int> &iParams, const std::map<std::string, double> &fParams)
{
    imgProc_.ipKind = kIPK_FILTER;
    imgProc_.roi = Geom::Path(roiBox);
    if (!disMat_.empty())
    {
        imgProc_.disMats.clear();
        imgProc_.iParams = iParams;
        imgProc_.fParams = fParams;
        ImageProcessFilter();
        Refresh(false);
    }
}

void CairoCanvas::UpdateEdge(const Geom::Rect &roiBox, const std::map<std::string, int> &iParams, const std::map<std::string, double> &fParams)
{
    imgProc_.ipKind = kIPK_EDGE;
    imgProc_.roi = Geom::Path(roiBox);
    if (!disMat_.empty())
    {
        imgProc_.disMats.clear();
        imgProc_.iParams = iParams;
        imgProc_.fParams = fParams;
        ImageProcessEdge();
        Refresh(false);
    }
}

void CairoCanvas::UpdateColorConvert(const Geom::Rect &roiBox, const std::map<std::string, int> &iParams, const std::map<std::string, double> &fParams)
{
    imgProc_.ipKind = kIPK_CONVERT;
    imgProc_.roi = Geom::Path(roiBox);
    if (!disMat_.empty())
    {
        imgProc_.disMats.clear();
        imgProc_.iParams = iParams;
        imgProc_.fParams = fParams;
        ImageProcessConvert();
        Refresh(false);
    }
}

void CairoCanvas::RemoveImageProcessData()
{
    imgProc_.ipKind = kIPK_NONE;
    imgProc_.roi.clear();
    imgProc_.disMats.clear();
    imgProc_.iParams.clear();
    imgProc_.fParams.clear();
    Refresh(false);
}

void CairoCanvas::OnSize(wxSizeEvent& e)
{
    if (!disMat_.empty())
    { 
        MoveAnchor(e.GetSize(), wxSize(disMat_.cols, disMat_.rows));
        Refresh(false);
    }
}

void CairoCanvas::OnEnterWindow(wxMouseEvent &e)
{
    sig_EnterWindow(e);
}

void CairoCanvas::OnLeaveWindow(wxMouseEvent &e)
{
    sig_LeaveWindow(e);
}

void CairoCanvas::OnLeftMouseDown(wxMouseEvent &e)
{
    if (!HasFocus())
    {
        SetFocus();
    }
    sig_LeftMouseDown(e);
}

void CairoCanvas::OnLeftMouseUp(wxMouseEvent &e)
{
    sig_LeftMouseUp(e);
}

void CairoCanvas::OnMouseMotion(wxMouseEvent &e)
{
    sig_MouseMotion(e);
}

void CairoCanvas::OnMiddleDown(wxMouseEvent &e)
{
    sig_MiddleDown(e);
}

void CairoCanvas::OnLeftDClick(wxMouseEvent &e)
{
    sig_LeftDClick(e);
}

void CairoCanvas::OnKeyDown(wxKeyEvent &e)
{
    sig_KeyDown(e);
}

void CairoCanvas::OnKeyUp(wxKeyEvent &e)
{
    sig_KeyUp(e);
}

void CairoCanvas::OnChar(wxKeyEvent &e)
{
    sig_Char(e);
}

void CairoCanvas::OnPaint(wxPaintEvent& e)
{
    wxPaintDC dc(this);
    PrepareDC(dc);

    auto cairoCtx = dc.GetImpl()->GetCairoContext();
    if (cairoCtx)
    {
#ifdef __linux__
        auto spCtx = Cairo::RefPtr<Cairo::Context>(new Cairo::Context((cairo_t *) cairoCtx));
#elif _WIN32
        auto spCtx = std::make_shared<Cairo::Context>((cairo_t *)cairoCtx);
#endif
        std::vector<Geom::Rect> invalidRects;
        wxRegionIterator upd(GetUpdateRegion());
        while (upd)
        {
            int vX = upd.GetX();
            int vY = upd.GetY();
            int vW = upd.GetW();
            int vH = upd.GetH();

            wxRealPoint tl = ScreenToImage(wxPoint(vX, vY));
            wxRealPoint br = ScreenToImage(wxPoint(vX + vW, vY + vH));
            invalidRects.emplace_back(tl.x, tl.y, br.x, br.y);

            upd++;
        }

        RenderImage(spCtx);
        RenderMarkers(spCtx, invalidRects);
        RenderRegions(spCtx, invalidRects);
        RenderContours(spCtx, invalidRects);
        RenderEntities(spCtx, invalidRects);
        RenderPath(spCtx);
        RenderRubberBand(spCtx);
    }
}

void CairoCanvas::OnEraseBackground(wxEraseEvent &e)
{
}

void CairoCanvas::OnContextMenu(wxContextMenuEvent& e)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            int numSel = station->GetNumSelected();
            int numDra = station->GetNumDrawable();

            wxMenu menu;
            menu.Append(kSpamID_DELETE_ENTITIES, wxT("Delete"))->Enable(numSel);
            menu.AppendSeparator();
            menu.Append(kSpamID_HIDE_ENTITIES, wxT("Hide"))->Enable(numSel);
            menu.Append(kSpamID_SHOW_ONLY_ENTITIES, wxT("Show Only"))->Enable(numSel);
            menu.Append(kSpamID_SHOW_REVERSE_ENTITIES, wxT("Show Reverse"))->Enable(numDra);
            menu.Append(kSpamID_SHOW_ALL_ENTITIES, wxT("Show All"))->Enable(numDra);
            menu.Append(kSpamID_HIDE_ALL_ENTITIES, wxT("Hide All"))->Enable(numDra);
            menu.AppendSeparator();
            menu.Append(kSpamID_PUSH_TO_BACK, wxT("Push to Back"))->Enable(numDra);
            menu.Append(kSpamID_BRING_TO_FRONT, wxT("Bring to Front"))->Enable(numDra);

            PopupMenu(&menu);
        }
    }
}

void CairoCanvas::OnDeleteEntities(wxCommandEvent &cmd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            SPDrawableNodeVector drawables = station->GeSelected();
            SPGeomNodeVector delGeoms;
            for (const auto &drawable : drawables)
            {
                auto geom = std::dynamic_pointer_cast<GeomNode>(drawable);
                if (geom)
                {
                    delGeoms.push_back(geom);
                }
            }

            if (!delGeoms.empty())
            {
                auto cmd = std::make_shared<DeleteGeomsCmd>(Spam::GetModel(), delGeoms);
                cmd->Do();
                SpamUndoRedo::AddCommand(cmd);
                Spam::SetStatus(StatusIconType::kSIT_NONE, cmd->GetDescription());
            }
        }
    }
}

void CairoCanvas::OnPushToBack(wxCommandEvent &cmd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            SPDrawableNodeVector drawables = station->GeSelected();
            SPGeomNodeVector delGeoms;

            Geom::OptRect refreshRect;
            for (const auto &drawable : drawables)
            {
                station->RemoveChild(drawable.get());
                refreshRect.unionWith(drawable->GetBoundingBox());
            }

            for (const auto &drawable : drawables)
            {
                station->Append(drawable);
            }

            DrawPathVector(Geom::PathVector(), refreshRect);
        }
    }
}

void CairoCanvas::OnBringToFront(wxCommandEvent &cmd)
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            SPDrawableNodeVector drawables = station->GeSelected();
            SPGeomNodeVector delGeoms;

            Geom::OptRect refreshRect;
            for (const auto &drawable : drawables)
            {
                station->RemoveChild(drawable.get());
                refreshRect.unionWith(drawable->GetBoundingBox());
            }

            station->GetChildren().insert(station->GetChildren().begin(), drawables.cbegin(), drawables.cend());
            DrawPathVector(Geom::PathVector(), refreshRect);
        }
    }
}

void CairoCanvas::OnTipTimer(wxTimerEvent &e)
{
    if (tipTimer_ && tipTimer_->GetId() == e.GetTimer().GetId())
    {
        if (!tip_ && !tipMessages_.empty() && !tipMessages_.front().empty())
        {
            wxBitmap iBitmap = Spam::GetBitmap(kICON_PURPOSE_TOOLBOX, tipIcon_);
            if (!iBitmap.IsOk() && !tipIcon_.empty())
            {
                iBitmap = wxArtProvider::GetBitmap(wxART_ERROR, wxART_OTHER);
            }
            if (1== tipMessages_.size())
            {
                tip_ = std::make_unique<InstructionTip>(this, tipMessages_.front(), iBitmap);
            }
            else
            {
                tip_ = std::make_unique<InstructionTip>(this, tipMessages_, iBitmap);
            }

            wxPoint pos = ClientToScreen(tipPos_);
            tip_->Position(pos+wxPoint(35, 0), wxSize());
            tip_->Show();
        }
    }
}

void CairoCanvas::DrawRegions(Cairo::RefPtr<Cairo::Context> &cr)
{
    double ux = 1;
    double uy = 1;
    cr->device_to_user_distance(ux, uy);
    if (ux < uy) ux = uy;

    for (const std::string &rgnName : rgnsVisiable_)
    {
        auto itf = rgnBufferZone_.find(rgnName);
        if (itf != rgnBufferZone_.end())
        {
            cr->save();
            wxColour clr;
            cr->set_source_rgba(clr.Red() / 255.0, clr.Green() / 255.0, clr.Blue() / 255.0, clr.Alpha() / 255.0);
            cr->set_line_width(ux);
            cr->stroke();
            cr->restore();
        }
    }
}

void CairoCanvas::ConpensateHandle(wxRect &invalidRect) const
{
    invalidRect.Inflate(32, 32);
}

void CairoCanvas::InvalidateDrawable(const SPDrawableNodeVector &des)
{
    Geom::OptRect boundRect;
    for (const auto &de : des)
    {
        if (de)
        {
            boundRect.unionWith(de->GetBoundingBox());
        }
    }

    if (boundRect && boundRect->area() > 0.)
    {
        wxRect invalidRect = ImageToScreen(boundRect);
        ConpensateHandle(invalidRect);
        Refresh(false, &invalidRect);
    }
}

void CairoCanvas::RenderImage(Cairo::RefPtr<Cairo::Context> &cr) const
{
    if (!disMat_.empty())
    {
        wxRegionIterator upd(GetUpdateRegion());
        while (upd)
        {
            int vX = upd.GetX();
            int vY = upd.GetY();
            int vW = upd.GetW();
            int vH = upd.GetH();

            wxPoint tl = ScreenToDispImage(wxPoint(vX, vY));
            wxPoint br = ScreenToDispImage(wxPoint(vX + vW, vY + vH));
            wxRect rcInvalid = wxRect(tl, br).Intersect(wxRect(0, 0, disMat_.cols, disMat_.rows));

            if (!rcInvalid.IsEmpty())
            {
                cv::Mat m(disMat_, cv::Range(rcInvalid.GetTop(), rcInvalid.GetBottom()), cv::Range(rcInvalid.GetLeft(), rcInvalid.GetRight()));

                auto imgSurf = Cairo::ImageSurface::create(m.ptr(), Cairo::Format::FORMAT_RGB24, m.cols, m.rows, m.step1());
                cr->set_source(imgSurf, anchorX_ + rcInvalid.GetX(), anchorY_ + rcInvalid.GetY());
                cr->paint();
            }

            if (!imgProc_.disMats.empty())
            {
                int begRow = 0;
                int begCol = 0;

                if (1 == imgProc_.disMats.size())
                {
                    Geom::OptRect vbox = Geom::bounds_fast(imgProc_.roi);
                    if (vbox)
                    {
                        Geom::OptRect ibox(0, 0, srcImg_.cols - 1, srcImg_.rows - 1);
                        if (ibox->contains(vbox->min()) &&
                            imgProc_.disMats.front().cols == disMat_.cols &&
                            imgProc_.disMats.front().rows == disMat_.rows &&
                            vbox->width() < 3 &&
                            vbox->height() < 3)
                        {
                            begRow = 0;
                            begCol = 0;
                        }
                        else
                        {
                            begRow = cvRound(vbox->top()*GetMatScale());
                            begCol = cvRound(vbox->left()*GetMatScale());
                        }
                    }
                }

                for (const cv::Mat &pyraMat : imgProc_.disMats)
                {
                    wxRect rcOld = wxRect(tl, br).Intersect(wxRect(begCol, begRow, pyraMat.cols, pyraMat.rows));
                    if (!rcOld.IsEmpty())
                    {
                        cv::Mat m(pyraMat, cv::Range(rcOld.GetTop() - begRow, rcOld.GetBottom() - begRow), cv::Range(rcOld.GetLeft() - begCol, rcOld.GetRight() - begCol));

                        auto imgSurf = Cairo::ImageSurface::create(m.ptr(), Cairo::Format::FORMAT_RGB24, m.cols, m.rows, m.step1());
                        cr->set_source(imgSurf, anchorX_ + rcOld.GetX(), anchorY_ + rcOld.GetY());
                        cr->paint();
                    }

                    begRow += pyraMat.rows;
                    begCol += pyraMat.cols;
                }
            }

            upd++;
        }
    }
}

void CairoCanvas::RenderMarkers(Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Geom::Rect> &invalidRects) const
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            for (const Geom::PathVector &marker : markers_)
            {
                Geom::PathVector pv = marker * Geom::Translate(0.5, 0.5);
                double ux = station->GetLineWidth(), uy = station->GetLineWidth();
                cr->device_to_user_distance(ux, uy);
                if (ux < uy) ux = uy;

                cr->save();
                cr->translate(anchorX_, anchorY_);
                cr->scale(GetMatScale(), GetMatScale());
                Geom::CairoPathSink cairoPathSink(cr->cobj());
                wxColour c = station->GetColor();
                cairoPathSink.feed(pv);
                cr->set_line_width(ux);
                cr->set_line_cap(Cairo::LineCap::LINE_CAP_SQUARE);
                cr->set_source_rgba(c.Red() / 255.0, c.Green() / 255.0, c.Blue() / 255.0, c.Alpha() / 255.0);
                if (std::string("fill") == station->GetDraw())
                {
                    cr->fill();
                }
                else
                {
                    cr->stroke();
                }
                cr->restore();
            }
        }
    }
}

void CairoCanvas::RenderRegions(Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Geom::Rect> &invalidRects) const
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            cr->save();
            cr->translate(anchorX_, anchorY_);
            cr->scale(GetMatScale(), GetMatScale());
            cr->set_line_cap(Cairo::LineCap::LINE_CAP_SQUARE);

            for (const auto &rgnItem : rgns_)
            {
                for (const auto &dRgn : rgnItem.second)
                {
                    if (!dRgn->GetRegion())
                    {
                        continue;
                    }
                    dRgn->Draw(cr, invalidRects);
                }
            }

            cr->restore();
        }
    }
}

void CairoCanvas::RenderContours(Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Geom::Rect> &invalidRects) const
{
    auto model = Spam::GetModel();
    if (model)
    {
        auto station = model->FindStationByUUID(stationUUID_);
        if (station)
        {
            Geom::Rect bbox;
            cr->save();
            cr->translate(anchorX_, anchorY_);
            cr->scale(GetMatScale(), GetMatScale());
            cr->set_line_cap(Cairo::LineCap::LINE_CAP_SQUARE);

            for (const auto &contrItem : contrs_)
            {
                for (const DispContour &dContr : contrItem.second)
                {
                    if (!dContr.cvContr || dContr.curves.size() != dContr.isClosed.size())
                    {
                        continue;
                    }

                    cr->set_line_width(dContr.lineWidth);
                    cr->set_source_rgba(dContr.lineColor[0], dContr.lineColor[1], dContr.lineColor[2], dContr.lineColor[3]);

                    for (int cc = 0; cc < static_cast<int>(dContr.curves.size()); ++cc)
                    {
                        bbox.setLeft(dContr.bboxs[cc].x);
                        bbox.setRight(dContr.bboxs[cc].x + dContr.bboxs[cc].width);
                        bbox.setTop(dContr.bboxs[cc].y);
                        bbox.setBottom(dContr.bboxs[cc].y + dContr.bboxs[cc].height);
                        if (IsRectNeedRefresh(bbox, invalidRects))
                        {
                            const auto &curve = dContr.curves[cc];
                            if (curve.size() > 1)
                            {
                                cr->move_to(curve.front().x + 0.5f, curve.front().y + 0.5f);
                                for (int vv = 1; vv < static_cast<int>(curve.size()); ++vv)
                                {
                                    cr->line_to(curve[vv].x + 0.5f, curve[vv].y + 0.5f);
                                }
                                if (dContr.isClosed[cc])
                                {
                                    cr->close_path();
                                }
                            }
                        }
                    }

                    cr->stroke();
                }
            }

            cr->restore();
        }
    }
}

void CairoCanvas::RenderRubberBand(Cairo::RefPtr<Cairo::Context> &cr) const
{
    if (!rubber_band_.empty())
    {
        Geom::Path pth(rubber_band_.get());
        pth *= Geom::Translate(0.5, 0.5);
        double ux = 1, uy = 1;
        cr->device_to_user_distance(ux, uy);
        if (ux < uy) ux = uy;

        cr->save();
        Geom::CairoPathSink cairoPathSink(cr->cobj());
        cairoPathSink.feed(pth);
        cr->set_line_width(ux);
        cr->set_line_cap(Cairo::LineCap::LINE_CAP_SQUARE);
        cr->set_source_rgba(1.0, 0.0, 1.0, 0.1);
        cr->fill_preserve();
        cr->set_source_rgba(1.0, 0.0, 1.0, 1.0);
        cr->stroke();
        cr->restore();
    }
}

void CairoCanvas::RenderEntities(Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Geom::Rect> &invalidRects) const
{
    auto station = GetStation();
    if (station)
    {
        cr->save();
        cr->translate(anchorX_, anchorY_);
        cr->scale(GetMatScale(), GetMatScale());
        for (const auto &c : station->GetChildren())
        {
            auto drawable = std::dynamic_pointer_cast<DrawableNode>(c);
            if (drawable)
            {
                drawable->Draw(cr);
            }
        }
        if (profileNode_) profileNode_->Draw(cr, invalidRects);
        cr->restore();
    }
}

void CairoCanvas::RenderPath(Cairo::RefPtr<Cairo::Context> &cr) const
{
    if (!path_vector_.empty())
    {
        cr->save();
        cr->translate(anchorX_, anchorY_);
        cr->scale(GetMatScale(), GetMatScale());

        wxColour strokeColor;
        strokeColor.SetRGBA(SpamConfig::Get<wxUint32>(cp_ToolGeomStrokePaint, wxBLUE->GetRGBA()));

        wxColour fillColor;
        fillColor.SetRGBA(SpamConfig::Get<wxUint32>(cp_ToolGeomFillPaint, wxBLUE->GetRGBA()));

        double ux = SpamConfig::Get<int>(cp_ToolGeomStrokeWidth, 1);
        double uy = ux;
        cr->device_to_user_distance(ux, uy);
        if (ux < uy) ux = uy;

        Geom::Translate aff = Geom::Translate(0.5, 0.5);
        Geom::PathVector paths = path_vector_ * aff;

        Geom::CairoPathSink cairoPathSink(cr->cobj());
        cairoPathSink.feed(paths);
        cr->set_source_rgba(fillColor.Red() / 255.0, fillColor.Green() / 255.0, fillColor.Blue() / 255.0, fillColor.Alpha() / 255.0);
        cr->fill_preserve();
        cr->set_line_width(ux);
        cr->set_source_rgba(strokeColor.Red() / 255.0, strokeColor.Green() / 255.0, strokeColor.Blue() / 255.0, strokeColor.Alpha() / 255.0);
        cr->stroke();
        cr->restore();
    }
}

void CairoCanvas::ConvertToDisplayMats(const std::vector<cv::Mat> &mats, std::vector<cv::Mat> &disMats)
{
    disMats.clear();
    for (const cv::Mat &m : mats)
    {
        if (!m.empty())
        {
            int dph = m.depth();
            int cnl = m.channels();

            if (CV_8U == dph && (1 == cnl || 3 == cnl || 4 == cnl))
            {
                cv::Mat srcMat;
                if (1 == cnl)
                {
                    cv::cvtColor(m, srcMat, cv::COLOR_GRAY2BGRA);
                }
                else if (3 == cnl)
                {
                    cv::cvtColor(m, srcMat, cv::COLOR_BGR2BGRA);
                }
                else
                {
                    srcMat = m;
                }

                const int toW = cvRound(GetMatScale()*srcMat.cols);
                const int toH = cvRound(GetMatScale()*srcMat.rows);

                if (toW == srcMat.cols && toH == srcMat.rows)
                {
                    disMats.push_back(srcMat);
                }
                else
                {
                    cv::Mat disMat;
                    cv::Size newSize(toW, toH);
                    const bool shrink = newSize.width < srcMat.cols || newSize.height < srcMat.rows;
                    cv::resize(srcMat, disMat, newSize, 0.0, 0.0, shrink ? cv::INTER_AREA : cv::INTER_NEAREST);
                    disMats.push_back(disMat);
                }
            }
        }
    }
}

cv::Mat CairoCanvas::GetSubImage(const Geom::Path &roi) const
{
    Geom::OptRect vbox = Geom::bounds_fast(imgProc_.roi);
    Geom::OptRect ibox(0, 0, srcImg_.cols - 1, srcImg_.rows - 1);
    vbox.intersectWith(ibox);

    cv::Mat cropMat;
    if (vbox && vbox->width() > 3. && vbox->height() > 3.)
    {
        const int t = cvRound(vbox->top());
        const int b = cvRound(vbox->bottom());
        const int l = cvRound(vbox->left());
        const int r = cvRound(vbox->right());
        cropMat = cv::Mat(srcImg_, cv::Range(t, b), cv::Range(l, r));
    }
    else
    {
        cropMat = srcImg_;
    }

    return cropMat;
}

cv::Mat CairoCanvas::GetImageToProcess(const cv::Mat &srcMat, const int channel)
{
    cv::Mat procMat;
    if (srcMat.channels() > 1)
    {
        if (channel < srcMat.channels())
        {
            std::vector<cv::Mat> cMats;
            cv::split(srcMat, cMats);
            procMat = cMats[channel];
        }
        else
        {
            if (3 == srcMat.channels())
            {
                cv::cvtColor(srcMat, procMat, cv::COLOR_BGR2GRAY);
            }
            else
            {
                cv::cvtColor(srcMat, procMat, cv::COLOR_BGRA2GRAY);
            }
        }
    }
    else
    {
        procMat = srcMat;
    }

    return procMat;
}

void CairoCanvas::ImageProcessBinary()
{
    if (kIPK_BINARY == imgProc_.ipKind)
    {
        imgProc_.disMats.clear();
        auto itMinGray = imgProc_.iParams.find(cp_ToolProcThresholdMin);
        auto itMaxGray = imgProc_.iParams.find(cp_ToolProcThresholdMax);
        auto itChannel = imgProc_.iParams.find(cp_ToolProcThresholdChannel);
        if (itMinGray != imgProc_.iParams.end() &&
            itMaxGray != imgProc_.iParams.end() &&
            itChannel != imgProc_.iParams.end() && 
            itMinGray->second >= 0 && itMinGray->second <= 255 &&
            itMaxGray->second >= 0 && itMinGray->second <= 255 &&
            itMinGray->second < itMaxGray->second &&
            itChannel->second >=0)
        {
            std::vector<cv::Mat> resImgs;
            cv::Mat cropMat = GetSubImage(imgProc_.roi);
            cv::Mat procMat = GetImageToProcess(cropMat, itChannel->second);
            resImgs.push_back(BasicImgProc::Binarize(procMat, itMinGray->second, itMaxGray->second));

            if (!resImgs.empty())
            {
                ConvertToDisplayMats(resImgs, imgProc_.disMats);
            }
        }
    }
}

void CairoCanvas::ImageProcessFilter()
{
    if (kIPK_FILTER == imgProc_.ipKind)
    {
        imgProc_.disMats.clear();
        auto itFilterType = imgProc_.iParams.find(cp_ToolProcFilterType);
        if (itFilterType != imgProc_.iParams.end())
        {
            wxBusyCursor wait;
            cv::Mat resImg;
            cv::Mat procMat = GetSubImage(imgProc_.roi);
            const int borderType = imgProc_.iParams[cp_ToolProcFilterBorderType];
            switch (itFilterType->second)
            {
            case 0://Box
            {
                const cv::Size ksize(imgProc_.iParams[cp_ToolProcFilterBoxKernelWidth], imgProc_.iParams[cp_ToolProcFilterBoxKernelHeight]);
                cv::boxFilter(procMat, resImg, -1, ksize, cv::Point(-1, -1), true, borderType);
                break;
            }

            case 1://Gaussian
            {
                const double sigmaX = imgProc_.fParams[cp_ToolProcFilterGaussianSigmaX];
                const double sigmaY = imgProc_.fParams[cp_ToolProcFilterGaussianSigmaY];
                const cv::Size ksize(imgProc_.iParams[cp_ToolProcFilterGaussianKernelWidth], imgProc_.iParams[cp_ToolProcFilterGaussianKernelHeight]);
                cv::GaussianBlur(procMat, resImg, ksize, sigmaX, sigmaY, borderType);
                break;
            }

            case 2://Median
            {
                const int ksize = imgProc_.iParams[cp_ToolProcFilterMedianKernelWidth];
                cv::medianBlur(procMat, resImg, ksize);
                break;
            }

            case 3://Bilateral
            {
                const int d = imgProc_.iParams[cp_ToolProcFilterBilateralDiameter];
                const double sigmaColor = imgProc_.fParams[cp_ToolProcFilterBilateralSigmaColor];
                const double sigmaSpace = imgProc_.fParams[cp_ToolProcFilterBilateralSigmaSpace];
                cv::bilateralFilter(procMat, resImg, d, sigmaColor, sigmaSpace, borderType);
                break;
            }

            default:
                break;
            }

            if (!resImg.empty())
            {
                std::vector<cv::Mat> resImgs(1, resImg);
                ConvertToDisplayMats(resImgs, imgProc_.disMats);
            }
        }
    }
}

void CairoCanvas::ImageProcessPyramid()
{
    if (kIPK_PYRAMID == imgProc_.ipKind)
    {
        imgProc_.disMats.clear();
        auto itLevel = imgProc_.iParams.find(cp_ToolProcPyramidLevel);
        if (itLevel != imgProc_.iParams.end() && itLevel->second > 1)
        {
            std::vector<cv::Mat> pyraImgs;
            cv::Mat cropImg = GetSubImage(imgProc_.roi);
            cv::buildPyramid(cropImg, pyraImgs, itLevel->second);

            if (pyraImgs.size() > 1)
            {
                std::vector<cv::Mat> rPyraImgs(pyraImgs.size() - 1);
                std::reverse_copy(pyraImgs.cbegin() + 1, pyraImgs.cend(), rPyraImgs.begin());
                ConvertToDisplayMats(rPyraImgs, imgProc_.disMats);
            }
        }
    }
}

void CairoCanvas::ImageProcessEdge()
{
    if (kIPK_EDGE == imgProc_.ipKind)
    {
        imgProc_.disMats.clear();
        auto itEdgeType = imgProc_.iParams.find(cp_ToolProcEdgeType);
        auto itEdgeChal = imgProc_.iParams.find(cp_ToolProcEdgeChannel);
        if (itEdgeType != imgProc_.iParams.end() && itEdgeChal != imgProc_.iParams.end())
        {
            wxBusyCursor wait;

            cv::Mat resImg;
            cv::Mat cropMat = GetSubImage(imgProc_.roi);
            cv::Mat procMat = GetImageToProcess(cropMat, itEdgeChal->second);
            const int threshLow  = imgProc_.iParams[cp_ToolProcEdgeCannyThresholdLow];
            const int threshHigh = imgProc_.iParams[cp_ToolProcEdgeCannyThresholdHigh];
            const cv::Size ksize(imgProc_.iParams[cp_ToolProcEdgeApertureSize], imgProc_.iParams[cp_ToolProcEdgeApertureSize]);
            switch (itEdgeType->second)
            {
            case 0://Canny with Sobel
            {
                cv::Mat blurImage, edges;
                cv::blur(procMat, blurImage, ksize);
                cv::Canny(blurImage, edges, threshLow, threshHigh, ksize.width);
                blurImage = cv::Scalar::all(0);
                procMat.copyTo(blurImage, edges);
                resImg = blurImage;
                break;
            }

            case 1://Canny with Scharr
            {
                cv::Mat dx, dy, blurImage, edges;
                cv::blur(procMat, blurImage, ksize);
                cv::Scharr(blurImage, dx, CV_16S, 1, 0);
                cv::Scharr(blurImage, dy, CV_16S, 0, 1);
                cv::Canny(dx, dy, edges, threshLow, threshHigh);
                blurImage = cv::Scalar::all(0);
                procMat.copyTo(blurImage, edges);
                resImg = blurImage;
                break;
            }

            default:
                break;
            }

            if (!resImg.empty())
            {
                std::vector<cv::Mat> resImgs(1, resImg);
                ConvertToDisplayMats(resImgs, imgProc_.disMats);
            }
        }
    }
}

void CairoCanvas::ImageProcessConvert()
{
    if (kIPK_CONVERT == imgProc_.ipKind)
    {
        imgProc_.disMats.clear();
        auto itConvChal = imgProc_.iParams.find(cp_ToolProcConvertChannel);
        if (itConvChal != imgProc_.iParams.end())
        {
            wxBusyCursor wait;

            cv::Mat resImg;
            cv::Mat cropMat = GetSubImage(imgProc_.roi);
            switch (cropMat.channels())
            {
            case 3:
                if (0 <= itConvChal->second && itConvChal->second <= 2)
                {
                    std::vector<cv::Mat> cMats;
                    cv::split(cropMat, cMats);
                    resImg = cMats[itConvChal->second];
                }
                else if (4 <= itConvChal->second && itConvChal->second <= 6)
                {
                    cv::Mat hlsImg;
                    cv::cvtColor(cropMat, hlsImg, cv::COLOR_BGR2HLS_FULL);
                    std::vector<cv::Mat> cMats;
                    cv::split(hlsImg, cMats);
                    resImg = cMats[itConvChal->second-4];
                }
                else
                {
                    cv::cvtColor(cropMat, resImg, cv::COLOR_BGR2GRAY);
                }
                break;

            case 4:
                if (0 <= itConvChal->second && itConvChal->second <= 3)
                {
                    std::vector<cv::Mat> cMats;
                    cv::split(cropMat, cMats);
                    resImg = cMats[itConvChal->second];
                }
                else if (5 <= itConvChal->second && itConvChal->second <= 7)
                {
                    cv::Mat bgrImg, hlsImg;
                    cv::cvtColor(cropMat, bgrImg, cv::COLOR_BGRA2BGR);
                    cv::cvtColor(bgrImg, hlsImg, cv::COLOR_BGR2HLS_FULL);
                    std::vector<cv::Mat> cMats;
                    cv::split(hlsImg, cMats);
                    resImg = cMats[itConvChal->second - 5];
                }
                else
                {
                    cv::cvtColor(cropMat, resImg, cv::COLOR_BGR2GRAY);
                }
                break;

            default: break;
            }

            if (!resImg.empty())
            {
                std::vector<cv::Mat> resImgs(1, resImg);
                ConvertToDisplayMats(resImgs, imgProc_.disMats);
            }
        }
    }
}

void CairoCanvas::MoveAnchor(const wxSize &sViewport, const wxSize &disMatSize)
{
    anchorX_ = std::max(0, (sViewport.GetWidth() - disMatSize.GetWidth()) / 2);
    anchorY_ = std::max(0, (sViewport.GetHeight() - disMatSize.GetHeight()) / 2);

    anchorX_ = 0;
    anchorY_ = 0;
}

void CairoCanvas::ScaleShowImage(const wxSize &sToSize)
{
    if (sToSize.GetWidth()>2 && sToSize.GetHeight()>2)
    {
        if (sToSize.GetWidth() == srcMat_.cols && sToSize.GetHeight() == srcMat_.rows)
        {
            disMat_ = srcMat_;
        }
        else
        {
            cv::Size newSize(sToSize.GetWidth(), sToSize.GetHeight());
            bool shrink = newSize.width<srcMat_.cols || newSize.height<srcMat_.rows;
            ScopedTimer st(wxT("cv::resize"));
            cv::resize(srcMat_, disMat_, newSize, 0.0, 0.0, shrink ? cv::INTER_AREA : cv::INTER_NEAREST);
        }
        switch (imgProc_.ipKind)
        {
        case kIPK_BINARY: ImageProcessBinary(); break;
        case kIPK_PYRAMID: ImageProcessPyramid(); break;
        case kIPK_FILTER: ImageProcessFilter(); break;
        case kIPK_EDGE: ImageProcessEdge(); break;
        case kIPK_CONVERT: ImageProcessConvert(); break;
        default: break;
        }
        Refresh(false);
    }
}

wxSize CairoCanvas::GetDispMatSize(const wxSize &sViewport, const wxSize &srcMatSize)
{
    if (srcMatSize.GetWidth() <= sViewport.GetWidth() &&
        srcMatSize.GetHeight() <= sViewport.GetHeight())
    {
        return srcMatSize;
    }
    else
    {
        return GetFitSize(sViewport, srcMatSize);
    }
}

wxSize CairoCanvas::GetFitSize(const wxSize &sViewport, const wxSize &srcMatSize)
{
    double fw = (srcMatSize.GetWidth()+0.0) / sViewport.GetWidth();
    double fh = (srcMatSize.GetHeight() + 0.0) / sViewport.GetHeight();

    if (fw > fh)
    {
        return wxSize(sViewport.GetWidth(), srcMatSize.GetHeight()/fw);
    }
    else
    {
        return wxSize(srcMatSize.GetWidth()/fh, sViewport.GetHeight());
    }
}

bool CairoCanvas::IsRectNeedRefresh(const Geom::Rect &bbox, const std::vector<Geom::Rect> &invalidRects)
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

wxDragResult DnDImageFile::OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult)
{
    if (GetData())
    {
        auto fdo = dynamic_cast<wxFileDataObject*>(GetDataObject());
        if (fdo)
        {
            auto cFiles = fdo->GetFilenames().GetCount();
            if (1 == cFiles)
            {
                wxString fileName = fdo->GetFilenames()[0];
                if (wxImage::CanRead(fileName))
                {
                    return wxFileDropTarget::OnDragOver(x, y, defResult);
                }
            }
        }
    }

    return wxDragNone;
}

wxDragResult DnDImageFile::OnEnter(wxCoord x, wxCoord y, wxDragResult defResult)
{
    return wxFileDropTarget::OnEnter(x, y, defResult);
}

bool DnDImageFile::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    auto cFiles = filenames.GetCount();
    if (1==cFiles)
    {
        wxString fileName = filenames[0];
        if (wxImage::CanRead(fileName))
        {
            DropImageEvent e(spamEVT_DROP_IMAGE, ownerPanel_->GetId(), ownerPanel_, fileName);
            wxTheApp->AddPendingEvent(e);
            return true;
        }
    }

    return false;
}