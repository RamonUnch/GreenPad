NAME       = vcc
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
PCH = $(INTDIR)\gp.pch
DEF = /D NDEBUG /D UNICODE /D _UNICODE /D USEGLOBALIME

COPT = /nologo $(DEF) /O1isyb1 /GA /GF /FD /Zc:wchar_t /Yu"stdafx.h" /Fp$(PCH) /Fd$(INTDIR) /W3 /MT /c
LOPT = /nologo /manifest:no bufferoverflowU.lib
ROPT = $(DEF) /L 0x411 /I "rsrc"

$(TARGET): PRE $(PCH) $(OBJS) $(RES)
	link $(LOPT) /OUT:$(TARGET) $(OBJS) $(RES) $(LIBS)

{rsrc}.rc{$(INTDIR)}.res:
	rc $(ROPT) /Fo$@ $**

{.}.cpp{$(INTDIR)}.obj:
	cl $(COPT) /Fo$@ $**
{kilib}.cpp{$(INTDIR)}.obj:
	cl $(COPT) /Fo$@ $**
{editwing}.cpp{$(INTDIR)}.obj:
	cl $(COPT) /Fo$@ $**
$(PCH): kilib\stdafx.cpp
	cl $(COPT) /Fo$(INTDIR)\stdafx.obj /Yc"stdafx.h" $**
