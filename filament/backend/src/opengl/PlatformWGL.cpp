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

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

    printf("--- create filament platformwgl driver.\n");
    
#define USE_ROXLU 0
#define USE_FILA 0
#define USE_GIVEN 0  /* Use the given `sharedGLContext` as our own context; experimenting with creating the shared context somewhere else. */
#define USE_SAME_AS_GLFW 1

#if USE_SAME_AS_GLFW

    printf("--- creating a context using similar code as how the context is created in GLFW when USE_GIVEN is set to 1\n");
    
    /* 
       When I create a GL context in my glfw test application and
       pass it into `createDriver` (as a `void* data[]`) I don't get
       an error when making the context current and I can even render.

       Here, I basically copy/pasted the same code from my glfw test
       to see if that works and verify I didn't make a typo/mistake
       somewhere.

       `data[0]` is the HDC of the GLFW window.
       `data[1]` is the context that was created by GLFW
    */

    /* Get the context and hdc to share with. */
    void** data = (void**)sharedGLContext;

    HDC curr_hdc = (HDC)data[0];
    if (nullptr == curr_hdc) {
      printf("--- Failed to get the `HDC` from our `HWND`. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    HGLRC curr_context = (HGLRC)data[1];
    if (nullptr == curr_context) {
      printf("--- Failed to get our current GL context. (exiting).\n");
      exit(EXIT_FAILURE);
    }

    /* 
       The next couple of lines create a temporary GL context that we need
       to be able to retrieve the `wglCreateContextAttribsARB()`. We cannot
       make the given `curr_context` (created by GLFW) current because that
       context is bound to another thread.
    */
    HGLRC tmp_context = nullptr;
    HDC tmp_hdc = nullptr;
    HWND tmp_hwnd = nullptr;
    HINSTANCE tmp_hinst = nullptr;
    {
      /* ======================= begin of tmp context ======================= */

      tmp_hinst = GetModuleHandle(nullptr);
      if (nullptr == tmp_hinst) {
        printf("--- Failed to get the HINSTANCE. (exiting).\n");
        exit(EXIT_FAILURE);
      }
  
      /* Register our dummy class */
      const char* tmp_class_name = "dummy-class";
      WNDCLASSEX tmp_class = {};
      tmp_class.cbSize = sizeof(tmp_class);
      tmp_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
      tmp_class.lpfnWndProc = window_proc;
      tmp_class.hInstance = tmp_hinst;
      tmp_class.hCursor = LoadCursor(NULL, IDC_ARROW);
      tmp_class.lpszClassName = tmp_class_name;

      ATOM tmp_atom = RegisterClassEx(&tmp_class);
      if (0 == tmp_atom) {
        printf("Failed to register the dummy class. (exiting).\n");
        exit(EXIT_FAILURE);
      }

      printf("--- create the dummy window.\n");
      
      /* Create a window for the class that we just registered. */
      tmp_hwnd = CreateWindow(
        tmp_class_name,
        "window title for dummy", 
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
        0, 0, 1, 1,                       
        nullptr,
        nullptr,                 
        tmp_hinst,
        nullptr
      );

      printf("--- done; we've got a dummy window.\n");

      if (nullptr == tmp_hwnd) {
        printf("Failed to create our dummy window. (exiting).\n");
        exit(EXIT_FAILURE);
      }

      /* Get the device context for the client area in the window. */
      tmp_hdc = GetDC(tmp_hwnd);
      if (nullptr == tmp_hdc) {
        printf("Failed to get the Device Context to our dummy window. (exiting).\n");
        exit(EXIT_FAILURE);
      }

      printf("--- We got a dummy HDC\n");

      /* 
         Now that we have a device context, we have to create a pixel
         format description that we can pass into
         `ChoosePixelFormat()`. Once we have a pixel format we can
         create our dummy rendering context. There are many more
         fields but here we only set the fields that are required for
         `ChoosePixelFormat()`.

         In `filament` the `dwLayerMask` is set to PFD_MAIN_PLANE. 
         [This reference][11] tells us that this field is not used anymore.
     
      */
      PIXELFORMATDESCRIPTOR tmp_pix_fmt = {};
      tmp_pix_fmt.nSize = sizeof(tmp_pix_fmt);
      tmp_pix_fmt.nVersion = 1;
      tmp_pix_fmt.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; /* Q: what flags should we use here; as this is a dummy only should be bother with e.g. duoble buffering?  A: See `ChoosePixelFormat()` that lists the requirements. */
      tmp_pix_fmt.iPixelType = PFD_TYPE_RGBA;
      tmp_pix_fmt.cColorBits = 32;
      tmp_pix_fmt.cAlphaBits = 8;
      tmp_pix_fmt.cDepthBits = 24;

      int tmp_pix_fmt_dx = ChoosePixelFormat(tmp_hdc, &tmp_pix_fmt);
      if (0 == tmp_pix_fmt_dx) {
        printf("Failed to select find a pixel format that matches our dummy pixel format descriptor. (exiting). \n");
        exit(EXIT_FAILURE);
      }

      printf("--- We found a dummy pixel format: %d\n", tmp_pix_fmt_dx);

      if (FALSE == SetPixelFormat(tmp_hdc, tmp_pix_fmt_dx, &tmp_pix_fmt)) {
        printf("Failed to set the pixel format for our dummy device context. (exiting).\n");
        exit(EXIT_FAILURE);
      }

      printf("--- We've set the pixel format on our dummy device context.\n");

      /*
        Now that the dummy device context know about the pixel format to use, we 
        can create a dummy OpenGL context that uses this pixel format. Once we have
        a dummy OpenGL context, we retrieve the extension that gives us
        `wglChoosePixelFormatARB()`
      */
      tmp_context = wglCreateContext(tmp_hdc);
      if (0 == tmp_context) {
        printf("Failed to create our dummy opengl context. (exiting).\n");
        exit(EXIT_FAILURE);
      }

      printf("--- We've created our dummy OpenGL context.\n");

      if (FALSE == wglMakeCurrent(tmp_hdc, tmp_context)) {
        printf("Failed to make our dummy OpenGL context current. (exiting).\n");
        exit(EXIT_FAILURE);
      }
      /* ======================= end of tmp context ======================= */
    } /* end of tmp context. */

    /*
    if (FALSE == wglMakeCurrent(curr_hdc, curr_context)) {
      printf("-- Failed to make the current context current. (exiting).\n");
      exit(EXIT_FAILURE);
    }
    */
    
    /* Experimental: create another GL context that can be used by Filament. */
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
    if (nullptr == wglCreateContextAttribsARB) {
      printf("--- `wglCreateContextAttibsARB` function hasn't been loaded yet. Did you init with `glad`? (exiting).\n");
      exit(EXIT_FAILURE);
    }
    
    int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_PROFILE_MASK_ARB,
      0
    };

    /* ################################################################# */
    /* 
       The call to `wglCreateContextAttribsARB()` fails here. The above
       code is almost identical to the code that I use in my test GLFW
       application. 

       I wonder if this is issue can be caused because we try to create
       a GL context that shares a context that was created in another
       thread? 

       Maybe we do have to create a temporary context? 

    /* ################################################################# */


    /* Create the context that Filament will use. */
    HGLRC fila_context = wglCreateContextAttribsARB(tmp_hdc, tmp_context, attribs);
    if (nullptr == fila_context) {
      printf("--- Failed to create another GL context for Filament. (exiting).\n");
      exit(EXIT_FAILURE);
    }

#endif

#if USE_GIVEN
    /*
      This version expects that `data[1]` contains a context that
      was created by the user. This is the only solution I have found
      which works with GLFW and Filament. 
     */
    void** data = (void**)sharedGLContext;
    HDC curr_hdc = (HDC)data[0];
    HGLRC shared_context = (HGLRC)data[1];
    printf("--- data[0]: %p (hdc)\n", data[0]);
    printf("--- data[1]: %p (HGLRC)\n", data[1]);
    
    mContext = (HGLRC)shared_context;
    wglMakeCurrent(curr_hdc, mContext);
    printf("--- Using given context as our own. (%p).\n", sharedGLContext);
    int result = bluegl::bind();
    ASSERT_POSTCONDITION(!result, "Unable to load OpenGL entry points.");
    return OpenGLDriverFactory::create(this, sharedGLContext);
#endif
    
#if USE_ROXLU

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

    void** data = (void**)sharedGLContext;
    curr_hdc = (HDC)data[0];
    HGLRC shared_context = (HGLRC)data[1];
    printf("--- data[0]: %p (hdc)\n", data[0]);
    printf("--- data[1]: %p (HGLRC)\n", data[1]);

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
    printf("--- Using pixel format: %d\n", curr_pix);
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
    //printf("--- GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    //printf("--- GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));

    /* Now, that we created a dummy format, we can (finally) get the `wglCreateContextAttribsARB()` function. */
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsRoxlu = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
    if (nullptr == wglCreateContextAttribsRoxlu) {
      printf("Failed to get wglCreateContextAttribs(). (exiting).\n");
      reportLastWindowsError();
      exit(EXIT_FAILURE);
    }
    
    int attribs_roxlu[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_PROFILE_MASK_ARB,
      0
    };

    /* 
       It doesn't make a different (or doesnt result in an
       error), when I use the dummy_hdc or the curr_hdc when
       calling `wglCreateContextAttribsARB() and
       wglMakeCurrent()`.

       Currently this results in a INVALID_OPERATION 
    */
    mContext = wglCreateContextAttribsRoxlu(dummy_hdc, (HGLRC)sharedGLContext, attribs_roxlu);
    //mContext = wglCreateContextAttribsRoxlu(curr_hdc, (HGLRC)shared_context, attribs_roxlu);
    if (nullptr == mContext) {
      DWORD err = GetLastError();
      printf("Failed to create GL context.%08X (exiting).\n", err);
      printf("Severity: %08X\n", HRESULT_SEVERITY(err));
      printf("Facility: %llu\n", HRESULT_FACILITY(err));
      printf("Code: %llu, %08X\n", HRESULT_CODE(err), (uint64_t)(HRESULT_CODE(err)));
      reportLastWindowsError();
      exit(EXIT_FAILURE);
    }

    wglMakeCurrent(curr_hdc, mContext);

    printf("--- curr_wnd: %p\n", curr_hwnd);
    printf("--- curr_pix: %d\n", curr_pix);
    printf("--- thread id in filament: %s\n", thread_id.c_str());
    printf("--- dummy_desc_fmt: %d\n", dummy_desc_fmt);
    printf("--- created GL context: %p\n", mContext);
    printf("--- shared GL context: %p\n", sharedGLContext);

    /* This fails with `lld-link: error: glGetString was replaced` when compiling with Clang */
    /*
    printf("--- GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    printf("--- GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));
    */

    int result = bluegl::bind();
    ASSERT_POSTCONDITION(!result, "Unable to load OpenGL entry points.");
    return OpenGLDriverFactory::create(this, sharedGLContext);

#endif
    
    /* --------------------------------------------------------------------------- */
    
#if USE_FILA

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

    /* This fails with `lld-link: error: glGetString was replaced` when compiling with Clang */
    /*
    printf("--- filament is using GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    printf("--- filament is using GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));
    */
    
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

    printf("--- createSwapChain on nativeWindow: %p, with flags: %llu.\n", nativeWindow, flags);
    
    // on Windows, the nativeWindow maps to a HWND
    HDC hdc = GetDC((HWND) nativeWindow);
    if (!ASSERT_POSTCONDITION_NON_FATAL(hdc,
                                        "Unable to create the SwapChain (nativeWindow = %p)", nativeWindow)) {
      reportLastWindowsError();
    }

    // We have to match pixel formats across the HDC and HGLRC (mContext)
    int pixelFormat = ChoosePixelFormat(hdc, &mPfd);
    if (0 == SetPixelFormat(hdc, pixelFormat, &mPfd)) {
      printf("--- failed to set the pixel format in createSwapChain. \n");
    }

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

    //printf("--- makeCurrent()\n");
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

/* ------------------------------------------------------- */
/* roxlu (2019-12-02) added this for testing ...           */
/* ------------------------------------------------------- */
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  return DefWindowProc(hwnd, msg, wparam, lparam);
}
/* ------------------------------------------------------- */
