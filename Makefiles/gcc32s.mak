
NAME       = 32sg
OBJ_SUFFIX = o

###############################################################################
TARGET = release/GPad$(NAME).exe
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

LIBS = -nostdlib -Wl,-e_entryp@0 -flto -fuse-linker-plugin -flto-partition=none \
 -lkernel32 \
 -luser32   \
 -lgdi32    \
 -lshell32  \
 -ladvapi32 \
 -lcomdlg32 \
 -lcomctl32 \
 -luuid     \
 -limm32    \
 -Wl,-dynamicbase,-nxcompat,--no-seh,--enable-auto-import,--disable-stdcall-fixup \
 -Wl,--tsaware,--large-address-aware,-s -s \
 -Wl,--major-subsystem-version=3,--minor-subsystem-version=10

PRE:
	-@if not exist release   mkdir release
	-@if not exist obj       mkdir obj
	-@if not exist $(INTDIR) mkdir $(INTDIR)
###############################################################################

RES = $(INTDIR)/gp_rsrc.o

VPATH    = editwing:kilib

CXXFLAGS = -nostdlib  -m32 -c -Os  -pie -fpie \
 -march=i386 -mtune=generic -mno-stack-arg-probe -momit-leaf-frame-pointer -mpreferred-stack-boundary=2 \
 -flto -fuse-linker-plugin  -flto-partition=none \
 -fomit-frame-pointer -fno-stack-check -fno-stack-protector -fno-threadsafe-statics -fno-use-cxa-get-exception-ptr \
 -fno-access-control -fno-enforce-eh-specs -fno-nonansi-builtins -fnothrow-opt -fno-optional-diags -fno-use-cxa-atexit \
 -fno-exceptions -fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables -fno-extern-tls-init -fno-rtti \
 -Wall -Wno-parentheses -Wformat-overflow=2 -Wuninitialized -Winit-self -Wnull-dereference -Wnonnull -Wno-unknown-pragmas -Wstack-usage=4096 -D_WIN32_WINNT=0x400 \
 -idirafter kilib \
 -D_MBCS -DNO_ASMTHUNK -DWIN32S -UDEBUG -U_DEBUG -DUSEGLOBALIME -DSUPERTINY -DTARGET_VER=310 -DUSE_ORIGINAL_MEMMAN -DSHORT_TABLEWIDTH
LOPT     = -m32 -mwindows -pie

ifneq ($(NOCHARSET),1)
CXXFLAGS += -finput-charset=cp932 -fexec-charset=cp1252
endif

$(TARGET) : $(OBJS) $(RES)
	g++ $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)
#	strip -s $(TARGET)
$(INTDIR)/%.o: rsrc/%.rc
	windres -DTARGET_VER=310 -Fpe-i386 -l=0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	g++ $(CXXFLAGS) -o$@ $<
