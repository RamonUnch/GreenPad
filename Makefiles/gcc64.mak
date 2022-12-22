
NAME       = gcc64
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
 -lkernel32 -nostdlib -Wl,-eentryp -flto -fuse-linker-plugin -flto-partition=none \
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
 -Wl,--tsaware,-s -s\

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
# -DSUPERTINY  -flto -fuse-linker-plugin
CXXFLAGS = -nostdlib -m64 -c -Os -mno-stack-arg-probe -momit-leaf-frame-pointer \
 -flto -fuse-linker-plugin -flto-partition=none \
 -fomit-frame-pointer -fno-stack-check -fno-stack-protector -fno-threadsafe-statics -fno-use-cxa-get-exception-ptr \
 -fno-access-control -fno-enforce-eh-specs -fno-nonansi-builtins -fnothrow-opt -fno-optional-diags -fno-use-cxa-atexit \
 -fno-exceptions -fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables -fno-extern-tls-init -fno-rtti \
 -Wno-narrowing -Wno-int-to-pointer-cast -Wstack-usage=4096 \
 -idirafter kilib -D_UNICODE -DUNICODE -UDEBUG -U_DEBUG -DUSEGLOBALIME -DSUPERTINY -DUSE_ORIGINAL_MEMMAN
LOPT     = -m64 -mwindows

ifneq ($(NOCHARSET),1)
CXXFLAGS += -finput-charset=cp932 -fexec-charset=cp932
endif

$(TARGET) : $(OBJS) $(RES)
	g++ $(LOPT) -o$(TARGET) $(OBJS) $(RES) $(LIBS)
#	strip -s $(TARGET)
$(INTDIR)/%.o: rsrc/%.rc
	windres -Fpe-x86-64 -l=0x411 -I rsrc $< -O coff -o$@
$(INTDIR)/%.o: %.cpp
	g++ $(CXXFLAGS) -o$@ $<
