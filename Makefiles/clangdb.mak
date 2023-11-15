
NAME       = clangdb
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GreenPad_$(NAME).exe
INTDIR = obj\$(NAME)

all: PRE $(TARGET)
MINGWI=-ID:\Straw\gcc12\i686-w64-mingw32\include
MINGWL=-LD:\Straw\gcc12\i686-w64-mingw32\lib

OBJS = \
 $(INTDIR)/thread.$(OBJ_SUFFIX)       \
 $(INTDIR)/log.$(OBJ_SUFFIX)          \
 $(INTDIR)/winutil.$(OBJ_SUFFIX)      \
 $(INTDIR)/textfile.$(OBJ_SUFFIX)     \
 $(INTDIR)/path.$(OBJ_SUFFIX)         \
 $(INTDIR)/cmdarg.$(OBJ_SUFFIX)       \
 $(INTDIR)/file.$(OBJ_SUFFIX)         \
 $(INTDIR)/find.$(OBJ_SUFFIX)         \
 $(INTDIR)/ctrl.$(OBJ_SUFFIX)         \
 $(INTDIR)/registry.$(OBJ_SUFFIX)     \
 $(INTDIR)/window.$(OBJ_SUFFIX)       \
 $(INTDIR)/string.$(OBJ_SUFFIX)       \
 $(INTDIR)/memory.$(OBJ_SUFFIX)       \
 $(INTDIR)/ip_cursor.$(OBJ_SUFFIX)    \
 $(INTDIR)/ip_scroll.$(OBJ_SUFFIX)    \
 $(INTDIR)/ip_wrap.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_draw.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_ctrl1.$(OBJ_SUFFIX)     \
 $(INTDIR)/ip_text.$(OBJ_SUFFIX)      \
 $(INTDIR)/ip_parse.$(OBJ_SUFFIX)     \
 $(INTDIR)/GpMain.$(OBJ_SUFFIX)       \
 $(INTDIR)/OpenSaveDlg.$(OBJ_SUFFIX)  \
 $(INTDIR)/Search.$(OBJ_SUFFIX)       \
 $(INTDIR)/RSearch.$(OBJ_SUFFIX)      \
 $(INTDIR)/ConfigManager.$(OBJ_SUFFIX) \
 $(INTDIR)/app.$(OBJ_SUFFIX)

LIBS = -static \
 -L. \
 -lkernel32 \
 -luser32   \
 -lgdi32    \
 -lshell32  \
 -ladvapi32 \
 -lcomdlg32 \
 -lcomctl32 \
 -lole32    \
 -luuid     \
 -limm32    \
 $(MINGWL)  \
 -Wl,-dynamicbase,-nxcompat,--no-seh \
 -Wl,--tsaware,--large-address-aware

# -Wl,--print-map \
# -static-libstdc++ \
#  -static-libgcc
PRE:
	-@if not exist release   mkdir release
	-@if not exist obj       mkdir obj
	-@if not exist $(INTDIR) mkdir $(INTDIR)
###############################################################################

RES = $(INTDIR)/gp_rsrc.o

VPATH    = editwing:kilib

CXXFLAGS = -m32 -g -c -Og  \
 -march=i386 -mno-stack-arg-probe $(MINGWI) \
 -Wall -Wno-parentheses -Wno-unknown-pragmas \
 -idirafter kilib -DNOWINBASEINTERLOCK \
 -D_UNICODE -DUNICODE -UDEBUG -U_DEBUG -DUSEGLOBALIME -DTARGET_VER=310
LOPT     = -m32

ifneq ($(NOCHARSET),1)
#CXXFLAGS += -finput-charset=cp932 -fexec-charset=cp932
endif

$(TARGET) : $(OBJS) $(RES)
	clang $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)

$(INTDIR)/%.o: rsrc/%.rc
	windres -DTARGET_VER=310 -Fpe-i386 -l=0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	clang $(CXXFLAGS) -o$@ $<
