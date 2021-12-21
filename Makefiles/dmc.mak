NAME       = dmc
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

RC  = rsrc\gp_rsrc.rc
RES = $(INTDIR)\gp_rsrc.res
#  -DSUPERTINY -DUSEGLOBALIME
COPT = -Bj -j0 -Ab -w2 -w7 -o -c -DNO_MLANG -DSTRICT -D_UNICODE -DUNICODE
LOPT = -Bj -mn -WA -L/su:Windows:4.0/exet:NT/onerror:noexe
ROPT = -j -32 -l0411

$(TARGET) : SETI $(OBJS) $(RES)
	dmc $(LOPT) -o$(TARGET) $(RES) $(OBJS) $(LIBS)
	-@del GreenPad_dmc.map
	brc32 $(RES) $(TARGET)
SETI:
	@set INCLUDE=kilib;$(INCLUDE)

OBJ\dmc\gp_rsrc.res  : rsrc\gp_rsrc.rc         ; brcc32 -m -c932 -l0x411 -fo$@ $**
OBJ\dmc\thread.obj   : kilib\thread.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\log.obj      : kilib\log.cpp           ; dmc $(COPT) -o$@ $**
OBJ\dmc\winutil.obj  : kilib\winutil.cpp       ; dmc $(COPT) -o$@ $**
OBJ\dmc\textfile.obj : kilib\textfile.cpp      ; dmc $(COPT) -o$@ $**
OBJ\dmc\path.obj     : kilib\path.cpp          ; dmc $(COPT) -o$@ $**
OBJ\dmc\cmdarg.obj   : kilib\cmdarg.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\file.obj     : kilib\file.cpp          ; dmc $(COPT) -o$@ $**
OBJ\dmc\find.obj     : kilib\find.cpp          ; dmc $(COPT) -o$@ $**
OBJ\dmc\ctrl.obj     : kilib\ctrl.cpp          ; dmc $(COPT) -o$@ $**
OBJ\dmc\registry.obj : kilib\registry.cpp      ; dmc $(COPT) -o$@ $**
OBJ\dmc\window.obj   : kilib\window.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\string.obj   : kilib\string.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\memory.obj   : kilib\memory.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\app.obj      : kilib\app.cpp           ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_cursor.obj : editwing\ip_cursor.cpp ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_scroll.obj : editwing\ip_scroll.cpp ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_wrap.obj   : editwing\ip_wrap.cpp   ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_draw.obj   : editwing\ip_draw.cpp   ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_ctrl1.obj  : editwing\ip_ctrl1.cpp  ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_text.obj   : editwing\ip_text.cpp   ; dmc $(COPT) -o$@ $**
OBJ\dmc\ip_parse.obj  : editwing\ip_parse.cpp  ; dmc $(COPT) -o$@ $**
OBJ\dmc\GpMain.obj        : GpMain.cpp         ; dmc $(COPT) -o$@ $**
OBJ\dmc\OpenSaveDlg.obj   : OpenSaveDlg.cpp    ; dmc $(COPT) -o$@ $**
OBJ\dmc\Search.obj        : Search.cpp         ; dmc $(COPT) -o$@ $**
OBJ\dmc\RSearch.obj       : RSearch.cpp        ; dmc $(COPT) -o$@ $**
OBJ\dmc\ConfigManager.obj : ConfigManager.cpp  ; dmc $(COPT) -o$@ $**
