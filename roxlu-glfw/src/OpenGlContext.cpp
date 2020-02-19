#include <OpenGlContext.h>

namespace poly {

  /* -------------------------------------------- */
  
  static LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  
  /* -------------------------------------------- */
  
  bool OpenGlContextSettings::isValid() {

    if (0 == name.size()) {
      printf("Error: name is not set. We want our `OpenGlContext` to have a name.\n");
      return false;
    }

    if (true == create_window) {
    
      if (0 == title.size()) {
        printf("When `create_window` is true, you should set `title`.\n");
        return false;
      }
    
      if (UINT32_MAX == x) {
        printf("When `create_window` is true, you should set `x`.\n");
        return false;
      }
    
      if (UINT32_MAX == y) {
        printf("When `create_window` is true, you should set `y`.\n");
        return false;
      }
    }
  
    return true;
  }

  /* -------------------------------------------- */

  OpenGlContext::~OpenGlContext() {
    shutdown();
  }

  /* -------------------------------------------- */

  int OpenGlContext::init(OpenGlContextSettings& cfg) {

    static bool did_register_class = false;
    PIXELFORMATDESCRIPTOR tmp_fmt = {};
    HGLRC shared_gl = nullptr;
    HWND tmp_hwnd = nullptr;
    HGLRC tmp_gl = nullptr;
    HDC tmp_hdc = nullptr;
    WNDCLASSEX wc = { };
    DWORD style_ex = 0;
    DWORD style = 0;
    int tmp_dx = -1;
    int r = 0;
    UINT fmt_count = 0;

    /* The pixel format attributes that we need. */
    const int pix_attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB, 32,
      WGL_ALPHA_BITS_ARB, 8,
      WGL_DEPTH_BITS_ARB, 24,
      WGL_STENCIL_BITS_ARB, 8,
      WGL_SAMPLE_BUFFERS_ARB, GL_FALSE,
      WGL_SAMPLES_ARB, 0,
      0
    };

    /* The OpenGL Rendering Context attributes we need. */
    int ctx_attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0
    };
  
    if (false == cfg.isValid()) {
      printf("Given `OpenGlContextSettings` are invalid.\n");
      r = -1;
      goto error;
    }

    settings = cfg;

    if (nullptr != hwnd) {
      printf("Already initialized. Probably not what you want.\n");
      r = -2;
      goto error;
    }

    /* Step 1: create a tmp window. */
    tmp_hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    if (nullptr == tmp_hwnd) {
      printf("Failed to create our tmp window.\n");
      r = -1;
      goto error;
    }

    /* Step 2: set the pixel format. */
    tmp_fmt.nSize = sizeof(tmp_fmt);
    tmp_fmt.nVersion = 1;
    tmp_fmt.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    tmp_fmt.iPixelType = PFD_TYPE_RGBA;
    tmp_fmt.cColorBits = 32;
    tmp_fmt.cAlphaBits = 8;
    tmp_fmt.cDepthBits = 24;

    tmp_hdc = GetDC(tmp_hwnd);
    if (nullptr == tmp_hdc) {
      printf("Failed to get the HDC from our tmp window.\n");
      r = -2;
      goto error;
    }

    tmp_dx = ChoosePixelFormat(tmp_hdc, &tmp_fmt);
    if (0 == tmp_dx) {
      printf("Failed to find a pixel format for our tmp hdc.\n");
      r = -3;
      goto error;
    }

    if (FALSE == SetPixelFormat(tmp_hdc, tmp_dx, &tmp_fmt)) {
      printf("Failed to set the pixel format on our tmp hdc.\n");
      r = -4;
      goto error;
    }

    /* Step 3. Create temporary GL context. */
    tmp_gl = wglCreateContext(tmp_hdc);
    if (nullptr == tmp_gl) {
      printf("Failed to create out temporary GL context.\n");
      r = -5;
      goto error;
    }

    if (FALSE == wglMakeCurrent(tmp_hdc, tmp_gl)) {
      printf("Failed to make our temporary GL context current.\n");
      r = -6;
      goto error;
    }

    /* Step 4. Get the extension we need for a more feature-rich context. */
    wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
    if (nullptr == wglChoosePixelFormatARB) {
      printf("Failed to get the `wglChoosePixelFormatARB()` function.\n");
      r = -7;
      goto error;
    }
  
    wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
    if (nullptr == wglCreateContextAttribsARB) {
      printf("wglGetProcAddress() failed.\n");
      r = -8;
      goto error;
    }
  
    /* Step 5: create the HWND for our main context. */
    if (true == settings.create_window) {

      if (false == did_register_class) {
      
        wc.cbSize  = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wc.lpfnWndProc = win_proc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "roxlu";
  
        if (0 == RegisterClassEx(&wc)) {
          printf("Failed to register the class.\n");
          r = -9;
          goto error;
        }

        did_register_class = true;
      }

      if (0 != getWindowStyle(style)) {
        printf("Failed to get the window style.\n");
        r = -10;
        goto error;
      }

      if (0 != getWindowStyleEx(style_ex)) {
        printf("Failed to get the window style_ex.\n");
        r = -11;
        goto error;
      }

      hwnd = CreateWindowExA(
        style_ex,
        "roxlu",
        settings.title.c_str(),
        style,
        settings.x, 
        settings.y, 
        settings.width,
        settings.height,
        NULL,
        NULL,
        wc.hInstance,
        NULL
      );
    }
    else {
    
      hwnd = CreateWindowA("STATIC", "dummy", 0, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    }

    if (nullptr == hwnd) {
      printf("Failed to create main window (hwnd).\n");
      r = -4;
      goto error;
    }  
  
    /* Step 6: set the pixel format */
    hdc = GetDC(hwnd);
    if (nullptr == hdc) {
      printf("Failed to get the HDC from our main window.\n");
      r = -5;
      goto error;
    }

    /* Find the best matching pixel format index. */
    if (FALSE == wglChoosePixelFormatARB(hdc, pix_attribs, NULL, 1, &dx, &fmt_count)) {
      printf("Failed to choose a valid pixel format for our main hdc.\n");
      r = -6;
      goto error;
    }

    /* Now that we have found the index, fill our format descriptor. */
    if (0 == DescribePixelFormat(hdc, dx, sizeof(fmt), &fmt)) {
      printf("Failed to fill our main pixel format descriptor.\n");
      r = -7;
      goto error;
    }

    if (FALSE == SetPixelFormat(hdc, dx, &fmt)) {
      printf("Failed to set the pixel format on our main dc.\n");
      r = -8;
      goto error;
    }

    /* Step 7: finally, create our main context. */
    if (nullptr != settings.shared_context) {
    
      if (nullptr == settings.shared_context->gl) {
        printf("Warning: You set a `shared_context` pointer, but it's not initialized.\n");
      }
    
      shared_gl = settings.shared_context->gl;
    }
  
    gl = wglCreateContextAttribsARB(hdc, shared_gl, ctx_attribs);
    if (nullptr == gl) {
      printf("Failed to create our main OpenGL context.\n");
      r = -8;
      goto error;
    }

  error:

    printf("DESTROY TMP HWND AND CONTEXT\n");
  
    if (nullptr != tmp_gl) {
      if (FALSE == wglDeleteContext(tmp_gl)) {
        printf("Failed to delete the `tmp_gl`.\n");
        r -= 3;
      }
    }
  
    if (nullptr != tmp_hwnd) {
      if (FALSE == DestroyWindow(tmp_hwnd)) {
        printf("Failed to destroy the `tmp_hwnd`.\n");
        r -= 5;
      }
    }

    tmp_gl = nullptr;
    tmp_hwnd = nullptr;

    if (r < 0) {
      shutdown();
    }

    return r;
  }

  int OpenGlContext::shutdown() {

    int r = 0;

    if (nullptr != gl) {
      if (FALSE == wglDeleteContext(gl)) {
        printf("Failed to cleanly delete the GL context.\n");
        r -= 5;
      }
    }
  
    if (nullptr != hwnd) {
      if (FALSE == DestroyWindow(hwnd)) {
        printf("Failed to clealy destroy the `hwnd`.\n");
        r -= 10;
      }
    }

    settings.x = UINT32_MAX;
    settings.y = UINT32_MAX;
    settings.width = UINT32_MAX;
    settings.height = UINT32_MAX;
    settings.name.clear();
    settings.title.clear();
    settings.create_window = true;
    settings.is_decorated = false;
    settings.is_resizable = false;
    settings.is_floating = false;
  
    wglChoosePixelFormatARB = nullptr;
    wglCreateContextAttribsARB = nullptr;
    
    gl = nullptr;
    hwnd = nullptr;
    hdc = nullptr;
    dx = -1;

    memset((char*)&msg, 0x00, sizeof(msg));
    memset((char*)&fmt, 0x00, sizeof(fmt));

    return r;
  }

  /* -------------------------------------------- */

  int OpenGlContext::show() {

    if (nullptr == hwnd) {
      printf("Cannot show windows, `hwnd` is nullptr. Did you initialize?\n");
      return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    return 0;
  }

  int OpenGlContext::processMessages() {

    if (nullptr == hwnd) {
      printf("Not initialized.\n");
      return -1;
    }

    int should_run = 0;

    while (0 != PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    
      if (msg.message == WM_QUIT) {
        should_run = -1;
      }
    
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    return should_run;
  }

  int OpenGlContext::makeCurrent() {
  
    if (nullptr == hdc) {
      printf("Cannot make current, `hdc` is nullptr.\n");
      return -1;
    }

    if (nullptr == gl) {
      printf("Cannot make currenct, `gl` is nullptr.\n");
      return -2;
    }

    if (FALSE == wglMakeCurrent(hdc, gl)) {
      printf("Failed to make current.\n");
      return -3;
    }

    return 0;
  }

  int OpenGlContext::swapBuffers() {

    if (nullptr == hdc) {
      printf("Cannot swap buffers, `hdc` is nullptr.\n");
      return -1;
    }

    if (FALSE == SwapBuffers(hdc)) {
      printf("Failed to swap the buffers.\n");
      return -2;
    }

    return 0;
  }

  /* -------------------------------------------- */

  int OpenGlContext::getWindowStyle(DWORD& result) {

    if (false == settings.isValid()) {
      printf("Cannot get window style; settings are invalid.\n");
      return -1;
    }

    result = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    if (true == settings.is_decorated) {
      result |= WS_SYSMENU;
      result |= WS_MINIMIZEBOX;
    }
    else {
      result |= WS_POPUP;
    }

    if (true == settings.is_resizable) {
      result |= WS_MAXIMIZEBOX;
      result |= WS_THICKFRAME;
    }
  
    return 0;
  }

  int OpenGlContext::getWindowStyleEx(DWORD& result) {

    if (false == settings.isValid()) {
      printf("Cannot get window style ex; settings are invalid.\n");
      return -1;
    }

    result = WS_EX_APPWINDOW;
  
    if (true == settings.is_floating) {
      result |= WS_EX_TOPMOST;
    }
  
    return 0;
  }

  /* -------------------------------------------- */

  int OpenGlContext::print() {

    if (0 == settings.name.size()) {
      printf("Cannot print, name not set. Did you initialize?\n");
      return -1;
    }

    if (nullptr == hdc) {
      printf("Cannot print, hdc is nullptr. Did you initialize?\n");
      return -2;
    }

    if (nullptr == gl) {
      printf("Cannot print, gl is nullptr. Did you initialize?\n");
      return -3;
    }
  
    if (FALSE == wglMakeCurrent(hdc, gl)) {
      printf("Cannot print. Failed to make the GL context current.\n");
      return -4;
    }

    if (nullptr == glGetString) {
      printf("Cannot print, no GL-functions loaded yet. Did you e.g. use `gladLoadGld()?\n");
      return -5;
    }
    
    printf("%s: HGLRC: %p\n", settings.name.c_str(), gl);
    printf("%s: GL_VERSION: %s\n", settings.name.c_str(), glGetString(GL_VERSION));
    printf("%s: GL_VENDOR: %s\n", settings.name.c_str(), glGetString(GL_VENDOR));

    return -6;
  }

  /* -------------------------------------------- */

  static LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  
    switch (msg) {

      case WM_KEYDOWN: {
      
        switch (wparam) {
          case VK_ESCAPE: {
            PostQuitMessage(0);
            return 0;
          }
        }
        break;
      }
      
      case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
      }
      
      default: {
        break;
      }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
  }
  
  /* -------------------------------------------- */
  
} /* namespace poly */
