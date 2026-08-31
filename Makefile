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
CFLAGS=-Wall -O -g -Weffc++ -c 
LFLAGS=
else
CFLAGS=-Wall -O2 -Weffc++ -c 
LFLAGS=-s -mwindows 
endif
CFLAGS += -Wno-write-strings
#CFLAGS += -Wno-stringop-truncation

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

# link library files
LiFLAGS = -Ider_libs
CFLAGS += -Ider_libs

CSRC=term_demo.cpp config.cpp

CSRC+=der_libs/common_funcs.cpp \
der_libs/common_win.cpp \
der_libs/vlistview.cpp \
der_libs/cterminal.cpp \
der_libs/terminal.cpp \
der_libs/statbar.cpp \
der_libs/winmsgs.cpp 

OBJS = $(CSRC:.cpp=.o) rc.o

BASE=terminal
BIN=$(BASE).exe

LIBS=
#LIBS=-lcomctl32 -lhtmlhelp 
#LIBS=-lgdi32 -lcomctl32 -lhtmlhelp -lolepro32 -lole32 -luuid

# Automatically parse the latest version block
VERSION := $(shell grep -oE '\[[0-9]+\.[0-9]+\]' CHANGELOG.md | head -n 1 | tr -d '[]')
DIST_ZIP := $(BASE)V$(VERSION).zip

# Force these action-only targets to always run
.PHONY: dist release update

#************************************************************
%.o: %.cpp
	$(TOOLS)\$(GNAME) $(CFLAGS) $< -o $@

#************************************************************
all: $(BIN)

clean:
	rm -vf $(BIN) $(OBJS) *.zip *.bak *~

dist:
	rm -f *.zip
	zip $(DIST_ZIP) *.exe 

# Your new automated release workflow
release: dist
	@cmd /C "@echo Preparing GitHub release for v$(VERSION)..."
	sed -n '/## \['$(VERSION)'\]/,/## \[/p' CHANGELOG.md | sed '$$d' > temp_notes.md
	gh release create v$(VERSION) ./$(DIST_ZIP) ./CHANGELOG.md --notes-file temp_notes.md
	rm temp_notes.md
	@cmd /C "@echo Release v$(VERSION) successfully uploaded to GitHub!"
	
# Your new update-in-place pipeline
update: dist
	@cmd /C "@echo Updating assets for existing release v$(VERSION)..."
	@# Uploads and overwrites the .zip file and CHANGELOG.md on GitHub
	gh release upload v$(VERSION) ./$(DIST_ZIP) ./CHANGELOG.md --clobber
	@cmd /C "@echo Release v$(VERSION) assets successfully updated on GitHub!"

wc:
	wc -l $(CSRC) *.rc

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:\llvm\bin\clang-tidy.exe $(CSRC)"

depend:
	makedepend $(CFLAGS) $(CSRC)

#************************************************************
$(BIN): $(OBJS)
	$(TOOLS)\$(GNAME) $(LFLAGS) $(OBJS) -o $@ $(LIBS)

rc.o: $(BASE).rc resource.h
	$(TOOLS)\$(WRNAME) $< -O coff -o $@

# DO NOT DELETE

term_demo.o: resource.h der_libs/common.h der_libs/commonw.h term_demo.h
term_demo.o: der_libs/statbar.h der_libs/cterminal.h der_libs/vlistview.h
term_demo.o: der_libs/terminal.h der_libs/winmsgs.h
config.o: der_libs/common.h term_demo.h
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
