
NAME       = gccdb
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GreenPad_$(NAME).exe
INTDIR = obj\$(NAME)

all: PRE $(TARGET)

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

# -DSUPERTINY  -fpermissive -flto -fuse-linker-plugin
#,--disable-reloc-section,--disable-runtime-pseudo-reloc
LIBS = \
 -L. -lunicows \
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
 -Wl,-dynamicbase,-nxcompat,--no-seh,--enable-auto-import,--disable-stdcall-fixup \
 -Wl,--disable-reloc-section,--disable-runtime-pseudo-reloc \
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
# -DSUPERTINY  -flto -fuse-linker-plugin -Wno-narrowing  -fwhole-program
CXXFLAGS = -m32 -g -c -Og \
 -march=i386 -mpreferred-stack-boundary=2 \
 -fstack-protector-all -fstack-protector-strong -fstack-check \
 -Wall -Wno-parentheses -Wno-unknown-pragmas -Wstack-usage=4096 -Warray-bounds=2 \
 -idirafter kilib \
 -D_UNICODE -DUNICODE -UDEBUG -U_DEBUG -DUSEGLOBALIME -DUNICOWS -DTARGET_VER=350
LOPT     = -m32

ifneq ($(NOCHARSET),1)
CXXFLAGS += -finput-charset=cp932 -fexec-charset=cp932
endif

$(TARGET) : $(OBJS) $(RES)
	g++ $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)

$(INTDIR)/%.o: rsrc/%.rc
	windres -Fpe-i386 -l=0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	g++ $(CXXFLAGS) -o$@ $<