//****************************************************
//  wizard.h - Declarations for Wizard's Castle
//  
//  Written by:   Daniel D. Miller
//****************************************************

extern HINSTANCE g_hinst ; //  top-level hinstance

//****************************************************************************
//  debug: message-reporting data 
//****************************************************************************

#define  DBG_VERBOSE       0x01
#define  DBG_WINMSGS       0x02
#define  DBG_RX_DEBUG      0x04
#define  DBG_CTASK_TRACE   0x08
#define  DBG_SMTP_RECV     0x10
#define  DBG_POLLING       0x20
#define  DBG_ETHERNET      0x40

extern uint dbg_flags ;
//**************************************************************
//  function prototypes
//**************************************************************

// extern uint cxClient, cyClient ;

//  winwiz.cpp
void status_message(char *msgstr);
void status_message(uint idx, char *msgstr);

