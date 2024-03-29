#include "disp2dmesh.h"
#include <ui/spam.h>
#include <numeric>
#include <algorithm>
#include <epoxy/gl.h>
#include <vtkFloatArray.h>
#include <vtkSignedCharArray.h>
#include <vtkIdTypeArray.h>
#include <vtkCellType.h>
#include <vtkPlanes.h>
#include <vtkAreaPicker.h>
#include <vtkProperty.h>
#include <vtkFieldData.h>
#include <vtkLookupTable.h>
#include <vtkExtractEdges.h>
#include <vtkCellData.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkUnstructuredGridGeometryFilter.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkSelectionNode.h>
#include <vtkPolyDataNormals.h>
#include <vtkTypeUInt64Array.h>
#include <vtkUnsignedCharArray.h>
#include <vtkCellLocator.h>
#include <vtkSelection.h>
#include <vtkAppendFilter.h>

SPDispNodes GL2DMeshNode::MakeNew(const vtkSmartPointer<vtkPolyData> &pdSource, const vtkSmartPointer<vtkOpenGLRenderer> &renderer)
{
    SPDispNodes dispNodes;
    if (pdSource)
    {
        const vtkIdType numPolys = pdSource->GetNumberOfPolys();
        const vtkIdType numStrips = pdSource->GetNumberOfStrips();
        const vtkIdType numCells = pdSource->GetNumberOfCells();
        if (numCells == numPolys || numCells == numStrips)
        {
            vtkNew<vtkAppendFilter> appendFilter;
            appendFilter->AddInputData(pdSource);
            appendFilter->Update();

            vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
            unstructuredGrid->ShallowCopy(appendFilter->GetOutput());
            return MakeNew(unstructuredGrid, renderer);
        }
    }

    return dispNodes;
}

SPDispNodes GL2DMeshNode::MakeNew(const vtkSmartPointer<vtkUnstructuredGrid> &ugSource, const vtkSmartPointer<vtkOpenGLRenderer> &renderer)
{
    if (ugSource)
    {
        vtkNew<vtkCellTypes> cellTypes;
        ugSource->GetCellTypes(cellTypes);
        for (vtkIdType cellId = 0; cellId < cellTypes->GetNumberOfTypes(); ++cellId)
        {
            const auto cellType = cellTypes->GetCellType(cellId);
            if ( VTK_TRIANGLE != cellType &&
                VTK_QUAD != cellType &&
                VTK_QUADRATIC_TRIANGLE != cellType &&
                VTK_QUADRATIC_QUAD != cellType)
            {
                return SPDispNodes();
            }
        }

        vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        surfaceFilter->SetPassThroughCellIds(true);
        surfaceFilter->SetPassThroughPointIds(true);
        surfaceFilter->SetNonlinearSubdivisionLevel(1);
        surfaceFilter->SetInputData(ugSource);

        vtkNew<vtkPolyDataNormals> normalsFilter;
        normalsFilter->SetInputConnection(surfaceFilter->GetOutputPort());
        normalsFilter->Update();

        SPDispNodes dispNodes;
        auto dispNode = std::make_shared<GL2DMeshNode>(this_is_private{ 0 });
        dispNode->renderer_ = renderer;
        dispNode->poly_data_ = normalsFilter->GetOutput();
        dispNode->unstructured_grid_ = ugSource;
        dispNode->SetDefaultDisplay();

        vtkNew<vtkExtractEdges> extractEdges;
        extractEdges->SetInputData(ugSource);
        extractEdges->Update();

        dispNode->elem_edge_poly_data_ = extractEdges->GetOutput();

        vtkNew<vtkPolyDataMapper> edgeMapper;
        edgeMapper->SetInputData(dispNode->elem_edge_poly_data_);

        dispNode->elem_edge_actor_ = vtkSmartPointer<vtkActor>::New();
        dispNode->elem_edge_actor_->SetMapper(edgeMapper);
        dispNode->elem_edge_actor_->GetProperty()->SetDiffuse(1.0);
        dispNode->elem_edge_actor_->GetProperty()->SetSpecular(0.3);
        dispNode->elem_edge_actor_->GetProperty()->SetSpecularPower(60.0);
        dispNode->elem_edge_actor_->GetProperty()->SetOpacity(1.0);
        dispNode->elem_edge_actor_->GetProperty()->SetLineWidth(1.5f);
        dispNode->elem_edge_actor_->GetProperty()->SetPointSize(5);
        dispNode->elem_edge_actor_->VisibilityOff();
        dispNode->renderer_->AddActor(dispNode->elem_edge_actor_);
        dispNodes.push_back(std::move(dispNode));

        return dispNodes;
    }

    return SPDispNodes();
}

GL2DMeshNode::GL2DMeshNode(const this_is_private&)
    : GLDispNode(GLDispNode::this_is_private{ 0 }, kENTITY_TYPE_2D_MESH)
{
}

GL2DMeshNode::~GL2DMeshNode()
{
    if (elem_edge_actor_)
    {
        renderer_->RemoveActor(elem_edge_actor_);
    }
}

const vtkColor4ub GL2DMeshNode::GetColor() const
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

const std::string GL2DMeshNode::GetName() const
{
    return std::string("Node");
}

void GL2DMeshNode::SetVisible(const int visible)
{
    if (actor_)
    {
        actor_->SetVisibility(visible);
    }

    if (elem_edge_actor_ && (kGREP_VTK_WIREFRAME == representation_ || kGREP_SURFACE_WITH_EDGE == representation_))
    {
        elem_edge_actor_->SetVisibility(visible);
    }
}

void GL2DMeshNode::ShowNode(const int visible)
{
    if (elem_edge_actor_ && elem_edge_actor_->GetVisibility())
    {
        elem_edge_actor_->GetProperty()->SetEdgeVisibility(visible);
        elem_edge_actor_->GetProperty()->SetVertexVisibility(visible);
    }
}

void GL2DMeshNode::SetRepresentation(const int rep)
{
    representation_ = rep;
    if (kGREP_VTK_WIREFRAME == representation_)
    {
        if (!elem_edge_poly_data_)
        {
            CreateElementEdgeActor();
        }

        if (actor_)
        {
            elem_edge_actor_->GetProperty()->SetVertexColor(actor_->GetProperty()->GetVertexColor());
            elem_edge_actor_->GetProperty()->SetDiffuseColor(actor_->GetProperty()->GetDiffuseColor());
        }

        if (elem_edge_actor_ && !elem_edge_actor_->GetVisibility())
        {
            elem_edge_actor_->VisibilityOn();
        }

        if (actor_ && actor_->GetVisibility())
        {
            actor_->VisibilityOff();
        }
    }
    else if (kGREP_SURFACE_WITH_EDGE == representation_)
    {
        if (!elem_edge_poly_data_)
        {
            CreateElementEdgeActor();
        }

        if (actor_)
        {
            elem_edge_actor_->GetProperty()->SetVertexColor(actor_->GetProperty()->GetVertexColor());
            elem_edge_actor_->GetProperty()->SetDiffuseColor(actor_->GetProperty()->GetEdgeColor());
        }

        if (elem_edge_actor_ && !elem_edge_actor_->GetVisibility())
        {
            elem_edge_actor_->VisibilityOn();
        }

        if (actor_ && !actor_->GetVisibility())
        {
            actor_->VisibilityOn();
        }
    }
    else
    {
        if (elem_edge_actor_ && elem_edge_actor_->GetVisibility())
        {
            elem_edge_actor_->VisibilityOff();
        }

        if (actor_ && !actor_->GetVisibility())
        {
            actor_->VisibilityOn();
        }
    }
}

void GL2DMeshNode::SetCellColor(const double *c)
{
    if (actor_)
    {
        actor_->GetProperty()->SetDiffuseColor(c);
        vtkLookupTable *lut = vtkLookupTable::SafeDownCast(mapper_->GetLookupTable());
        if (lut)
        {
            lut->SetTableValue(0, c);
        }
    }

    if (kGREP_VTK_WIREFRAME == representation_ && elem_edge_actor_)
    {
        elem_edge_actor_->GetProperty()->SetDiffuseColor(c);
    }
}

void GL2DMeshNode::SetEdgeColor(const double *c)
{
    if (actor_)
    {
        actor_->GetProperty()->SetEdgeColor(c);
    }

    if (elem_edge_actor_)
    {
        elem_edge_actor_->GetProperty()->SetEdgeColor(c);
    }
}

void GL2DMeshNode::SetNodeColor(const double *c)
{
    if (actor_)
    {
        actor_->GetProperty()->SetVertexColor(c);
    }

    if (elem_edge_actor_)
    {
        elem_edge_actor_->GetProperty()->SetVertexColor(c);
    }
}

vtkIdType GL2DMeshNode::Select2DCells(vtkPlanes *frustum)
{
    return SelectFacets(frustum);
}

vtkIdType GL2DMeshNode::HideSelectedCells()
{
    vtkIdType numSelStatusChanged = 0;
    const std::string arrName("vtkOriginalCellIds");
    vtkUnsignedCharArray* ghosts = unstructured_grid_->GetCellGhostArray();
    vtkIntArray *colorIndexs = vtkIntArray::SafeDownCast(poly_data_->GetCellData()->GetScalars());
    vtkIdTypeArray *cellIds = vtkIdTypeArray::SafeDownCast(poly_data_->GetCellData()->GetArray(arrName.c_str()));
    if (ghosts && colorIndexs && cellIds && cellIds->GetNumberOfValues() == colorIndexs->GetNumberOfValues())
    {
        const vtkIdType numFacets = colorIndexs->GetNumberOfValues();
        for (vtkIdType ii = 0; ii < numFacets; ++ii)
        {
            if (kCell_Color_Index_Selected == colorIndexs->GetValue(ii) || kCell_Color_Index_Selected_And_Highlight == colorIndexs->GetValue(ii))
            {
                const vtkIdType cellId = cellIds->GetValue(ii);
                ghosts->SetValue(cellId, ghosts->GetValue(cellId) | vtkDataSetAttributes::HIDDENCELL);
                numSelStatusChanged += 1;
            }
        }

        if (numSelStatusChanged > 0)
        {
            vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
            surfaceFilter->SetPassThroughCellIds(true);
            surfaceFilter->SetPassThroughPointIds(true);
            surfaceFilter->SetNonlinearSubdivisionLevel(1);
            surfaceFilter->SetInputData(unstructured_grid_);

            vtkNew<vtkPolyDataNormals> normalsFilter;
            normalsFilter->SetInputConnection(surfaceFilter->GetOutputPort());
            normalsFilter->Update();

            poly_data_ = normalsFilter->GetOutput();
            SetSelectionDefaultDisplay();

            vtkNew<vtkExtractEdges> extractEdges;
            extractEdges->SetInputData(unstructured_grid_);
            extractEdges->Update();

            elem_edge_poly_data_ = extractEdges->GetOutput();
            elem_edge_actor_->GetMapper()->SetInputDataObject(elem_edge_poly_data_);
            renderer_->AddActor(elem_edge_actor_);
        }
    }

    return numSelStatusChanged;
}

void GL2DMeshNode::SetDefaultDisplay()
{
    const std::string arrName("vtkOriginalCellIds");
    SortFacets(poly_data_, arrName);

    mapper_ = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper_->SetInputData(poly_data_);

    actor_ = vtkSmartPointer<vtkActor>::New();
    actor_->SetMapper(mapper_);

    actor_->GetProperty()->SetDiffuse(1.0);
    actor_->GetProperty()->SetSpecular(0.3);
    actor_->GetProperty()->SetSpecularPower(60.0);
    actor_->GetProperty()->SetOpacity(1.0);
    actor_->GetProperty()->SetLineWidth(1.f);
    actor_->GetProperty()->SetPointSize(5);

    vtkSmartPointer<vtkNamedColors> colors = vtkSmartPointer<vtkNamedColors>::New();
    vtkSmartPointer<vtkIntArray> colorIndexs = vtkSmartPointer<vtkIntArray>::New();
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(4);
    lut->Build();
    lut->SetTableValue(0, colors->GetColor4d("Wheat").GetData());
    lut->SetTableValue(1, colors->GetColor4d("DarkOrange").GetData());
    lut->SetTableValue(2, colors->GetColor4d("Gold").GetData());
    lut->SetTableValue(3, colors->GetColor4d("Orange").GetData());
    mapper_->SetScalarRange(0, 3);
    mapper_->SetLookupTable(lut);

    colorIndexs->SetNumberOfTuples(poly_data_->GetNumberOfCells());
    colorIndexs->FillValue(0);

    poly_data_->GetCellData()->SetScalars(colorIndexs);
    renderer_->AddActor(actor_);

    vtkNew<vtkTypeUInt64Array> guids;
    guids->SetName("GUID_TAG");
    guids->SetNumberOfComponents(2);
    guids->SetNumberOfTuples(1);
    guids->SetValue(0, guid_.part1);
    guids->SetValue(1, guid_.part2);
    poly_data_->GetFieldData()->AddArray(guids);

    cell_loc_ = vtkSmartPointer<vtkCellLocator>::New();
    cell_loc_->SetDataSet(poly_data_);
    cell_loc_->Update();

    vtkUnsignedCharArray* ghosts = unstructured_grid_->GetCellGhostArray();
    if (!ghosts)
    {
        unstructured_grid_->AllocateCellGhostArray();
        ghosts = unstructured_grid_->GetCellGhostArray();
    }

    if (ghosts)
    {
        constexpr vtkTypeUInt8 visMask = ~vtkDataSetAttributes::HIDDENCELL;
        const vtkIdType numVals = ghosts->GetNumberOfValues();
        for (vtkIdType ii = 0; ii < numVals; ++ii)
        {
            ghosts->SetValue(ii, ghosts->GetValue(ii) & visMask);
        }
    }
}

void GL2DMeshNode::SetSelectionDefaultDisplay()
{
    const std::string arrName("vtkOriginalCellIds");
    SortFacets(poly_data_, arrName);

    actor_->GetMapper()->SetInputDataObject(poly_data_);

    vtkSmartPointer<vtkIntArray> colorIndexs = vtkSmartPointer<vtkIntArray>::New();
    colorIndexs->SetNumberOfTuples(poly_data_->GetNumberOfCells());
    colorIndexs->FillValue(0);

    poly_data_->GetCellData()->SetScalars(colorIndexs);
    renderer_->AddActor(actor_);

    vtkNew<vtkTypeUInt64Array> guids;
    guids->SetName("GUID_TAG");
    guids->SetNumberOfComponents(2);
    guids->SetNumberOfTuples(1);
    guids->SetValue(0, guid_.part1);
    guids->SetValue(1, guid_.part2);
    poly_data_->GetFieldData()->AddArray(guids);

    cell_loc_ = vtkSmartPointer<vtkCellLocator>::New();
    cell_loc_->SetDataSet(poly_data_);
    cell_loc_->Update();
}

void GL2DMeshNode::CreateElementEdgeActor()
{
    vtkNew<vtkExtractEdges> extractEdges;
    extractEdges->SetInputData(poly_data_);
    extractEdges->Update();

    elem_edge_poly_data_ = extractEdges->GetOutput();

    vtkNew<vtkPolyDataMapper> edgeMapper;
    edgeMapper->SetInputData(elem_edge_poly_data_);

    elem_edge_actor_ = vtkSmartPointer<vtkActor>::New();
    elem_edge_actor_->SetMapper(edgeMapper);

    elem_edge_actor_->GetProperty()->SetDiffuse(1.0);
    elem_edge_actor_->GetProperty()->SetSpecular(0.3);
    elem_edge_actor_->GetProperty()->SetSpecularPower(60.0);
    elem_edge_actor_->GetProperty()->SetOpacity(1.0);
    elem_edge_actor_->GetProperty()->SetLineWidth(1.5f);
    elem_edge_actor_->GetProperty()->SetPointSize(5);
    elem_edge_actor_->VisibilityOff();

    renderer_->AddActor(elem_edge_actor_);
}
