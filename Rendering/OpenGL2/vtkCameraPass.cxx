/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCameraPass.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkRenderState.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkMatrix4x4.h"
#include "vtkCamera.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLError.h"

vtkStandardNewMacro(vtkCameraPass);
vtkCxxSetObjectMacro(vtkCameraPass,DelegatePass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkCameraPass::vtkCameraPass()
{
  this->DelegatePass=0;
  this->AspectRatioOverride = 1.0;
}

// ----------------------------------------------------------------------------
vtkCameraPass::~vtkCameraPass()
{
  if(this->DelegatePass!=0)
  {
      this->DelegatePass->Delete();
  }
}

// ----------------------------------------------------------------------------
void vtkCameraPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AspectRatioOverride: " << this->AspectRatioOverride
    << endl;
  os << indent << "DelegatePass:";
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
}

void vtkCameraPass::GetTiledSizeAndOrigin(
  const vtkRenderState* render_state,
  int* width, int* height, int *originX,
  int* originY)
{
  vtkRenderer *ren = render_state->GetRenderer();
  ren->GetTiledSizeAndOrigin(width, height, originX, originY);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkCameraPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkRenderer *ren=s->GetRenderer();

  if (!ren->IsActiveCameraCreated())
  {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet.
    // If is very unlikely that this can occur - if this
    // renderer is part of a vtkRenderWindow, the camera
    // will already have been created as part of the
    // DoStereoRender() method.

    // this is ren->GetActiveCameraAndResetIfCreated();
    ren->GetActiveCamera();
    ren->ResetCamera();
  }

  vtkCamera *camera=ren->GetActiveCamera();

  int lowerLeft[2];
  int usize;
  int vsize;
  vtkOpenGLFramebufferObject *fbo=vtkOpenGLFramebufferObject::SafeDownCast(s->GetFrameBuffer());

  vtkOpenGLRenderWindow *win=vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  win->MakeCurrent();

  if(fbo==0)
  {
    unsigned int dfbo = win->GetDefaultFrameBufferId();
    if (dfbo)
    {
      // If the render window is using an FBO to render into, we ensure that
      // it's selected.
      glBindFramebuffer(GL_FRAMEBUFFER, dfbo);
    }

    // find out if we should stereo render
    bool stereo = win->GetStereoRender()==1;
    this->GetTiledSizeAndOrigin(s, &usize,&vsize,lowerLeft,lowerLeft+1);

    // if were on a stereo renderer draw to special parts of screen
    if(stereo)
    {
      switch (win->GetStereoType())
      {
        case VTK_STEREO_CRYSTAL_EYES:
          if (camera->GetLeftEye())
          {
            if(win->GetDoubleBuffer())
            {
              glDrawBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
              glReadBuffer(static_cast<GLenum>(win->GetBackLeftBuffer()));
            }
            else
            {
              glDrawBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
              glReadBuffer(static_cast<GLenum>(win->GetFrontLeftBuffer()));
            }
          }
          else
          {
            if(win->GetDoubleBuffer())
            {
              glDrawBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
              glReadBuffer(static_cast<GLenum>(win->GetBackRightBuffer()));
            }
            else
            {
              glDrawBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
              glReadBuffer(static_cast<GLenum>(win->GetFrontRightBuffer()));
            }
          }
          break;
        case VTK_STEREO_LEFT:
          camera->SetLeftEye(1);
          break;
        case VTK_STEREO_RIGHT:
          camera->SetLeftEye(0);
          break;
        default:
          break;
      }
    }
    else
    {
      if (win->GetDoubleBuffer())
      {
        glDrawBuffer(static_cast<GLenum>(win->GetBackBuffer()));

        // Reading back buffer means back left. see OpenGL spec.
        // because one can write to two buffers at a time but can only read from
        // one buffer at a time.
        glReadBuffer(static_cast<GLenum>(win->GetBackBuffer()));
      }
      else
      {
        glDrawBuffer(static_cast<GLenum>(win->GetFrontBuffer()));

        // Reading front buffer means front left. see OpenGL spec.
      // because one can write to two buffers at a time but can only read from
      // one buffer at a time.
        glReadBuffer(static_cast<GLenum>(win->GetFrontBuffer()));
      }
    }
  }
  else
  {
    // FBO size. This is the renderer size as a renderstate is per renderer.
    int size[2];
    fbo->GetLastSize(size);
    usize=size[0];
    vsize=size[1];
    lowerLeft[0]=0;
    lowerLeft[1]=0;
    // we assume the drawbuffer state is already initialized before.
  }

  // Save the current viewport and camera matrices.
  GLint saved_viewport[4];
  glGetIntegerv(GL_VIEWPORT, saved_viewport);
  GLboolean saved_scissor_test;
  glGetBooleanv(GL_SCISSOR_TEST, &saved_scissor_test);
  GLint saved_scissor_box[4];
  glGetIntegerv(GL_SCISSOR_BOX, saved_scissor_box);

  glViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0], lowerLeft[1], usize, vsize);

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase()
      && !ren->GetIsPicking())
  {
    ren->Clear();
  }

  // Done with camera initialization. The delegate can be called.
  vtkOpenGLCheckErrorMacro("failed after camera initialization");

  if(this->DelegatePass!=0)
  {
    this->DelegatePass->Render(s);
    this->NumberOfRenderedProps+=
      this->DelegatePass->GetNumberOfRenderedProps();
  }
  else
  {
    vtkWarningMacro(<<" no delegate.");
  }
  vtkOpenGLCheckErrorMacro("failed after delegate pass");

  // Restore changed context.
  glViewport(saved_viewport[0], saved_viewport[1], saved_viewport[2],
    saved_viewport[3]);
  glScissor(saved_scissor_box[0], saved_scissor_box[1], saved_scissor_box[2],
    saved_scissor_box[3]);
  if (saved_scissor_test)
  {
    glEnable(GL_SCISSOR_TEST);
  }
  else
  {
    glDisable(GL_SCISSOR_TEST);
  }

  vtkOpenGLCheckErrorMacro("failed after restore context");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkCameraPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
