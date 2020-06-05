#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdbool.h>

RECT get_monitor_resolution(HWND hwnd)
{
  MONITORINFO minfo;
  minfo.cbSize = sizeof(minfo);
  GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &minfo);
  return minfo.rcMonitor;
}

BOOL CALLBACK find_thread_topwindow(HWND hwnd, LPARAM lParam)
{
  HWND* pphWnd = (HWND*)lParam;

  if (GetParent(hwnd))
  {
    *pphWnd = NULL;
    return TRUE;
  }

  *pphWnd = hwnd;
  return FALSE;
}

BOOL CALLBACK resize_vimtextarea(HWND hwnd, LPARAM lParam)
{
  char lpszClassName[100];
  GetClassName(hwnd, lpszClassName, 100);
  if (strcmp(lpszClassName, "VimTextArea") == 0) 
  {
    const LPCRECT r = (LPCRECT)lParam;

    const int cx = r->right-r->left;
    const int cy = r->bottom-r->top;

    SetWindowPos(hwnd, NULL, 0, 0, cx, cy, 
      SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
  }

  return TRUE;
}

struct save_vim_wnd_info_t
{
  DWORD style;
  DWORD exstyle;
  bool maximized;
  RECT r;
  LONG user_data;
};

LONG _declspec(dllexport) toggle_full_screen()
{
  HWND vim_hwnd = NULL;

  const DWORD threadId = GetCurrentThreadId();
  EnumThreadWindows(threadId, find_thread_topwindow, (LPARAM)&vim_hwnd);

  if (NULL == vim_hwnd)
    return 1;

  const bool fullscreen = (GetWindowLong(vim_hwnd, GWL_STYLE) & WS_CAPTION) != 0;

  if (fullscreen)
  {
    struct save_vim_wnd_info_t *info = GlobalAlloc(GMEM_FIXED, sizeof(struct save_vim_wnd_info_t));
    info->maximized = IsZoomed(vim_hwnd);

    if (info->maximized)
      SendMessage(vim_hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);

    info->style = GetWindowLong(vim_hwnd, GWL_STYLE);
    info->exstyle = GetWindowLong(vim_hwnd, GWL_EXSTYLE);
    GetWindowRect(vim_hwnd, &info->r);

    info->user_data = SetWindowLong(vim_hwnd, GWL_USERDATA, (LONG)info);

    SetWindowLong(vim_hwnd, GWL_STYLE, 
      info->style & ~(WS_CAPTION|WS_THICKFRAME));
    SetWindowLong(vim_hwnd, GWL_EXSTYLE, 
      info->exstyle & ~(WS_EX_WINDOWEDGE));

    const RECT r = get_monitor_resolution(vim_hwnd);

    SetWindowPos(vim_hwnd, NULL, r.left, r.top, r.right-r.left, r.bottom-r.top,
      SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);

    EnumChildWindows(vim_hwnd, resize_vimtextarea, (LPARAM)&r);
  }
  else
  {
    struct save_vim_wnd_info_t *info = (struct save_vim_wnd_info_t*)GetWindowLong(vim_hwnd, GWL_USERDATA);

    SetWindowLong(vim_hwnd, GWL_STYLE, 
      info->style | (WS_CAPTION|WS_THICKFRAME));
    SetWindowLong(vim_hwnd, GWL_EXSTYLE, 
      info->exstyle | WS_EX_WINDOWEDGE);
    SetWindowLong(vim_hwnd, GWL_USERDATA, info->user_data);

    SetWindowPos(vim_hwnd, NULL, info->r.left, info->r.top, 
      info->r.right-info->r.left, info->r.bottom-info->r.top,
      SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_SHOWWINDOW);

    GlobalFree(info);
  }

  return 0;
}
