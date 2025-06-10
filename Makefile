USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = NO

ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
else
TOOLS=d:\tdm32\bin
endif

#*****************************************************************************
# notes on compiler quirks, using MinGW/G++ V4.3.3
# if I build with -O3, I get following warnings from g++ :
#   wfuncs.cpp: In function 'int light_a_flare(HWND__*)':
#   wfuncs.cpp:338: warning: array subscript is above array bounds
# where light_a_flare() starts at line 779 !!
# If I build with -O2, I get no such warnings.
# In either case, PcLint V9 is giving no warnings on this code.
#*****************************************************************************

ifeq ($(USE_DEBUG),YES)
CFLAGS=-Wall -O -g -mwindows 
LFLAGS=
else
CFLAGS=-Wall -O2 -mwindows 
LFLAGS=-s
endif
CFLAGS += -Wno-write-strings
#CFLAGS += -Wno-stringop-truncation

# link library files
LiFLAGS = -Ider_libs
CFLAGS += -Ider_libs
CSRC=der_libs/common_funcs.cpp \
der_libs/common_win.cpp \
der_libs/vlistview.cpp \
der_libs/cterminal.cpp \
der_libs/terminal.cpp \
der_libs/statbar.cpp \
der_libs/winmsgs.cpp 

CSRC+=term_demo.cpp

#  clang-tidy options
CHFLAGS = -header-filter=.*
CHTAIL = -- -Ider_libs
ifeq ($(USE_64BIT),YES)
CHTAIL += -DUSE_64BIT
endif
ifeq ($(USE_UNICODE),YES)
CHTAIL += -DUNICODE -D_UNICODE
endif

OBJS = $(CSRC:.cpp=.o) rc.o

BASE=terminal
BIN=$(BASE).exe

LIBS=
#LIBS=-lcomctl32 -lhtmlhelp 
#LIBS=-lgdi32 -lcomctl32 -lhtmlhelp -lolepro32 -lole32 -luuid

#************************************************************
%.o: %.cpp
	$(TOOLS)\g++ $(CFLAGS) -Weffc++ -c $< -o $@

#************************************************************
all: $(BIN)

clean:
	rm -vf $(BIN) $(OBJS) *.zip *.bak *~

dist:
	rm -f $(BASE).zip
	zip $(BASE).zip *.exe 

wc:
	wc -l *.cpp *.rc

check:
	cmd /C "d:\clang\bin\clang-tidy.exe $(CHFLAGS) $(CSRC) $(CHTAIL)"

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CSRC)"

depend:
	makedepend $(CFLAGS) $(CSRC)

#************************************************************
$(BASE).exe: $(OBJS)
	$(TOOLS)\g++ $(CFLAGS) $(LFLAGS) $(OBJS) -o $@ $(LIBS)

rc.o: $(BASE).rc 
	$(TOOLS)\windres $< -O coff -o $@

# DO NOT DELETE

der_libs/common_funcs.o: der_libs/common.h
der_libs/common_win.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/common.h der_libs/commonw.h
der_libs/vlistview.o: der_libs/vlistview.h
der_libs/cterminal.o: der_libs/common.h der_libs/commonw.h
der_libs/cterminal.o: der_libs/cterminal.h der_libs/vlistview.h
der_libs/terminal.o: der_libs/common.h der_libs/commonw.h
der_libs/terminal.o: der_libs/cterminal.h der_libs/vlistview.h
der_libs/terminal.o: der_libs/terminal.h der_libs/winmsgs.h
der_libs/statbar.o: der_libs/common.h der_libs/commonw.h der_libs/statbar.h
term_demo.o: resource.h der_libs/common.h der_libs/commonw.h term_demo.h
term_demo.o: der_libs/statbar.h der_libs/cterminal.h der_libs/vlistview.h
term_demo.o: der_libs/terminal.h der_libs/winmsgs.h
