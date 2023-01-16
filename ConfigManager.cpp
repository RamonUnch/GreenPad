
#include "stdafx.h"
#include "ConfigManager.h"
#include "rsrc/resource.h"
#include "RSearch.h"
using namespace ki;
using namespace editwing;



void BootNewProcess( const TCHAR* cmd ); // in GpMain.cpp

void SetFontSize(LOGFONT *font, HDC hDC, int fsiz, int fx)
{
	font->lfWidth          = 0;
	HDC h;
	h = hDC? hDC: ::GetDC( NULL ); // fallback to current screen DC
	font->lfHeight = -MulDiv(fsiz, ::GetDeviceCaps(h, LOGPIXELSY), 72);
	if(fx) font->lfWidth = -MulDiv(fx, ::GetDeviceCaps(h, LOGPIXELSX), 72);
	if(!hDC) ::ReleaseDC( NULL, h ); // release if captured!
}

void VConfig::SetFont( const TCHAR* fnam, int fsiz, uchar fontCS, LONG fw, BYTE ff, int fx, int qual )
{
	mem00(&font, sizeof(font));
	fontsize              = fsiz;
	fontwidth             = fx;
//	font.lfHeight         = 0;
//	font.lfWidth          = 0;
//	font.lfEscapement     = 0;
//	font.lfOrientation    = 0;
	font.lfWeight         = fw; // FW_DONTCARE;
	font.lfItalic         = ff&1; // FALSE
	font.lfUnderline      = ff&2; // FALSE
	font.lfStrikeOut      = ff&4; // FALSE
	font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	font.lfQuality        = qual;
	font.lfPitchAndFamily = VARIABLE_PITCH|FF_DONTCARE;
	font.lfCharSet        = fontCS;

	my_lstrcpyn( font.lfFaceName, fnam, LF_FACESIZE );

	SetFontSize( &font, NULL, fsiz, fx);
}

//-------------------------------------------------------------------------
// 設定項目管理。
// SetDocTypeで切り替えると、文書タイプ依存の項目を自動で
// 入れ替えたり色々。
//-------------------------------------------------------------------------

ConfigManager::ConfigManager()
{
	// デフォルトのレイアウト設定は何よりも先に読んでおく
	DocType d;
	d.name.Load( IDS_DEFAULT );
	d.layfile   = TEXT("default.lay");
	d.loaded = false; // set unloaded flag.
	LoadLayout( &d );
	dtList_.Add( d );
	curDt_ = dtList_.begin();

	// ini読み込み
	LoadIni();
	SetDocTypeByName( newfileDoctype_ );
	LOGGER( "ConfigManager() initialized" );
}

ConfigManager::~ConfigManager()
{
	// ini保存
	SaveIni();
}

bool ConfigManager::SetDocTypeByName( const ki::String& nam )
{
	// MessageBox(NULL, curDt_->name.c_str(), nam.c_str(), 0);
	if( curDt_->name == nam && curDt_->loaded )
		return 0; // Nothing to do.
	curDt_             = dtList_.begin();
	DtList::iterator b = dtList_.begin();
	DtList::iterator e = dtList_.end();
	for( ; b!=e; ++b )
		if( b->name == nam )
		{
			curDt_ = b;
			break;
		}
	LoadLayout( &*curDt_ );
	return 1;
}

int ConfigManager::SetDocType( const Path& fname )
{
	const unicode* uname = fname.ConvToWChar();

	int ct = 1;
	DtList::iterator i=dtList_.begin(), e=dtList_.end();
	if( fname.len() > 0 )
	{
		for( ++i; i!=e; ++i, ++ct )
			if( i->pattern.len() > 0 )
			{
				const unicode* upat = i->pattern.ConvToWChar();
				bool b = MatchDocType( uname, upat );
				i->pattern.FreeWCMem( upat );
				if( b ) break;
			}
		if( i == e )
			ct=0, i=dtList_.begin(); // 適切なのが見つからなければ[標準]。
	}
	else
	{
		ct = 0;
	}

	SetDocTypeByName( i->name );

	fname.FreeWCMem( uname );
	return ct;
}

bool ConfigManager::MatchDocType
	( const unicode* fname, const unicode* pat )
{
	// pattern と fname とのマッチをとって判定…
	return reg_match( pat, fname, false );
}



//-------------------------------------------------------------------------
// 設定ダイアログ関連
//-------------------------------------------------------------------------

struct ConfigDlg : public ki::DlgImpl
{
private:
	typedef ConfigManager::DtList::iterator DTI;
	ulong curSel_;
	void SaveDt()
	{
		DTI p=myDtl_.begin(), e=myDtl_.end();
		for( ulong ct=0; p!=e && ct!=curSel_; ++ct,++p );
		if( p==e ) return;

		TCHAR buf[MAX_PATH];
		SendMsgToItem(IDC_DT_PAT, WM_GETTEXT,
			countof(buf),reinterpret_cast<LPARAM>(buf));
		p->pattern = buf;

		getComboBoxText(IDC_PAT_KWD, buf);
		p->kwdfile = buf;

		getComboBoxText(IDC_PAT_LAY, buf);
		p->layfile = buf;
	}
	void SelDt(ulong i)
	{
		DTI p=myDtl_.begin(), e=myDtl_.end();
		for( ulong ct=0; p!=e && ct!=i; ++ct,++p );
		if( p==e ) return;

		curSel_ = i;
		SendMsgToItem(IDC_DT_PAT, WM_SETTEXT, p->pattern.c_str());
		if( p->kwdfile.len()==0 )
			SendMsgToItem(IDC_PAT_KWD, CB_SETCURSEL);
		else
			SendMsgToItem(IDC_PAT_KWD,CB_SELECTSTRING,p->kwdfile.c_str());
		SendMsgToItem(IDC_PAT_LAY,CB_SELECTSTRING,p->layfile.c_str());
	}
	void on_deldoctype()
	{
		ulong ct;
		DTI p=myDtl_.begin(), e=myDtl_.end();
		for( ct=0; p!=e && ct!=curSel_; ++ct,++p );
		if( p==e ) return;

		String msg = TEXT("[");
		msg += p->name, msg += TEXT("]"), msg += String(IDS_OKTODEL);
		if( IDNO ==
			MsgBox( msg.c_str(), String(IDS_APPNAME).c_str(), MB_YESNO ) )
			return;

		myDtl_.Del(p);
		SendMsgToItem( IDC_DOCTYPELIST, LB_DELETESTRING, ct );
		SelDt(0);
		if( ct+1 == (ulong)SendMsgToItem(IDC_NEWDT,CB_GETCURSEL) )
			SendMsgToItem( IDC_NEWDT, CB_SETCURSEL );
		SendMsgToItem( IDC_NEWDT, CB_DELETESTRING, ct+1 );
	}
	void on_newdoctype()
	{
		struct NewDocTypeDlg : public DlgImpl
		{
			NewDocTypeDlg(HWND wnd)
				: DlgImpl(IDD_ADDDOCTYPE) { GoModal(wnd); }
			virtual bool on_ok()
			{
				TCHAR buf[MAX_PATH];
				SendMsgToItem(IDC_NAME, WM_GETTEXT,
					countof(buf),reinterpret_cast<LPARAM>(buf));
				name = buf;
				SendMsgToItem(IDC_EXT, WM_GETTEXT,
					countof(buf),reinterpret_cast<LPARAM>(buf));
				ext=buf;
				return true;
			}
			String name;
			String ext;
		} dlg( hwnd() );
		if( IDOK == dlg.endcode() )
		{
			ConfigManager::DocType ndt;
			TCHAR buf[MAX_PATH];
			getComboBoxText( IDC_PAT_KWD, buf );
			ndt.kwdfile = buf;

			getComboBoxText( IDC_PAT_LAY, buf );
			ndt.layfile = buf;

			ndt.name = dlg.name;
			ndt.pattern = TEXT(".*\\.")+dlg.ext+TEXT("$");
			myDtl_.Add(ndt);
			SendMsgToItem( IDC_DOCTYPELIST, LB_ADDSTRING,
				ndt.name.c_str() );
			SendMsgToItem( IDC_NEWDT, CB_ADDSTRING,
				ndt.name.c_str() );
		}
	}

public:
	ConfigDlg( ConfigManager& cfg, HWND wnd )
		: DlgImpl( IDD_CONFIG )
		, curSel_( 0xffffffff )
		, cfg_( cfg )
	{
		for( DTI i=++cfg_.dtList_.begin(); i!=cfg_.dtList_.end(); ++i )
			myDtl_.Add( *i );
		GoModal( wnd );
	}

private:
	void on_init()
	{
		SendMsgToItem(IDC_LATEST_NUM, WM_SETTEXT,
			String().SetInt(cfg_.mrus_).c_str() );
		if( cfg_.undoLimit() == -1 )
		{
			SendMsgToItem(IDC_UNDOLIM1, BM_SETCHECK, BST_CHECKED);
			SendMsgToItem(IDC_UNDO_CT, WM_SETTEXT, TEXT("20") );
		}
		else
		{
			SendMsgToItem(IDC_UNDOLIM2, BM_SETCHECK, BST_CHECKED);
			SendMsgToItem(IDC_UNDO_CT, WM_SETTEXT,
				String().SetInt(cfg_.undoLimit()).c_str() );
		}
		if( cfg_.countByUnicode() )
		{
			SendMsgToItem(IDC_COUNTBYLETTER,  BM_SETCHECK, BST_CHECKED);
			SendMsgToItem(IDC_COUNTBYLETTER2, BM_SETCHECK);
		}
		else
		{
			SendMsgToItem(IDC_COUNTBYLETTER,  BM_SETCHECK);
			SendMsgToItem(IDC_COUNTBYLETTER2, BM_SETCHECK, BST_CHECKED);
		}

		SendMsgToItem(IDC_TXTFILT, WM_SETTEXT,
			cfg_.txtFileFilter().c_str() );
		SendMsgToItem(IDC_EXTGREP, WM_SETTEXT,
			cfg_.grepExe().c_str() );

		if( cfg_.openSame() )
			SendMsgToItem(IDC_OPENSAME, BM_SETCHECK, BST_CHECKED);
		if( cfg_.rememberWindowSize_ )
			SendMsgToItem(IDC_REMSIZE, BM_SETCHECK, BST_CHECKED);
		if( cfg_.rememberWindowPlace_ )
			SendMsgToItem(IDC_REMPLACE, BM_SETCHECK, BST_CHECKED);

		CharSetList& csl = cfg_.GetCharSetList();
		for(ulong i=1; i<csl.size(); ++i)
			SendMsgToItem( IDC_NEWCS, CB_ADDSTRING, csl[i].longName );
		SendMsgToItem( IDC_NEWCS, CB_SETCURSEL, csl.findCsi(cfg_.newfileCharset_)-1 );
		SendMsgToItem( IDC_NEWLB, CB_ADDSTRING, TEXT("CR") );
		SendMsgToItem( IDC_NEWLB, CB_ADDSTRING, TEXT("LF") );
		SendMsgToItem( IDC_NEWLB, CB_ADDSTRING, TEXT("CRLF") );
		SendMsgToItem( IDC_NEWLB, CB_SETCURSEL, cfg_.newfileLB_ );

		ulong nfd_idx=0, nfd_cnt=1;
		SendMsgToItem( IDC_NEWDT, CB_ADDSTRING,
			cfg_.dtList_.begin()->name.c_str() );
		for( DTI i=myDtl_.begin(), e=myDtl_.end(); i!=e; ++i,++nfd_cnt )
		{
			SendMsgToItem( IDC_DOCTYPELIST, LB_ADDSTRING,
				i->name.c_str() );
			SendMsgToItem( IDC_NEWDT, CB_ADDSTRING,
				i->name.c_str() );
			if( i->name == cfg_.newfileDoctype_ )
				nfd_idx = nfd_cnt;
		}
		SendMsgToItem( IDC_NEWDT, CB_SETCURSEL, nfd_idx );

		FindFile f;
		WIN32_FIND_DATA fd;
		f.Begin( (Path(Path::Exe)+=TEXT("type\\*.kwd")).c_str() );
		SendMsgToItem( IDC_PAT_KWD, CB_ADDSTRING, TEXT("") );
		while( f.Next(&fd) )
			SendMsgToItem( IDC_PAT_KWD, CB_ADDSTRING, fd.cFileName );
		f.Begin( (Path(Path::Exe)+=TEXT("type\\*.lay")).c_str() );
		while( f.Next(&fd) )
			SendMsgToItem( IDC_PAT_LAY, CB_ADDSTRING, fd.cFileName );

		SelDt(0);

		SetCenter( hwnd(), ::GetParent(hwnd()) );
	}

	// Gets the string from the currently selected item
	// of the specified combobox idc and open it as a file
	// buf MUST of length MAX_PATH
	bool getComboBoxText( UINT idc, TCHAR buf[MAX_PATH] )
	{
		buf[0]=TEXT('\0');
		int idx = SendMsgToItem(idc, CB_GETCURSEL);
		int len = SendMsgToItem(idc, CB_GETLBTEXTLEN, idx);
		if( 0 < len && len < MAX_PATH )
		{
			return 0 < SendMsgToItem( idc, CB_GETLBTEXT, idx, reinterpret_cast<LPARAM>(buf) );
		}
		return false;
	}
	void NewProcessFromComboBox( UINT idc )
	{
		TCHAR buf[MAX_PATH];
		if( getComboBoxText( idc, buf ) )
			BootNewProcess( (TEXT("-c0 \"")+Path(Path::Exe)+
				TEXT("type\\")+buf+TEXT("\"") ).c_str() );
	}

	bool on_command( UINT cmd, UINT id, HWND ctrl )
	{
		switch( cmd )
		{
		case LBN_SELCHANGE:
			SaveDt();
			SelDt( (ulong)SendMsgToItem( IDC_DOCTYPELIST, LB_GETCURSEL ) );
			break;
		default:
			switch( id )
			{
			case IDC_EDITKWD:
				NewProcessFromComboBox(IDC_PAT_KWD);
				break;
			case IDC_EDITLAY:
				NewProcessFromComboBox(IDC_PAT_LAY);
				break;
			case IDC_NEWDOCTYPE:
				on_newdoctype();
				break;
			case IDC_DELDOCTYPE:
				on_deldoctype();
				break;
			default:
				return false;
			}
			break;
		}
		return true;
	}

	bool on_ok()
	{
		TCHAR buf[100];
		SendMsgToItem(IDC_LATEST_NUM, WM_GETTEXT,
			countof(buf),reinterpret_cast<LPARAM>(buf));
		cfg_.mrus_ = String::GetInt(buf);
		cfg_.mrus_ = Min(Max(0, cfg_.mrus_), 20);

		if( BST_CHECKED == SendMsgToItem(IDC_UNDOLIM1, BM_GETCHECK) )
		{
			cfg_.undoLimit_ = -1;
		}
		else
		{
			SendMsgToItem(IDC_UNDO_CT, WM_GETTEXT,
				countof(buf),reinterpret_cast<LPARAM>(buf));
			cfg_.undoLimit_ = String::GetInt(buf);
		}

		SendMsgToItem(IDC_TXTFILT, WM_GETTEXT,
			countof(buf),reinterpret_cast<LPARAM>(buf));
		cfg_.txtFilter_ = buf;

		SendMsgToItem(IDC_EXTGREP, WM_GETTEXT,
			countof(buf),reinterpret_cast<LPARAM>(buf));
		cfg_.grepExe_ = buf;

		cfg_.openSame_ =
			( BST_CHECKED==SendMsgToItem(IDC_OPENSAME, BM_GETCHECK) );
		cfg_.rememberWindowSize_ =
			( BST_CHECKED==SendMsgToItem(IDC_REMSIZE, BM_GETCHECK) );
		cfg_.rememberWindowPlace_ =
			( BST_CHECKED==SendMsgToItem(IDC_REMPLACE, BM_GETCHECK) );

		cfg_.countbyunicode_ =
			( BST_CHECKED==SendMsgToItem(IDC_COUNTBYLETTER, BM_GETCHECK) );

		cfg_.newfileCharset_ = cfg_.GetCharSetList()[1+SendMsgToItem(IDC_NEWCS, CB_GETCURSEL)].ID;
		cfg_.newfileLB_ = (lbcode) SendMsgToItem(IDC_NEWLB, CB_GETCURSEL);
		size_t nfd_idx=SendMsgToItem(IDC_NEWDT, CB_GETCURSEL), nfd_cnt=1;
		cfg_.newfileDoctype_ = String( IDS_DEFAULT );

		SaveDt();
		cfg_.dtList_.DelAfter( ++cfg_.dtList_.begin() );
		for( DTI i=myDtl_.begin(), e=myDtl_.end(); i!=e; ++i, ++nfd_cnt )
		{
			cfg_.dtList_.Add( *i );
			if( nfd_idx == nfd_cnt )
				cfg_.newfileDoctype_ = i->name;
		}
		return true;
	}

private:
	ConfigManager& cfg_;
	ConfigManager::DtList myDtl_;
};

bool ConfigManager::DoDialog( const ki::Window& parent )
{
	LoadIni();
	{
		ConfigDlg dlg(*this, parent.hwnd());
		if( IDOK != dlg.endcode() )
			return false;
		curDt_ = dtList_.begin(); // とりあえず
	}
	inichanged_=1;
	SaveIni();
	return true;
}



//-------------------------------------------------------------------------
// *.lay ファイルからの読み込み処理, Process reading from *.lay file
//-------------------------------------------------------------------------

namespace {
	static ulong ToByte( unicode* str )
	{
		ulong c = str[0];
			 if( L'a' <= str[0] ) c -= (L'a' - 10);
		else if( L'A' <= str[0] ) c -= (L'A' - 10);
		else                      c -=  L'0';
		c = c*16 + str[1];
			 if( L'a' <= str[1] ) c -= (L'a' - 10);
		else if( L'A' <= str[1] ) c -= (L'A' - 10);
		else                      c -=  L'0';
		return c;
	}
	static ulong GetColor( unicode* str )
	{
		return ToByte(str) + (ToByte(str+2)<<8) + (ToByte(str+4)<<16);
	}
	static int GetInt( unicode* str )
	{
		int c = 0;
		int s = 1;
		if( *str == L'-' )
			s=-1, ++str;
		for( ; *str!=L'\0'; ++str )
			c = c * 10 + *str - L'0';
		return c*s;
	}
}

void ConfigManager::LoadLayout( ConfigManager::DocType* dt )
{
  // １．省略値として…

	DtList::iterator ref = dtList_.begin();
	if( ref != dtList_.end() && ref->loaded )
	{
		// default.layがロードされていればそれを使う
		dt->vc        = ref->vc;
		dt->wrapWidth = ref->wrapWidth;
		dt->wrapType  = ref->wrapType;
		dt->wrapSmart = ref->wrapSmart;
		dt->showLN    = ref->showLN;
		dt->fontCS    = ref->fontCS;
		dt->fontQual  = ref->fontQual;
	}
	else
	{
		// 組み込みのデフォルト設定をロード, Load built-in default settings
		dt->vc.SetTabStep( 4 );
		dt->vc.color[TXT] =
		dt->vc.color[LN]  =
		dt->vc.color[CMT] = ::GetSysColor(COLOR_WINDOWTEXT); // RGB(0,0,0);
		dt->vc.color[KWD] = RGB(0,90,230);
		dt->vc.color[BG]  = ::GetSysColor(COLOR_WINDOW); // RGB(255,255,255);
		dt->vc.color[CTL] = RGB(230,190,230);

		dt->vc.sc[scEOF] = true;  // Show end of file with [EOF]
		dt->vc.sc[scEOL] = true;  // Show End of line with '/'
		dt->vc.sc[scHSP] = false; // ' ' (ASCII space)
		dt->vc.sc[scZSP] = true;  // '　' (0x3000)
		dt->vc.sc[scTAB] = true;  // Show tabs with '>'

		dt->vc.SetFont( TEXT("FixedSys"), 11, dt->fontCS );

		dt->wrapWidth  = 80;
		dt->wrapType   = -1;
		dt->wrapSmart  = true;
		dt->showLN     = true;
		dt->fontCS     = DEFAULT_CHARSET;
		dt->fontQual   = DEFAULT_QUALITY;
	}
	dt->loaded     = true;

  // ２．*.layファイルからの読み込み
	// MessageBox(NULL, dt->layfile.c_str(), TEXT("Loading"), 0);
	TextFileR tf( UTF16LE );
	if( tf.Open( (Path(Path::Exe)+TEXT("type\\")+dt->layfile).c_str() ) )
	{
		String fontname;
		int    fontsize=0;
		int    fontxwidth=0;
		LONG   fontweight=FW_DONTCARE;
		BYTE   fontflags=0;
		int    x;
		bool   clfound = false;
		dt->fontCS = DEFAULT_CHARSET;

		unicode buf[1024], *ptr=buf+3;
		while( tf.state() != 0 ) // !EOF
		{
			size_t len = tf.ReadLine( buf, countof(buf)-1 );
			if( len<=3 || buf[0] == L';' || buf[2]!=L'=' )
				continue;
			buf[len] = L'\0';

			switch( (buf[0]<<8)|buf[1] ) // ASCII only
			{
			case 0x6374: // ct: COLOR-TEXT
				dt->vc.color[TXT] = GetColor(ptr);
				break;
			case 0x636B: // ck: COLOR-KEYWORD
				dt->vc.color[KWD] = GetColor(ptr);
				break;
			case 0x6362: // cb: COLOR-BACKGROUND
				dt->vc.color[BG ] = GetColor(ptr);
				break;
			case 0x6363: // cc: COLOR-COMMENT
				dt->vc.color[CMT] = GetColor(ptr);
				break;
			case 0x636E: // cn: COLOR-CONTROL
				dt->vc.color[CTL] = GetColor(ptr);
				break;
			case 0x636C: // cl: COLOR-LINE
				clfound = true;
				dt->vc.color[LN] = GetColor(ptr);
				break;
			case 0x6666: // ff: FONT FLAGS
				fontflags = GetInt(ptr);
				break;
			case 0x6674: // ft: FONT
				fontname = ptr;
				break;
			case 0x6677: // fw: FONT WEIGHT
				fontweight = GetInt(ptr);
				break;
			case 0x6678: // fw: FONT X Width
				fontxwidth = GetInt(ptr);
				break;
			case 0x737A: // sz: SIZE
				fontsize = GetInt(ptr);
				break;
			case 0x6373: // cs: FONT-CHAR-SET
				dt->fontCS = GetInt(ptr);
				break;
			case 0x7462: // tb: TAB
				dt->vc.SetTabStep( GetInt(ptr) );
				break;
			case 0x7363: // sc: SPECIAL-CHAR
				x = GetInt(ptr);
				dt->vc.sc[scZSP] = (0!=x%10); x/=10;
				dt->vc.sc[scHSP] = (0!=x%10); x/=10;
				dt->vc.sc[scTAB] = (0!=x%10); x/=10;
				dt->vc.sc[scEOL] = (0!=x%10); x/=10;
				dt->vc.sc[scEOF] = (0!=x%10);
				break;
			case 0x7770: // wp: WRAP-TYPE
				dt->wrapType = GetInt(ptr);
				break;
			case 0x7777: // ww: WRAP-WIDTH
				dt->wrapWidth = GetInt(ptr);
				break;
			case 0x7773: // ws WRAP-SMART
				dt->wrapSmart = (0!=GetInt(ptr));
			case 0x6C6E: // ln: LINE-NO
				dt->showLN = (0!=GetInt(ptr));
				break;
			case 0x6671: // fq: Font Quality
				dt->fontQual = GetInt(ptr);
				break;
			}
		}

		if( !clfound )
			dt->vc.color[LN] = dt->vc.color[TXT];
		if( fontname.len()!=0 && fontsize!=0 )
			dt->vc.SetFont( fontname.c_str(), fontsize, dt->fontCS
						, fontweight, fontflags, fontxwidth, dt->fontQual );
	}
}



//-------------------------------------------------------------------------
// *.ini ファイルからの読み込み/書き込み処理
//-------------------------------------------------------------------------

static const TCHAR s_sharedConfigSection[] = TEXT("SharedConfig");

void ConfigManager::LoadIni()
{
	inichanged_=0;
	{
		FileW fp;
		Path inipath(Path::Exe);
		inipath+=Path(Path::ExeName).body();
		inipath+=TEXT(".ini");
		if( !inipath.exist() && fp.Open(inipath.c_str()) )
		{
			static const char s_defaultIni[] =
			"[DocType]\r\n"
			"1=Assembly\r\n"
			"2=B2E\r\n"
			"3=C/C++\r\n"
			"4=C#\r\n"
			"5=D\r\n"
			"6=Delphi\r\n"
			"7=Java\r\n"
			"8=HTML\r\n"
			"9=CSS\r\n"
			"10=Perl\r\n"
			"11=Ruby\r\n"
			"12=PHP\r\n"
			"13=Python\r\n"
			"14=Lua\r\n"
			"15=Java Script\r\n"
			"16=Erlang\r\n"
			"17=Haskell\r\n"
			"18=OCaml\r\n"
			"19=INI\r\n"
			"20=UnicodeText\r\n"
			"\r\n"

			"[Assembly]\r\n"
			"Pattern=.*\\.asm$\r\n"
			"Keyword=asm.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[B2E]\r\n"
			"Pattern=.*\\.b2e$\r\n"
			"Keyword=b2e.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[C/C++]\r\n"
			"Pattern=.*(\\.(c|cpp|cxx|cc|h|hpp)|include\\\\[^\\.]+)$\r\n"
			"Keyword=C.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[C#]\r\n"
			"Pattern=.*\\.cs$\r\n"
			"Keyword=C#.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[D]\r\n"
			"Pattern=.*\\.d$\r\n"
			"Keyword=D.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Delphi]\r\n"
			"Pattern=.*\\.pas$\r\n"
			"Keyword=Delphi.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Java]\r\n"
			"Pattern=.*\\.java$\r\n"
			"Keyword=Java.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[HTML]\r\n"
			"Pattern=.*(\\.html|\\.htm|temporary internet files\\\\.+)$\r\n"
			"Keyword=HTML.kwd\r\n"
			"Layout=html.lay\r\n"
			"\r\n"
			"[CSS]\r\n"
			"Pattern=.*\\.css$\r\n"
			"Keyword=CSS.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[PHP]\r\n"
			"Pattern=.*\\.(php|php3|php4)$\r\n"
			"Keyword=PHP.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Perl]\r\n"
			"Pattern=.*\\.pl$\r\n"
			"Keyword=Perl.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Python]\r\n"
			"Pattern=.*\\.py$\r\n"
			"Keyword=Python.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Ruby]\r\n"
			"Pattern=.*\\.rb$\r\n"
			"Keyword=Ruby.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Lua]\r\n"
			"Pattern=.*\\.lua$\r\n"
			"Keyword=Lua.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Java Script]\r\n"
			"Pattern=.*\\.js$\r\n"
			"Keyword=JavaScript.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Erlang]\r\n"
			"Pattern=.*\\.erl$\r\n"
			"Keyword=Erlang.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[Haskell]\r\n"
			"Pattern=.*\\.l?hs$\r\n"
			"Keyword=Haskell.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[OCaml]\r\n"
			"Pattern=.*\\.mli?$\r\n"
			"Keyword=OCaml.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[INI]\r\n"
			"Pattern=.*\\.ini$\r\n"
			"Keyword=ini.kwd\r\n"
			"\r\n"
			"[Perl]\r\n"
			"Pattern=.*\\.(pl|pm|cgi)$\r\n"
			"Keyword=perl.kwd\r\n"
			"Layout=program.lay\r\n"
			"\r\n"
			"[UnicodeText]\r\n"
			"Layout=unitext.lay\r\n"
			"\r\n";
			fp.Write( s_defaultIni, sizeof(s_defaultIni)-1 );
		}
	}

	// 共通の設定の読み取りセクション
	sharedConfigMode_ = ini_.HasSectionEnabled( s_sharedConfigSection );
	if( sharedConfigMode_ )
		ini_.SetSection( s_sharedConfigSection );
	else
		ini_.SetSectionAsUserName();

	ini_.CacheSection();

	// 共通の設定
	undoLimit_ = ini_.GetInt( TEXT("UndoLimit"), -1 );
	txtFilter_ = ini_.GetStr( TEXT("TxtFilter"),
		TEXT("*.txt;*.htm;*.html;*.css;*.js;*.d;*.c;*.cpp;*.cc;*.cxx;*.h;*.hpp;*.php;*.php3;*.ini;*.log;*.inf") );
	grepExe_   = ini_.GetStr( TEXT("GrepExe"), TEXT("") );
	openSame_  = ini_.GetBool( TEXT("OpenSame"), false );
	countbyunicode_ = ini_.GetBool( TEXT("CountUni"), false );
	showStatusBar_ = ini_.GetBool( TEXT("StatusBar"), true );

	dateFormat_   = ini_.GetStr( TEXT("DateFormat"), TEXT("HH:mm yyyy/MM/dd") );

	// wnd
	rememberWindowSize_  = ini_.GetBool( TEXT("RememberWindowSize"), false );
	rememberWindowPlace_ = ini_.GetBool( TEXT("RememberWindowPos"), false );
	if( rememberWindowPlace_ )
	{
		wndX_ = ini_.GetInt( TEXT("WndX"), CW_USEDEFAULT );
		wndY_ = ini_.GetInt( TEXT("WndY"), CW_USEDEFAULT );
	}
	if( rememberWindowSize_ )
	{
		wndW_ = ini_.GetInt( TEXT("WndW"), CW_USEDEFAULT );
		wndH_ = ini_.GetInt( TEXT("WndH"), CW_USEDEFAULT );
		wndM_ = ini_.GetBool( TEXT("WndM"), false );
	}
	// Exit with the ESC key?
	useQuickExit_ = ini_.GetBool( TEXT("QuickExit"), false );

	// Print Margins
	SetRect(&rcPMargins_, 500, 500, 500, 500);
	ini_.GetRect( TEXT("PMargin"), &rcPMargins_, &rcPMargins_);

	// TODO: MRU
	mrus_ = ini_.GetInt( TEXT("MRU"), 0 );
	mrus_ = Min(Max(0, mrus_), 20);

	// 新規ファイル関係
	newfileCharset_ = ini_.GetInt( TEXT("NewfileCharset"), charSets_.defaultCs() );
	if(newfileCharset_ == -1) newfileCharset_ = 1252; // 1.07.4 bugfix
	if(!::IsValidCodePage(newfileCharset_)) newfileCharset_ = ::GetACP();
	newfileDoctype_ = ini_.GetStr( TEXT("NewfileDoctype"), String( IDS_DEFAULT ) );
	newfileLB_      = (lbcode) ini_.GetInt( TEXT("NewfileLB"), CRLF );

	// 文書タイプリストの０番以外のクリア
	dtList_.DelAfter( ++dtList_.begin() );

	String s, r;

	ini_.SetSection( TEXT("DocType") );
	ini_.CacheSection();
	for( int i=1; true; ++i )
	{
		// 文書タイプ名を読み込み
		s.SetInt(i);
		r = ini_.GetStr( s.c_str(), String() );
		if( r.len() == 0 )
			break;

		// その文書タイプを実際に読み込み
		ki::IniFile tmp_ini;
		tmp_ini.SetSection( r.c_str() );
		{
			tmp_ini.CacheSection();
			DocType d;
			d.name      = r;
			d.layfile   = tmp_ini.GetStr( TEXT("Layout"),TEXT("default.lay"));
			d.kwdfile   = tmp_ini.GetStr( TEXT("Keyword"), String() );
			d.pattern   = tmp_ini.GetStr( TEXT("Pattern"), String() );
			dtList_.Add( d );
		}
	}
}

void ConfigManager::SaveIni()
{
	if(!inichanged_)
		return;
	inichanged_=0;

	Path inipath(Path::Exe);
	inipath+=Path(Path::ExeName).body();
	inipath+=TEXT(".ini");
	if( inipath.isReadOnly() )
		return;

	// 共通の設定の書き込みセクション
	if( sharedConfigMode_ )
		ini_.SetSection( s_sharedConfigSection );
	else
		ini_.SetSectionAsUserName();

	// 共通の設定
	ini_.PutInt( TEXT("UndoLimit"), undoLimit_ );
	ini_.PutStr( TEXT("TxtFilter"), txtFilter_.c_str() );
	ini_.PutStr( TEXT("GrepExe"), grepExe_.c_str() );
	ini_.PutBool( TEXT("OpenSame"), openSame_ );
	ini_.PutBool( TEXT("CountUni"), countbyunicode_ );
	ini_.PutBool( TEXT("StatusBar"), showStatusBar_ );

	// Cannot be modified from the GUI
	// ini_.PutStr( TEXT("DateFormat"), dateFormat_.c_str() );

	// Wnd
	ini_.PutBool( TEXT("RememberWindowSize"), rememberWindowSize_ );
	ini_.PutBool( TEXT("RememberWindowPos"), rememberWindowPlace_ );
	if (rememberWindowPlace_) {
		ini_.PutInt( TEXT("WndX"), wndX_ );
		ini_.PutInt( TEXT("WndY"), wndY_ );
	}
	if (rememberWindowSize_) {
		ini_.PutInt( TEXT("WndW"), wndW_ );
		ini_.PutInt( TEXT("WndH"), wndH_ );
		ini_.PutBool( TEXT("WndM"), wndM_ );
	}
	// Exit with the ESC key? (Cannot Not yet be modified from GUI)
	// ini_.PutBool(TEXT("QuickExit"), useQuickExit_);

	// 新規ファイル関係
	ini_.PutInt( TEXT("NewfileCharset"), newfileCharset_ );
	ini_.PutStr( TEXT("NewfileDoctype"), newfileDoctype_.c_str() );
	ini_.PutInt( TEXT("NewfileLB"),      newfileLB_      );

	// Print Margins
	ini_.PutRect( TEXT("PMargin"), &rcPMargins_);

	// MRU
	ini_.PutInt( TEXT("MRU"), mrus_ );

	// DocType
	for(DtList::iterator i=++dtList_.begin(); i!=dtList_.end(); ++i )
	{
		ini_.SetSection( i->name.c_str() );
		ini_.PutStr( TEXT("Pattern"), i->pattern.c_str() );
		ini_.PutStr( TEXT("Keyword"), i->kwdfile.c_str() );
		ini_.PutStr( TEXT("Layout"), i->layfile.c_str() );
	}

	ulong ct=1;
	ini_.SetSection( TEXT("DocType") );
	for(DtList::iterator i=++dtList_.begin(); i!=dtList_.end(); ++i,++ct)
		ini_.PutStr( String().SetInt(ct).c_str(), i->name.c_str() );
	ini_.PutStr( String().SetInt(ct).c_str(), TEXT("") );

}



//-------------------------------------------------------------------------
// [最近使ったファイル]関係
//-------------------------------------------------------------------------

namespace {
	static const TCHAR* const s_mrulock = TEXT("GreenPad_MRUMutex");
}

bool ConfigManager::AddMRU( const ki::Path& fname )
{
	if(!mrus_) return false;

	// メモリ内のMRUリストを更新
	{
		int i;
		for( i=0; i<mrus_; ++i ) // countof(mru_)
			if( mru_[i] == fname )
			{++i; break;}

		for( --i; i>0; --i )
			mru_[i] = mru_[i-1];
		mru_[0] = fname;
	}

	// iniへ保存
	{ // Restrict Mutex context
		Mutex mx(s_mrulock);
		if( mx.isLocked() ) {
			ini_.SetSectionAsUserName();
			const String key = TEXT("MRU");
			for( int i=0; i<mrus_; ++i ) //countof(mru_)
				ini_.PutPath(
					(key+String().SetInt(i+1)).c_str(), mru_[i] );
		}
	}

	return true;
}

int ConfigManager::SetUpMRUMenu( HMENU m, UINT id )
{
	if (!mrus_) return 0; // Nothing to do

	// iniから読み込み
	{ // Restrict Mutex context
		Mutex mx(s_mrulock);
		if( mx.isLocked() ) {
			ini_.SetSectionAsUserName();
			const String key = TEXT("MRU");
			for( int i=0; i<mrus_; ++i )
				mru_[i] = ini_.GetPath(
					(key+String().SetInt(i+1)).c_str(), Path() );
		}
	}

	// 全項目を削除
	while( ::DeleteMenu( m, 0, MF_BYPOSITION ) );

	// メニュー構築
	for( int i=0; i<mrus_; ++i )
	{
		if( i>=mrus_ || mru_[i].len()==0 )
		{
			if( i==0 )
			{
				::InsertMenu( m, i, MF_BYPOSITION|MF_GRAYED, id, TEXT("no files") );
			}
			break;
		}
		String cpt = (String)TEXT("&") + String().SetInt(i+1).c_str() + (String)TEXT(" ");
		cpt += mru_[i].CompactIfPossible(60);
		::InsertMenu( m, i, MF_BYPOSITION, id + i, const_cast<TCHAR*>(cpt.c_str()) );
	}
	return mrus_;
}

Path ConfigManager::GetMRU( int no ) const
{
	return (0<=no && no<mrus_ ? mru_[no] : Path());
}


//-------------------------------------------------------------------------
// ウインドウサイズ復元処理
//-------------------------------------------------------------------------

void ConfigManager::RememberWnd( ki::Window* wnd )
{
	if( this->rememberWindowPlace_ || this->rememberWindowSize_ )
	{
		RECT rc;
		wnd->getPos(&rc);
		WINDOWPLACEMENT wp = {sizeof(wp)};
		::GetWindowPlacement( wnd->hwnd(), &wp );

		if( wp.showCmd==SW_SHOWNORMAL || wp.showCmd == SW_MAXIMIZE )
			wndM_ = (wp.showCmd == SW_MAXIMIZE);
		if( wp.showCmd==SW_SHOWNORMAL )
		{
			wndX_ = rc.left;
			wndY_ = rc.top;
			wndW_ = rc.right- rc.left;
			wndH_ = rc.bottom - rc.top;
		}
		inichanged_=1;
		// SaveIni();
	}
}

//-------------------------------------------------------------------------
// [文書タイプ]サブメニューの作成
//-------------------------------------------------------------------------

void ConfigManager::SetDocTypeMenu( HMENU m, UINT idstart )
{
	// 全項目を削除
	while( ::DeleteMenu( m, 0, MF_BYPOSITION ) );

	DtList::iterator i=dtList_.begin(), e=dtList_.end();
	for( int ct=0; i!=e; ++i, ++ct )
	{
		::InsertMenu( m, ct, MF_BYPOSITION|(i==curDt_ ? MFS_CHECKED : MFS_UNCHECKED)
					, idstart + ct, const_cast<TCHAR*>(i->name.c_str()) );
	}
}

void ConfigManager::SetDocTypeByMenu( int pos, HMENU m )
{
	int ct=0;
	DtList::iterator i=dtList_.begin(), e=dtList_.end();
	for( ; i!=e; ++i, ++ct )
	{
		if( ct == pos )
		{
			curDt_ = i;
			LoadLayout( &*curDt_ );
			::CheckMenuItem( m, ct, MF_BYPOSITION|MF_CHECKED);
		}
		else
		{
			::CheckMenuItem( m, ct, MF_BYPOSITION|MF_UNCHECKED);
		}
	}
}

void ConfigManager::CheckMenu( HMENU m, int pos )
{
	int ct=0;
	DtList::iterator i=dtList_.begin(), e=dtList_.end();
	for( ; i!=e; ++i, ++ct )
	{
		if( ct == pos )
		{
			::CheckMenuItem( m, ct, MF_BYPOSITION|MF_CHECKED);
		}
		else
		{
			::CheckMenuItem( m, ct, MF_BYPOSITION|MF_UNCHECKED);
		}
	}
}

