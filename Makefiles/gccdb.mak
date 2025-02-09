
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
 -Wl,-nxcompat,--no-seh,--enable-auto-import \
 -Wl,--disable-reloc-section \
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
#  -fstack-protector-all -fstack-protector-strong -fstack-check

WARNINGS = \
   -Wall \
   -Wextra \
   -Wno-unused-parameter \
   -Wno-missing-field-initializers \
   -Wno-cast-function-type \
   -Wno-implicit-fallthrough \
   -Wno-register \
   -Wno-parentheses \
   -Wformat-overflow=2 \
   -Wuninitialized \
   -Wtype-limits \
   -Winit-self \
   -Wnull-dereference \
   -Wnonnull \
   -Wno-unknown-pragmas \
   -Wstack-usage=4000 \
   -Wsuggest-override

# -Wanalyzer-too-complex -Wanalyzer-possible-null-argument -Wanalyzer-use-of-uninitialized-value
#ANA = -fanalyzer -Wno-analyzer-use-of-uninitialized-value -Wno-analyzer-possible-null-argument -Wno-analyzer-malloc-leak
# -Wno-analyzer-possible-null-argument -Wno-analyzer-use-of-uninitialized-value

CXXFLAGS = -m32 -g -c -Og -gdwarf-2 -fno-inline-functions -fno-inline -fno-stack-protector -fomit-frame-pointer \
 -march=i386 -mpreferred-stack-boundary=2 -mno-stack-arg-probe -Warray-bounds=2 \
 -idirafter kilib \
 -D_UNICODE -DUNICODE -UDEBUG -U_DEBUG -DUSEGLOBALIME -DUNICOWS -DTARGET_VER=350 \
 $(ANA) $(WARNINGS)

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
