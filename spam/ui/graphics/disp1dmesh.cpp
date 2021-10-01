#include "disp1dmesh.h"
#include <numeric>
#include <algorithm>
#include <epoxy/gl.h>
#include <vtkProperty.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkUnstructuredGridGeometryFilter.h>

SPDispNodes GL1DMeshNode::MakeNew(const vtkSmartPointer<vtkPolyData> &pdSource, const vtkSmartPointer<vtkOpenGLRenderer> &renderer)
{
    SPDispNodes dispNodes;
    if (pdSource)
    {
        const vtkIdType numLines = pdSource->GetNumberOfLines();
        const vtkIdType numCells = pdSource->GetNumberOfCells();
        if (numCells == numLines)
        {
            auto dispNode = std::make_shared<GL1DMeshNode>(this_is_private{ 0 });
            dispNode->renderer_ = renderer;
            dispNode->poly_data_ = pdSource;
            dispNode->SetDefaultDisplay();
            dispNodes.push_back(std::move(dispNode));
        }
    }

    return dispNodes;
}

SPDispNodes GL1DMeshNode::MakeNew(const vtkSmartPointer<vtkUnstructuredGrid> &ugSource, const vtkSmartPointer<vtkOpenGLRenderer> &renderer)
{
    if (ugSource)
    {
        vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        surfaceFilter->SetNonlinearSubdivisionLevel(0);
        surfaceFilter->SetInputData(ugSource);
        surfaceFilter->Update();

        return GL1DMeshNode::MakeNew(surfaceFilter->GetOutput(), renderer);
    }

    return SPDispNodes();
}

GL1DMeshNode::GL1DMeshNode(const this_is_private&)
    : GLDispNode(GLDispNode::this_is_private{ 0 }, kENTITY_TYPE_1D_MESH)
{
}

GL1DMeshNode::~GL1DMeshNode()
{
    renderer_->RemoveActor(actor_);
}

const vtkColor4ub GL1DMeshNode::GetColor() const
{
    double colors[3] = { 0 };
    if (actor_)
    {
        actor_->GetProperty()->GetDiffuseColor(colors);
    }

    unsigned char r = static_cast<unsigned char>(colors[0] * 255.0);
    unsigned char g = static_cast<unsigned char>(colors[1] * 255.0);
    unsigned char b = static_cast<unsigned char>(colors[2] * 255.0);

    return vtkColor4ub(r, g, b);
}

const std::string GL1DMeshNode::GetName() const
{
    return std::string("Node");
}

void GL1DMeshNode::SetVisible(const int visible)
{
    if (actor_)
    {
        actor_->SetVisibility(visible);
    }
}

void GL1DMeshNode::ShowNode(const int visible)
{
    if (actor_)
    {
        actor_->GetProperty()->SetVertexVisibility(visible);
        actor_->GetProperty()->SetEdgeVisibility(visible);
    }
}

void GL1DMeshNode::SetRepresentation(const int vtkNotUsed(rep))
{
}

void GL1DMeshNode::SetCellColor(const double *c)
{
    if (actor_)
    {
        actor_->GetProperty()->SetDiffuseColor(c);
    }
}

void GL1DMeshNode::SetEdgeColor(const double *vtkNotUsed(c))
{
}

void GL1DMeshNode::SetNodeColor(const double *c)
{
    if (actor_)
    {
        actor_->GetProperty()->SetVertexColor(c);
    }
}

void GL1DMeshNode::SetDefaultDisplay()
{
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(poly_data_);

    actor_ = vtkSmartPointer<vtkActor>::New();
    actor_->SetMapper(mapper);

    actor_->GetProperty()->SetDiffuse(1.0);
    actor_->GetProperty()->SetSpecular(0.3);
    actor_->GetProperty()->SetSpecularPower(60.0);
    actor_->GetProperty()->SetOpacity(1.0);
    actor_->GetProperty()->SetLineWidth(1.f);
    actor_->GetProperty()->SetPointSize(5);

    renderer_->AddActor(actor_);
}