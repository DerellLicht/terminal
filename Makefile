USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = NO
USE_CLANG = NO
# sadly, cygwin mingw does not support gdiplus...
USE_CYGWIN = NO

# the legacy version of qualify.cpp, does not depend upon c++ string class
USE_LEGACY = NO

include der_libs\tool_select.mak

ifeq ($(USE_DEBUG),YES)
CFLAGS=-Wall -O -g -mwindows 
LFLAGS=
else
CFLAGS=-Wall -O2 -mwindows 
LFLAGS=-s
endif
CFLAGS += -Wno-write-strings
#CFLAGS += -Wno-stringop-truncation

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

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

LINTFILES=lintdefs.cpp lintdefs.ref.h 

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

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:\llvm\bin\clang-tidy.exe $(CSRC)"

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) $(LINTFILES) $(CSRC)"

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
