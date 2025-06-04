//****************************************************************************
//  Copyright (c) 1985-2025  Daniel D Miller
//  Demo program for terminal class
//
//  Written by:  Dan Miller
//****************************************************************************

static const char *Version = "Terminal program, Version 1.00" ;

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

//lint -esym(714, dbg_flags)
//lint -esym(759, dbg_flags)
//lint -esym(765, dbg_flags)
uint dbg_flags = 0
   // | DBG_WINMSGS
   ;

static uint cxClient = 0 ;
static uint cyClient = 0 ;

// static HMENU hMainMenu = NULL ;

static CStatusBar *MainStatusBar = NULL;
// static HWND hToolTip ;  /* Tooltip handle */

//  user-defined Windows messages
// static const UINT WM_ARE_YOU_ME = (WM_USER + 106) ;

//*****************************************************************
//lint -esym(756, attrib_table_t)
typedef struct attrib_table_s {
   COLORREF fgnd ;
   COLORREF bgnd ;
} attrib_table_t ;

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
//lint -esym(749, TERM_INFO, TERM_QUERY)
//  indices into term_atable[]
enum {
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
   term_atable[TERM_NORMAL].fgnd = GetSysColor(COLOR_WINDOWTEXT) ;
   term_atable[TERM_NORMAL].bgnd = std_bgnd ;

   //  set standard background for other color sets which use it
   term_atable[TERM_PLAYER_HIT].bgnd = std_bgnd ;
   term_atable[TERM_MONSTER_HIT].bgnd = std_bgnd ;
   term_atable[TERM_RUNESTAFF].bgnd = std_bgnd ;
   term_atable[TERM_DEATH].bgnd = std_bgnd ;
   term_atable[TERM_ATMOSPHERE].bgnd = std_bgnd ;
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
   set_term_attr(TERM_NORMAL);
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
   set_term_attr(TERM_NORMAL) ;
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
   set_term_attr(TERM_NORMAL) ;
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
static uint screen_width  = 0 ;
static uint screen_height = 0 ;

static void ww_get_monitor_dimens(HWND hwnd)
{
   HMONITOR currentMonitor;      // Handle to monitor where fullscreen should go
   MONITORINFO mi;               // Info of that monitor
   currentMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
   mi.cbSize = sizeof(MONITORINFO);
   if (GetMonitorInfo(currentMonitor, &mi) != FALSE) {
      screen_width  = mi.rcMonitor.right  - mi.rcMonitor.left ;
      screen_height = mi.rcMonitor.bottom - mi.rcMonitor.top ;
   }
   // curr_dpi = GetScreenDPI() ;
}

//***********************************************************************
static void center_window(void)
{
   ww_get_monitor_dimens(hwndMain);
   
   RECT myRect ;
   GetWindowRect(hwndMain, &myRect) ;
   // GetClientRect(hwnd, &myRect) ;
   uint dialog_width = (myRect.right - myRect.left) ;
   uint dialog_height = (myRect.bottom - myRect.top) ;

   uint x0 = (screen_width  - dialog_width ) / 2 ;
   uint y0 = (screen_height - dialog_height) / 2 ;

   SetWindowPos(hwndMain, HWND_TOP, x0, y0, 0, 0, SWP_NOSIZE) ;
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

   // set_up_working_spaces(hwnd) ; //  do this *before* tooltips !!
   //***************************************************************************
   //  add tooltips and bitmaps
   //***************************************************************************
   // create_and_add_tooltips(hwnd, 150, 100, 10000, main_tooltips);

   // RECT rWindow;
   // unsigned stTop ;
   RECT myRect ;
   // GetWindowRect(hwnd, &myRect) ;
   GetClientRect(hwnd, &myRect) ;
   cxClient = (myRect.right - myRect.left) ;
   cyClient = (myRect.bottom - myRect.top) ;

   center_window() ;
   //  setting main menu, breaks status bar !!
   //  setup_main_menu(hwnd) ;
   
   //****************************************************************
   //  create/configure status bar
   //****************************************************************
   MainStatusBar = new CStatusBar(hwnd) ;
   MainStatusBar->MoveToBottom(cxClient, cyClient) ;
   //  re-position status-bar parts
   {
   int sbparts[3];
   sbparts[0] = (int) (6 * cxClient / 10) ;
   sbparts[1] = (int) (8 * cxClient / 10) ;
   sbparts[2] = -1;
   MainStatusBar->SetParts(3, &sbparts[0]);
   }
   
   //****************************************************************
   //  create/configure terminal
   //****************************************************************
   setup_terminal_window(hwnd, MainStatusBar->height(), IDB_ADD_LINE, IDC_TERMINAL);
   set_local_terminal_colors() ;
   sprintf(msgstr, "terminal size: columns=%u, rows=%u",
      term_get_columns(), term_get_rows());
   status_message(msgstr);
   termout(msgstr);
}

//***********************************************************************
static LRESULT CALLBACK TermProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   static uint line_num = 0 ;

   //***************************************************
   //  debug: log all windows messages
   //***************************************************
   if (dbg_flags & DBG_WINMSGS) {
      switch (iMsg) {
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
         syslog("TOP [%s]\n", lookup_winmsg_name(iMsg)) ;
         break;
      }
   }

   switch(iMsg) {
   case WM_INITDIALOG:
      do_init_dialog(hwnd) ;
      return TRUE;

   case WM_NOTIFY:
      return term_notify(hwnd, lParam) ;

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
   }  //lint !e744  switch(iMsg) 

   return false;
}

//***********************************************************************
//lint -esym(1784, WinMain)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
   {
   g_hinst = hInstance;

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

