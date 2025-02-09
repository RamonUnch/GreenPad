
NAME       = gccO2
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GreenPad_gccO2.exe
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

LIBS = -nostdlib -Wl,-e_entryp@0 -flto -fuse-linker-plugin -flto-partition=none -fwhole-program \
 -L. -lunicows -fno-ident \
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
 -Wl,--tsaware,--large-address-aware,-s -s\

WARNINGS = \
   -Wall \
   -Wno-register \
   -Wno-parentheses \
   -Wformat-overflow=2 \
   -Wuninitialized \
   -Winit-self \
   -Wnull-dereference \
   -Wnonnull \
   -Wno-unknown-pragmas \
   -Wstack-usage=4096 \
   -Wsuggest-override \
   -Wno-alloc-size-larger-than \


DEFINES = -DSTDFAX_FPATH \
    -D_WIN32_WINNT=0x400 \
    -D_UNICODE -DUNICODE -DUNICOWS \
    -UDEBUG -U_DEBUG \
    -DUSEGLOBALIME \
    -DSUPERTINY \
    -DTARGET_VER=350 \
    -DUSE_ORIGINAL_MEMMAN \


PRE:
	-@if not exist release   mkdir release
	-@if not exist obj       mkdir obj
	-@if not exist $(INTDIR) mkdir $(INTDIR)
###############################################################################

RES = $(INTDIR)/gp_rsrc.o

VPATH    = editwing:kilib
# -DSUPERTINY  -flto -fuse-linker-plugin -Wno-narrowing  -fwhole-program  -fno-tree-loop-distribute-patterns

CXXFLAGS = \
	-nostdlib -m32 -c -O2 \
	-march=i386 \
	-mtune=generic \
	-mno-stack-arg-probe \
	-momit-leaf-frame-pointer \
	-mpreferred-stack-boundary=2 \
	-flto -fuse-linker-plugin -fwhole-program  \
	-fmerge-all-constants \
	-fallow-store-data-races \
	-fno-tree-loop-distribute-patterns \
	-fomit-frame-pointer \
	-fno-stack-check \
	-fno-stack-protector \
	-fno-threadsafe-statics \
	-fno-use-cxa-get-exception-ptr \
	-fno-access-control \
	-fno-enforce-eh-specs \
	-fno-nonansi-builtins \
	-fnothrow-opt \
	-fno-optional-diags \
	-fno-use-cxa-atexit \
	-fno-exceptions \
	-fno-dwarf2-cfi-asm \
	-fno-asynchronous-unwind-tables \
	-fno-extern-tls-init \
	-fno-rtti \
	-fno-ident \
	$(WARNINGS) $(DEFINES) \
	-idirafter kilib

LOPT     = -m32 -mwindows

ifneq ($(NOCHARSET),1)
CXXFLAGS += -finput-charset=cp932 -fexec-charset=cp932
endif

$(TARGET) : $(OBJS) $(RES)
	g++ $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS) gcckinka.lk
#	strip -s $(TARGET)
$(INTDIR)/%.o: rsrc/%.rc
	windres -DTARGET_VER=350 -Fpe-i386 -l=0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	g++ $(CXXFLAGS) -o$@ $<
