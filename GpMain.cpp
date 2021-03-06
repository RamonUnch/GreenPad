#include "stdafx.h"
#include "rsrc/resource.h"
#include "GpMain.h"
using namespace ki;
using namespace editwing;



//-------------------------------------------------------------------------
// 新規プロセス起動
//-------------------------------------------------------------------------

void BootNewProcess( const TCHAR* cmd = TEXT("") )
{
	STARTUPINFO         sti;
	PROCESS_INFORMATION psi;
	::GetStartupInfo( &sti );

	// On Windows NT3.x/Win9x we cannot have the exe name between ""
	// On NT5 it seems ok to use "exe name.exe" "file name"
	// Otherwise we try SHORT/NAME.EXE "file name"
	// I do not know why the heck it is like this.
	bool quotedexe = app().getOSVer() >= 500 && app().isNT();
	String fcmd;
	if( quotedexe ) fcmd = TEXT("\"");
	fcmd += quotedexe? Path(Path::ExeName): Path(Path::ExeName).BeShortStyle();
	if( quotedexe ) fcmd += TEXT("\" ");
	else fcmd += TEXT(" ");
	fcmd += cmd;
	// MessageBox(NULL, (TCHAR*)fcmd.c_str(), (TCHAR*)Path(Path::ExeName).c_str(), MB_OK);

	if( ::CreateProcess( NULL, (TCHAR*)fcmd.c_str(),
			NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
			&sti, &psi ) )
	{
		::CloseHandle( psi.hThread );
		::CloseHandle( psi.hProcess );
	}
}



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
	SetText( str_=str, 1 );
}

inline void GpStBar::SetLbText( int lb )
{
	// 改行コード表示領域にSetTextする
	static const TCHAR* const lbstr[] = {TEXT("CR"),TEXT("LF"),TEXT("CRLF")};
	SetText( lbstr[lb_=lb], 2 );
}

int GpStBar::AutoResize( bool maximized )
{
	// 文字コード表示領域を確保しつつリサイズ
	int h = StatusBar::AutoResize( maximized );
	int w[] = { width()-5, width()-5, width()-5 };

	HDC dc = ::GetDC( hwnd() );
	SIZE s;
	if( ::GetTextExtentPoint( dc, TEXT("BBBBM"), 5, &s ) ) // Line Ending
		w[1] = w[2] - s.cx;
	if( ::GetTextExtentPoint( dc, TEXT("BBBWWWW"), 7, &s ) ) // Charset
		w[0] = w[1] - s.cx;

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

	// サイズ変更。子窓を適当に移動。
	case WM_SIZE:
		if( wp==SIZE_MAXIMIZED || wp==SIZE_RESTORED )
		{
			int ht = stb_.AutoResize( wp==SIZE_MAXIMIZED );
			edit_.MoveTo( 0, 0, LOWORD(lp), HIWORD(lp)-ht );
			cfg_.RememberWnd(this);
		}
		break;

	// ウインドウ移動
	case WM_MOVE:
		{
			RECT rc;
			getPos(&rc);
			cfg_.RememberWnd(this);
		}
		break;

	case WM_NCCALCSIZE:{
		// Handle WM_NCCALCSIZE to avoid ugly resizing
		int ret = WndImpl::on_message( msg, wp, lp );
		NCCALCSIZE_PARAMS *nc = (NCCALCSIZE_PARAMS *)lp;
		RECT wnd;
		GetWindowRect(hwnd(), &wnd);
		if( wp && (wnd.left != nc->lppos->x || wnd.top != nc->lppos->y))
		{ // Resized from the top or left (or both)
			// Window will BitBlt between those two rects:
			CopyRect(&nc->rgrc[1], &wnd); // Destination
			CopyRect(&nc->rgrc[2], &wnd); // Source
			POINT pt = { 0, 0 };
			ClientToScreen(hwnd(), &pt); // client coord
			pt.x -= wnd.left;
			pt.y -= wnd.top;

			// Adjust rects so that it does not include SB nor scrollbars.
			nc->rgrc[2].right  -= Max((long)24, (long)((wnd.right-wnd.left) - nc->lppos->cx+24));
			nc->rgrc[2].bottom -= Max((long)45, (long)((wnd.bottom-wnd.top) - nc->lppos->cy+45));

			// Do not include caption+menu in BitBlt
			nc->rgrc[1].left = nc->lppos->x + pt.x;
			nc->rgrc[1].top  = nc->lppos->y + pt.y;
			nc->rgrc[2].top  += pt.y;
			nc->rgrc[2].left += pt.x;

			return WVR_VALIDRECTS;
		}
		return ret;
		}break;

	case WM_ERASEBKGND:{
		// Uncomment to see in black the area that will be repainted
//		Sleep(200);
//		RECT rc;
//		GetClientRect(hwnd(), &rc);
//		FillRect((HDC)wp, &rc,  (HBRUSH)GetStockObject(BLACK_BRUSH));
//		Sleep(200);
		return 1;
		}break;

	// システムコマンド。終了ボタンとか。
	case WM_SYSCOMMAND:
		if( wp==SC_CLOSE || wp==SC_DEFAULT )
			on_exit();
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	// 右クリックメニュー
	case WM_CONTEXTMENU:
		if( reinterpret_cast<HWND>(wp) == edit_.hwnd() )
			::TrackPopupMenu(
				::GetSubMenu( ::GetMenu(hwnd()), 1 ), // 編集メニュー表示
				GetSystemMetrics(SM_MENUDROPALIGNMENT)|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
				static_cast<SHORT>(LOWORD(lp)), static_cast<SHORT>(HIWORD(lp)), 0, hwnd(), NULL );
		else
			return WndImpl::on_message( msg, wp, lp );
		break;

	// メニューのグレーアウト処理
	case WM_INITMENU:
	case WM_INITMENUPOPUP:
		on_initmenu( reinterpret_cast<HMENU>(wp), msg==WM_INITMENUPOPUP );
		break;

	// Ｄ＆Ｄ
	case WM_DROPFILES:
		on_drop( reinterpret_cast<HDROP>(wp) );
		break;

	// MRU
	case GPM_MRUCHANGED:
		SetupMRUMenu();
		break;

	// NOTIFY
	case WM_NOTIFY:
		if( wp == 1787 // Status Bar ID to check before[]
		&& ((NMHDR*)lp)->code == NM_DBLCLK )
			on_reopenfile();
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
	case ID_CMD_DELETE: if( edit_.getCursor().isSelected() ){ edit_.getCursor().Del();} break;
	case ID_CMD_SELECTALL:  edit_.getCursor().Home(true,false);
	                        edit_.getCursor().End(true,true);   break;
	case ID_CMD_DATETIME:   on_datetime();                      break;
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
	case ID_CMD_QUOTE:      edit_.getCursor().QuoteSelection(false);break;
	case ID_CMD_UNQUOTE:    edit_.getCursor().QuoteSelection(true); break;

	// Search
	case ID_CMD_FIND:       search_.ShowDlg();  break;
	case ID_CMD_FINDNEXT:   search_.FindNext(); break;
	case ID_CMD_FINDPREV:   search_.FindPrev(); break;
	case ID_CMD_JUMP:       on_jump(); break;
	case ID_CMD_GREP:       on_grep();break;

	// View
	case ID_CMD_NOWRAP:     edit_.getView().SetWrapType( wrap_=-1 ); break;
	case ID_CMD_WRAPWIDTH:  edit_.getView().SetWrapType( wrap_=cfg_.wrapWidth() ); break;
	case ID_CMD_WRAPWINDOW: edit_.getView().SetWrapType( wrap_=0 ); break;
	case ID_CMD_CONFIG:     on_config();    break;
	case ID_CMD_STATUSBAR:  on_statusBar(); break;

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
	int  cs;
	if( ShowOpenDlg( &fn, &cs ) )
		Open( fn, cs, true );
}

void GreenPadWnd::on_reopenfile()
{
	if( !isUntitled() )
	{
		ReopenDlg dlg( charSets_, csi_ );
		dlg.GoModal( hwnd() );
		if( dlg.endcode()==IDOK && AskToSave() )
		{
			// if csi > F0000 it means it is equal to the desized CP+0x100000
			int csi = dlg.csi();
			int cp = (csi > 0xF0000)? csi-0x100000: charSets_[csi].ID;
			OpenByMyself( filename_, cp, false );
		}
	}
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
		OpenByMyself(filename_, charSets_[csi_].ID, false);
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
void GreenPadWnd::on_pagesetup()
{
#if UNICOWS || !defined(TARGET_VER) ||  defined(TARGET_VER) && TARGET_VER>=351
	if (app().is351p())
	{
		PAGESETUPDLG psd;
		mem00(&psd, sizeof(psd));
		psd.lStructSize = sizeof(psd);
		// FIXME: use local units...
		psd.Flags = PSD_INTHOUSANDTHSOFINCHES|PSD_DISABLEORIENTATION|PSD_DISABLEPAPER|PSD_DISABLEPRINTER|PSD_MARGINS;
		psd.hwndOwner = hwnd();
		CopyRect(&psd.rtMargin, cfg_.PMargins());
		PageSetupDlg(&psd);
		cfg_.SetPrintMargins(&psd.rtMargin);
	}
	else
	{
		MsgBox(TEXT("You need at least Windows NT 3.51!\n"), NULL, MB_OK);
	}
#endif
}
void GreenPadWnd::on_print()
{
	doc::Document& d = edit_.getDoc();
	const unicode* buf;
	ulong dpStart = 0, len = 0;
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
	// FileName * - GreenPad
	String name;
	name += isUntitled() ? TEXT("untitled") : filename_.name();
	if( edit_.getDoc().isModified() ) name += TEXT(" *");
	name += TEXT(" - ");
	name += String(IDS_APPNAME).c_str();

	// Set DOCINFO structure
	DOCINFO di = { sizeof(DOCINFO) };
	di.lpszDocName = name.c_str();
	di.lpszOutput = (LPTSTR) NULL;
	di.lpszDatatype = (LPTSTR) NULL;
	di.fwType = 0;

	int nError = ::StartDoc(thePrintDlg.hDC, &di);
	if (nError == SP_ERROR)
	{
		TCHAR tmp[128];
		::wsprintf(tmp,TEXT("StartDoc Error #%d - please check printer."),::GetLastError());
		MsgBox(tmp, String(IDS_APPNAME).c_str(), MB_OK|MB_TASKMODAL );
		return;
		// Handle the error intelligently
	}
	// Setup printing font.
	LOGFONT lf;
	lf.lfEscapement     = cfg_.vConfig().font.lfEscapement;
	lf.lfOrientation    = cfg_.vConfig().font.lfOrientation;
	lf.lfWeight         = cfg_.vConfig().font.lfWeight;
	lf.lfItalic         = cfg_.vConfig().font.lfItalic;
	lf.lfUnderline      = cfg_.vConfig().font.lfUnderline;
	lf.lfStrikeOut      = cfg_.vConfig().font.lfStrikeOut;
	lf.lfCharSet        = cfg_.vConfig().font.lfCharSet;
	lf.lfOutPrecision   = cfg_.vConfig().font.lfOutPrecision;
	lf.lfClipPrecision  = cfg_.vConfig().font.lfClipPrecision;
	lf.lfQuality        = cfg_.vConfig().font.lfQuality;
	lf.lfPitchAndFamily = cfg_.vConfig().font.lfPitchAndFamily;
	my_lstrcpy(lf.lfFaceName, cfg_.vConfig().font.lfFaceName);
	SetFontSize(&lf, thePrintDlg.hDC, cfg_.vConfig().fontsize, cfg_.vConfig().fontwidth);
	HFONT printfont = ::CreateFontIndirect(&lf);
	::SelectObject( thePrintDlg.hDC, printfont );

	::StartPage(thePrintDlg.hDC);

	// Get Printer Caps
	int cWidthPels, cHeightPels, cLineHeight;
	cWidthPels = ::GetDeviceCaps(thePrintDlg.hDC, HORZRES); // px
	cHeightPels = ::GetDeviceCaps(thePrintDlg.hDC, VERTRES);// px
	int logpxx = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSX);// px/in
	int logpxy = GetDeviceCaps(thePrintDlg.hDC, LOGPIXELSY);// px/in

	// Get Line height
	RECT rctmp;
	rctmp.left = 0;
	rctmp.top = 0;
	rctmp.right = cWidthPels;
	rctmp.bottom = cHeightPels;
	::DrawTextW(thePrintDlg.hDC, L"#", 1, &rctmp, DT_CALCRECT|DT_LEFT|DT_WORDBREAK|DT_EXPANDTABS|DT_EDITCONTROL);
	cLineHeight = rctmp.bottom-rctmp.top;

	RECT rcMargins;
	// Convert config margins in Inches to pixels
	rcMargins.left   = (cfg_.PMargins()->left  * logpxx)/1000;
	rcMargins.top    = (cfg_.PMargins()->top   * logpxy)/1000;
	rcMargins.right  = (cfg_.PMargins()->right * logpxx)/1000;
	rcMargins.bottom = (cfg_.PMargins()->left  * logpxy)/1000;

	RECT rcPrinter = { rcMargins.left
				, rcMargins.top
				, cWidthPels - rcMargins.left - rcMargins.right
				, cHeightPels - rcMargins.top - rcMargins.bottom };

	int nThisLineHeight, nChars = 0, nHi = 0, nLo = 0;
	const unicode* uStart;

	// Process with multiple copies
	do {
		if(procCopies)
		{
			::StartPage(thePrintDlg.hDC);
			rcPrinter.top = rcMargins.top;
			rcPrinter.left = rcMargins.left;
			rcPrinter.right = cWidthPels   - (rcMargins.left+rcMargins.right);
			rcPrinter.bottom = cHeightPels - (rcMargins.top+rcMargins.bottom);
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
				rctmp = rcPrinter;
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
					nThisLineHeight = ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rctmp
											, DT_CALCRECT|DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX|DT_EDITCONTROL);
					if (rcPrinter.top+nThisLineHeight < rcPrinter.bottom)
						nLo = nChars;
					if (rcPrinter.top+nThisLineHeight > rcPrinter.bottom)
						nHi = nChars;
					if (nLo == nHi - 1)
						nChars = nHi = nLo;
					if (nLo < nHi)
						nChars = nLo + (nHi - nLo)/2;
				}
				rcPrinter.top += ::DrawTextW(thePrintDlg.hDC, uStart, nChars, &rcPrinter, DT_WORDBREAK|DT_NOCLIP|DT_EXPANDTABS|DT_NOPREFIX|DT_EDITCONTROL);
				if(uStart+nChars == buf+len) // Line end
				{
					nChars = 0;
					++dpStart;
				}
			}

			// turn to new page
			if( (dpStart<e) && (rcPrinter.top + cLineHeight + 5 > rcPrinter.bottom) )
			{
				::EndPage(thePrintDlg.hDC);
				::StartPage(thePrintDlg.hDC);
				rcPrinter.top = rcMargins.top;
				rcPrinter.left = rcMargins.left;
				rcPrinter.right = cWidthPels   - (rcMargins.left+rcMargins.right);
				rcPrinter.bottom = cHeightPels - (rcMargins.top+rcMargins.bottom);
			}
		}
		// EndPage Does not reste page attributes on NT/9x
		::EndPage(thePrintDlg.hDC);
	} while(++procCopies < totalCopies);

	// Close Printer
	::DeleteObject(printfont);
	::EndDoc(thePrintDlg.hDC);
	::DeleteDC(thePrintDlg.hDC);


	::GlobalUnlock(thePrintDlg.hDevNames);
	::GlobalUnlock(thePrintDlg.hDevMode);

	// 解放する。
	::GlobalFree(thePrintDlg.hDevNames);
	::GlobalFree(thePrintDlg.hDevMode);

}

void GreenPadWnd::on_exit()
{
	search_.SaveToINI( cfg_.getImpl() );
	if( AskToSave() )
		Destroy();
}

void GreenPadWnd::on_initmenu( HMENU menu, bool editmenu_only )
{
	::EnableMenuItem( menu, ID_CMD_CUT, MF_BYCOMMAND|(edit_.getCursor().isSelected() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_COPY, MF_BYCOMMAND|(edit_.getCursor().isSelected() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_DELETE, MF_BYCOMMAND|(edit_.getCursor().isSelected() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_UNDO, MF_BYCOMMAND|(edit_.getDoc().isUndoAble() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_REDO, MF_BYCOMMAND|(edit_.getDoc().isRedoAble() ? MF_ENABLED : MF_GRAYED) );
#ifndef NO_IME
	::EnableMenuItem( menu, ID_CMD_RECONV, MF_BYCOMMAND|(edit_.getCursor().isSelected() && ime().IsIME() && ime().CanReconv() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_TOGGLEIME, MF_BYCOMMAND|(ime().IsIME() ? MF_ENABLED : MF_GRAYED) );
#endif
	if( editmenu_only )
	{
		LOGGER("GreenPadWnd::ReloadConfig on_initmenu end");
		return;
	}

	::EnableMenuItem( menu, ID_CMD_SAVEFILE, MF_BYCOMMAND|(isUntitled() || edit_.getDoc().isModified() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_REOPENFILE, MF_BYCOMMAND|(!isUntitled() ? MF_ENABLED : MF_GRAYED) );
	::EnableMenuItem( menu, ID_CMD_GREP, MF_BYCOMMAND|(cfg_.grepExe().len()>0 ? MF_ENABLED : MF_GRAYED) );

	::CheckMenuItem( menu, ID_CMD_NOWRAP, MF_BYCOMMAND|(wrap_==-1?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWIDTH, MF_BYCOMMAND|(wrap_>0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_WRAPWINDOW, MF_BYCOMMAND|(wrap_==0?MF_CHECKED:MF_UNCHECKED));
	::CheckMenuItem( menu, ID_CMD_STATUSBAR, cfg_.showStatusBar()?MF_CHECKED:MF_UNCHECKED );

	LOGGER("GreenPadWnd::ReloadConfig on_initmenu end");
}

void GreenPadWnd::on_drop( HDROP hd )
{
	UINT iMax = ::DragQueryFile( hd, 0xffffffff, NULL, 0 );
	for( UINT i=0; i<iMax; ++i )
	{
		// Get length of the i string for array size.
		UINT len = ::DragQueryFile( hd, i, NULL, 0)+1;
		len = Max(len, (UINT)MAX_PATH); // ^ the Above may fail on NT3.1
		TCHAR *str = new TCHAR [len];
		::DragQueryFile( hd, i, str, len );
		Open( str, AutoDetect );
        delete [] str;
	}
	::DragFinish( hd );
}

void GreenPadWnd::on_jump()
{
	struct JumpDlg : public DlgImpl {
		JumpDlg(HWND w) : DlgImpl(IDD_JUMP), w_(w) { GoModal(w); }
		void on_init() {
			SetCenter(hwnd(),w_); ::SetFocus(item(IDC_LINEBOX)); }
		bool on_ok() {
			TCHAR str[100];
			::GetWindowText( item(IDC_LINEBOX), str, countof(str) );
			LineNo = String(str).GetInt();
			return true;
		}
		int LineNo; HWND w_;
	} dlg(hwnd());

	if( IDOK == dlg.endcode() )
		JumpToLine( dlg.LineNo );
}

void GreenPadWnd::on_grep()
{
	Path g = cfg_.grepExe();
	if( g.len() != 0 )
	{
		Path d;
		if( filename_.len() )
			(d = filename_).BeDirOnly().BeBackSlash(false);
		else
			d = Path(Path::Cur);

		String fcmd;
		for( int i=0, e=g.len(); i<e; ++i )
			if( g[i]==TEXT('%') )
			{
				if( g[i+1]==TEXT('1') || g[i+1]==TEXT('D') ) // '1' for bkwd compat
					++i, fcmd += d;
				else if( g[i+1]==TEXT('F') )
					++i, fcmd += filename_;
				else if( g[i+1]==TEXT('N') )
					++i, fcmd += filename_.name();
			}
			else
				fcmd += g[i];

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
	TCHAR buf[255], tmp[255];
	::GetTimeFormat
		( LOCALE_USER_DEFAULT, 0, NULL, g.len()?const_cast<TCHAR*>(g.c_str()):TEXT("HH:mm yyyy/MM/dd"), buf, countof(buf));
	::GetDateFormat
		( LOCALE_USER_DEFAULT, 0, NULL, buf, tmp,countof(tmp));
	edit_.getCursor().Input( tmp, my_lstrlen(tmp) );
}

void GreenPadWnd::on_doctype( int no )
{
	if( HMENU m = ::GetSubMenu( ::GetSubMenu(::GetMenu(hwnd()),3),4 ) )
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
	struct MyFindWindowExstruct *param=(MyFindWindowExstruct *)lParam;
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
	if (app().isNewShell()) {
		// In UNICOWS mode FindWindowEx is dynamically
		// imported and is implemented for 95/NT4+
		return FindWindowEx(parent, after, lpszClass, lpszWindow);
	}
  # endif
	// Fallback to the WIP FindWindowEx re-implementation.
	struct MyFindWindowExstruct param = {after, lpszClass, lpszWindow, NULL};
	EnumChildWindows(parent, MyFindWindowExProc, (LPARAM)&param);
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
		HWND last=next, pos;
		while( last != NULL )
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
	if( next==hwnd() )
	{
		while( next != NULL )
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( pos!=hwnd())
			MyShowWnd( pos );
	}
	else
	{
		while( next!=hwnd() && next!=NULL )
			next = ::MyFindWindowEx( NULL,pos=next,className_,NULL );
		if( next!=NULL )
			MyShowWnd( pos );
	}
}

void GreenPadWnd::on_statusBar()
{
	stb_.SetStatusBarVisible( !stb_.isStatusBarVisible() );
	cfg_.ShowStatusBarSwitch();

	WINDOWPLACEMENT wp = {sizeof(wp)};
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
	if( edit_.getDoc().isBusy() && ((++busy_cnt)&0xff) )
		return;

	ulong cad = c.ad;
	if( ! cfg_.countByUnicode() )
	{
		// ShiftJIS風のByte数カウント
		const unicode* cu = edit_.getDoc().tl(c.tl);
		const ulong tab = NZero(cfg_.vConfig().tabstep);
		cad = 0;
		for( ulong i=0; i<c.ad; ++i )
			if( cu[i] == L'\t' )
				cad = (cad/tab+1)*tab;
			else if( cu[i]<0x80 || (0xff60<=cu[i] && cu[i]<=0xff9f) )
				cad = cad + 1;
			else
				cad = cad + 2;
	}

	String str;
	str += TEXT('(');
	str += String().SetInt(c.tl+1);
	str += TEXT(',');
	str += String().SetInt(cad+1);
	str += TEXT(')');
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
		str += TEXT(" - (");
		str += String().SetInt(s.tl+1);
		str += TEXT(',');
		str += String().SetInt(sad+1);
		str += TEXT(')');
	}
	stb_.SetText( str.c_str() );
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
	if( HMENU m = ::GetSubMenu( ::GetSubMenu(::GetMenu(hwnd()),3),4 ) )
	{
		cfg_.SetDocTypeMenu( m, ID_CMD_DOCTYPE );
		::DrawMenuBar( hwnd() );
	}
}

void GreenPadWnd::UpdateWindowName()
{
	// タイトルバーに表示される文字列の調整
	// [FileName *] - GreenPad
	String name;
	name += TEXT('[');
	name += isUntitled() ? TEXT("untitled") : filename_.name();
	if( edit_.getDoc().isModified() ) name += TEXT(" *");
	name += TEXT("] - ");
	name += String(IDS_APPNAME).c_str();

	SetText( name.c_str() );
	// Try to show CP number in the StBar
	if(csi_ >= 0xf0f00000 && csi_ < 0xf1000000) {
		TCHAR cpname[10];
		::wsprintf(cpname,TEXT("CP%d"), csi_ & 0xfffff);
		stb_.SetCsText( cpname );
	} else {
		stb_.SetCsText( csi_==0xffffffff?TEXT("UNKN"):charSets_[csi_].shortName );
	}
	stb_.SetLbText( lb_ );
}

void GreenPadWnd::SetupMRUMenu()
{
	int nmru=0;
	HMENU mparent = ::GetSubMenu(::GetMenu(hwnd()),0);
	if( HMENU m = ::GetSubMenu(mparent, 12) )
	{
		nmru = cfg_.SetUpMRUMenu( m, ID_CMD_MRU );
		::DrawMenuBar( hwnd() );
	}
	::EnableMenuItem(mparent, 12, MF_BYPOSITION|(nmru?MF_ENABLED:MF_GRAYED));
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
	// 文書タイプロード, document type
	if( !noSetDocType )
	{
		int t = cfg_.SetDocType( filename_ );
		if( HMENU m = ::GetSubMenu( ::GetSubMenu(::GetMenu(hwnd()),3),4 ) )
			cfg_.CheckMenu( m, t );
	}
	LOGGER("GreenPadWnd::ReloadConfig DocTypeLoaded");

	// Undo回数制限, limit undo
	edit_.getDoc().SetUndoLimit( cfg_.undoLimit() );

	// 行番号, line numbers
	bool ln = cfg_.showLN();
	edit_.getView().ShowLineNo( ln );

	// 折り返し方式, warp method
	wrap_ = cfg_.wrapType();
	edit_.getView().SetWrapType( wrap_ );

	// 色・フォント, colors and font
	VConfig vc = cfg_.vConfig();
	edit_.getView().SetFont( vc );
	LOGGER("GreenPadWnd::ReloadConfig ViewConfigLoaded");

	// キーワードファイル, keyword file
	Path kwd = cfg_.kwdFile();
	FileR fp;
	if( kwd.len()!=0 && fp.Open(kwd.c_str()) )
		edit_.getDoc().SetKeyword((const unicode*)fp.base(),fp.size()/2);
	else
		edit_.getDoc().SetKeyword(NULL,0);
	LOGGER("GreenPadWnd::ReloadConfig KeywordLoaded");
}



//-------------------------------------------------------------------------
// 開く処理
//-------------------------------------------------------------------------

bool GreenPadWnd::ShowOpenDlg( Path* fn, int* cs )
{
	// [Open][Cancel] 開くファイル名指定ダイアログを表示
	String flst[] = {
		String(IDS_TXTFILES),
		String(cfg_.txtFileFilter()),
		String(IDS_ALLFILES),
		String(TEXT("*.*"))
	};
	aarr<TCHAR> filt = OpenFileDlg::ConnectWithNull(flst,countof(flst));

	OpenFileDlg ofd( charSets_ );
	bool ok = ofd.DoModal( hwnd(), filt.get(), filename_.c_str() );
	if( ok )
	{
		*fn = ofd.filename();
		*cs = charSets_[ofd.csi()].ID;
	}

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
			cmd += String().SetInt( cs );
			cmd += TEXT(" \"");
			cmd += fn;
			cmd += TEXT('\"');
		BootNewProcess( cmd.c_str() );
		return true;
	}
}
BOOL CALLBACK GreenPadWnd::SendMsgToFriendsProc(HWND hwnd, LPARAM lPmsg)
{
	TCHAR classn[256];
	if(::IsWindow(hwnd))
	{
		::GetClassName(hwnd, classn, countof(classn));
		if (!my_lstrcmp(classn, className_))
			::PostMessage(hwnd, (UINT)lPmsg, 0, 0);
	}
	return TRUE; // Next hwnd
}
BOOL GreenPadWnd::SendMsgToAllFriends(UINT msg)
{
	return EnumWindows(SendMsgToFriendsProc, (LPARAM)msg);
}
bool GreenPadWnd::OpenByMyself( const ki::Path& fn, int cs, bool needReConf, bool always )
{
//	MsgBox(fn.c_str(), TEXT("File"), 0);
	// ファイルを開けなかったらそこでおしまい。
	aptr<TextFileR> tf( new TextFileR(cs) );

	if( !tf->Open( fn.c_str(), always ) )
	{
		// ERROR!
		MsgBox( fn.c_str(), String(IDS_OPENERROR).c_str() );
		return false;
	}

	// 自分内部の管理情報を更新
	if( fn[0]==TEXT('\\') || (fn[0]==TEXT('\\')&&fn[1]==TEXT('\\')) || fn[1]==TEXT(':') )
		// Absolute path: '\file', 'x:\file', '\\share\file', '\\?\...', 'c:\\file' etc.
		filename_ = fn;
	else
		filename_ = Path( Path::Cur ) + fn;
	if( tf->size() )
	{
		csi_      = charSets_.findCsi( tf->codepage() );
		if( csi_ == 0xffffffff )
			csi_       = 0xf0f00000 | tf->codepage();

		if( tf->nolb_found() )
			lb_       = cfg_.GetNewfileLB();
		else
			lb_       = tf->linebreak();
	}
	else
	{ // 空ファイルの場合は新規作成と同じ扱い
		csi_      = cfg_.GetNewfileCsi();
		lb_       = cfg_.GetNewfileLB();
	}
	filename_.BeShortLongStyle();

	// カレントディレクトリを、ファイルのある位置以外にしておく
	// （こうしないと、開いているファイルのあるディレクトリが削除できない）
	::SetCurrentDirectory( Path(filename_).BeDriveOnly().c_str() );

	// 文書タイプに応じて表示を更新
	if( needReConf )
		ReloadConfig();

	// 開く
	edit_.getDoc().ClearAll();
	stb_.SetText(TEXT("Loading file..."));
	edit_.getDoc().OpenFile( tf );

	// タイトルバー更新
	UpdateWindowName();

	// [最近使ったファイル]へ追加
	if( cfg_.AddMRU( filename_ ) )
		SendMsgToAllFriends(GPM_MRUCHANGED);

	return true;
}



//-------------------------------------------------------------------------
// 保存処理
//-------------------------------------------------------------------------

bool GreenPadWnd::ShowSaveDlg()
{
	// [Save][Cancel] 保存先ファイル名指定ダイアログを表示
	String flst[] = {
		String(IDS_ALLFILES),
		String(TEXT("*.*"))
	};
	aarr<TCHAR> filt = SaveFileDlg::ConnectWithNull( flst, countof(flst) );

	SaveFileDlg sfd( charSets_, csi_, lb_ );
	stb_.SetText( TEXT("Saving file...") );
	if( !sfd.DoModal( hwnd(), filt.get(), filename_.c_str() ) )
		return false;

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
			String(IDS_ASKTOSAVE).c_str(),
			String(IDS_APPNAME).c_str(),
			MB_YESNOCANCEL|MB_ICONQUESTION
		);
		if( answer == IDYES )    return Save_showDlgIfNeeded();
		if( answer == IDCANCEL ) return false;
	}
	return true;
}

bool GreenPadWnd::Save()
{
//	TextFileW tf( charSets_[csi_].ID, lb_ );

	int save_Csi;

	if(csi_ == 0xffffffff)
		save_Csi = ::GetACP();
	else if(csi_ >= 0xf0f00000 && csi_ < 0xf1000000)
		save_Csi = csi_ & 0xfffff;
	else
		save_Csi = charSets_[csi_].ID;

	TextFileW tf( save_Csi, lb_ );

	if( tf.Open( filename_.c_str() ) )
	{
		// 無事ファイルに保存できた場合
		edit_.getDoc().SaveFile( tf );
		UpdateWindowName();
		// [最近使ったファイル]更新
		if( cfg_.AddMRU( filename_ ) )
			SendMsgToAllFriends(GPM_MRUCHANGED);
		return true;
	}

	// Error!
	MsgBox( filename_.c_str(), String(IDS_SAVEERROR).c_str() );
	return false;
}



//-------------------------------------------------------------------------
// メインウインドウの初期化
//-------------------------------------------------------------------------

GreenPadWnd::ClsName GreenPadWnd::className_ = TEXT("GreenPad MainWnd");

GreenPadWnd::GreenPadWnd()
	: WndImpl  ( className_, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES )
	, charSets_( cfg_.GetCharSetList() )
	, csi_     ( cfg_.GetNewfileCsi() )
	, lb_      ( cfg_.GetNewfileLB() )
	, search_  ( *this, edit_ )
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
	stb_.Create( hwnd() );
	edit_.Create( NULL, hwnd(), 0, 0, 100, 100 );
	LOGGER("GreenPadWnd::on_create edit created");
	edit_.getDoc().AddHandler( this );
	edit_.getCursor().AddHandler( this );
	stb_.SetStatusBarVisible( cfg_.showStatusBar() );

	LOGGER("GreenPadWnd::on_create halfway");

	search_.LoadFromINI( cfg_.getImpl() );
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
		LOGGER( "GreenPadWnd::StartUp updatewindowname end" );
	}

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

	Argv  arg;
	ulong   i;

	LOGGER( "argv processed" );

  //-- まずオプションスイッチを処理

	int optL = -1;
	int optC = 0;

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

	GreenPadWnd wnd;
	if( !wnd.StartUp(file,optC,optL) )
		return -1;

	LOGGER( "kmain() startup ok" );

  //-- メインループ

//	wnd.ShowUp2();
//	LOGGER( "showup!" );
	wnd.MsgLoop();

	LOGGER( "fin" );
	return 0;
}
