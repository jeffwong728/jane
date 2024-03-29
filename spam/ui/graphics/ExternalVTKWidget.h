/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExternalVTKWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ExternalVTKWidget - Use VTK rendering in an external window/application
// .SECTION Description
// ExternalVTKWidget provides an easy way to render VTK objects in an external
// environment using the VTK rendering framework without drawing a new window.

#ifndef __ExternalVTKWidget_h
#define __ExternalVTKWidget_h

#include "vtkExternalOpenGLRenderWindow.h"
#include "vtkExternalOpenGLRenderer.h"
#include "vtkObject.h"

// Class that maintains an external render window.
class ExternalVTKWidget : public vtkObject
{
public:
  static ExternalVTKWidget* New();
  vtkTypeMacro(ExternalVTKWidget, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Set/Get an external render window for the ExternalVTKWidget.
  // Since this is a special environment, the methods are limited to use
  // vtkExternalOpenGLRenderWindow only.
  // \sa vtkExternalOpenGLRenderWindow
  vtkExternalOpenGLRenderWindow* GetRenderWindow(void);
  void SetRenderWindow(vtkExternalOpenGLRenderWindow* renWin);

  // Creates a new renderer and adds it to the render window.
  // Returns a handle to the created renderer.
  // NOTE: To get a list of renderers, one must go through the RenderWindow API
  // i.e. ExternalVTKWidget->GetRenderWindow()->GetRenderers()
  // \sa vtkRenderWindow::GetRenderers()
  vtkExternalOpenGLRenderer* AddRenderer();

protected:
  ExternalVTKWidget();
  ~ExternalVTKWidget() override;

  vtkExternalOpenGLRenderWindow* RenderWindow;

private:
  ExternalVTKWidget(const ExternalVTKWidget&) = delete;
  void operator=(const ExternalVTKWidget&) = delete;
};

#endif //__ExternalVTKWidget_h
