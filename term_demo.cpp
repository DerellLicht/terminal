//****************************************************************************
//  Copyright (c) 1985-2026  Daniel D Miller
//  Demo program for terminal class
//
//  Written by:  Dan Miller
//****************************************************************************

static const char *Version = "Terminal program, Version 1.01" ;

#include <windows.h>
#include <stdio.h>   //  vsprintf, sprintf, which supports %f

#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "term_demo.h"
#include "statbar.h"
#include "cterminal.h" 
#include "terminal.h" 
#include "winmsgs.h"

//***********************************************************************
static HINSTANCE g_hinst = 0;

static HWND hwndMain ;

uint cxClient = 0 ;
uint cyClient = 0 ;

// static HMENU hMainMenu = NULL ;

// static CStatusBar *MainStatusBar = NULL;
static std::unique_ptr<CStatusBar> MainStatusBar {};
// static HWND hToolTip ;  /* Tooltip handle */

//  user-defined Windows messages
// static const UINT WM_ARE_YOU_ME = (WM_USER + 106) ;

//*******************************************************************
//  *** BEGIN Claude resize data block
//*******************************************************************
// Claude 08/14/26 - smallest listview height (pixels) we'll allow the
// live-resize floor to shrink down to, so a few rows stay visible/usable
// no matter how far the user drags the bottom edge up.
#define  MIN_LISTVIEW_VISIBLE_DY   80

// Claude 08/14/26
// MINMAXINFO's ptMinTrackSize/ptMaxTrackSize are WINDOW (outer) dimensions,
// not client-area dimensions -- but cxClient/cyClient come from GetClientRect(),
// which excludes the caption/border. Pinning ptMinTrackSize.x==ptMaxTrackSize.x
// directly to cxClient tells Windows "the whole window, borders included, is
// only as wide as the client area" -- i.e. a few pixels *too narrow* by exactly
// the border width. That's the "shrinks by a few pixels on first width-drag"
// symptom. Fix: measure the real window-minus-client delta once at init, and
// add it back in whenever a track size is derived from a client dimension.
static int dx_frame = 0;   //  window width  - client width
static int dy_frame = 0;   //  window height - client height

// Claude 08/14/26 - term_window_height tracks the LISTVIEW's current height
// and gets recalculated on every resize (see resize_font_dialog). It is NOT
// a safe floor for WM_GETMINMAXINFO, because by the time a live drag is
// underway its value has already moved. min_application_window_height is
// the true floor: computed once in do_init_dialog from the fixed pieces
// (top controls + a minimum usable listview height + status bar + frame)
// and never modified afterward.
static uint min_application_window_height = 0;

//*******************************************************************
//  *** END Claude resize data block
//*******************************************************************
//*****************************************************************
//lint -esym(756, attrib_table_t)
typedef struct attrib_table_s {
   COLORREF fgnd ;
   COLORREF bgnd ;
} attrib_table_t ;

//****************************************************************************
//lint -esym(749, TERM_INFO, TERM_QUERY)
//  indices into term_atable[]
// enum {
enum class eTermAttr : uint8_t {
TERM_NORMAL = 0,
TERM_INFO,
TERM_QUERY,
TERM_PLAYER_HIT,
TERM_MONSTER_HIT,
TERM_RUNESTAFF,
TERM_DEATH,
TERM_ATMOSPHERE
} ;

#define  NUM_TERM_ATTR_ENTRIES   8
static attrib_table_t term_atable[NUM_TERM_ATTR_ENTRIES] = {
   { WIN_CYAN,    WIN_BLACK },   // TERM_NORMAL 
   { WIN_BCYAN,   WIN_GREY },    // TERM_INFO
   { WIN_YELLOW,  WIN_BLUE },    // TERM_QUERY
   { WIN_RED,     WIN_BLACK },   // TERM_PLAYER_HIT
   { WIN_BLUE,    WIN_BLACK },   // TERM_MONSTER_HIT
   { WIN_GREY,    WIN_BLACK },   // TERM_RUNESTAFF
   { WIN_BBLUE,   WIN_BLACK },   // TERM_DEATH
   { WIN_GREEN,   WIN_BLACK }    // TERM_ATMOSPHERE
} ;

//****************************************************************************
static void set_local_terminal_colors(void)
{
   COLORREF std_bgnd = GetSysColor(COLOR_WINDOW) ;
   term_atable[(uint) eTermAttr::TERM_NORMAL].fgnd = GetSysColor(COLOR_WINDOWTEXT) ;
   term_atable[(uint) eTermAttr::TERM_NORMAL].bgnd = std_bgnd ;

   //  set standard background for other color sets which use it
   term_atable[(uint) eTermAttr::TERM_PLAYER_HIT].bgnd = std_bgnd ;
   term_atable[(uint) eTermAttr::TERM_MONSTER_HIT].bgnd = std_bgnd ;
   term_atable[(uint) eTermAttr::TERM_RUNESTAFF].bgnd = std_bgnd ;
   term_atable[(uint) eTermAttr::TERM_DEATH].bgnd = std_bgnd ;
   term_atable[(uint) eTermAttr::TERM_ATMOSPHERE].bgnd = std_bgnd ;
}

//********************************************************************
static void set_term_attr(uint atidx)
{
   if (atidx >= NUM_TERM_ATTR_ENTRIES) {
      syslog("set_term_attr: invalid index %u\n", atidx) ;
      return ;
   }
   term_set_attr(term_atable[atidx].fgnd, term_atable[atidx].bgnd) ;
}

//*******************************************************************
//lint -esym(714, status_message)
//lint -esym(759, status_message)
//lint -esym(765, status_message)
void status_message(char *msgstr)
{
   MainStatusBar->show_message(msgstr);
}

void status_message(uint idx, char *msgstr)
{
   MainStatusBar->show_message(idx, msgstr);
}

//****************************************************************************
//  small font-dependent layout fudge factor; shared by do_init_dialog's
//  min-height calculation and resize_font_dialog's live layout so the two
//  stay consistent with each other.
//****************************************************************************
static int get_dy_offset(void)
{
   return 0 ;
}

//****************************************************************************
static uint get_terminal_top(void)
{
   static uint local_ctrl_top = 0 ;
   if (local_ctrl_top == 0) {
      local_ctrl_top = get_bottom_line(hwndMain, IDB_CLOSE) ;
      local_ctrl_top += 3 ;
      // syslog("CommPort: ctrl_top = %u, or %u\n", local_ctrl_top, win_ctrl_top+3) ;
   }
   return local_ctrl_top ;
}  //lint !e715

//********************************************************************
//lint -esym(714, termout)
//lint -esym(759, termout)
//lint -esym(765, termout)
int termout(const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr((uint) eTermAttr::TERM_NORMAL);
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//********************************************************************
//lint -esym(714, term_append)
//lint -esym(759, term_append)
//lint -esym(765, term_append)
int term_append(const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr((uint) eTermAttr::TERM_NORMAL) ;
   term_append(consoleBuffer);
   va_end(al);
   return 1;
}

//********************************************************************
//lint -esym(714, term_replace)
//lint -esym(759, term_replace)
//lint -esym(765, term_replace)
int term_replace(const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr((uint) eTermAttr::TERM_NORMAL) ;
   term_replace(consoleBuffer);
   va_end(al);
   return 1;
}

//********************************************************************
//  this *cannot* be called with a color attribute;
//  it must be called with an index into term_atable[] !!
//********************************************************************
//lint -esym(714, put_color_msg)
//lint -esym(759, put_color_msg)
//lint -esym(765, put_color_msg)
int put_color_msg(uint idx, const char *fmt, ...)
{
   char consoleBuffer[MAX_TERM_CHARS + 1];
   va_list al; //lint !e522

   va_start(al, fmt);   //lint !e1055 !e530
   vsprintf(consoleBuffer, fmt, al);   //lint !e64
   set_term_attr(idx) ;
   term_put(consoleBuffer);
   va_end(al);
   return 1;
}

//***********************************************************************
//  setting main menu, breaks status bar !!
//***********************************************************************
// static void setup_main_menu(HWND hwnd)
// {
//    hMainMenu = LoadMenu(g_hinst, MAKEINTRESOURCE(IDM_MAINMENU));
//    SetMenu(hwnd, hMainMenu);
//    // update_summary_options_menu() ;   //  initial setup
// }

//****************************************************************
//  Claude 08/15/26 - restore previously-saved window size/position
//  from the .ini file. client_height/window_left/window_top were
//  populated by init_config() above (which creates a default config
//  file if one doesn't exist yet, so these are always valid here --
//  no first-run guard needed). Width is never saved/restored since
//  it's always locked to the dialog's fixed layout.
//****************************************************************
static void restore_dialog_settings(HWND hwnd)
{
   uint restored_win_width  = cxClient + (uint) dx_frame ;   //  width never changes
   uint restored_win_height = client_height + (uint) dy_frame ;

   //  clamp height to the same bounds WM_GETMINMAXINFO enforces --
   //  screen resolution may have changed since this was last saved
   if (restored_win_height < min_application_window_height) {
      restored_win_height = min_application_window_height ;
   }
   uint max_win_height = (uint) get_screen_height() ;
   if (restored_win_height > max_win_height) {
      restored_win_height = max_win_height ;
   }

   //  clamp position to the current monitor (get_screen_width/height
   //  reflect get_monitor_dimens(hwnd), already called above) so a saved
   //  position from a monitor that's since been unplugged, or a screen
   //  res that's since shrunk, doesn't put us off-screen
   uint restored_left = window_left ;
   uint restored_top  = window_top ;
   uint scr_cx = (uint) get_screen_width() ;
   uint scr_cy = (uint) get_screen_height() ;
   if (restored_left + restored_win_width > scr_cx) {
      restored_left = (restored_win_width < scr_cx) ? (scr_cx - restored_win_width) : 0 ;
   }
   if (restored_top + restored_win_height > scr_cy) {
      restored_top = (restored_win_height < scr_cy) ? (scr_cy - restored_win_height) : 0 ;
   }

   //  applying this here (after all child controls exist) triggers
   //  WM_SIZE synchronously, which runs resize_font_dialog() and lays
   //  out the status bar/listview/etc. for the restored height --
   //  no separate relayout call needed
   SetWindowPos(hwnd, NULL, (int) restored_left, (int) restored_top,
      (int) restored_win_width, (int) restored_win_height, SWP_NOZORDER) ;
}

//***********************************************************************
static void do_init_dialog(HWND hwnd)
{
   char msgstr[81] ;
   // hwndTopLevel = hwnd ;   //  do I need this?
   wsprintfA(msgstr, "%s", Version) ;
   SetWindowTextA(hwnd, msgstr) ;

   SetClassLongA(hwnd, GCL_HICON,   (LONG) LoadIcon(g_hinst, (LPCTSTR)WINWIZICO));
   SetClassLongA(hwnd, GCL_HICONSM, (LONG) LoadIcon(g_hinst, (LPCTSTR)WINWIZICO));

   hwndMain = hwnd ;
   get_monitor_dimens(hwnd);

   RECT myRect ;
   // GetWindowRect(hwnd, &myRect) ;
   GetClientRect(hwnd, &myRect) ;
   cxClient = (myRect.right - myRect.left) ;
   cyClient = (myRect.bottom - myRect.top) ;

   // Claude 08/14/26 - measure actual border/caption size once, from live
   // window+client rects, rather than guessing at SM_CXFRAME/SM_CYCAPTION
   // (which can be wrong under theming/DPI). Used to convert client-size
   // values into the window-size values WM_GETMINMAXINFO actually wants.
   {
   RECT winRect ;
   GetWindowRect(hwnd, &winRect) ;
   dx_frame = (winRect.right - winRect.left) - (int) cxClient ;
   dy_frame = (winRect.bottom - winRect.top) - (int) cyClient ;
   // syslog("frame delta: dx_frame=%d, dy_frame=%d\n", dx_frame, dy_frame) ;
   }

   init_config();
   
   center_dialog_on_screen(hwnd);
   //  setting main menu, breaks status bar !!
   //  setup_main_menu(hwnd) ;
   
   //****************************************************************
   //  create/configure status bar
   //****************************************************************
   // MainStatusBar = new CStatusBar(hwnd) ;
   MainStatusBar = std::make_unique<CStatusBar>(hwnd);
   MainStatusBar->MoveToBottom(cxClient, cyClient) ;
   //  re-position status-bar parts
   {
   int sbparts[3];
   sbparts[0] = (int) (6 * cxClient / 10) ;
   sbparts[1] = (int) (8 * cxClient / 10) ;
   sbparts[2] = -1;
   MainStatusBar->SetParts(3, &sbparts[0]);
   }
   
   // Claude 08/14/26 - the real, permanent floor for WM_GETMINMAXINFO.
   // Same shape as resize_font_dialog's live layout math, just solved for
   // the smallest acceptable listview height (MIN_LISTVIEW_VISIBLE_DY)
   // instead of the current one. Computed once, here, and never touched
   // again -- see the comment on the variable itself.
   min_application_window_height = get_terminal_top() + MIN_LISTVIEW_VISIBLE_DY
      + MainStatusBar->height() + (uint) get_dy_offset() + (uint) dy_frame ;

   //****************************************************************
   //  create/configure terminal
   //****************************************************************
   setup_terminal_window(hwnd, MainStatusBar->height(), IDB_ADD_LINE, IDC_TERMINAL);
   set_local_terminal_colors() ;
   
   sprintf(msgstr, "terminal size: columns=%u, rows=%u",
      term_get_columns(), term_get_rows());
   status_message(msgstr);
   termout(msgstr);
   
   sprintf(msgstr, "monitor dimens: %ux%u pixels", get_screen_width(), get_screen_height());
   termout(msgstr);
   
   //  restore previously-saved window size/position from the .ini file. 
   restore_dialog_settings(hwnd);
}

//********************************************************************************************
//  okay, this function originally gave inaccurate results,
//  because the rectangle passed by WM_SIZING was from GetWindowRect(),
//  which included the unwanted border area, rather than from
//  GetClientRect(), which works with get_bottom_line().
//********************************************************************************************
static void resize_font_dialog()
{
   RECT myRect ;
   char msgstr[81] ;
   // syslog("resize terminal, drag=%s\n", (resize_on_drag) ? "true" : "false") ;

   //  if resizing on drag-and-drop, re-read main-dialog size
   // BOOL gcr_ok = 
   GetClientRect(hwndMain, &myRect) ;
   // new_window_width  = (uint) (myRect.right - myRect.left) ;
   uint new_window_height = (uint) (myRect.bottom - myRect.top) ;
   // syslog("resize: cyClient: %u, new_window_height: %u, rect=(%ld,%ld,%ld,%ld), gcr_ok=%d, err=%lu\n",
   //    cyClient, new_window_height,
   //    (long) myRect.left, (long) myRect.top, (long) myRect.right, (long) myRect.bottom,
   //    (int) gcr_ok, gcr_ok ? 0ul : (unsigned long) GetLastError());

   if (cyClient == new_window_height  ||  new_window_height == 0) {
       return ;
   }

   cyClient = new_window_height ;

   int dy_offset = get_dy_offset() ;

   MainStatusBar->MoveToBottom(cxClient, cyClient-1) ;
   //  resize the terminal (cols)
   int dyi = (int) cyClient - dy_offset - (int) get_terminal_top() - MainStatusBar->height() ;
   term_resize(cxClient, dyi);
   
   sprintf(msgstr, "terminal size: columns=%u, rows=%u",
      term_get_columns(), term_get_rows());
   status_message(msgstr);
   // termout(msgstr);
   
   save_cfg_file();
}

//*************************************************************************************
// Claude: WM_SIZE — this is the only place you actually move/resize child controls. 
// Dialogs don't auto-relayout children on resize; you compute the height delta 
// and grow the listview by exactly that much, leaving the top controls alone.
//*************************************************************************************
// static const char *size_type_name(WPARAM wParam)
// {
//    switch (wParam) {
//    case SIZE_RESTORED:  return "SIZE_RESTORED" ;
//    case SIZE_MINIMIZED: return "SIZE_MINIMIZED" ;
//    case SIZE_MAXIMIZED: return "SIZE_MAXIMIZED" ;
//    case SIZE_MAXSHOW:   return "SIZE_MAXSHOW" ;
//    case SIZE_MAXHIDE:   return "SIZE_MAXHIDE" ;
//    default:             return "SIZE_??" ;
//    }
// }

static bool do_size(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   // syslog("do_size: type=%s, lParam dx=%d dy=%d, IsIconic=%d\n",
   //    size_type_name(wParam), (int) LOWORD(lParam), (int) HIWORD(lParam),
   //    (int) IsIconic(hwnd));
   resize_font_dialog();
   return true ;
}

//*************************************************************************************
// Claude: WM_SIZING itself isn't needed for this shape of problem — 
// it's for constraining to an aspect ratio or snapping to a grid during the drag. 
// Locking width via WM_GETMINMAXINFO is simpler and sufficient here.
//*************************************************************************************
// static 
bool do_sizing(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   //  handle main-dialog resizing
   switch (message) {
   case WM_SIZING:
      switch (wParam) {
      case WMSZ_BOTTOMLEFT:
      case WMSZ_BOTTOMRIGHT:
      case WMSZ_TOPLEFT:
      case WMSZ_TOPRIGHT:
      case WMSZ_LEFT:
      case WMSZ_RIGHT:
      case WMSZ_TOP:
      case WMSZ_BOTTOM:
         resize_font_dialog();
         return true;

      default:
         break;
      }
      break;
   }  //lint !e744
   return false ;
}

//*************************************************************************************
//  DDM 01/29/17 - These minima are not actually working;
//  Perhaps this is due to Windowblinds ??
//  Yes; this works fine on standard Windows 7
//*************************************************************************************
//  Claude 08/12/26
//  WM_GETMINMAXINFO — this is where you lock the width and bound the height.
//  Setting ptMinTrackSize.x == ptMaxTrackSize.x (both equal to the dialog's current
//  width) is enough to make the left/right borders un-draggable — you don't need
//  WM_SIZING for that. Height min comes from your own "smallest useful layout"
//  calculation; height max comes from SystemParametersInfo(SPI_GETWORKAREA, ...) 
//  so the dialog can't be dragged off the bottom of the screen.
//*************************************************************************************
static bool do_getminmaxinfo(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   LPMINMAXINFO lpTemp = (LPMINMAXINFO) lParam;
   POINT        ptTemp;
   // syslog("set minimum to %ux%u\n", cxClient, cyClient);
   
   //  Claude 08/14/26 - cxClient is a CLIENT-area size; ptMinTrackSize/
   //  ptMaxTrackSize must be WINDOW sizes (border+caption included), so add
   //  the frame delta captured at init. Width is pinned min==max to lock
   //  horizontal resize; that pin must land on the real current window
   //  width or Windows will fight the live window size every time this
   //  fires and can degenerate the rect mid-drag.
   //
   //  Height floor comes from min_application_window_height.
   //  min_application_window_height is computed once in do_init_dialog 
   //  and never changes, which is what a track-size floor needs to be.
   
   //  set minimum dimensions
   ptTemp.x = (LONG) cxClient + dx_frame ;
   ptTemp.y = (LONG) min_application_window_height ;
   lpTemp->ptMinTrackSize = ptTemp;
   // uint dxmin = ptTemp.x ;
   // uint dymin = ptTemp.y ;
   //  set maximum dimensions
   ptTemp.x = (LONG) cxClient + dx_frame ;
   ptTemp.y = get_screen_height() ;
   lpTemp->ptMaxTrackSize = ptTemp;
   // lpTemp->ptMaxSize = ptTemp;
   // syslog("gmmi: dxmin: %u, dxmax: %u, dymin: %u, dymax: %ld\n", dxmin, ptTemp.x, dymin, (long) ptTemp.y);
   return true ;
}

//***********************************************************************
static LRESULT CALLBACK TermProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static uint line_num = 0 ;

   //***************************************************
   //  debug: log all windows messages
   //***************************************************
   if (dbg_flags & DBG_WINMSGS) {
      switch (message) {
      //  list messages to be ignored
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORDLG:
      case WM_MOUSEMOVE:
      case 295:  //  WM_CHANGEUISTATE
      case WM_NCMOUSEMOVE:
      case WM_NCMOUSELEAVE:
      case WM_NCHITTEST:
      case WM_SETCURSOR:
      case WM_ERASEBKGND:
      case WM_TIMER:
      case WM_NOTIFY:
      case WM_COMMAND:  //  prints its own msgs below
         break;
      default:
         syslog("TOP [%s]\n", lookup_winmsg_name(message)) ;
         break;
      }
   }

   switch(message) {
   case WM_INITDIALOG:
      do_init_dialog(hwnd) ;
      return TRUE;

   case WM_NOTIFY:
      return term_notify(hwnd, lParam) ;

   case WM_EXITSIZEMOVE:
      {
      RECT rect ;
      GetWindowRect(hwnd, &rect);
      window_top = rect.top ;
      window_left = rect.left ;
      save_cfg_file();
      }
      break ;
   
   case WM_GETMINMAXINFO:
      do_getminmaxinfo(hwnd, message, wParam, lParam) ;
      return FALSE;

   case WM_SIZE:
      do_size(hwnd, message, wParam, lParam) ;
      return TRUE ;

   //  this is only required if width is fixed in dialog
   case WM_WINDOWPOSCHANGING:
      {
      WINDOWPOS* pos = (WINDOWPOS*)lParam;
      if (!(pos->flags & SWP_NOSIZE))
         pos->cx = cxClient;   // hardcoded, no private_data needed
      break;
      }      
      return TRUE ;

   //***********************************************************************************************
   //  04/16/14 - unfortunately, I cannot use WM_SIZE, nor any other message, to draw my graphics,
   //  because some other message occurs later and over-writes my work...
   //***********************************************************************************************
   case WM_COMMAND:
      {  //  create local context
      DWORD cmd = HIWORD (wParam) ;
      DWORD target = LOWORD(wParam) ;

      switch (cmd) {
      case FVIRTKEY:  //  keyboard accelerators: WARNING: same code as CBN_SELCHANGE !!
         //  fall through to BM_CLICKED, which uses same targets
      case BN_CLICKED:
         switch(target) {
         
         case IDB_ADD_LINE:
            line_num++ ;
            put_color_msg((line_num % 8), "Line number %u", line_num) ;
            break ;
            
         case IDB_CLOSE:
            PostMessageA(hwnd, WM_CLOSE, 0, 0);
            break;
         } //lint !e744  switch target
         return true;
      } //lint !e744  switch cmd
      break;
      }  //lint !e438 !e10  end local context

   //********************************************************************
   //  application shutdown handlers
   //********************************************************************
   case WM_CLOSE:
      DestroyWindow(hwnd);
      break;

   case WM_DESTROY:
      PostQuitMessage(0);
      break;

   // default:
   //    return false;
   }  //lint !e744  switch(message) 

   return false;
}

//***********************************************************************
//lint -esym(1784, WinMain)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
   {
   g_hinst = hInstance;
   load_exec_filename() ;     //  get our executable name

   HWND hwnd = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, (DLGPROC) TermProc) ;
   if (hwnd == NULL) {
      syslog("CreateDialog: %s\n", get_system_message()) ;
      return 0;
   }

   MSG Msg;
   while(GetMessage(&Msg, NULL,0,0)) {
      if(!IsDialogMessage(hwnd, &Msg)) {
          TranslateMessage(&Msg);
          DispatchMessage(&Msg);
      }
   }

   return (int) Msg.wParam ;
}  //lint !e715

