
#include "kilib/stdafx.h"
#include "kilib/find.h"
#include "ConfigManager.h"
#include "rsrc/resource.h"
#include "RSearch.h"
using namespace ki;
using namespace editwing;



void BootNewProcess( const TCHAR* cmd ); // in GpMain.cpp

//-------------------------------------------------------------------------
// 設定項目管理。
// SetDocTypeで切り替えると、文書タイプ依存の項目を自動で
// 入れ替えたり色々。
//-------------------------------------------------------------------------

ConfigManager::ConfigManager()
{
	// デフォルトのレイアウト設定は何よりも先に読んでおく
	DocType d;
	d.name    = RzsString(IDS_DEFAULT).c_str();
	d.layfile = TEXT("default.lay");
	d.loaded  = false; // set unloaded flag.
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

struct ConfigDlg A_FINAL: public ki::DlgImpl
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
		GetItemText(IDC_DT_PAT, countof(buf), buf);
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
		SetItemText(IDC_DT_PAT, p->pattern.c_str());
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
		msg += p->name, msg += TEXT("]"), msg += RzsString(IDS_OKTODEL).c_str();
		if( IDNO ==
			MsgBox( msg.c_str(), RzsString(IDS_APPNAME).c_str(), MB_YESNO ) )
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
		struct NewDocTypeDlg A_FINAL: public DlgImpl
		{
			NewDocTypeDlg(HWND wnd)
				: DlgImpl(IDD_ADDDOCTYPE) { GoModal(wnd); }
			bool on_ok() override
			{
				TCHAR buf[MAX_PATH];
				GetItemText(IDC_NAME, countof(buf), buf);
				name = buf;
				GetItemText(IDC_EXT, countof(buf), buf);
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
			SendMsgToItem( IDC_DOCTYPELIST, LB_ADDSTRING, ndt.name.c_str() );
			SendMsgToItem( IDC_NEWDT, CB_ADDSTRING, ndt.name.c_str() );
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
	void on_init() override
	{
		TCHAR tmp[INT_DIGITS+1];
		SetItemText( IDC_LATEST_NUM, Int2lStr(tmp, cfg_.mrus_) );
		if( cfg_.undoLimit() == -1 )
		{
			CheckItem(IDC_UNDOLIM1);
			SetItemText(IDC_UNDO_CT, TEXT("20") );
		}
		else
		{
			CheckItem(IDC_UNDOLIM2);
			SetItemText(IDC_UNDO_CT, Int2lStr(tmp, cfg_.undoLimit()) );
		}
		if( cfg_.countByUnicode() )
		{
			CheckItem(IDC_COUNTBYLETTER);
			UncheckItem(IDC_COUNTBYLETTER2);
		}
		else
		{
			UncheckItem(IDC_COUNTBYLETTER);
			CheckItem(IDC_COUNTBYLETTER2);
		}

		SetItemText(IDC_TXTFILT, cfg_.txtFileFilter().c_str() );
		SetItemText(IDC_EXTGREP, cfg_.grepExe().c_str() );

		if( cfg_.openSame() )
			CheckItem(IDC_OPENSAME);
		if( cfg_.rememberWindowSize_ )
			CheckItem(IDC_REMSIZE);
		if( cfg_.rememberWindowPlace_ )
			CheckItem(IDC_REMPLACE);

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
			SendMsgToItem( IDC_DOCTYPELIST, LB_ADDSTRING, i->name.c_str() );
			SendMsgToItem( IDC_NEWDT, CB_ADDSTRING, i->name.c_str() );
			if( i->name == cfg_.newfileDoctype_ )
				nfd_idx = nfd_cnt;
		}
		SendMsgToItem( IDC_NEWDT, CB_SETCURSEL, nfd_idx );

		FindFile f;
		WIN32_FIND_DATA fd;
		Path exepath(Path::Exe);
		exepath += TEXT("type\\*.kwd");
		f.Begin( exepath.c_str() );
		SendMsgToItem( IDC_PAT_KWD, CB_ADDSTRING, TEXT("") );
		while( f.Next(&fd) )
			SendMsgToItem( IDC_PAT_KWD, CB_ADDSTRING, fd.cFileName );

		// Add ini section [xxx.lay] to the the layout the droplist
		{
			IniFile ini;
			TCHAR buf[512];
			buf[0] = TEXT('\0');
			size_t sz = ::GetPrivateProfileString(TEXT("DocType"), NULL, TEXT(""), buf, countof(buf), ini.getName());
			for( TCHAR *p = buf; p<buf+sz && *p; )
			{
				//MessageBox(NULL, p, NULL, 0);
				size_t pplen = my_lstrlen(p);
				if( pplen > 4 && !my_lstrcmpiAscii(p+pplen - 4, TEXT(".lay") ) )
					SendMsgToItem( IDC_PAT_LAY, CB_ADDSTRING, p );

				p += pplen + 1; // Next...
			}
		}
		// Add all *.lay files to the layout the droplist
		exepath.TrimRight(3);
		exepath += TEXT("lay");
		f.Begin( exepath.c_str() );
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
		{
			String cmd = TEXT("-c0 \"") + Path(Path::Exe);
			cmd += TEXT("type\\");
			cmd += buf;
			cmd += TEXT('\"');
			BootNewProcess( cmd.c_str() );
		}
	}

	bool on_command( UINT cmd, UINT id, HWND ctrl ) override
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

	bool on_ok() override
	{
		TCHAR buf[100];
		GetItemText(IDC_LATEST_NUM, countof(buf), buf);
		cfg_.mrus_ = String::GetInt(buf);
		cfg_.mrus_ = Min(Max(0, cfg_.mrus_), 20);

		if( isItemChecked(IDC_UNDOLIM1) )
		{
			cfg_.undoLimit_ = -1;
		}
		else
		{
			GetItemText(IDC_UNDO_CT, countof(buf), buf);
			cfg_.undoLimit_ = String::GetInt(buf);
		}

		GetItemText(IDC_TXTFILT, countof(buf), buf);
		cfg_.txtFilter_ = buf;

		GetItemText(IDC_EXTGREP, countof(buf), buf);
		cfg_.grepExe_ = buf;

		cfg_.openSame_            = isItemChecked(IDC_OPENSAME);
		cfg_.rememberWindowSize_  = isItemChecked(IDC_REMSIZE);
		cfg_.rememberWindowPlace_ = isItemChecked(IDC_REMPLACE);

		cfg_.countbyunicode_ = isItemChecked(IDC_COUNTBYLETTER);
		cfg_.newfileCharset_ = cfg_.GetCharSetList()[1+SendMsgToItem(IDC_NEWCS, CB_GETCURSEL)].ID;
		cfg_.newfileLB_ = (lbcode) SendMsgToItem(IDC_NEWLB, CB_GETCURSEL);
		size_t nfd_idx=SendMsgToItem(IDC_NEWDT, CB_GETCURSEL), nfd_cnt=1;
		cfg_.newfileDoctype_ = RzsString( IDS_DEFAULT ).c_str();

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
#ifndef UNICODE
	static ulong ToByte( const unicode str[2] )
	{
		ulong c = str[0];
		if     ( L'a' <= str[0] ) c -= (L'a' - 10);
		else if( L'A' <= str[0] ) c -= (L'A' - 10);
		else                      c -=  L'0';
		c = c*16 + str[1];
		if     ( L'a' <= str[1] ) c -= (L'a' - 10);
		else if( L'A' <= str[1] ) c -= (L'A' - 10);
		else                      c -=  L'0';
		return c;
	}
	static ulong GetColor( const unicode* str )
	{
		ulong val = 0;
		for(int i=0; i<=2 && str && str[1]; ++i,str+=2 )
			val |= ToByte(str) << (i*8);
		return val;
	}
	static int GetInt( const unicode* str )
	{
		int c = 0;
		int s = 1;
		if( *str == L'-' )
			s=-1, ++str;
		for( ; *str!=L'\0'; ++str )
			c = c * 10 + *str - L'0';
		return c*s;
	}
#else
	static inline int GetInt( const unicode* str )
		{ return String::GetInt( str ); }
	static inline ulong GetColor( const unicode* str )
		{ ulong v = Hex2Ulong( str ); return ((v&0x0000FF)<<16) | ((v&0x00FF00)<<0) | ((v&0xFF0000)>>16); }
#endif
}
// Brightness approximation, that does not take gamma into account.
#define COLBRIGHTNESS(x) ( (218*GetRValue(x) + 732*GetGValue(x) + 74*GetBValue(x))>>10 )
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
		COLORREF bgcol = ::GetSysColor(COLOR_WINDOW); // RGB(255,255,255);
		bool brightmode = COLBRIGHTNESS(bgcol) > 128;
		dt->vc.SetTabStep( 4 );
		dt->vc.color[TXT] =
		dt->vc.color[LN]  = ::GetSysColor(COLOR_WINDOWTEXT); // RGB(0,0,0);
		dt->vc.color[CMT] = brightmode? RGB(0,128,0): RGB(255,255,128);
		dt->vc.color[KWD] = brightmode? RGB(0,0,128): RGB(128,255,255);
		dt->vc.color[BG]  = bgcol;
		dt->vc.color[CTL] = brightmode? RGB(192,160,192) : RGB(80,64,80);

		// EOF=0, EOL=1, TAB=2, HSP=3, ZSP=4
		dt->vc.sc = 027 ; //010 111 b

		dt->wrapWidth  = 80;
		dt->wrapType   = -1;
		dt->wrapSmart  = true;
		dt->showLN     = true;
		dt->fontCS     = DEFAULT_CHARSET;
		dt->fontQual   = DEFAULT_QUALITY;

		dt->vc.SetFont( TEXT("FixedSys"), 12, dt->fontCS );
	}
	dt->loaded     = true;

  // ２．*.layファイルからの読み込み
	// MessageBox(NULL, dt->layfile.c_str(), TEXT("Loading"), 0);
	unicode buf[512];
	size_t len = GetLayData(dt->layfile.c_str(), buf, countof(buf));

	// Rad buffer
	if( len )
	{
		TCHAR  fontname[LF_FACESIZE];
		int    fontsize = dt->vc.fontsize;
		int    fontxwidth=0;
		LONG   fontweight=FW_DONTCARE;
		BYTE   fontflags=0;
		bool   clfound = false;
		dt->fontCS = DEFAULT_CHARSET;

		// Read the whole file at once.
		unicode *nptr=buf,*ptr;
		for( ptr=buf; ptr<buf+len; ptr=nptr ) // !EOF
		{
			// Get to next line
			while( *nptr != L'\0' &&  *nptr != L'\r' && *nptr != L'\n' && *nptr != L'|' )
				nptr++;
			*nptr++ = L'\0'; // zero out endline.
			nptr += *(nptr) == L'\n'; // Skip eventual \n

			if( nptr-ptr < 3 || ptr[0] == L';' || ptr[2] != L'=' )
				continue;

			unicode XXoption = (ptr[0]<<8) | ptr[1];
			ptr += 3; // go just after the = sign

			switch( XXoption ) // ASCII only
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
				#ifdef UNICODE
				my_lstrcpysW( fontname, countof(fontname), ptr );
				#else
				WideCharToMultiByte(CP_ACP, 0, ptr, -1, fontname, countof(fontname), NULL, NULL);
				#endif
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
				if( ptr + 4 < buf+len )
					dt->vc.sc = ( (ptr[0] != L'0') << 0 )
					          | ( (ptr[1] != L'0') << 1 )
					          | ( (ptr[2] != L'0') << 2 )
					          | ( (ptr[3] != L'0') << 3 )
					          | ( (ptr[4] != L'0') << 4 );
				break;
			case 0x7770: // wp: WRAP-TYPE
				dt->wrapType = GetInt(ptr);
				break;
			case 0x7777: // ww: WRAP-WIDTH
				dt->wrapWidth = GetInt(ptr);
				break;
			case 0x7773: // ws WRAP-SMART
				dt->wrapSmart = (0!=GetInt(ptr));
				break;
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
		if( fontname[0]!=TEXT('\0') && fontsize!=0 )
			dt->vc.SetFont( fontname, fontsize, dt->fontCS
						, fontweight, fontflags, fontxwidth, dt->fontQual );
	}
}

// Try to open a file in the type\ folder and fallback to
size_t ConfigManager::GetLayData(const TCHAR *name, unicode *buf, size_t buf_len)
{
	size_t len = 0;
	Path full_filename = (Path(Path::Exe) += TEXT("type\\")) += name;

//	if( !buf || buf_len == 0 )
//	{
//		// Return 1 if exists 0 otherwise
//		if( full_filename.isFile() )
//			return 1;
//
//		IniFile ini;
//		TCHAR tmp[4];
//		// Here GetPrivateProfileSection will return 2 if section exists and 0 if it does not.
//		return countof(tmp)-2 == GetPrivateProfileSection(name, tmp, countof(tmp), ini.getName() );
//	}

	if( full_filename.isFile() )
	{
		TextFileR tf( UTF16LE );
		if( tf.Open( full_filename.c_str() ) )
		{
			// Read the whole file at once.
			len = tf.ReadBuf( buf, buf_len-1 );
			buf[len] = L'\0'; // NULL terminate in case.

			return len;
		}
	}

	// file not found in the type directorry,
	// look for a section in the main ini file.
	IniFile ini;
#ifdef UNICODE
	len = GetPrivateProfileStringW( L"DocType", name, L"", buf, buf_len, ini.getName() );
#else
	char *tmp = (char *)TS.alloc( buf_len * sizeof(char) );
	if( tmp )
	{
		len = ::GetPrivateProfileStringA( "DocType", name, "", tmp, buf_len, ini.getName() );
		len = ::MultiByteToWideChar( CP_ACP, 0, tmp, len+1, buf, buf_len );
		TS.freelast( tmp, buf_len * sizeof(char) );
	}
#endif

	return len;
}



//-------------------------------------------------------------------------
// *.ini ファイルからの読み込み/書き込み処理
//-------------------------------------------------------------------------

static const TCHAR s_sharedConfigSection[] = TEXT("SharedConfig");

void ConfigManager::LoadIni()
{
	ki::IniFile ini_;
	inichanged_=0;

	// 共通の設定の読み取りセクション
	sharedConfigMode_ = ini_.SetSectionAsUserNameIfNotShared( s_sharedConfigSection );

	// 共通の設定
	zoom_      = (short)ini_.GetInt( TEXT("Zoom"), 100 );
	undoLimit_ = ini_.GetInt( TEXT("UndoLimit"), -1 );
	txtFilter_ = ini_.GetStr( TEXT("TxtFilter"),
		TEXT("*.txt;*.htm;*.html;*.css;*.js;*.d;*.c;*.cpp;*.cc;*.cxx;*.h;*.hpp;*.php;*.php3;*.ini;*.log;*.inf") );
	grepExe_   = ini_.GetStr( TEXT("GrepExe"), TEXT("cmd.exe /k cd \"%D\"") );
	helpExe_   = ini_.GetStr( TEXT("HelpExe"), TEXT("") );
	openSame_  = ini_.GetBool( TEXT("OpenSame"), false );
	countbyunicode_ = ini_.GetBool( TEXT("CountUni"), true );
	// By default we have stb on NT3.10.404 Win32s 1.30.159
	// And chicago builds that are not too early.
	bool havestb =  app().isNTOSVerLarger( MKVER(3,10,404) )
	            ||( app().is9xOSVerLarger( MKVER(4,00, 99) ) )
	            ||( app().isWin32s() && app().getOSBuild() >= 159 );
	showStatusBar_ = ini_.GetBool( TEXT("StatusBar"), havestb );

	dateFormat_   = ini_.GetStr( TEXT("DateFormat"), TEXT("HH:mm yyyy/MM/dd") );

	// wnd
	rememberWindowSize_  = ini_.GetBool( TEXT("RememberWindowSize"), false );
	rememberWindowPlace_ = ini_.GetBool( TEXT("RememberWindowPos"), false );

	static const RECT defpos = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
	CopyRect(&wndPos_, &defpos);
	if( rememberWindowPlace_ || rememberWindowSize_ )
	{
		ini_.GetRect( TEXT("WndPos"), &wndPos_, &defpos );
		if( rememberWindowSize_ )
			wndM_ = ini_.GetBool( TEXT("WndM"), false );
	}
	// Exit with the ESC key?
	useQuickExit_ = ini_.GetBool( TEXT("QuickExit"), false );
	// Use the old Windows 3.x Open/Save dialog style?
	useOldOpenSaveDlg_ = ini_.GetBool( TEXT("OldOpenSaveDlg"), false );
	warnOnModified_    = ini_.GetBool( TEXT("WarnOnModified"), true );

	// Print Margins
	SetRect(&rcPMargins_, 500, 500, 500, 500);
	ini_.GetRect( TEXT("PMargin"), &rcPMargins_, &rcPMargins_);

	// TODO: MRU
	mrus_ = ini_.GetInt( TEXT("MRU"), 0 );
	mrus_ = Min(Max(0, mrus_), 20);

	// 新規ファイル関係
	newfileCharset_ = ini_.GetInt( TEXT("NewfileCharset"), charSets_.defaultCs() );
	if(newfileCharset_ == -1) newfileCharset_ = 1252; // 1.07.4 bugfix
	int neededCP = TextFileR::neededCodepage(newfileCharset_);
	if( neededCP > 0 && !::IsValidCodePage(neededCP) )
		newfileCharset_ = ::GetACP();
	newfileDoctype_ = ini_.GetStr( TEXT("NewfileDoctype"), RzsString( IDS_DEFAULT ).c_str() );
	newfileLB_      = (lbcode) ini_.GetInt( TEXT("NewfileLB"), CRLF );

	// 文書タイプリストの０番以外のクリア
	dtList_.DelAfter( ++dtList_.begin() );
	ReadAllDocTypes( ini_.getName() );

	LOGGER( "ConfigManager::LoadIni() LOADED!" );
}

void ConfigManager::ReadAllDocTypes( const TCHAR *ininame )
{
	static const char s_defaultIni[] =
		"1=Assembly,program.lay,asm.kwd,.*\\.asm$\0"
		"2=B2E,program.lay,b2e.kwd,.*\\.b2e$\0"
		"3=C/C++,program.lay,C.kwd,.*(\\.(c|cpp|cxx|cc|h|hpp)|include\\\\[^\\.]+)$\0"
		"4=C#,program.lay,C#.kwd,.*\\.cs$\0"
		"5=D,program.lay,D.kwd,.*\\.d$\0"
		"6=Delphi,program.lay,Delphi.kwd,.*\\.pas$\0"
		"7=Fortran,program.lay,Fortran.kwd,.*\\.(f|for|f90|f95|f03|f15)$\0"
		"8=Java,program.lay,Java.kwd,.*\\.java$\0"
		"9=HTML,html.lay,HTML.kwd,.*(\\.html|\\.htm|temporary internet files\\\\.+)$\0"
		"10=CSS,program.lay,CSS.kwd,.*\\.css$\0"
		"11=Perl,program.lay,Perl.kwd,.*\\.(pl|pm|cgi)$\0"
		"12=Ruby,program.lay,Ruby.kwd,.*\\.rb$\0"
		"13=PHP,program.lay,PHP.kwd,.*\\.(php|php3|php4)$\0"
		"14=Python,program.lay,Python.kwd,.*\\.py$\0"
		"15=Lua,program.lay,Lua.kwd,.*\\.lua$\0"
		"16=Java Script,program.lay,JS.kwd,.*\\.js$\0"
		"17=Erlang,program.lay,Erlang.kwd,.*\\.erl$\0"
		"18=Haskell,program.lay,Haskell.kwd,.*\\.l?hs$\0"
		"19=OCaml,program.lay,OCaml.kwd,.*\\.mli?$\0"
		"20=INI,,ini.kwd,.*\\.ini$\0"
		"21=UnicodeText,unitext.lay,,\0\0";

	DocType d;
	// [DocType]
	// 1=Asm...
	// 2=...
	// ...
	DWORD seclen = 0;
	DWORD buflen = 4096 / sizeof(TCHAR);
	TCHAR *sec = NULL;
	while(1)
	{
		sec = (TCHAR *)TS.alloc( buflen * sizeof(TCHAR) );
		if( !sec ) break;
		seclen = GetPrivateProfileSection( TEXT("DocType"), sec, buflen, ininame );
		if(seclen == buflen-2)
		{
			TS.freelast( sec, buflen * sizeof(TCHAR) );
			buflen <<= 1;
			continue;
		}
		break;
	}
	if (seclen == 0 || !sec)
	{
		if(!sec)
		{
			buflen = countof(s_defaultIni);
			sec = (TCHAR*)TS.alloc( buflen * sizeof(TCHAR) );
			if(!sec) return;
		}
		for( size_t i=0; i<countof(s_defaultIni); i++)
			sec[i] = s_defaultIni[i];
		seclen = countof(s_defaultIni)-1;
	}

	TCHAR *p=sec, *end = p + seclen;

	for( ; p < end && *p; )
	{
		while( *p != TEXT('=') && p < end ) p++;
		if( *p != TEXT('=') )
			break;
		p++;

		if( !*p )
			break;

		// 4 Coma separated values to split ie:
		// 1=Assembly,program.lay,asm.kwd,.*\.asm$
		size_t i=0;
		const TCHAR *substrings[3] = { TEXT(""), TEXT(""), TEXT("") };
		for( size_t j=0; j<countof(substrings) && p[i] ; )
		{
			if( p[i] == TEXT(',') )
			{
				p[i] = TEXT('\0');
				substrings[j++] = &p[++i];
			}
			else
				++i;
		}

		// その文書タイプを実際に読み込み
		d.name      = p;
		d.layfile   = *substrings[0] ? substrings[0] : TEXT("default.lay");
		d.kwdfile   = substrings[1];
		d.pattern   = substrings[2];

		dtList_.Add( d );

		p += i;
		while( p<end && *p ) p++; // go to end of string
		p++; // skip NUL
	}
	TS.freelast( sec, buflen * sizeof(TCHAR) );
}

void ConfigManager::SaveIni()
{
	if(!inichanged_)
		return;
	inichanged_=0;

	ki::IniFile ini_;
	if( Path::isReadOnly( ini_.getName() ) )
		return;

	// [DocType]
	TCHAR strnum[ULONG_DIGITS+1];
	String tmp;
	ulong ct=1;
	for(DtList::iterator i=++dtList_.begin(); i!=dtList_.end(); ++i )
	{
		// X=Name,Layout.lay,Kewords.kwd,.*\.extension$
		tmp = i->name.c_str(); tmp += TEXT(',');
		tmp += i->layfile;     tmp += TEXT(',');
		tmp += i->kwdfile;     tmp += TEXT(',');
		tmp += i->pattern;
		ini_.PutStrinSect( Ulong2lStr(strnum, ct), TEXT("DocType"), tmp.c_str() );
		++ct; // Next DocType
	}
	ini_.PutStrinSect( Ulong2lStr(strnum, ct), TEXT("DocType"), TEXT("") );


	// 共通の設定の書き込みセクション
	ini_.SetSectionAsUserNameIfNotShared( s_sharedConfigSection );

	// 共通の設定
	ini_.PutInt( TEXT("Zoom"), zoom_ );
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
	if (rememberWindowPlace_ || rememberWindowSize_) {
		//RECT rc = { wndX_, wndY_, wndW_, wndH_ };
		ini_.PutRect( TEXT("WndPos"), &wndPos_ );
		if (rememberWindowSize_)
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

	ki::IniFile ini;
	// iniへ保存
	{ // Restrict Mutex context
		Mutex mx(s_mrulock);
		if( mx.isLocked() )
		{
			ini.SetSectionAsUserName();
			TCHAR key[3+INT_DIGITS+1];
			my_lstrcpy( key, TEXT("MRU") );
			for( int i=0; i<mrus_; ++i )
			{
				my_lstrcpy( key+3, SInt2Str(i+1).c_str() );
				ini.PutPath( key, mru_[i] );
			}
		}
	}

	return true;
}

int ConfigManager::SetUpMRUMenu( HMENU m, UINT id )
{
	if (!mrus_) return 0; // Nothing to do

	ki::IniFile ini;
	// iniから読み込み
	{ // Restrict Mutex context
		Mutex mx(s_mrulock);
		if( mx.isLocked() )
		{
			ini.SetSectionAsUserName();
			TCHAR key[3+INT_DIGITS+1];
			my_lstrcpy( key, TEXT("MRU") );
			for( int i=0; i<mrus_; ++i )
			{
				my_lstrcpy( key+3, SInt2Str(i+1).c_str() );
				mru_[i] = ini.GetPath(key, TEXT("") );
			}
		}
	}

	// 全項目を削除
	while( ::DeleteMenu( m, 0, MF_BYPOSITION ) );

	// メニュー構築
	enum { MAX_ENTRY_LEN = 60 };
	TCHAR buf[MAX_ENTRY_LEN+INT_DIGITS+4];
	buf[0] = TEXT('&');
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
		TCHAR *end = my_lstrkpy( buf+1, SInt2Str(i+1).c_str() );
		*end++ = TEXT(' ');
		mru_[i].CompactIfPossible( end, MAX_ENTRY_LEN );
		::InsertMenu( m, i, MF_BYPOSITION, id + i, buf );
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

void ConfigManager::RememberWnd( const ki::Window* wnd )
{
	if( this->rememberWindowPlace_ || this->rememberWindowSize_ )
	{
		WINDOWPLACEMENT wp; wp.length = sizeof(wp);
		::GetWindowPlacement( wnd->hwnd(), &wp );

		if( wp.showCmd==SW_SHOWNORMAL || wp.showCmd == SW_MAXIMIZE )
			wndM_ = (wp.showCmd == SW_MAXIMIZE);
		if( wp.showCmd==SW_SHOWNORMAL )
		{
			wnd->getPos(&wndPos_);
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

