
NAME       = clang
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GreenPad_clang.exe
INTDIR = obj/$(NAME)

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

LIBS = -nostdlib -Wl,-e_entryp@0 \
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
 -Wl,-dynamicbase,-nxcompat,--no-seh,--enable-auto-import \
 -Wl,--tsaware,--large-address-aware,-s -s \
 $(MINGWL)

WARNINGS = \
   -Wall \
   -Wextra \
   -Wno-pragma-pack \
   -Wno-expansion-to-defined \
   -Wno-macro-redefined \
   -Wno-unused-parameter \
   -Wno-unused-value \
   -Wno-missing-field-initializers \
   -Wno-implicit-fallthrough \
   -Wno-register \
   -Wno-parentheses \
   -Wno-ignored-attributes \
   -Wuninitialized \
   -Wtype-limits \
   -Winit-self \
   -Wnull-dereference \
   -Wnonnull \
   -Wno-unknown-pragmas


DEFINES = -D_WIN32_WINNT=0x400 \
    -D_UNICODE -DUNICODE \
    -UDEBUG -U_DEBUG \
    -DSUPERTINY \
    -DTARGET_VER=310 \
    -DUSEGLOBALIME \
    -DUSE_ORIGINAL_MEMMAN 


PRE:
	-@if not exist release   mkdir release
	-@if not exist obj       mkdir obj
	-@if not exist $(INTDIR) mkdir $(INTDIR)
###############################################################################

RES = $(INTDIR)/gp_rsrc.o

VPATH    = editwing:kilib
# -DSUPERTINY  -flto -fuse-linker-plugin -Wno-narrowing  -fwhole-program  -fno-tree-loop-distribute-patterns

#CXXFLAGS = \
#	-nostdlib -m32 -c -O1 \
#	-march=i386 \
#	-mtune=generic \
#	-mno-stack-arg-probe \
#	-momit-leaf-frame-pointer \
#	-fmerge-all-constants \
#	-fomit-frame-pointer \
#	-fno-stack-check \
#	-fno-stack-protector \
#	-fno-threadsafe-statics \
#	-fno-access-control \
#	-fno-use-cxa-atexit \
#	-fno-exceptions \
#	-fno-dwarf2-cfi-asm \
#	-fno-asynchronous-unwind-tables \
#	-fno-rtti \
#	$(WARNINGS) $(DEFINES) $(MINGWI) \
#	-idirafter kilib


CXXFLAGS = \
	-nostdlib -m32 -c -O1 \
	-march=i386 \
	-mtune=generic \
	-fno-access-control \
	-fno-use-cxa-atexit \
	-fno-exceptions \
	-fno-dwarf2-cfi-asm \
	-fno-rtti \
	$(WARNINGS) $(DEFINES) $(MINGWI) \
	-idirafter kilib

LOPT     = -m32 -mwindows

ifneq ($(NOCHARSET),1)
#CXXFLAGS += -finput-charset=932 -fexec-charset=932
endif

$(TARGET) : $(OBJS) $(RES)
	clang $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)
#	strip -s $(TARGET)

$(INTDIR)/%.o: rsrc/%.rc
	windres -DTARGET_VER=310 -Fpe-i386 -l=0x411 -I rsrc $< -O coff -o$@

$(INTDIR)/%.o: %.cpp
	clang $(CXXFLAGS) -o$@ $<
