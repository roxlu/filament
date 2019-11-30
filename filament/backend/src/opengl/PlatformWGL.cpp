/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PlatformWGL.h"

#include <Wingdi.h>

#include "OpenGLDriverFactory.h"

#ifdef _MSC_VER
// this variable is checked in BlueGL.h (included from "gl_headers.h" right after this), 
// and prevents duplicate definition of OpenGL apis when building this file. 
// However, GL_GLEXT_PROTOTYPES need to be defined in BlueGL.h when included from other files.
#define FILAMENT_PLATFORM_WGL
#endif

#include "gl_headers.h"

#include "Windows.h"
#include <GL/gl.h>
#include "GL/glext.h"
#include "GL/wglext.h"

#include <utils/Log.h>
#include <utils/Panic.h>

/* @todo (roxlu): included these for testing */
#include <thread>
#include <sstream>
#include <mutex>
#include <thread>
/* @todo end. */
namespace {

  void reportLastWindowsError() {
    LPSTR lpMessageBuffer = nullptr;
    DWORD dwError = GetLastError();

    if (dwError == 0) {
      return;
    }

    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      dwError,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      lpMessageBuffer,
      0, nullptr
    );

    utils::slog.e << "Windows error code: " << dwError << ". " << lpMessageBuffer
                  << utils::io::endl;

    LocalFree(lpMessageBuffer);
  }

} // namespace

namespace filament {

  using namespace backend;

  Driver* PlatformWGL::createDriver(void* const sharedGLContext) noexcept {

#define USE_ROXLU 1
    
#if USE_ROXLU == 1

    /* --------------------------------------------------------------------------- */
    /*
      The code below tries to retrieve the pixel format from the
      window that is currently used. This is just an experiment
      which allows us to retrieve a pixel format that is
      identical; when the pixel format is identical as the one
      that is used by the shared context, we're sure that the
      same GL implementation is used and the contexts are
      compatible.

      Note: the function `createDriver()` is called from another
      thread. Therefore you can't simply make the given
      `sharedGLContext()` the current context.
     */
    HWND curr_hwnd = nullptr;
    curr_hwnd = GetActiveWindow(); /* returns nullptr. */
    curr_hwnd = GetForegroundWindow();  /* returns the correct window that has been created by GLFW */
 
    if (nullptr == curr_hwnd) {
      printf("Failed to get the current Window handle. (exiting). \n");
      exit(EXIT_FAILURE);
    }

    HDC curr_hdc = GetDC(curr_hwnd);
    if (nullptr == curr_hdc) {
      printf("Failed to get the current Device Context (exiting).\n");
      exit(EXIT_FAILURE);
    }

    int curr_pix = GetPixelFormat(curr_hdc);
    if (0 == curr_pix) {
      printf("Failed to get the current pixel format. (exiting).\n");
      exit(EXIT_FAILURE);
    }


    /* 
       This function does not run it the main thread, which is
       why we can't "reuse" the given shared context because we
       can't make it current here which would allows us to
       retrieve `wglCreateContextAttribsARB()`. Instead we have
       to create a new dummy context and retrieve
       `wglCreateContextAttribsARB()` again.
    */
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string thread_id = oss.str();

    HWND dummy_hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    if (nullptr == dummy_hwnd) {
      printf("Failed to create a dummy window. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    HDC dummy_hdc = GetDC(dummy_hwnd);
    if (dummy_hdc == NULL) {
      printf("Failed to get the HDC from the dummy hwnd. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    /* Now that we have a dummy we have to set the pixelformat. */
    PIXELFORMATDESCRIPTOR dummy_pix_fmt = {};
    int dummy_desc_fmt = DescribePixelFormat(dummy_hdc, curr_pix, sizeof(dummy_pix_fmt), &dummy_pix_fmt);
    if (0 == dummy_desc_fmt) {
      printf("Failed to describe the pixel format. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    if (FALSE == SetPixelFormat(dummy_hdc, curr_pix, &dummy_pix_fmt)) {
      printf("Failed to set the pixel format on the dummy hdc. (exiting).\n");
      exit(EXIT_FAILURE);
    }
    
    HGLRC dummy_ctx = wglCreateContext(dummy_hdc);
    if (nullptr == dummy_ctx) {
      printf("Failed to create the dummy context. (exiting). \n");
      exit(EXIT_FAILURE);
    }

    if (FALSE == wglMakeCurrent(dummy_hdc, dummy_ctx)) {
      printf("Failed to make our dummy context the current one. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    /* Now, that we created a dummy format, we can (finally) get the `wglCreateContextAttribsARB()` function. */
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsRoxlu = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
    if (nullptr == wglCreateContextAttribsRoxlu) {
      printf("Failed to get wglCreateContextAttribs(). (exiting).\n");
      exit(EXIT_FAILURE);
    }
    
    int attribs_roxlu[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_PROFILE_MASK_ARB,
      0
    };

    mContext = wglCreateContextAttribsRoxlu(curr_hdc, (HGLRC)sharedGLContext, attribs_roxlu);
    if (nullptr == mContext) {
      printf("Failed to create GL context. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    wglMakeCurrent(curr_hdc, mContext);

    printf("--- curr_wnd: %p\n", curr_hwnd);
    printf("--- curr_pix: %d\n", curr_pix);
    printf("--- thread id in filament: %s\n", thread_id.c_str());
    printf("--- dummy_desc_fmt: %d\n", dummy_desc_fmt);
    printf("--- created GL context: %p\n", mContext);
    printf("--- GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    printf("--- GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));

    int result = bluegl::bind();
    ASSERT_POSTCONDITION(!result, "Unable to load OpenGL entry points.");
    return OpenGLDriverFactory::create(this, sharedGLContext);

#endif
    
    /* --------------------------------------------------------------------------- */
    
#if USE_ROXLU == 0

    mPfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
      PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
      32,                   // Colordepth of the framebuffer.
      0, 0, 0, 0, 0, 0,
      0,
      0,
      0,
      0, 0, 0, 0,
      24,                   // Number of bits for the depthbuffer
      0,                    // Number of bits for the stencilbuffer
      0,                    // Number of Aux buffers in the framebuffer.
      PFD_MAIN_PLANE,
      0,
      0, 0, 0
    };

    int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_PROFILE_MASK_ARB,
      0
    };

    HGLRC tempContext = NULL;

    mHWnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    HDC whdc = mWhdc = GetDC(mHWnd);
    if (whdc == NULL) {
      utils::slog.e << "CreateWindowA() failed" << utils::io::endl;
      goto error;
    }

    int pixelFormat = ChoosePixelFormat(whdc, &mPfd);
    SetPixelFormat(whdc, pixelFormat, &mPfd);

    printf("--- using pix: %d\n", pixelFormat);

    // We need a tmp context to retrieve and call wglCreateContextAttribsARB.
    tempContext = wglCreateContext(whdc);
    if (!wglMakeCurrent(whdc, tempContext)) {
      utils::slog.e << "wglMakeCurrent() failed, whdc=" << whdc << ", tempContext=" <<
        tempContext << utils::io::endl;
      goto error;
    }

    printf("--- filament is using GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    printf("--- filament is using GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));
    
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribs =
      (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
    mContext = wglCreateContextAttribs(whdc, (HGLRC) sharedGLContext, attribs);
    if (!mContext) {
      utils::slog.e << "wglCreateContextAttribs() failed, whdc=" << whdc << utils::io::endl;
      goto error;
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tempContext);
    tempContext = NULL;

    if (!wglMakeCurrent(whdc, mContext)) {
      utils::slog.e << "wglMakeCurrent() failed, whdc=" << whdc << ", mContext=" <<
        mContext << utils::io::endl;
      goto error;
    }

    int result = bluegl::bind();
    ASSERT_POSTCONDITION(!result, "Unable to load OpenGL entry points.");
    return OpenGLDriverFactory::create(this, sharedGLContext);

  error:
    if (tempContext) {
      wglDeleteContext(tempContext);
    }
    reportLastWindowsError();
    terminate();
    return NULL;
#endif    
  }

  void PlatformWGL::terminate() noexcept {
    wglMakeCurrent(NULL, NULL);
    if (mContext) {
      wglDeleteContext(mContext);
      mContext = NULL;
    }
    if (mHWnd && mWhdc) {
      ReleaseDC(mHWnd, mWhdc);
      DestroyWindow(mHWnd);
      mHWnd = NULL;
      mWhdc = NULL;
    } else if (mHWnd) {
      DestroyWindow(mHWnd);
      mHWnd = NULL;
    }
    bluegl::unbind();
  }

  Platform::SwapChain* PlatformWGL::createSwapChain(void* nativeWindow, uint64_t& flags) noexcept {
    // on Windows, the nativeWindow maps to a HWND
    HDC hdc = GetDC((HWND) nativeWindow);
    if (!ASSERT_POSTCONDITION_NON_FATAL(hdc,
                                        "Unable to create the SwapChain (nativeWindow = %p)", nativeWindow)) {
      reportLastWindowsError();
    }

    // We have to match pixel formats across the HDC and HGLRC (mContext)
    int pixelFormat = ChoosePixelFormat(hdc, &mPfd);
    SetPixelFormat(hdc, pixelFormat, &mPfd);

    SwapChain* swapChain = (SwapChain*) hdc;
    return swapChain;
  }

  Platform::SwapChain* PlatformWGL::createSwapChain(uint32_t width, uint32_t height, uint64_t& flags) noexcept {
    // TODO: implement headless SwapChain
    return nullptr;
  }

  void PlatformWGL::destroySwapChain(Platform::SwapChain* swapChain) noexcept {
    HDC dc = (HDC) swapChain;
    HWND window = WindowFromDC(dc);
    ReleaseDC(window, dc);
    // make this swapChain not current (by making a dummy one current)
    wglMakeCurrent(mWhdc, mContext);
  }

  void PlatformWGL::makeCurrent(Platform::SwapChain* drawSwapChain,
                                Platform::SwapChain* readSwapChain) noexcept {

    printf("--- makeCurrent()\n");
    ASSERT_PRECONDITION_NON_FATAL(drawSwapChain == readSwapChain,
                                  "PlatformWGL does not support distinct draw/read swap chains.");
    HDC hdc = (HDC)(drawSwapChain);
    if (hdc != NULL) {
      BOOL success = wglMakeCurrent(hdc, mContext);
      if (!ASSERT_POSTCONDITION_NON_FATAL(success, "wglMakeCurrent() failed. hdc = %p", hdc)) {
        reportLastWindowsError();
        wglMakeCurrent(0, NULL);
      }
    }
  }

  void PlatformWGL::commit(Platform::SwapChain* swapChain) noexcept {
    HDC hdc = (HDC)(swapChain);
    if (hdc != NULL) {
      SwapBuffers(hdc);
    }
  }

  //TODO Implement WGL fences
  Platform::Fence* PlatformWGL::createFence() noexcept {
    return nullptr;
  }

  void PlatformWGL::destroyFence(Fence* fence) noexcept {
  }

  backend::FenceStatus PlatformWGL::waitFence(Fence* fence, uint64_t timeout) noexcept {
    return backend::FenceStatus::ERROR;
  }

} // namespace filament
