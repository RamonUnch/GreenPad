NAME       = wccAXP
OBJ_SUFFIX = obj

###############################################################################
TARGET = release\GreenPad_$(NAME).exe
INTDIR = obj\$(NAME)

all: PRE $(TARGET)

OBJS = \
 $(INTDIR)\thread.$(OBJ_SUFFIX)      \
 $(INTDIR)\log.$(OBJ_SUFFIX)         \
 $(INTDIR)\winutil.$(OBJ_SUFFIX)     \
 $(INTDIR)\textfile.$(OBJ_SUFFIX)    \
 $(INTDIR)\path.$(OBJ_SUFFIX)        \
 $(INTDIR)\cmdarg.$(OBJ_SUFFIX)      \
 $(INTDIR)\file.$(OBJ_SUFFIX)        \
 $(INTDIR)\find.$(OBJ_SUFFIX)        \
 $(INTDIR)\ctrl.$(OBJ_SUFFIX)        \
 $(INTDIR)\registry.$(OBJ_SUFFIX)    \
 $(INTDIR)\window.$(OBJ_SUFFIX)      \
 $(INTDIR)\string.$(OBJ_SUFFIX)      \
 $(INTDIR)\memory.$(OBJ_SUFFIX)      \
 $(INTDIR)\app.$(OBJ_SUFFIX)         \
 $(INTDIR)\ip_cursor.$(OBJ_SUFFIX)   \
 $(INTDIR)\ip_scroll.$(OBJ_SUFFIX)   \
 $(INTDIR)\ip_wrap.$(OBJ_SUFFIX)     \
 $(INTDIR)\ip_draw.$(OBJ_SUFFIX)     \
 $(INTDIR)\ip_ctrl1.$(OBJ_SUFFIX)    \
 $(INTDIR)\ip_text.$(OBJ_SUFFIX)     \
 $(INTDIR)\ip_parse.$(OBJ_SUFFIX)    \
 $(INTDIR)\GpMain.$(OBJ_SUFFIX)      \
 $(INTDIR)\OpenSaveDlg.$(OBJ_SUFFIX) \
 $(INTDIR)\Search.$(OBJ_SUFFIX)      \
 $(INTDIR)\RSearch.$(OBJ_SUFFIX)     \
 $(INTDIR)\ConfigManager.$(OBJ_SUFFIX)

LIBS = \
 kernel32.lib \
 user32.lib   \
 gdi32.lib    \
 shell32.lib  \
 advapi32.lib \
 comdlg32.lib \
 comctl32.lib \
 ole32.lib    \
 imm32.lib

PRE:
	-@if not exist release   mkdir release
	-@if not exist obj       mkdir obj
	-@if not exist $(INTDIR) mkdir $(INTDIR)
###############################################################################

RES = $(INTDIR)\gp_rsrc.res
DEF = /DNDEBUG /DUNICODE /D_UNICODE /DNO_IME /DSTDFAX_FPATH /DTARGET_VER=310 /DNO_ASMTHUNK

COPT = $(DEF) -bt=nt /FD /Fd$(INTDIR) /Ikilib /W3 /c /zk0 /xds /otexan /za /s /Wcd=391 /Wcd=014
# /nodefaultlib
LOPT = /release
ROPT = $(DEF) /L 0x411 /I "rsrc" /DTARGET_VER=310

$(TARGET) : $(OBJS) $(RES)
	wclAXP $(LOPT) /OUT:$(TARGET) /SUBSYSTEM:WINDOWS $(OBJS) $(RES) $(LIBS)

{rsrc}.rc{$(INTDIR)}.res:
	rc $(ROPT) /Fo$@ $**

{.}.cpp{$(INTDIR)}.obj:
	wclAXP $(COPT) /Fo$@ $**
{kilib}.cpp{$(INTDIR)}.obj:
	wclAXP $(COPT) /Fo$@ $**
{editwing}.cpp{$(INTDIR)}.obj:
	wclAXP $(COPT) /Fo$@ $**
