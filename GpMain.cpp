
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "GpMain.h"
using namespace ki;
using namespace editwing;

//-------------------------------------------------------------------------
// 新規プロセス起動
//-------------------------------------------------------------------------

void BootNewProcess( const TCHAR* cmd = TEXT("") )
{
	// On Windows NT3.x/Win9x we cannot have the exe name between ""
	// On NT5 it seems ok to use "exe name.exe" "file name"
	// Otherwise we try SHORT/NAME.EXE "file name"
	// I do not know why the heck it is like this.
	bool quotedexe = app().getOSVer() >= 0x0500 && app().isNT();
	String fcmd;
	if( quotedexe ) fcmd = TEXT("\"");
	fcmd += quotedexe? Path(Path::ExeName): Path(Path::ExeName).BeShortStyle();
	if( quotedexe ) fcmd += TEXT("\" ");
	else fcmd += TEXT(" ");
	fcmd += cmd;
//	MessageBox(NULL, fcmd.c_str(), Path(Path::ExeName).c_str(), MB_OK);

#ifdef UNICOWS
	if( app().isNT() )
	{
		PROCESS_INFORMATION psi;
		STARTUPINFOW        stiw;
		::GetStartupInfoW( &stiw );
		const wchar_t *p = fcmd.ConvToWChar();
		if( !p ) return;
		if( ::CreateProcessW( NULL, const_cast<wchar_t*>(p),
				NULL, NULL, 0, NORMAL_PRIORITY_CLASS, NULL, NULL,
				&stiw, &psi ) )
		{
			::CloseHandle( psi.hThread );
			::CloseHandle( psi.hProcess );
		}
		fcmd.FreeWCMem(p);
	}
	else
	{
		PROCESS_INFORMATION psi;
		STARTUPINFOA         stia;
		::GetStartupInfoA( &stia );
		const char *p = fcmd.ConvToChar();
		if( !p ) return;
		if( ::CreateProcessA(NULL, const_cast<char*>(p),
				NULL, NULL, 0, NORMAL_PRIORITY_CLASS, NULL, NULL,
				&stia, &psi ) )
		{
			::CloseHandle( psi.hThread );
			::CloseHandle( psi.hProcess );
		}
		fcmd.FreeCMem(p);
	}
#else // non unicows mode.
	PROCESS_INFORMATION psi;
	STARTUPINFO         sti;
	::GetStartupInfo( &sti );
	if( ::CreateProcess( NULL, const_cast<TCHAR*>(fcmd.c_str()),
			NULL, NULL, 0, NORMAL_PRIORITY_CLASS, NULL, NULL,
			&sti, &psi ) )
	{
		::CloseHandle( psi.hThread );
		::CloseHandle( psi.hProcess );
	}
#endif // UNICOWS
}

static HMENU getDocTypeSubMenu(HWND hwnd) { return GetSubMenu( ::GetSubMenu(::GetMenu(hwnd),3),9 ); }

//-------------------------------------------------------------------------
// ステータスバー制御
//-------------------------------------------------------------------------

inline GpStBar::GpStBar()
	: str_(NULL)
	, lb_(2)
{
}

inline void GpStBar::SetCsText( const TCHAR* str )
{
	// 文字コード表示領域にSetTextする
	SetText( str_=str, CS_PART );
}

inline void GpStBar::SetLbText( int lb )
{
	// 改行コード表示領域にSetTextする
	static const TCHAR* const lbstr[] = {TEXT("CR"),TEXT("LF"),TEXT("CRLF")};
	SetText( lbstr[lb_=lb], LB_PART );
}

void GpStBar::SetUnicode( const unicode *uni )
{
	TCHAR buf[ULONG_DIGITS+2+1];

	ulong  cc = uni[0];
	if( isHighSurrogate(uni[0]) )
		cc = 0x10000 + ( ((uni[0]-0xD800)&0x3ff)<<10 ) + ( (uni[1]-0xDC00)&0x3ff );

	TCHAR *t = const_cast<TCHAR*>(LPTR2Hex( buf+2, cc ));
	*--t = TEXT('+'); *--t = TEXT('U');
	SetText( t, UNI_PART );
}

void GpStBar::SetZoom( short z )
{
	TCHAR buf[INT_DIGITS+1 + 2];
	const TCHAR *b = Int2lStr(buf, z);
	buf[INT_DIGITS+2] = TEXT('\0');
	buf[INT_DIGITS+1] = TEXT('%');
	buf[INT_DIGITS+0] = TEXT(' ');
	SetText(b, GpStBar::ZOOM_PART);
}

int GpStBar::AutoResize( bool maximized )
{
	// 文字コード表示領域を確保しつつリサイズ
	int h = StatusBar::AutoResize( maximized );
	int w[] = { width()-150, width()-50, width()-50, width()-50, width() };

	HDC dc = ::GetDC( hwnd() );
	SIZE s;
	if( ::GetTextExtentPoint( dc, TEXT("CRLF1M"), 6, &s ) ) // Line Ending
		w[3] = w[4] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("BBBWWW (100)"), 12, &s ) ) // Charset
		w[1] = w[2] = w[3] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("U+100000"), 8, &s ) ) // Unicode disp.
		w[1] = w[2] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("990 %"), 5, &s ) ) // Percentage disp.
		w[0] = Max( w[1] - s.cx, (long)(width()/3) );

	::ReleaseDC( hwnd(), dc );

	SetParts( countof(w), w );
	SetCsText( str_ );
	SetLbText( lb_ );
	return h;
}



//-------------------------------------------------------------------------
// ディスパッチャ
//-------------------------------------------------------------------------

LRESULT GreenPadWnd::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	// アクティブ化。EditCtrlにフォーカスを。
	case WM_ACTIVATE:
		if( LOWORD(wp) != WA_INACTIVE )
			edit_.SetFocus();
		break;

	// We check if the timestamp was changed when GreenPad's window
	// gets activated
	case WM_ACTIVATEAPP:
		if( cfg_.warnOnModified() && (BOOL)wp == TRUE )
			PostMessage(hwnd(), WMU_CHECKFILETIMESTAMP, 0, 0);
		return WndImpl::on_message( msg, wp, lp );

	// サイズ変更。子窓を適当に移動。
	case WM_SIZE:
		if( wp==SIZE_MAXIMIZED || wp==SIZE_RESTORED )
		{
			int ht = stb_.AutoResize( wp==SIZE_MAXIMIZED );
			edit_.MoveTo( 0, 0, LOWORD(lp), HIWORD(lp)-ht );
			cfg_.RememberWnd(this);
		}
		break;

	case WM_MOUSEWHEEL:
		if( wp & MK_CONTROL )
		{
			short delta_zoom = (SHORT)HIWORD(wp) / 12;
			if(delta_zoom == 0)
				delta_zoom += (SHORT)HIWORD(wp) > 0 ? 1 : -1;
			on_setzoom( cfg_.GetZoom() + delta_zoom );
			return 0;
		}
		break;

	#ifdef PM_DPIAWARE
	case 0x02E0: // WM_DPICHANGED
		if( lp )
		{	// We need to set the font again so that it scales to
			// The new monitor DPI.
			edit_.getView().SetFont( cfg_.vConfig(), cfg_.GetZoom() );

			// Resize the window to the advised RECT
			RECT *rc = reinterpret_cast<RECT *>(lp);
			::SetWindowPos( hwnd(), NULL,
				rc->left, rc->top, rc->right-rc->left, rc->bottom-rc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
		} break;
	#endif // PM_DPIAWARE

	// ウインドウ移動
	case WM_MOVE:
		{
			RECT rc;
			getPos(&rc);
			cfg_.RememberWnd(this);
		}
		break;

//	case WM_ERASEBKGND:{
//		// Uncomment to see in black the area that will be repainted
//		Sleep(200);
//		RECT rc;
//		GetClientRect(hwnd(), &rc);
//		FillRect((HDC)wp, &rc,  (HBRUSH)GetStockObject(BLACK_BRUSH));
//		Sleep(200);
//		return 1;
//		}break;

	// システムコマンド。終了ボタンとか。
	case WM_SYSCOMMAND:
		if( wp==SC_CLOSE || wp==SC_DEFAULT )
			on_exit();
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	// 右クリックメニュー, right-click menu
	case WM_CONTEXTMENU:
		if( reinterpret_cast<HWND>(wp) == edit_.hwnd() )
			::TrackPopupMenu(
				::GetSubMenu( ::GetMenu(hwnd()), 1 ), // 編集メニュー表示, Edit menu display
				GetSystemMetrics(SM_MENUDROPALIGNMENT)|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
				static_cast<SHORT>(LOWORD(lp)), static_cast<SHORT>(HIWORD(lp)), 0, hwnd(), NULL );
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	// メニューのグレーアウト処理, Menu gray-out processing
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
		on_initmenu( reinterpret_cast<HMENU>(wp), msg==WM_INITMENUPOPUP );
		break;

	// Ｄ＆Ｄ
	case WM_DROPFILES:
		on_drop( reinterpret_cast<HDROP>(wp) );
		break;

	#ifndef NO_OLEDNDSRC
	case WM_NCRBUTTONDOWN: {
		if( wp == HTSYSMENU
		&& !isUntitled()
		&& coolDragDetect( hwnd(), /*pt=*/lp, WM_NCRBUTTONUP, PM_NOREMOVE )  )
		{
			// Allow dragging filename out of system tray with Right button.
			const unicode *fnu = filename_.ConvToWChar();
			if( fnu )
			{
				OleDnDSourceTxt doDrag( fnu, my_lstrlenW(fnu), DROPEFFECT_COPY );
				filename_.FreeWCMem(fnu);
			}
			break;
		}
	} return WndImpl::on_message( msg, wp, lp );

//	case WM_NCRBUTTONUP:
//		LOGGER( "WM_NCRBUTTONUP" );
//		return WndImpl::on_message( msg, wp, lp );

	#endif // NO_OLEDNDSRC

	// MRU
	case GPM_MRUCHANGED:
		SetupMRUMenu();
		break;

	// NOTIFY
	case WM_NOTIFY:
		if( wp == 1787 // Status Bar ID to check before[]
		&& (reinterpret_cast<NMHDR*>(lp))->code == NM_DBLCLK )
		{
			RECT rc;
			DWORD msgpos = GetMessagePos();
			POINT pt = { (short)LOWORD(msgpos), (short)HIWORD(msgpos) };
			ScreenToClient(stb_.hwnd(), &pt);
			if( stb_.SendMsg( SB_GETRECT, 0, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				on_jump();
			else if( stb_.SendMsg( SB_GETRECT, 1, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				//stb_.SetZoom(100), cfg_.SetZoom( 100 ), edit_.getView().SetFont( cfg_.vConfig(), 100 );
				on_zoom();
			else if( stb_.SendMsg( SB_GETRECT, 2, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) )
				on_insertuni();
			else/* if( stb_.SendMsg( SB_GETRECT, 3, reinterpret_cast<LPARAM>(&rc) ) && PtInRect(&rc, pt) ) */
				on_reopenfile();
		}
		break;

	case WMU_CHECKFILETIMESTAMP:
		if( !isUntitled()
		&&  old_filetime_.dwLowDateTime != 0
		&&  old_filetime_.dwHighDateTime != 0 )
		{
			FILETIME new_filetime = filename_.getLastWriteTime();
			if( ::CompareFileTime(&old_filetime_, &new_filetime) < 0 )
			{
				// Send Up click event in case the window was activated with a click.
				SendMessage(edit_.getView().hwnd(), WM_LBUTTONUP, 0, GetMessagePos());
				old_filetime_.dwLowDateTime=old_filetime_.dwHighDateTime=0;
				if( IDYES == MsgBox(RzsString(IDS_MODIFIEDOUT).c_str(), RzsString(IDS_APPNAME).c_str(), MB_YESNO) )
					on_refreshfile(); // Reload if asked.
				// Reset old time stamp either case.
				// Note: Timestamp must also be updated when Opening/Saving a file.
				old_filetime_ = filename_.getLastWriteTime();
			}
		}
		break;

	// その他
	default:
		return WndImpl::on_message( msg, wp, lp );
	}

	return 0;
}

bool GreenPadWnd::on_command( UINT id, HWND ctrl )
{
	switch( id )
	{
	// Window
	case ID_CMD_NEXTWINDOW: on_nextwnd(); break;
	case ID_CMD_PREVWINDOW: on_prevwnd(); break;

	// File
	case ID_CMD_NEWFILE:    on_newfile();   break;
	case ID_CMD_OPENFILE:   on_openfile();  break;
	case ID_CMD_REOPENFILE: on_reopenfile();break;
	case ID_CMD_OPENELEVATED: on_openelevated(filename_); break;
	case ID_CMD_REFRESHFILE:on_refreshfile();break;
	case ID_CMD_SAVEFILE:   on_savefile();  break;
	case ID_CMD_SAVEFILEAS: on_savefileas();break;
	case ID_CMD_PRINT:      on_print();     break;
	case ID_CMD_PAGESETUP:  on_pagesetup(); break;
	case ID_CMD_SAVEEXIT:   if(Save_showDlgIfNeeded()) on_exit();  break;
	case ID_CMD_DISCARDEXIT: Destroy();     break;
	case ID_CMD_EXIT:       on_exit();      break;
	case ID_CMD_QUICKEXIT:  if(cfg_.useQuickExit()) on_exit();      break;

	// Edit
	case ID_CMD_UNDO:       edit_.getDoc().Undo();              break;
	case ID_CMD_REDO:       edit_.getDoc().Redo();              break;
	case ID_CMD_CUT:        edit_.getCursor().Cut();            break;
	case ID_CMD_COPY:       edit_.getCursor().Copy();           break;
	case ID_CMD_PASTE:      edit_.getCursor().Paste();          break;
	case ID_CMD_DELETE: if( edit_.getCursor().isSelected() ){ edit_.getCursor().Del(false);} break;
	case ID_CMD_SELECTALL:  edit_.getCursor().Home(true,false);
	                        edit_.getCursor().End(true,true);   break;
	case ID_CMD_DATETIME:   on_datetime();                      break;
	case ID_CMD_INSERTUNI:  on_insertuni();                     break;
	case ID_CMD_ZOOMDLG:    on_zoom();                          break;
	case ID_CMD_ZOOMRZ:     on_setzoom( 100 );                  break;
	case ID_CMD_ZOOMUP:     on_setzoom( cfg_.GetZoom() + 10 );  break;
	case ID_CMD_ZOOMDN:     on_setzoom( cfg_.GetZoom() - 10 );  break;
#ifndef NO_IME
	case ID_CMD_RECONV:     on_reconv();                        break;
	case ID_CMD_TOGGLEIME:  on_toggleime();                     break;
#endif
    // More edit
	case ID_CMD_UPPERCASE:  edit_.getCursor().UpperCaseSel();      break;
	case ID_CMD_LOWERCASE:  edit_.getCursor().LowerCaseSel();      break;
	case ID_CMD_INVERTCASE: edit_.getCursor().InvertCaseSel();     break;
	case ID_CMD_TTSPACES:   edit_.getCursor().TTSpacesSel();       break;
	case ID_CMD_SFCHAR:     edit_.getCursor().StripFirstChar();    break;
	case ID_CMD_SLCHAR:     edit_.getCursor().StripLastChar();     break;
	case ID_CMD_ASCIIFY:    edit_.getCursor().ASCIIFy();           break;

	// Normalizations forms C=1, D=2, KC=5, KD=6.
	case ID_CMD_UNINORMC:   edit_.getCursor().UnicodeNormalize(1); break;
	case ID_CMD_UNINORMD:   edit_.getCursor().UnicodeNormalize(2); break;
	case ID_CMD_UNINORMKC:  edit_.getCursor().UnicodeNormalize(5); break;
	case ID_CMD_UNINORMKD:  edit_.getCursor().UnicodeNormalize(6); break;

	case ID_CMD_QUOTE:      edit_.getCursor().QuoteSelection(false);break;
	case ID_CMD_UNQUOTE:    edit_.getCursor().QuoteSelection(true); break;
	case ID_CMD_DELENDLINE: edit_.getCursor().DelToEndline(false); break;
	case ID_CMD_DELSTALINE: edit_.getCursor().DelToStartline(false); break;
	case ID_CMD_DELENDFILE: edit_.getCursor().DelToEndline(true); break;
	case ID_CMD_DELSTAFILE: edit_.getCursor().DelToStartline(true); break;

	// Search
	case ID_CMD_FIND:       search_.ShowDlg();  break;
	case ID_CMD_FINDNEXT:   search_.FindNext(); break;
	case ID_CMD_FINDPREV:   search_.FindPrev(); break;
	case ID_CMD_JUMP:       on_jump(); break;
	case ID_CMD_GREP:       on_grep();break;
	case ID_CMD_HELP:       on_help();break;
	case ID_CMD_OPENSELECTION: on_openselection(); break;
	case ID_CMD_SELECTIONLEN: on_showselectionlen(); break;

	// View
	case ID_CMD_NOWRAP:     edit_.getView().SetWrapType( wrap_=-1 ); break;
	case ID_CMD_WRAPWIDTH:  edit_.getView().SetWrapType( wrap_=cfg_.wrapWidth() ); break;
	case ID_CMD_WRAPWINDOW: edit_.getView().SetWrapType( wrap_=0 ); break;
	case ID_CMD_CONFIG:     on_config();    break;
	case ID_CMD_STATUSBAR:  on_statusBar(); break;

	// Help
	case ID_CMD_HELPABOUT: on_helpabout(); break;

	// DocType
	default: if( ID_CMD_DOCTYPE <= id ) {
			on_doctype( id - ID_CMD_DOCTYPE );
			break;
		} else if( ID_CMD_MRU <= id ) {
			on_mru( id - ID_CMD_MRU );
			break;
		}

	// Default
		return false;
	}
	return true;
}

bool GreenPadWnd::PreTranslateMessage( MSG* msg )
{
	// 苦肉の策^^;
	if( search_.TrapMsg(msg) )
		return true;
	// キーボードショートカット処理
	return 0 != ::TranslateAccelerator( hwnd(), accel_, msg );
}



//-------------------------------------------------------------------------
// コマンド処理
//-------------------------------------------------------------------------

void GreenPadWnd::on_dirtyflag_change( bool )
{
	UpdateWindowName();
}

void GreenPadWnd::on_newfile()
{
	BootNewProcess();
}

void GreenPadWnd::on_openfile()
{
	Path fn;
	int  cs = 0;
	if( ShowOpenDlg( &fn, &cs ) )
		Open( fn, cs, true );
}

void GreenPadWnd::on_helpabout()
{
	// Crazy double macro so that an int define
	// Can be seen as a string
	#define SHARP(x) #x
	#define STR(x) SHARP(x)

	#if defined(UNICOWS)
		#define UNIANSI TEXT(" (Unicows)")
	#elif defined(UNICODE)
		#define UNIANSI TEXT(" (Unicode)")
	#elif defined(_MBCS)
		#define UNIANSI TEXT(" (MBCS)")
	#else
		#define UNIANSI TEXT(" (ANSI)")
	#endif

	#if defined(__GNUC__)
		#define COMPILER TEXT( "GNU C Compiler - " __VERSION__ )
	#elif defined(_MSC_VER)
//		#define COMPILER TEXT("Visual C++ - ")  TEXT(STR(_MSC_VER))
		#define COMPILER TEXT("Visual C++ - ")  + (String)SInt2Str((_MSC_VER-600)/100).c_str() + TEXT(".") + SInt2Str(_MSC_VER%100).c_str() +
	#elif defined(__WATCOMC__)
		#define COMPILER TEXT("Open Watcom - ") TEXT(STR(__WATCOMC__))
	#elif defined(__BORLANDC__)
		#define COMPILER TEXT("Borland C++ - ") TEXT(STR(__BORLANDC__))
	#elif defined(__DMC__)
		#define COMPILER TEXT("Borland C++ - ") TEXT(STR(__DMC__))
	#elif defined(__INTEL_COMPILER)
		#define COMPILER TEXT("Borland C++ - ") TEXT(STR(__INTEL_COMPILER))
	#elif defined(__clang__)
		#define COMPILER TEXT("LLVM Clang - ")  TEXT(STR(__clang_major__)) TEXT(".") TEXT(STR(__clang_minor__))
	#else
		//#error Unknown compiler, consider adding it to the list.
		#define COMPILER TEXT( "?\n" )
	#endif

	#if defined(WIN32S)
		#if defined(OLDWIN32S)
			#define TARGETOS TEXT("Win32s beta")
		#else
			#define TARGETOS TEXT("Win32s")
		#endif
	#elif defined(UNICOWS)
		#define TARGETOS TEXT("Windows 9x/NT")
	#elif defined(UNICODE)
		#define TARGETOS TEXT("Windows NT")
	#else
		#define TARGETOS TEXT("Windows 9x")
	#endif

	#if defined(TARGET_VER)
		#if TARGET_VER == 303
			#define TGVER TEXT(" 3.10.340")
		#elif TARGET_VER == 310
			#define TGVER TEXT(" 3.10")
		#elif TARGET_VER == 350
			#define TGVER TEXT(" 3.50+")
		#else // TARGET_VER >= 351
			#define TGVER TEXT(" 3.51+")
		#endif
	#else
		#if defined(_M_AMD64) || defined(_M_X64) || defined(WIN64)
			// XP/NT5.1 is the first x64 version of Windows.
			#define TGVER TEXT(" 5.1")
		#elif defined(_M_IA64)
			// 2000/NT5.0 is the first IA64 version of Windows.
			#define TGVER TEXT(" 5.0+")
		#elif defined(_M_ARM64) || defined(_M_ARM)
			// 8/NT6.2 is the first ARM version of Windows.
			#define TGVER TEXT(" 6.2+")
		#else
			// Default to NT3.51/95 (I guess...)
			#define TGVER TEXT(" 3.51+")
		#endif
	#endif //TARGET_VER

	#if defined(NO_OLEDNDSRC) && defined(NO_OLEDNDTAR)
		#define USEOLE TEXT(" ")
	#else
		#define USEOLE TEXT(" OLE ")
	#endif //OLE

	#if defined(_M_AMD64) || defined(_M_X64)
		#define PALT TEXT( " - x86_64" )
	#elif defined(_M_IA64)
		#define PALT TEXT( "- IA64" )
	#elif defined(_M_ARM64)
		#define PALT TEXT( " - ARM64" )
	#elif defined(_M_ARM)
		#define PALT TEXT( " - ARM" )
	#elif defined(_M_IX86)
		#define PALT TEXT( " - i386" )
	#elif defined(_M_ALPHA)
		#define PALT TEXT( " - Alpha" )
	#elif defined(_M_MRX000) || defined(_MIPS_)
		#define PALT TEXT( " - MIPS" )
	#elif defined(_M_PPC)
		#define PALT TEXT( " - PowerPC" )
	#endif
	// Show Help->About dialog box.
	struct AboutDlg A_FINAL: public DlgImpl {
		AboutDlg(HWND parent) : DlgImpl(IDD_ABOUTDLG), parent_( parent ) { GoModal(parent_); }
		void on_init() override
		{
			String s = RzsString(IDS_APPNAME).c_str();
			s += TEXT(" - ") TEXT( VER_FILEVERSIONSTR ) UNIANSI TEXT("\r\n")
			     COMPILER TEXT(" on ") TEXT( __DATE__ ) TEXT("\r\n")
			     TARGETOS TGVER USEOLE PALT TEXT("\r\n")
			     TEXT("Running on ");

			if( app().isNT() )
				s += TEXT("Windows NT ");
			else if( app().isWin32s() )
				s += TEXT("Win32s ");
			else
				s+= TEXT("Windows ");
			WORD osver = app().getOSVer();
			WORD osbuild = app().getOSBuild();
			s += SInt2Str( HIBYTE(osver) ).c_str(); s += TEXT(".");
			s += SInt2Str( LOBYTE(osver) ).c_str(); s += TEXT(".");
			s += SInt2Str( osbuild ).c_str();

			SetItemText(IDC_ABOUTSTR, s.c_str());
			SetItemText(IDC_ABOUTURL, TEXT("https://github.com/RamonUnch/GreenPad"));
			SetCenter(hwnd(), parent_);
		}
		HWND parent_;
	} ahdlg (hwnd());

	#undef UNIANSI
	#undef COMPILER
	#undef TGVER
	#undef USEOLE
	#undef PALT
}

void GreenPadWnd::on_reopenfile()
{
	ReopenDlg dlg( charSets_, csi_ );
	dlg.GoModal( hwnd() );
	if( dlg.endcode() == IDOK )
	{	// User pressed OK
		csi_ = dlg.csi(); // Change current csi_
		if( !isUntitled() )
		{	// We have a file to reopen
			if( AskToSave() )
			{
				int cs = resolveCSI(csi_);
				OpenByMyself( filename_, cs, false );
			}
		}
		else
		{	// Simply update statusbar
			UpdateWindowName();
		}
	}
}

// Resolves a csi into a usable cs
int GreenPadWnd::resolveCSI(int csi) const
{
	return ( (UINT)csi >= 0xf0f00000 && (UINT)csi < 0xf1000000 )? csi & 0xfffff
	     : (0 < csi && csi < (int)charSets_.size())? charSets_[csi].ID: 0;
}
void GreenPadWnd::on_openelevated(const ki::Path& fn)
{
#if !defined(TARGET_VER) || TARGET_VER > 303
	// Only supported since Windows 2000
	if( app().getOSVer() < 0x0500 )
		return;

	const view::VPos *cur, *sel;
	edit_.getCursor().getCurPosUnordered(&cur, &sel);
	int cp = resolveCSI(csi_);

	String cmdl  = TEXT( "-c"); cmdl += SInt2Str(cp).c_str();
	       cmdl += TEXT(" -l"); cmdl += SInt2Str(cur->tl+1).c_str();
	       cmdl += TEXT(" \""); cmdl += fn.c_str(); cmdl += TEXT("\"");

	TCHAR exename[MAX_PATH];
	Path::GetExeName( exename );

//	MsgBox( cmdl.c_str(), exename );
	HINSTANCE ret = ShellExecute(NULL, TEXT("runas"), exename, cmdl.c_str(), NULL, SW_SHOWNORMAL);
	if( (LONG_PTR)ret > 32 )
		::ExitProcess(0); // Dirty quick exit.
#endif
}
// Keeps cursor position...
// TOTO: also set VPos so that it does not scroll...
void GreenPadWnd::on_refreshfile()
{
	if( !isUntitled() )
	{
		const view::VPos *cur, *sel;
		bool selected = edit_.getCursor().getCurPosUnordered(&cur, &sel);
		unsigned sline = cur->tl, scol = cur->ad;
		unsigned eline = sel->tl, ecol = sel->ad;
		int cp = resolveCSI(csi_);

		OpenByMyself(filename_, cp, false);
		if (selected) edit_.getCursor().MoveCur(DPos(eline, ecol), false);
		edit_.getCursor().MoveCur(DPos(sline, scol), selected);
	}
}

void GreenPadWnd::on_savefile()
{
	Save_showDlgIfNeeded();
}

void GreenPadWnd::on_savefileas()
{
	if( ShowSaveDlg() )
	{
		Save();
		ReloadConfig(); // 文書タイプに応じて表示を更新
	}
}
BOOL GreenPadWnd::myPageSetupDlg(LPPAGESETUPDLG lppsd)
{
#if !defined(TARGET_VER) || TARGET_VER>=351
	return PageSetupDlg(lppsd);

#elif defined(UNICOWS)
	// In unicows mode the PageSetupDlg is already
	// dynamically loaded, so we just check for OSVer.
	if( app().isNTOSVerLarger(MKVER(3,51,1057))
	||  app().is9xOSVerLarger(MKVER(4,00,950)) )
	{
		return PageSetupDlg(lppsd); // NT3.51/95+
	}
	else
	{
		MsgBox( TEXT("Unable to use PageSetupDlg() function") );
		return FALSE;
	}

#else
	// Dynamically load ourself the ANSI or Unicode version
	// for pure ANIS or pure Unicode mode.
	// We can Load/Free the dll every time because performances
	// are not important.
	BOOL ret = FALSE;
	HINSTANCE h = LoadLibrary( TEXT("COMDLG32.DLL") );
	if( h )
	{
		#define FTYPE ( BOOL (WINAPI *)(LPPAGESETUPDLG lppsd) )
		BOOL (WINAPI *dyn_PageSetupDlg)(LPPAGESETUPDLG lppsd);
		#ifdef _UNICODE
		dyn_PageSetupDlg = FTYPE GetProcAddress(h, "PageSetupDlgW");
		#else
		dyn_PageSetupDlg = FTYPE GetProcAddress(h, "PageSetupDlgA");
		#endif
		if( dyn_PageSetupDlg )
			ret = dyn_PageSetupDlg(lppsd);
		FreeLibrary( h );
		#undef FTYPE
	}
	return ret;
#endif
}
void GreenPadWnd::on_pagesetup()
{
	PAGESETUPDLG psd;
	mem00(&psd, sizeof(psd));
	psd.lStructSize = sizeof(psd);
	// FIXME: use local units.
	psd.Flags = PSD_INTHOUSANDTHSOFINCHES|PSD_DISABLEORIENTATION|PSD_DISABLEPAPER|PSD_DISABLEPRINTER|PSD_MARGINS;
	psd.hwndOwner = hwnd();
	CopyRect(&psd.rtMargin, cfg_.PMargins());
	if( myPageSetupDlg(&psd) )
		cfg_.SetPrintMargins(&psd.rtMargin);
}

void GreenPadWnd::SetFontSizeforDC(LOGFONT *font, HDC hDC, int fsiz, int fx)
{
	font->lfWidth          = 0;
	font->lfHeight = -MulDiv(fsiz, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
	if(fx) font->lfWidth = -MulDiv(fx, ::GetDeviceCaps(hDC, LOGPIXELSX), 72);
}
void GreenPadWnd::on_print()
{
	doc::Document& d = edit_.getDoc();
	const unicode* buf;
	ulong /*dpStart = 0,*/ len = 0;
	short procCopies = 0, totalCopies = 0;

	// Setup print dialog
	PRINTDLG thePrintDlg = { sizeof(thePrintDlg) };
	thePrintDlg.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_HIDEPRINTTOFILE;
	thePrintDlg.nCopies = 1;
	thePrintDlg.hwndOwner = hwnd();
	thePrintDlg.hDevNames = NULL;
	thePrintDlg.hDevMode = NULL;

	if (PrintDlg(&thePrintDlg) == 0) {
		// cancelled
		return;
	}

	totalCopies = thePrintDlg.nCopies;

	// タイトルに表示される文字列の調整
	// [FileName *] - GreenPad
	TCHAR name[1+MAX_PATH+6+32+1];
	GetTitleText( name );

	// Set DOCINFO structure
	DOCINFO di = { sizeof(DOCINFO) };
	di.lpszDocName = name;
	di.lpszOutput = (LPTSTR) NULL;
	di.lpszDatatype = (LPTSTR) NULL;
	di.fwType = 0;

	int nError = ::StartDoc(thePrintDlg.hDC, &di);
	if (nError == SP_ERROR)
	{
		TCHAR tmp[128];
		::wsprintf(tmp,TEXT("StartDoc Error #%d - please check printer."),::GetLastError());
		MsgBox(tmp, RzsString(IDS_APPNAME).c_str(), MB_OK|MB_TASKMODAL );
		return;
		// Handle the error intelligently
	}
	// Setup printing font.
	LOGFONT lf;
	memmove( &lf, &cfg_.vConfig().font, sizeof(lf) );
	SetFontSizeforDC(&lf, thePrintDlg.hDC, cfg_.vConfig().fontsize, cfg_.vConfig().fontwidth);
	HFONT printfont = ::CreateFontIndirect(&lf);
	HFONT oldfont = (HFONT)::SelectObject( thePrintDlg.hDC, printfont );

	::StartPage(thePrintDlg.hDC);

	// Get Printer Caps
	int cWidthPels, cHeightPels, cLineHeight;
	cWidthPels = ::GetDeviceCaps(thePrintDlg.hDC, HORZRES); // px
	cHeightPels = ::GetDeviceCaps(thePrintDlg.hDC, VERTRES);// px
	int logpxx = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSX);// px/in
	int logpxy = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSY);// px/in

	// Get Line height
	RECT rctmp = {0, 0, cWidthPels, cHeightPels};
	::DrawTextW(thePrintDlg.hDC, L"#", 1, &rctmp, DT_CALCRECT|DT_LEFT|DT_WORDBREAK|DT_EXPANDTABS|DT_EDITCONTROL);
	cLineHeight = rctmp.bottom-rctmp.top;

	// Convert config margins in Inches to pixels
	const RECT rcMargins = {
		(cfg_.PMargins()->left  * logpxx)/1000,
		(cfg_.PMargins()->top   * logpxy)/1000,
		(cfg_.PMargins()->right * logpxx)/1000,
		(cfg_.PMargins()->left  * logpxy)/1000,
	};
	const RECT rcFullPage = {
		rcMargins.left , rcMargins.top,
		cWidthPels - rcMargins.left - rcMargins.right,
		cHeightPels - rcMargins.top - rcMargins.bottom
	};
	RECT rcPrinter; // Mutates in the loop
	CopyRect(&rcPrinter, &rcFullPage);

	int nThisLineHeight, nChars = 0, nHi = 0, nLo = 0;
	const unicode* uStart;

	// Process with multiple copies
	#define myDTFLAGS DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX|DT_EDITCONTROL
	do {
		if( procCopies )
		{
			::StartPage(thePrintDlg.hDC);
			CopyRect(&rcPrinter, &rcFullPage);
		}
		// Print
		for( ulong e=d.tln(), dpStart=0; dpStart<e; )
		{
			len = d.len(dpStart);
			buf = d.tl(dpStart);
			if(!len)
			{	// Empty Line
				rcPrinter.top += cLineHeight;
				++dpStart;
			}
			else // non-empty line
			{
				CopyRect(&rctmp, &rcPrinter);
				nHi = len;
				nLo = 0;
				if(!nChars)
				{
					uStart = buf;
					nChars = len;
				}
				else
				{
					uStart += nChars;
					nHi = nChars = len-nChars;
				}

				while (nLo < nHi) { // Find maximum number of chars can be printed
					rctmp.top = rcPrinter.top;
					nThisLineHeight = ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rctmp, DT_CALCRECT|myDTFLAGS);
					if (rcPrinter.top+nThisLineHeight < rcPrinter.bottom)
						nLo = nChars;
					if (rcPrinter.top+nThisLineHeight > rcPrinter.bottom)
						nHi = nChars;
					if (nLo == nHi - 1)
						nChars = nHi = nLo;
					if (nLo < nHi)
						nChars = nLo + (nHi - nLo)/2;
				}
				rcPrinter.top += ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rcPrinter, myDTFLAGS);
				if(uStart+nChars == buf+len) // Line end
				{
					nChars = 0;
					++dpStart;
				}
			}

			// Turn to new page if needed
			if( (dpStart<e) && (rcPrinter.top + cLineHeight + 5 > rcPrinter.bottom) )
			{
				::EndPage(thePrintDlg.hDC);
				::StartPage(thePrintDlg.hDC);
				CopyRect(&rcPrinter, &rcFullPage);
			}
		}
		// EndPage Does not reset page attributes on NT/9x
		::EndPage(thePrintDlg.hDC);
	} while( ++procCopies < totalCopies );
	#undef myDTFLAGS

	// Close Printer
	::SelectObject(thePrintDlg.hDC, oldfont);
	::DeleteObject(printfont);
	::EndDoc(thePrintDlg.hDC);
	::DeleteDC(thePrintDlg.hDC);

	// 解放する。
	::GlobalFree(thePrintDlg.hDevNames);
	::GlobalFree(thePrintDlg.hDevMode);

}

void GreenPadWnd::on_exit()
{
	search_.SaveToINI();
	if( AskToSave() )
		Destroy();
}

void GreenPadWnd::on_initmenu( HMENU menu, bool editmenu_only )
{
	UINT gray_when_unselected = MF_BYCOMMAND|(edit_.getCursor().isSelected()? MF_ENABLED: MF_GRAYED);
	::EnableMenuItem( menu, ID_CMD_CUT,    gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_COPY,   gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_DELETE, gray_when_unselected);
	::EnableMenuItem( menu, ID_CMD_UNDO,   MF_BYCOMMAND|(edit_.getDoc().isUndoAble()? MF_ENABLED: MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_REDO,   MF_BYCOMMAND|(edit_.getDoc().isRedoAble()? MF_ENABLED: MF_GRAYED) );

	::EnableMenuItem( menu, ID_CMD_UPPERCASE, gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_LOWERCASE, gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_INVERTCASE,gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_TTSPACES,  gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_ASCIIFY,   gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_SFCHAR,    gray_when_unselected );
	::EnableMenuItem( menu, ID_CMD_SLCHAR,    gray_when_unselected );
//	::EnableMenuItem( menu, ID_CMD_QUOTE,     gray_when_unselected );
//	::EnableMenuItem( menu, ID_CMD_UNQUOTE,   gray_when_unselected );

#ifndef NO_IME
	::EnableMenuItem( menu, ID_CMD_RECONV, MF_BYCOMMAND|(edit_.getCursor().isSelected() && ime().IsIME() && ime().CanReconv() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_TOGGLEIME, MF_BYCOMMAND|(ime().IsIME() ? MF_ENABLED : MF_GRAYED) );
#endif
	if( editmenu_only )
	{
		LOGGER("GreenPadWnd::on_initmenu end (edit menu only)");
		return;
	}

	::EnableMenuItem( menu, ID_CMD_SAVEFILE, MF_BYCOMMAND|(isUntitled() || edit_.getDoc().isModified() ? MF_ENABLED : MF_GRAYED) );
	// ::EnableMenuItem( menu, ID_CMD_REOPENFILE, MF_BYCOMMAND|(!isUntitled() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_OPENELEVATED, MF_BYCOMMAND|( app().getOSVer() >= 0x0500 ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_GREP, MF_BYCOMMAND|(cfg_.grepExe().len()>0 ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_OPENSELECTION, gray_when_unselected );

	::CheckMenuItem( menu, ID_CMD_NOWRAP, MF_BYCOMMAND|(wrap_==-1?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWIDTH, MF_BYCOMMAND|(wrap_>0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWINDOW, MF_BYCOMMAND|(wrap_==0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_STATUSBAR, cfg_.showStatusBar()?MF_CHECKED:MF_UNCHECKED );

	LOGGER("GreenPadWnd::on_initmenu end (full init)");
}

void GreenPadWnd::on_drop( HDROP hd )
{
	UINT iMax = ::myDragQueryFile( hd, 0xffffffff, NULL, 0 );
	for( UINT i=0; i<iMax; ++i )
	{
		// Get length of the i string for array size.
		UINT len = ::myDragQueryFile( hd, i, NULL, 0)+1;
		len = Max(len, (UINT)MAX_PATH); // ^ the Above may fail on NT3.1
		TCHAR *str = (TCHAR *)TS.alloc( sizeof(TCHAR) * len );
		if( str )
		{
			::myDragQueryFile( hd, i, str, len );
			Open( str, AutoDetect );
			TS.freelast( str, sizeof(TCHAR) * len );
		}
	}
	::DragFinish( hd );
}

void GreenPadWnd::on_jump()
{
	const view::VPos *cur, *sel;
	edit_.getCursor().getCurPosUnordered(&cur, &sel);
	struct JumpDlg A_FINAL: public DlgImpl {
		JumpDlg( HWND w, int cur_line, int cur_col )
			: DlgImpl( IDD_JUMP )
			, LineNo ( cur_line )
			, ColNo  ( cur_col  )
			, w_( w )
			{ GoModal(w); }
		void on_init() override
		{
			SetCenter(hwnd(),w_); ::SetFocus(item(IDC_LINEBOX));
		}
		bool on_ok() override
		{
			// Jump to <+/->LINE,<+/->COLUMN
			// Use + or - for relative movement.
			TCHAR str[64], *pcol=NULL;
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			for( TCHAR *p=str; *p ;p++  )
			{
				if( *p == TEXT(',') )
				{
					*p = TEXT('\0'); // NULL separator
					pcol = ++p;
					break;
				}
			}
			if( *str != TEXT(',') )
				LineNo = newnumber( LineNo, str );

			if( pcol )
				ColNo = newnumber( ColNo, pcol );
			else
				ColNo = 1; // Set column number to 1 if undef.
			return true;
		}
		int newnumber( int number, const TCHAR *str )
		{
			if( str[0] == TEXT('+') || str[0] == TEXT('-'))
			{
				number += String::GetInt(str); // relative
				// prevent warparound for relative movements
				if( number <= 0 ) number = 1;
			}
			else
			{
				number = String::GetInt(str);
			}
			return number;
		}

		int LineNo;
		int ColNo;
		HWND w_;
	} dlg( hwnd(), cur->tl+1, cur->ad+1 );

	if( IDOK == dlg.endcode() )
		edit_.getCursor().MoveCur( DPos(dlg.LineNo-1, dlg.ColNo-1), false );
}

void GreenPadWnd::on_openselection()
{
#define isAbsolutePath(x) ( x[0] == L'\\' || (x[0] && x[1] == L':') )
	String cmd = TEXT("-c0 \"");
	aarr<unicode> sel = edit_.getCursor().getSelectedStr();
	// Remove trailing CRLFs.
	size_t slen = my_lstrlenW( sel.get() );
	while( slen-- && (sel[ slen ] == L'\r' || sel[ slen ] == L'\n') )
		sel[ slen ] = L'\0';

#if !defined(TARGET_VER) || TARGET_VER > 303
	if( my_instringW( sel.get(), L"http://")
	||  my_instringW( sel.get(), L"https://")
	||  my_instringW( sel.get(), L"ftp://")
	||  my_instringW( sel.get(), L"ftps://") )
	{
		// We have an URL.
		cmd = sel.get();
		ShellExecute(NULL, TEXT("open"), cmd.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return;
	}
#endif
	if( !isAbsolutePath( sel ) )
	{
		// We got a relative path, get a directorry for it.
		Path d;
		if( filename_.len() )
			(d = filename_).BeDirOnly().BeBackSlash(true);
		else
			d = Path(Path::Cur);

		cmd += d;
	}

	cmd += sel.get();
	cmd += TEXT("\""); // -c0 "Path\To\File.ext"
	BootNewProcess( cmd.c_str() );
#undef isAbsolutePath
}
void GreenPadWnd::on_showselectionlen()
{
	const view::VPos *a, *b;
	edit_.getCursor().getCurPos(&a, &b);
	ulong len = edit_.getDoc().getRangeLength(*a, *b);
	TCHAR buf[ULONG_DIGITS+1];
	if( stb_.isVisible() )
		stb_.SetText( Ulong2lStr(buf, len) );
	else
		MsgBox( Ulong2lStr(buf, len), TEXT("Length in UTF-16 chars") );
}
void GreenPadWnd::on_grep()
{
	on_external_exe_start( cfg_.grepExe() );
}
void GreenPadWnd::on_help()
{
	on_external_exe_start( cfg_.helpExe() );
}
void GreenPadWnd::on_external_exe_start(const Path &g)
{
	if( g.len() != 0 )
	{
		Path d;
		if( filename_.len() )
			(d = filename_).BeDirOnly().BeBackSlash(false);
		else
			d = Path(Path::Cur);

		String fcmd;
		for( size_t i=0, e=g.len(); i<e; ++i )
		{
			if( g[i]==TEXT('%') )
			{
				if( g[i+1]==TEXT('1') || g[i+1]==TEXT('D') ) // '1' for bkwd compat
					++i, fcmd += d; // File's file path only
				else if( g[i+1]==TEXT('F') ) // Full file path+name
					++i, fcmd += filename_;
				else if( g[i+1]==TEXT('N') ) // File name only
					++i, fcmd += filename_.name();
				else if( g[i+1]==TEXT('S') ) // Current selection
					++i, fcmd += edit_.getCursor().getSelectedStr().get();
			}
			else
			{
				fcmd += g[i];
			}
		}

		PROCESS_INFORMATION psi;
		STARTUPINFO         sti = {sizeof(STARTUPINFO)};
		//sti.dwFlags = STARTF_USESHOWWINDOW;
		//sti.wShowWindow = SW_SHOWNORMAL;
		if( ::CreateProcess( NULL, const_cast<TCHAR*>(fcmd.c_str()),
				NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
				&sti, &psi ) )
		{
			::CloseHandle( psi.hThread );
			::CloseHandle( psi.hProcess );
		}
	}
}

void GreenPadWnd::on_datetime()
{
	String g = cfg_.dateFormat();

#if defined(WIN32S) || defined(TARGET_VER) && TARGET_VER <= 303
	HMODULE h = GetModuleHandle(TEXT("KERNEL32.DLL"));
	if( !app().isNT() )
	{	// Dynamically import GetTime/DateFormat on win32s build
		// So that it can run on NT3.1
		CHAR buf[255], tmp[255]="";
		CHAR *sfmt=NULL;
		const CHAR *lpFormat = g.len()?sfmt=const_cast<CHAR*>(g.ConvToChar()):"HH:mm yyyy/MM/dd";

		typedef int (WINAPI *GetDTFormat_type)( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime,LPCSTR lpFormat, LPSTR lpTimeStr,int cchTime);
		GetDTFormat_type MyGetTimeFormatA = (GetDTFormat_type)GetProcAddress(h, "GetTimeFormatA");
		GetDTFormat_type MyGetDateFormatA = (GetDTFormat_type)GetProcAddress(h, "GetDateFormatA");
		if( MyGetTimeFormatA )
			MyGetTimeFormatA( LOCALE_USER_DEFAULT, 0, NULL, lpFormat, buf, countof(buf));
		if( MyGetDateFormatA )
			MyGetDateFormatA( LOCALE_USER_DEFAULT, 0, NULL, buf, tmp,countof(tmp));

		if( tmp[0] ) // Not empty sucess!
			edit_.getCursor().Input( tmp, my_lstrlenA(tmp) );
		if( sfmt ) g.FreeCMem(sfmt);
		return;
	}
	else
	{	// If we are under NT then we need to use unicode version.
		WCHAR buf[255], tmp[255]=L"";
		wchar_t *wfmt = NULL;
		const WCHAR *lpFormat = g.len()?wfmt=const_cast<WCHAR*>(g.ConvToWChar()):L"HH:mm yyyy/MM/dd";

		typedef int (WINAPI *GetDTFormat_typeW)(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime,LPCWSTR lpFormat, LPWSTR lpTimeStr,int cchTime);
		GetDTFormat_typeW MyGetTimeFormatW = (GetDTFormat_typeW)GetProcAddress(h, "GetTimeFormatW");
		GetDTFormat_typeW MyGetDateFormatW = (GetDTFormat_typeW)GetProcAddress(h, "GetDateFormatW");
		if( MyGetTimeFormatW )
			MyGetTimeFormatW( LOCALE_USER_DEFAULT, 0, NULL, lpFormat, buf, countof(buf));
		if( MyGetDateFormatW )
			MyGetDateFormatW( LOCALE_USER_DEFAULT, 0, NULL, buf, tmp,countof(tmp));

		if( tmp[0] ) // Not empty sucess!
			edit_.getCursor().Input( tmp, my_lstrlenW(tmp) );
		if(wfmt) g.FreeWCMem(wfmt);
		return;
	}
#else
	TCHAR buf[255], tmp[255]=TEXT("");
	const TCHAR *lpFormat = g.len()?const_cast<TCHAR*>(g.c_str()):TEXT("HH:mm yyyy/MM/dd");

	::GetTimeFormat
		( LOCALE_USER_DEFAULT, 0, NULL, lpFormat, buf, countof(buf));
	::GetDateFormat
		( LOCALE_USER_DEFAULT, 0, NULL, buf, tmp,countof(tmp));

	edit_.getCursor().Input( tmp, my_lstrlen(tmp) );
#endif
}
void GreenPadWnd::on_insertuni()
{
	struct InsertUnicode A_FINAL: public DlgImpl {
		InsertUnicode(HWND w) : DlgImpl(IDD_JUMP), utf32_(0xffffffff), w_(w) { GoModal(w); }
		void on_init() override
		{
			SetCenter( hwnd(), w_ );
			SetItemText( IDC_LINLABEL, TEXT("&U+") );
			SetItemText( IDOK, RzsString(IDS_INSSERT).c_str() );
			SetText( RzsString(IDS_INSERTUNI).c_str() );

			::SetFocus(item(IDC_LINEBOX));
		}
		bool on_ok() override
		{
			TCHAR str[32]; str[0] = TEXT('\0');
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			if( !*str )
				return true;
			if( str[0] == TEXT('.') )
				// .decimal
				utf32_ = String::GetInt(str+1);
			else if( str[0] == TEXT('o') || str[0] == TEXT('O') )
				// Octal
				utf32_ = Octal2Ulong(str+1);
			else
				// heXadecimal (default)
				utf32_ = Hex2Ulong( str + (str[0] == TEXT('x') || str[0] == TEXT('X')) );

			return true;
		}
		qbyte utf32_;
		HWND w_;
	} dlg(hwnd());

	if( IDOK == dlg.endcode() && dlg.utf32_ != 0xffffffff )
	{
		edit_.getCursor().InputUTF32( dlg.utf32_ );
	}
}

// Show zoom dialog
void GreenPadWnd::on_zoom()
{
	struct ZoomDlg A_FINAL: public DlgImpl {
		ZoomDlg(HWND w, short zoom) : DlgImpl(IDD_JUMP), zoom_(zoom), w_(w) { GoModal(w); }
		void on_init() override
		{
			SetCenter( hwnd(), w_ );
			SetItemText( IDC_LINLABEL, TEXT("%") );
			SetItemText( IDOK, /**/ TEXT("OK") );
			SetText( RzsString(IDS_ZOOMPC).c_str() );
			SetItemText( IDC_LINEBOX, SInt2Str(zoom_).c_str() );

			HWND ed = item(IDC_LINEBOX);
			::SetFocus(ed);
			::SendMessage(ed, EM_SETSEL, 0, -1);
		}
		bool on_ok() override
		{
			TCHAR str[16]; str[0] = TEXT('\0');
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			if( !*str ) { zoom_ = 100; return true; }
			zoom_ = String::GetInt(str);
			return true;
		}
		short zoom_;
		HWND w_;
	} dlg(hwnd(), cfg_.GetZoom());

	short zoom = dlg.zoom_;
	if( IDOK == dlg.endcode() && zoom != cfg_.GetZoom() )
	{
		on_setzoom( zoom );
	}
}


void GreenPadWnd::on_setzoom( short zoom )
{
	zoom = Clamp((short)0, zoom, (short)990);
	edit_.getView().SetFont( cfg_.vConfig(), zoom );
	cfg_.SetZoom( zoom );
	stb_.SetZoom( zoom );
}

void GreenPadWnd::on_doctype( int no )
{
	if( HMENU m = getDocTypeSubMenu( hwnd() ) )
	{
		cfg_.SetDocTypeByMenu( no, m );
		ReloadConfig( true );
	}
}

void GreenPadWnd::on_config()
{
	if( cfg_.DoDialog(*this) )
	{
		SetupSubMenu();
		SetupMRUMenu();
		ReloadConfig(false);
	}
}
#if defined(TARGET_VER) && TARGET_VER<=350
/* WIP: Re-implementation of FindWindowEx for NT3.x */
struct MyFindWindowExstruct {
	HWND after;
	LPCTSTR lpszClass;
	LPCTSTR lpszWindow;
	HWND ret;
};
static BOOL CALLBACK MyFindWindowExProc(HWND hwnd, LPARAM lParam)
{
	struct MyFindWindowExstruct *param =
		reinterpret_cast<MyFindWindowExstruct*>(lParam);
	param->ret = NULL;
	// Skip windows before we reach the after HWND.
	if (param->after) {
		if (param->after == hwnd)
			param->after = NULL; // Stop skipping windows

		return TRUE; // Next window
	}
	// Start looking for the window we want...
	TCHAR tmpstr[256];
	bool classmatch = false, titlematch = false;

	if (param->lpszClass) {
		GetClassName(hwnd, tmpstr, countof(tmpstr));
		// Class matches...
		classmatch = !my_lstrcmp(param->lpszClass, tmpstr);
	} else {
		classmatch = true;
	}

	if(param->lpszWindow) {
		GetWindowText(hwnd, tmpstr, countof(tmpstr));
		titlematch = !my_lstrcmp(param->lpszClass, tmpstr);
	} else {
		titlematch = true;
	}
	if (classmatch && titlematch) {
		param->ret = hwnd; // Save hwnd in param
		return FALSE; // Stop enumarating
	}
	return TRUE; // Next window
}
static HWND MyFindWindowEx(HWND parent, HWND after, LPCTSTR lpszClass, LPCTSTR lpszWindow)
{
  # if !defined (TARGET_VER) || defined(UNICODE) && defined(UNICOWS)
	if( app().isNewShell() )
	{
		// In UNICOWS mode FindWindowEx is dynamically
		// imported and is implemented for 95/NT4+
		return FindWindowEx(parent, after, lpszClass, lpszWindow);
	}
  # endif
	// Fallback to the WIP FindWindowEx re-implementation.
	struct MyFindWindowExstruct param = {after, lpszClass, lpszWindow, NULL};
	EnumChildWindows(parent, MyFindWindowExProc, reinterpret_cast<LPARAM>(&param));
	return param.ret;
}
#else // TARGET_VER
#define MyFindWindowEx FindWindowEx
#endif // TARGET_VER == NT3.x
static void MyShowWnd( HWND wnd )
{
	if( ::IsIconic(wnd) )
		::ShowWindow( wnd, SW_RESTORE );
	::BringWindowToTop( wnd );
}

void GreenPadWnd::on_nextwnd()
{
	if( HWND next = ::MyFindWindowEx( NULL, hwnd(), className_, NULL ) )
	{
		int i=0;;
		HWND last=next, pos=NULL;
		while( last != NULL && i++ < 1024 )
			last = ::MyFindWindowEx( NULL, pos=last, className_, NULL );
		if( pos != next )
			::SetWindowPos( hwnd(), pos,
				0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW );
		MyShowWnd( next );
	}
}

void GreenPadWnd::on_prevwnd()
{
	HWND pos=NULL, next=::MyFindWindowEx( NULL,NULL,className_,NULL );
	int i=0;
	if( next==hwnd() )
	{
		while( next != NULL && i++ < 1024 )
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( pos!=hwnd())
			MyShowWnd( pos );
	}
	else
	{
		while( next!=hwnd() && next!=NULL && i++ < 1024)
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( next!=NULL )
			MyShowWnd( pos );
	}
}

void GreenPadWnd::on_statusBar()
{
	stb_.SetStatusBarVisible( !stb_.isStatusBarVisible() );
	cfg_.ShowStatusBarSwitch();

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	::GetWindowPlacement( hwnd(), &wp );
	if( wp.showCmd != SW_MINIMIZE )
	{
		const int ht = stb_.AutoResize( wp.showCmd == SW_MAXIMIZE );
		RECT rc;
		getClientRect(&rc);
		edit_.MoveTo( 0, 0, rc.right, rc.bottom-ht );
	}
}

void GreenPadWnd::on_move( const DPos& c, const DPos& s )
{
	static int busy_cnt = 0;
	if( !stb_.isVisible() )
		return;
	if( edit_.getDoc().isBusy() && ((++busy_cnt)&0xff) )
		return;

	if( c == s )
	{
		// Update U+XXXXh text in the StatusBar.
		const unicode* su = edit_.getDoc().tl(c.tl);
		stb_.SetUnicode( su+c.ad /*- (c.ad!=0 && c.ad==edit_.getDoc().len(c.tl) ) */);
	}
	else
	{
		TCHAR buf[ULONG_DIGITS+1];
		ulong N = s.tl == c.tl
			? Max(c.ad, s.ad) - Min(c.ad, s.ad)
			: Max(c.tl, s.tl) - Min(c.tl, s.tl);
		stb_.SetText( Ulong2lStr(buf, N), GpStBar::UNI_PART );
	}

	if( c == old_cur_ && s == old_sel_ )
		return; // Nothing to do

	old_cur_ = c;
	old_sel_ = s;

	ulong cad = c.ad;
	if( ! cfg_.countByUnicode() )
	{
		// ShiftJIS風のByte数カウント
		const unicode* cu = edit_.getDoc().tl(c.tl);
		const uint tab = NZero(cfg_.vConfig().tabstep);
		cad = 0;
		for( ulong i=0; i<c.ad; ++i )
			if( cu[i] == L'\t' )
				cad = (cad/tab+1)*tab;
			else if( cu[i]<0x80 || (0xff60<=cu[i] && cu[i]<=0xff9f) )
				cad = cad + 1;
			else
				cad = cad + 2;
	}

	TCHAR str[ULONG_DIGITS*4 + 10], *end = str;
	TCHAR tmp[ULONG_DIGITS+1];
	*end++ = TEXT('(');
	end = my_lstrkpy( str+1, Ulong2lStr(tmp, c.tl+1) );
	*end++ = TEXT(',');
	end = my_lstrkpy( end, Ulong2lStr(tmp, cad+1) );
	*end++ = TEXT(')');
	*end = TEXT('\0');
	if( c != s )
	{
		ulong sad = s.ad;
		if( ! cfg_.countByUnicode() )
		{
			// ShiftJIS風のByte数カウント
			const unicode* su = edit_.getDoc().tl(s.tl);
			sad = 0;
			for( ulong i=0; i<s.ad; ++i )
				sad += su[i]<0x80 || (0xff60<=su[i] && su[i]<=0xff9f) ? 1 : 2;
		}
		end = my_lstrkpy( end, TEXT(" - (") );
		end = my_lstrkpy( end, Ulong2lStr(tmp, s.tl+1) );
		*end++ = TEXT(',');
		end = my_lstrkpy( end, Ulong2lStr(tmp, sad+1) );
		*end++ = TEXT(')');
		*end = TEXT('\0');
	}
	stb_.SetText( str );
}

void GreenPadWnd::on_reconv()
{
	edit_.getCursor().Reconv();
}
void GreenPadWnd::on_toggleime()
{
	edit_.getCursor().ToggleIME();
}

//-------------------------------------------------------------------------
// ユーティリティー
//-------------------------------------------------------------------------

void GreenPadWnd::JumpToLine( ulong ln )
{
	edit_.getCursor().MoveCur( DPos(ln-1,0), false );
}

void GreenPadWnd::SetupSubMenu()
{
	if( HMENU m = getDocTypeSubMenu( hwnd() ) )
	{
		cfg_.SetDocTypeMenu( m, ID_CMD_DOCTYPE );
		::DrawMenuBar( hwnd() );
	}
}

void GreenPadWnd::GetTitleText( TCHAR *name )
{
	TCHAR *end = name+1;
	name[0] = TEXT('[');
	end = my_lstrkpy( end, isUntitled() ? TEXT("untitled") : filename_.name() );
	if( edit_.getDoc().isModified() )
		end = my_lstrkpy( end, TEXT(" *") );
	end = my_lstrkpy( end, TEXT("] - ") );
	app().LoadString(IDS_APPNAME, end, 32);
}

void GreenPadWnd::UpdateWindowName()
{
	// タイトルバーに表示される文字列の調整
	// [FileName *] - GreenPad
	{
		TCHAR name[1+MAX_PATH+6+32+1];
		GetTitleText( name );
		SetText( name );
	}

	// Try to show CP number in the StBar
	static TCHAR cpname[32];
	TCHAR tmp[INT_DIGITS+1];
	if( (UINT)csi_==0xffffffff )
	{	// Unknow cs
		stb_.SetCsText( TEXT("UNKN") );
	}
	else if((UINT)csi_ >= 0xf0f00000 && (UINT)csi_ < 0xf1000000)
	{	// cs number is specified
		cpname[0] = TEXT('C'); cpname[1] = TEXT('P');
		my_lstrkpy( cpname+2, Int2lStr(tmp, csi_ & 0xfffff) );
		stb_.SetCsText( cpname );
	}
	else if (0 <= csi_ && csi_ < (int)charSets_.size() )
	{	// Get cs name from charSets_ list
		TCHAR *end = my_lstrkpy(cpname, charSets_[csi_].shortName);
		*end++ = TEXT(' ');
		*end++ = TEXT('(');
		end = my_lstrkpy( end, Int2lStr(tmp, charSets_[csi_].ID) );
		*end++ = TEXT(')');
		*end = TEXT('\0');
		stb_.SetCsText( cpname );
	} else {
		// csi_ does not match any pattern.
		stb_.SetCsText( Int2lStr(cpname, csi_) );
	}
	stb_.SetLbText( lb_ );
}

void GreenPadWnd::SetupMRUMenu()
{
	int nmru=0;
	HMENU mparent = ::GetSubMenu(::GetMenu(hwnd()),0);
	if( HMENU m = ::GetSubMenu(mparent, 13) )
	{
		nmru = cfg_.SetUpMRUMenu( m, ID_CMD_MRU );
		::DrawMenuBar( hwnd() );
	}
	::EnableMenuItem(mparent, 13, MF_BYPOSITION|(nmru?MF_ENABLED:MF_GRAYED));
}

void GreenPadWnd::on_mru( int no )
{
	Path fn = cfg_.GetMRU(no);
	if( fn.len() != 0 )
		Open( fn, AutoDetect );
}



//-------------------------------------------------------------------------
// 設定更新処理
//-------------------------------------------------------------------------

void GreenPadWnd::ReloadConfig( bool noSetDocType )
{
	LOGGER("GreenPadWnd::ReloadConfig begin");
	// 文書タイプロード, document type
	if( !noSetDocType )
	{
		int t = cfg_.SetDocType( filename_ );
		if( HMENU m = getDocTypeSubMenu( hwnd() ) )
			cfg_.CheckMenu( m, t );
	}
	LOGGER("GreenPadWnd::ReloadConfig DocTypeLoaded");

	// Undo回数制限, limit undo
	edit_.getDoc().SetUndoLimit( cfg_.undoLimit() );

	wrap_ = cfg_.wrapType(); //       wt,    smart wrap,      line number,    Font...
	edit_.getView().SetWrapLNandFont( wrap_, cfg_.wrapSmart(), cfg_.showLN(), cfg_.vConfig(), cfg_.GetZoom() );
	LOGGER("GreenPadWnd::ReloadConfig ViewConfigLoaded");

	// キーワードファイル, keyword file
	Path kwd = cfg_.kwdFile();
	FileR fp;
	if( kwd.len()!=0 && kwd.isFile() && fp.Open(kwd.c_str()) )
		edit_.getDoc().SetKeyword(reinterpret_cast<const unicode*>(fp.base()),fp.size()/sizeof(unicode));
	else
		edit_.getDoc().SetKeyword(NULL,0);
	LOGGER("GreenPadWnd::ReloadConfig KeywordLoaded, end");
}



//-------------------------------------------------------------------------
// 開く処理
//-------------------------------------------------------------------------

bool GreenPadWnd::ShowOpenDlg( Path* fn, int* cs )
{
	// [Open][Cancel] 開くファイル名指定ダイアログを表示
	RzsString txtfiles(IDS_TXTFILES);
	RzsString allfiles(IDS_ALLFILES);
	const TCHAR *flst[] = {
		txtfiles.c_str(),
		cfg_.txtFileFilter().c_str(),
		allfiles.c_str(),
		TEXT("*.*")
	};
	aarr<TCHAR> filt = OpenFileDlg::ConnectWithNull(flst, countof(flst));

	OpenFileDlg ofd( charSets_, cfg_.useOldOpenSaveDlg() );
	bool ok = ofd.DoModal( hwnd(), filt.get(), filename_.c_str() );
	if( ok )
	{
		LOGGER( "GreenPadWnd::ShowOpenDlg ok" );
		*fn = ofd.filename();
		*cs = resolveCSI( ofd.csi() );
	}

	LOGGERF( TEXT("GreenPadWnd::ShowOpenDlg end, asked cs = %d"), (int)*cs );
	return ok;
}

bool GreenPadWnd::Open( const ki::Path& fn, int cs, bool always )
{
	if( isUntitled() && !edit_.getDoc().isModified() )
	{
		// 無題で無変更だったら自分で開く
		return OpenByMyself( fn, cs, true, always );
	}
	else
	{
		// 同じ窓で開くモードならそうする
		if( cfg_.openSame() )
			return ( AskToSave() ? OpenByMyself( fn, cs, true, true ) : true );

		// そうでなければ他へ回す
		String
			cmd  = TEXT("-c");
			cmd += SInt2Str( cs ).c_str();
			cmd += TEXT(" \"");
			cmd += fn;
			cmd += TEXT('\"');
		BootNewProcess( cmd.c_str() );
		return true;
	}
}
BOOL CALLBACK GreenPadWnd::PostMsgToFriendsProc(HWND hwnd, LPARAM lPmsg)
{
	TCHAR classn[256];
	if(::IsWindow(hwnd))
	{
		if( ::GetClassName(hwnd, classn, countof(classn))
		&&  !my_lstrcmp(classn, className_) )
			::PostMessage(hwnd, (UINT)lPmsg, 0, 0);
	}
	return TRUE; // Next hwnd
}
BOOL GreenPadWnd::PostMsgToAllFriends(UINT msg)
{
	return EnumWindows(PostMsgToFriendsProc, static_cast<LPARAM>(msg));
}
bool GreenPadWnd::OpenByMyself( const ki::Path& fn, int cs, bool needReConf, bool always )
{
	//MsgBox(fn.c_str(), TEXT("File:"), 0);
	LOGGERS( fn.c_str() );
	// ファイルを開けなかったらそこでおしまい。
	TextFileR tf(cs);

	if( !tf.Open( fn.c_str(), always ) )
	{
		// ERROR!
		int err = GetLastError();
		RzsString ids_oerr(IDS_OPENERROR);
		String fnerror = fn; fnerror+= TEXT('\n'); fnerror += ids_oerr.c_str();
		fnerror += SInt2Str(err).c_str();
		if( err == ERROR_ACCESS_DENIED )
		{
			if ( fn.isDirectory() )
			{ // We cannot open dir yet
				fnerror += RzsString(IDS_CANTOPENDIR).c_str(); // Can not open directory!
				MsgBox( fnerror.c_str(), ids_oerr.c_str(), MB_OK );
				return false;
			}
			// cannot open file for READ.
			// Directly try to open elevated.
			//fnerror += TEXT(": Access Denied\n\nTry to open elevated?");
			//if (IDYES == MsgBox( fnerror.c_str(), RzsString(IDS_OPENERROR).c_str(), MB_YESNO ))
			on_openelevated(fn);
			return false;
		}
		MsgBox( fnerror.c_str(), ids_oerr.c_str() );
		return false; // Failed to open.
	}
	else if ( app().getOSVer() >= 0x0500 )
	{
		// Check for Write access and Prompt the user
		// if he would like to elevate so the file can be written.
		// Detect netwrk path \\... \\?\UNC
		int drivestart=0;
		bool networkpath = false;;
		if( fn[0] == TEXT('\\') && fn[1] == TEXT('\\') )
		{
			if( fn[2] != '?' && fn[2] != '.' ) // \\server style
			{
				networkpath = true;
			}
			else if( fn[3] == '\\' ) // \\?\ style
			{
				if( fn[4] && fn[5] == ':') // \\?\X:
				{
					drivestart = 4;
					networkpath = false;
				}
				else if(
				    fn[7] == '\\'  // \\?\UNC\server style
				&& (fn[4] == 'U' || fn[4] == 'u')
				&& (fn[5] == 'N' || fn[5] == 'n')
				&& (fn[6] == 'C' || fn[6] == 'c') )
				{
					networkpath = true;
				}
			}
		}
		// Do not try to open the file for reading if it is a network
		// path, a netword drive or a CDROM.
		bool notry = networkpath;
		if (!notry)
		{ // Not a network path check if it is a network drive or cd
			TCHAR drive[4];
			drive[0] = fn[drivestart+0];
			drive[1] = fn[drivestart+1];
			drive[2] = fn[drivestart+2];
			drive[3] = TEXT('\0');
			UINT DT = GetDriveType(drive);
			notry = !(DT&DRIVE_REMOTE) || !(DT&DRIVE_CDROM);
		}

		TextFileW tfw( cs, lb_ );
		if (!notry && !tfw.Open( fn.c_str() ) && GetLastError() == ERROR_ACCESS_DENIED )
		{
			if( !networkpath )
			{
				String fnerror = fn; fnerror += RzsString(IDS_NOWRITEACESS).c_str();
				if ( IDYES == MsgBox( fnerror.c_str(), RzsString(IDS_OPENERROR).c_str(), MB_YESNO ) )
				{
					on_openelevated(fn);
					return false;
				}
			}
		}
	}


	// 自分内部の管理情報を更新, Update internal management information
	if( fn[0]==TEXT('\\') || (fn[0] && fn[1]==TEXT(':')) )
		// Absolute path: '\file', 'x:\file', '\\share\file', '\\?\...', 'c:\\file' etc.
		filename_ = fn;
	else
		filename_ = Path( Path::Cur ) + fn;
	if( tf.size() )
	{
		csi_      = charSets_.findCsi( tf.codepage() );
		if( (UINT)csi_ == 0xffffffff )
			csi_       = 0xf0f00000 | tf.codepage();

		if( tf.nolb_found() )
			lb_       = cfg_.GetNewfileLB();
		else
			lb_       = tf.linebreak();
	}
	else
	{ // 空ファイルの場合は新規作成と同じ扱い
	  // If the file is empty, it is treated as if it were newly created.
		csi_      = cfg_.GetNewfileCsi();
		lb_       = cfg_.GetNewfileLB();
	}
	filename_.BeShortLongStyle();

	// カレントディレクトリを、ファイルのある位置以外にしておく
	// （こうしないと、開いているファイルのあるディレクトリが削除できない）
	// Make sure the current directory is somewhere other than where the file is located.
	// (otherwise the directory with the open file cannot be deleted)
	::SetCurrentDirectory( Path(filename_).BeDriveOnly().c_str() );

	// 文書タイプに応じて表示を更新
	// Update display according to document type
	if( needReConf )
		ReloadConfig();

	// 開く
	edit_.getDoc().ClearAll();
	stb_.SetText( RzsString(IDS_LOADING).c_str() );
	old_filetime_ = filename_.getLastWriteTime(); // Save timestamp
	//::EnableWindow(edit_.hwnd(), FALSE);
	edit_.getDoc().OpenFile( tf );
	//::EnableWindow(edit_.hwnd(), TRUE);
	stb_.SetText( TEXT("(1,1)") );

	// タイトルバー更新
	UpdateWindowName();

	// [最近使ったファイル]へ追加
	if( cfg_.AddMRU( filename_ ) )
		PostMsgToAllFriends(GPM_MRUCHANGED);

	return true;
}



//-------------------------------------------------------------------------
// 保存処理
//-------------------------------------------------------------------------

bool GreenPadWnd::ShowSaveDlg()
{
	// [Save][Cancel] 保存先ファイル名指定ダイアログを表示
	RzsString allfiles(IDS_ALLFILES);
	const TCHAR *flst[] = {
		allfiles.c_str(),
		TEXT("*.*")
	};
	aarr<TCHAR> filt = SaveFileDlg::ConnectWithNull( flst, countof(flst) );

	SaveFileDlg sfd( charSets_, csi_, lb_, cfg_.useOldOpenSaveDlg() );
	StatusBar::SaveRestoreText SaveRest(stb_);
	stb_.SetText( TEXT("Saving file...") );
	if( !sfd.DoModal( hwnd(), filt.get(), filename_.c_str() ) )
		return false;

	const int csi = sfd.csi();
	bool invalidCS = false;
	if( (UINT)csi == 0xffffffff )
		invalidCS = true;
	else if( (UINT)csi >= 0xf0f00000 && (UINT)csi < 0xf1000000 )
	{
		int neededcs = TextFileR::neededCodepage( resolveCSI(csi) );
		// neededcs i 0 in case it is internaly handled.
		invalidCS = neededcs != 0 && !::IsValidCodePage( neededcs );
	}
	if( invalidCS )
	{
		MsgBox( RzsString(IDS_INVALIDCP).c_str(), NULL, MB_OK);
		return false; // Fail if selected codepage is invalid.
	}

	filename_ = sfd.filename();
	csi_      = sfd.csi();
	lb_       = sfd.lb();

	return true;
}

bool GreenPadWnd::Save_showDlgIfNeeded()
{
	bool wasUntitled = isUntitled();

	// [Save][Cancel] ファイル名未定ならダイアログ表示
	if( isUntitled() )
		if( !ShowSaveDlg() )
			return false;
	if( Save() )
	{
		if( wasUntitled )
			ReloadConfig(); // 文書タイプに応じて表示を更新
		return true;
	}
	return false;
}

bool GreenPadWnd::AskToSave()
{
	// 変更されていたら、
	// [Yes][No][Cancel] 保存するかどうか尋ねる。
	// 保存するなら
	// [Save][Cancel]    ファイル名未定ならダイアログ表示

	if( edit_.getDoc().isModified() )
	{
		int answer = MsgBox(
			RzsString(IDS_ASKTOSAVE).c_str(),
			RzsString(IDS_APPNAME).c_str(),
			MB_YESNOCANCEL|MB_ICONQUESTION
		);
		if( answer == IDYES )    return Save_showDlgIfNeeded();
		if( answer == IDCANCEL ) return false;
	}
	return true;
}

bool GreenPadWnd::Save()
{
	int save_Csi;

	if((UINT)csi_ == 0xffffffff)
		save_Csi = ::GetACP();
	else
		save_Csi = resolveCSI(csi_);

	if (!save_Csi) save_Csi = ::GetACP(); // in case

	{ // Reduce scope of tf
		TextFileW tf( save_Csi, lb_ );

		if( !tf.Open( filename_.c_str() ) )
		{
			// Error!
			DWORD err = GetLastError();
			String fnerror = filename_ + TEXT("\n\nError #");
			fnerror += SInt2Str(err).c_str();
			MsgBox( fnerror.c_str(), RzsString(IDS_SAVEERROR).c_str() );
			return false;
		}
		// 無事ファイルに保存できた場合
		edit_.getDoc().SaveFile( tf );
	}
	UpdateWindowName();
	// [最近使ったファイル]更新
	if( cfg_.AddMRU( filename_ ) )
		PostMsgToAllFriends(GPM_MRUCHANGED);

	old_filetime_ = filename_.getLastWriteTime(); // Save timestamp
	return true;
}



//-------------------------------------------------------------------------
// メインウインドウの初期化
//-------------------------------------------------------------------------

GreenPadWnd::ClsName GreenPadWnd::className_ = TEXT("GreenPad MainWnd");

GreenPadWnd::GreenPadWnd()
	#ifndef FORCE_RTL_LAYOUT
	: WndImpl  ( className_, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES)
	#else
	: WndImpl  ( className_, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES|WS_EX_LAYOUTRTL )
	#endif
	, search_  ( *this, edit_ )
	, charSets_( cfg_.GetCharSetList() )
	, old_cur_ ( DPos(0,0) )
	, old_sel_ ( DPos(0,0) )
	, csi_     ( cfg_.GetNewfileCsi() )
	, lb_      ( cfg_.GetNewfileLB() )
	, wrap_    ( -1 )
{
	LOGGER( "GreenPadWnd::Construct begin" );

	static WNDCLASS wc;
	wc.style         = 0; //CS_HREDRAW|CS_VREDRAW;
	wc.hIcon         = app().LoadIcon( IDR_MAIN );
	wc.hCursor       = app().LoadOemCursor( IDC_ARROW );
	wc.lpszMenuName  = MAKEINTRESOURCE( IDR_MAIN );
	wc.lpszClassName = className_;
	WndImpl::Register( &wc );
#ifndef NO_IME
	ime().EnableGlobalIME( true );
#endif
	LOGGER( "GreenPadWnd::Construct end" );
}

void GreenPadWnd::on_create( CREATESTRUCT* cs )
{
	LOGGER("GreenPadWnd::on_create begin");

	accel_ = app().LoadAccel( IDR_MAIN );
	edit_.Create( NULL, hwnd(), 0, 0, 100, 100 );
	LOGGER("GreenPadWnd::on_create edit created");
	edit_.getDoc().AddHandler( this );
	edit_.getCursor().AddHandler( this );
	// Create status bar
	stb_.SetParent(hwnd()); // Only if it must be shown
	stb_.SetStatusBarVisible( cfg_.showStatusBar() );

	LOGGER("GreenPadWnd::on_create halfway");

	search_.LoadFromINI();
	SetupSubMenu();
	SetupMRUMenu();

	LOGGER("GreenPadWnd::on_create menu");
}

bool GreenPadWnd::StartUp( const Path& fn, int cs, int ln )
{
	LOGGER( "GreenPadWnd::StartUp begin" );
	Create( 0, 0, cfg_.GetWndX(), cfg_.GetWndY(), cfg_.GetWndW(), cfg_.GetWndH(), 0 );
	ShowUp2(); LOGGER( "showup!" );
	LOGGER( "GreenPadWnd::Created" );
	if( fn.len()==0 || !OpenByMyself( fn, cs ) )
	{
		LOGGER( "for new file..." );

		// ファイルを開か(け)なかった場合
		ReloadConfig( fn.len()==0 );
		LOGGER( "GreenPadWnd::StartUp reloadconfig end" );
		UpdateWindowName();
		stb_.SetText( TEXT("(1,1)") );
		LOGGER( "GreenPadWnd::StartUp updatewindowname end" );
	}
	stb_.SetZoom( cfg_.GetZoom() );

	// 指定の行へジャンプ
	if( ln != -1 )
		JumpToLine( ln );

	LOGGER( "GreenPadWnd::StartUp end" );
	return true;
}

void GreenPadWnd::ShowUp2()
{
	Window::ShowUp( cfg_.GetWndM() ? SW_MAXIMIZE : SW_SHOW );
}


//-------------------------------------------------------------------------
// スタートアップルーチン
//	コマンドラインの解析を行う
//-------------------------------------------------------------------------

int kmain()
{
	// MsgBox(GetCommandLine(), TEXT("Command Line"), MB_OK);
	LOGGER( "kmain() begin" );
	GreenPadWnd wnd;
	{
		Argv  arg;
		ulong   i;
		int optL = -1;
		int optC = 0;

		LOGGER( "argv processed" );

	  //-- まずオプションスイッチを処理

		for( i=1; i<arg.size() && arg[i][0]==TEXT('-'); ++i )
			switch( arg[i][1] )
			{
			case TEXT('c'):
				optC = String::GetInt( arg[i]+2 );
				break;
			case TEXT('l'):
				optL = String::GetInt( arg[i]+2 );
				break;
			}

		LOGGER( "option processed" );

	  //-- 次にファイル名
		Path file;
		if( i < arg.size() )
		{
			file = arg[i];
			if( !file.isFile() )
			{
				ulong j; // ""無しで半スペ入りでもそれなりに対処
				for( j=i+1; j<arg.size(); ++j )
				{
					file += ' ';
					file += arg[j];
					if( file.isFile() )
						break;
				}

				if( j==arg.size() )
					file = arg[i];
				else
					i=j;
			}
		}

		LOGGER( "filename processed" );

	  //-- 余ってる引数があれば、それで新規プロセス起動

		if( ++i < arg.size() )
		{
			String cmd;
			for( ; i<arg.size(); ++i )
			{
				cmd += TEXT('\"');
				cmd += arg[i];
				cmd += TEXT("\" ");
			}
			::BootNewProcess( cmd.c_str() );
		}

		LOGGER( "newprocess booted" );

	  //-- メインウインドウ発進
		if( !wnd.StartUp(file, optC, optL) )
			return -1;

		TS.reset();
	}

  //-- メインループ

	LOGGER( "kmain() startup ok" );

	wnd.MsgLoop();

	LOGGER( "fin" );
	return 0;
}
