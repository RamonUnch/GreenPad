#include "stdafx.h"
#include "rsrc/resource.h"
#include "kilib/kilib.h"
#include "OpenSaveDlg.h"
using namespace ki;

//------------------------------------------------------------------------
// 文字コードリスト
//------------------------------------------------------------------------

// merge 3 lists into 1 #define for easier management
//  format: CHARSET_VALUE("jp-text", "en-text", "short-name")
//  keep forward slash at line-end for non-last-line of list
//  ordering are important as Enroll/EnrollS/EnrollL uses list in same order
#define CHARSETS_LIST \
	CHARSET_VALUE("自動判定",			"AutoDetect",			"") \
	CHARSET_VALUE("日本語(ShiftJIS)",	"Japanese(ShiftJIS)",	"SJIS") \
	CHARSET_VALUE("日本語(EUC)",		"Japanese(EUC)",		"EUC") \
	CHARSET_VALUE("日本語(ISO-2022-JP)","Japanese(ISO-2022-JP)","JIS") \
	CHARSET_VALUE("韓国語(EUC-KR)",		"Korean(EUC-KR)",		"UHC") \
	CHARSET_VALUE("韓国語(ISO-2022-KR)","Korean(ISO-2022-KR)",	"I2022KR") \
	CHARSET_VALUE("韓国語(Johab)",		"Korean(Johab)",		"Johab") \
	CHARSET_VALUE("中国語(GB18030)",	"Chinese(GB18030)",		"GB18030") \
	CHARSET_VALUE("中国語(GB2312)",		"Chinese(GB2312)",		"GBK") \
	CHARSET_VALUE("中国語(ISO-2022-CN)","Chinese(ISO-2022-CN)",	"I2022CN") \
	CHARSET_VALUE("中国語(HZ)",			"Chinese(HZ)",			"HZ") \
	CHARSET_VALUE("中国語(Big5)",		"Chinese(Big5)",		"BIG5") \
	CHARSET_VALUE("中国語(EUC-TW/CNS)",	"Chinese(EUC-TW/CNS)",	"CNS") \
	CHARSET_VALUE("中国語(TCA)",		"Chinese(TCA)",			"TCA") \
	CHARSET_VALUE("中国語(ETen)",		"Chinese(ETen)",		"ETEN") \
	CHARSET_VALUE("中国語(IBM 5550)",	"Chinese(IBM 5550)",	"IBM5550") \
	CHARSET_VALUE("中国語(Teletext)",	"Chinese(Teletext)",	"TLTEXT") \
	CHARSET_VALUE("中国語(Wang)",		"Chinese(Wang)",		"WANG") \
	CHARSET_VALUE("UTF-1",				"UTF-1",				"UTF1") \
	CHARSET_VALUE("UTF-1(BOM)",			"UTF-1(BOM)",			"UTF1") \
	CHARSET_VALUE("UTF-5",				"UTF-5",				"UTF5") \
	CHARSET_VALUE("UTF-7",				"UTF-7",				"UTF7") \
	CHARSET_VALUE("UTF-8",				"UTF-8",				"UTF8") \
	CHARSET_VALUE("UTF-8N",				"UTF-8N",				"UTF8") \
	CHARSET_VALUE("UTF-9(1997)",		"UTF-9(1997)",			"UTF9") \
	CHARSET_VALUE("UTF-9(1997,BOM)",	"UTF-9(1997,BOM)",		"UTF9") \
	CHARSET_VALUE("UTF-16BE(BOM)",		"UTF-16BE(BOM)",		"U16BE") \
	CHARSET_VALUE("UTF-16LE(BOM)",		"UTF-16LE(BOM)",		"U16LE") \
	CHARSET_VALUE("UTF-16BE",			"UTF-16BE",				"U16BE") \
	CHARSET_VALUE("UTF-16LE",			"UTF-16LE",				"U16LE") \
	CHARSET_VALUE("UTF-32BE(BOM)",		"UTF-32BE(BOM)",		"U32BE") \
	CHARSET_VALUE("UTF-32LE(BOM)",		"UTF-32LE(BOM)",		"U32LE") \
	CHARSET_VALUE("UTF-32BE",			"UTF-32BE",				"U32BE") \
	CHARSET_VALUE("UTF-32LE",			"UTF-32LE",				"U32LE") \
	CHARSET_VALUE("SCSU",				"SCSU",					"SCSU") \
	CHARSET_VALUE("BOCU",				"BOCU",					"BOCU") \
	CHARSET_VALUE("FSS-UTF(19920902)",	"FSS-UTF(19920902)",	"FSSUTF") \
	CHARSET_VALUE("FSS-UTF(19920902,BOM)","FSS-UTF(19920902,BOM)","FSSUTF") \
	CHARSET_VALUE("欧米(DOS)",			"Latin-1(DOS)",			"LN1DOS") \
	CHARSET_VALUE("欧米",				"Latin-1",				"LTN1") \
	CHARSET_VALUE("中欧(DOS)",			"Latin-2(DOS)",			"LN2DOS") \
	CHARSET_VALUE("中欧",				"Latin-2",				"LTN2") \
	CHARSET_VALUE("キリル語(IBM)",		"Cyrillic(IBM)",		"CYRIBM") \
	CHARSET_VALUE("キリル語(MS-DOS)",	"Cyrillic(MS-DOS)",		"CYRDOS") \
	CHARSET_VALUE("キリル語(Windows)",	"Cyrillic(Windows)",	"CYRL") \
	CHARSET_VALUE("キリル語(KOI8-R)",	"Cyrillic(KOI8-R)",		"KO8R") \
	CHARSET_VALUE("キリル語(KOI8-U)",	"Cyrillic(KOI8-U)",		"KO8U") \
	CHARSET_VALUE("タイ語",				"Thai",					"THAI") \
	CHARSET_VALUE("トルコ語(DOS)",		"Turkish(DOS)",			"TRKDOS") \
	CHARSET_VALUE("トルコ語",			"Turkish",				"TRK") \
	CHARSET_VALUE("バルト語(IBM)",		"Baltic(IBM)",			"BALIBM") \
	CHARSET_VALUE("バルト語",			"Baltic",				"BALT") \
	CHARSET_VALUE("ベトナム語",			"Vietnamese",			"VTNM") \
	CHARSET_VALUE("ギリシャ語(IBM)",	"Greek(IBM)",			"GRKIBM") \
	CHARSET_VALUE("ギリシャ語(MS-DOS)",	"Greek(MS-DOS)",		"GRKDOS") \
	CHARSET_VALUE("ギリシャ語",			"Greek",				"GRK") \
	CHARSET_VALUE("アラビア語(IBM)",	"Arabic(IBM)",			"ARAIBM") \
	CHARSET_VALUE("アラビア語(MS-DOS)",	"Arabic(MS-DOS)",		"ARADOS") \
	CHARSET_VALUE("アラビア語",			"Arabic",				"ARA") \
	CHARSET_VALUE("ヘブライ語(DOS)",	"Hebrew(DOS)",			"HEBDOS") \
	CHARSET_VALUE("ヘブライ語",			"Hebrew",				"HEB") \
	CHARSET_VALUE("ポルトガル語(DOS)",	"Portuguese(DOS)",		"PRT") \
	CHARSET_VALUE("アイスランド語(DOS)","Icelandic(DOS)",		"ICE") \
	CHARSET_VALUE("フランス語(カナダ)(DOS)","Canadian French(DOS)","CFR") \
	CHARSET_VALUE("MSDOS(北欧)",		"MSDOS(Nodic)",			"NODIC") \
	CHARSET_VALUE("MSDOS(us)",			"MSDOS(us)",			"DOS")

CharSetList::CharSetList()
	: list_( 30 )
{
	static const TCHAR* const lnmJp[] = {
#define CHARSET_VALUE(a,b,c) TEXT(a),
CHARSETS_LIST
#undef CHARSET_VALUE
	};

	static const TCHAR* const lnmEn[] = {
#define CHARSET_VALUE(a,b,c) TEXT(b),
CHARSETS_LIST
#undef CHARSET_VALUE
	};

	static const TCHAR* const snm[] = {
#define CHARSET_VALUE(a,b,c) TEXT(c),
CHARSETS_LIST
#undef CHARSET_VALUE
	};

	// 日本語環境なら日本語表示を選ぶ
	const TCHAR* const * lnm = (::GetACP()==932 ? lnmJp : lnmEn);

	// いちいち書くの面倒なので短縮表記(^^;
	CsInfo cs;
	#define Enroll(_id,_nm)   cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=LOAD|SAVE,    list_.Add( cs )
	#define EnrollS(_id,_nm)  cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=SAVE,         list_.Add( cs )
	#define EnrollL(_id,_nm)  cs.ID=_id,             \
		cs.longName=lnm[_nm], cs.shortName=snm[_nm], \
		cs.type=LOAD,         list_.Add( cs )

	// 適宜登録
	                               EnrollL( AutoDetect,      0 );
	if( ::IsValidCodePage(932) )   Enroll(  SJIS,            1 )
	                             , Enroll(  EucJP,           2 )
	                             , Enroll(  IsoJP,           3 );
	if( ::IsValidCodePage(949) )   Enroll(  UHC,             4 )
	                             , Enroll(  IsoKR,           5 );
	if( ::IsValidCodePage(1361) )  Enroll(  Johab,           6 );
	if( ::IsValidCodePage(54936) ) Enroll(  GB18030,         7 );
	if( ::IsValidCodePage(936) )   Enroll(  GBK,             8 )
	                             , Enroll(  IsoCN,           9 )
	                             , Enroll(  HZ   ,          10 );
	if( ::IsValidCodePage(950) )   Enroll(  Big5 ,          11 );
	if( ::IsValidCodePage(20000) ) Enroll(  CNS  ,          12 );
	if( ::IsValidCodePage(20001) ) Enroll(  TCA  ,          13 );
	if( ::IsValidCodePage(20002) ) Enroll(  ETen  ,         14 );
	if( ::IsValidCodePage(20003) ) Enroll(  IBM5550,        15 );
	if( ::IsValidCodePage(20004) ) Enroll( Teletext,        16 );
	if( ::IsValidCodePage(20005) ) Enroll(  Wang  ,         17 );
	/* if( always ) */             EnrollS( UTF1,           18 );
	                               Enroll( UTF1Y,           19 );
	                               Enroll(  UTF5,           20 );
	                               Enroll(  UTF7,           21 );
	                               Enroll(  UTF8,           22 );
	                               EnrollS( UTF8N,          23 );
	                               EnrollS(  UTF9,          24 );
	                               Enroll(  UTF9Y,          25 );
	                               EnrollS( UTF16b,         26 );
	                               EnrollS( UTF16l,         27 );
	                               Enroll(  UTF16BE,        28 );
	                               Enroll(  UTF16LE,        29 );
	                               EnrollS( UTF32b,         30 );
	                               EnrollS( UTF32l,         31 );
	                               Enroll(  UTF32BE,        32 );
	                               Enroll(  UTF32LE,        33 );
	                               Enroll(  SCSU,           34 );
	                               Enroll(  BOCU1,          35 );
	                               EnrollS(  OFSSUTF,       36 );
	                               Enroll(  OFSSUTFY,       37 );
	if( ::IsValidCodePage(850) )   Enroll(  WesternDOS,     38 );
	                               Enroll(  Western,        39 );
	if( ::IsValidCodePage(852) )   Enroll(  CentralDOS,     40 );
	if( ::IsValidCodePage(28592) ) Enroll(  Central,        41 );
	if( ::IsValidCodePage(855) )   Enroll(  CyrillicIBM,    42 );
	if( ::IsValidCodePage(866) )   Enroll(  CyrillicDOS,    43 );
	if( ::IsValidCodePage(28595) ) Enroll(  Cyrillic,       44 );
	if( ::IsValidCodePage(20866) ) Enroll(  Koi8R,          45 );
	if( ::IsValidCodePage(21866) ) Enroll(  Koi8U,          46 );
	if( ::IsValidCodePage(874) )   Enroll(  Thai,           47 );
	if( ::IsValidCodePage(857) )   Enroll(  TurkishDOS,     48 );
	if( ::IsValidCodePage(1254) )  Enroll(  Turkish,        49 );
	if( ::IsValidCodePage(775) )   Enroll(  BalticIBM,      50 );
	if( ::IsValidCodePage(1257) )  Enroll(  Baltic,         51 );
	if( ::IsValidCodePage(1258) )  Enroll( Vietnamese,      52 );
	if( ::IsValidCodePage(737) )   Enroll(  GreekIBM,       53 );
	if( ::IsValidCodePage(869) )   Enroll(  GreekMSDOS,     54 );
	if( ::IsValidCodePage(28597) ) Enroll(  Greek,          55 );
	if( ::IsValidCodePage(720) )   Enroll(  ArabicIBM,      56 );
	if( ::IsValidCodePage(864) )   Enroll(  ArabicMSDOS,    57 );
	if( ::IsValidCodePage(1256) )  Enroll(  Arabic,         58 );
	if( ::IsValidCodePage(862) )   Enroll(  HebrewDOS,      59 );
	if( ::IsValidCodePage(1255) )  Enroll(  Hebrew,         60 );
	if( ::IsValidCodePage(860) )   Enroll(  Portuguese,     61 );
	if( ::IsValidCodePage(861) )   Enroll(  Icelandic,      62 );
	if( ::IsValidCodePage(863) )   Enroll(  CanadianFrench, 63 );
	if( ::IsValidCodePage(865) )   Enroll(  Nordic,         64 );
	                               Enroll(  DOSUS,          65 );

	// 終了
	#undef Enroll
	#undef EnrollS
	#undef EnrollL
}

int CharSetList::defaultCs() const
{
	return ::GetACP();
/*
	switch( ::GetACP() )
	{
	case 932: return SJIS;
	case 936: return GBK;
	case 949: return UHC;
	case 950: return Big5;
	default:  return Western;
	}
*/
}

ulong CharSetList::defaultCsi() const
{
	return findCsi( defaultCs() );
}

ulong CharSetList::findCsi( int cs ) const
{
	for( ulong i=0,ie=list_.size(); i<ie; ++i )
		if( list_[i].ID == cs )
			return i;
	return 0xffffffff;
}



//------------------------------------------------------------------------
// 「開く」ダイアログ
//------------------------------------------------------------------------

namespace
{
	// 関数終了時に、カレントディレクトリを元に戻す
	class CurrentDirRecovery
	{
		Path cur_;
	public:
		CurrentDirRecovery() : cur_(Path::Cur) {}
		~CurrentDirRecovery() { ::SetCurrentDirectory(cur_.c_str()); }
	};
}

OpenFileDlg* OpenFileDlg::pThis;

bool OpenFileDlg::DoModal( HWND wnd, const TCHAR* fltr, const TCHAR* fnm )
{
	CurrentDirRecovery cdr;
	TCHAR filepath_[MAX_PATH];

	if( fnm == NULL )
	{
		filename_[0] = TEXT('\0');
		filepath_[0] = TEXT('\0');
	}
	else
	{
		// Limit to MAX_PATH because fnm can be longer
		// And SHELL API does not handle UNC anyway!
		my_lstrcpyn(filepath_, fnm, MAX_PATH);
		my_lstrcpyn(filename_, fnm, MAX_PATH);

		int i = 0;
		int j = -1;

		while(filepath_[i] != TEXT('\0'))
		{
			if(filepath_[i] == TEXT('\\'))
			{
				j = i;
			}
			i++;
		}

		int x = 0;
		for (i = j+1; filepath_[i] != TEXT('\0'); i++)
		{
			filename_[x++] = filepath_[i];
		}
		filename_[x++] = TEXT('\0');
		filepath_[j+1] = TEXT('\0');
	}

	OPENFILENAME ofn = {sizeof(ofn)};
	ofn.hwndOwner      = wnd;
	ofn.hInstance      = app().hinst();
	ofn.lpstrFilter    = fltr;
	ofn.lpstrFile      = filename_;
	ofn.nMaxFile       = countof(filename_);
    ofn.lpstrInitialDir= filepath_;
	ofn.lpfnHook       = OfnHook;
	ofn.Flags = OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY  |
				OFN_ENABLEHOOK    |
				OFN_ENABLESIZING  |
				OFN_ENABLETEMPLATE|
				OFN_LONGNAMES     |
				OFN_CREATEPROMPT;

	if (app().isNewShell())
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENFILEHOOK);
	}
	else
	{
		// Running under Windows NT, use the old look template.
		ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);
	}

	pThis = this;
	BOOL ret = ::GetOpenFileName(&ofn);
	if(ret != TRUE) {
		DWORD ErrCode = ::GetLastError();

		if(!ErrCode || ErrCode == ERROR_NO_MORE_FILES) {
			// user pressed Cancel button
		} else if((ErrCode == ERROR_INVALID_PARAMETER || ErrCode == ERROR_CALL_NOT_IMPLEMENTED || ErrCode == ERROR_INVALID_ACCEL_HANDLE) && ((ofn.Flags & OFN_EXPLORER) == OFN_EXPLORER)) {
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			ret = ::GetOpenFileName(&ofn);
		} else {
			TCHAR tmp[128];
			::wsprintf(tmp,TEXT("GetOpenFileName Error #%d."),ErrCode);
			MessageBox( wnd, tmp, String(IDS_APPNAME).c_str(), MB_OK );
		}
	}
	return ( ret != 0 );
}

UINT_PTR CALLBACK OpenFileDlg::OfnHook( HWND dlg, UINT msg, WPARAM wp, LPARAM lp )
{

	if( msg==WM_INITDIALOG )
	{
		// コンボボックスを埋めて、「自動選択」を選ぶ
		ComboBox cb( dlg, IDC_CODELIST );
		const CharSetList& csl = pThis->csl_;
		for( ulong i=0; i<csl.size(); ++i )
			if( csl[i].type & 2 ) // 2:=LOAD
				cb.Add( csl[i].longName );
		cb.Select( csl[0].longName );
	}
	else if( msg==WM_NOTIFY ||( msg==WM_COMMAND && LOWORD(wp)==1 ))
	{
		// OKが押されたら、文字コードの選択状況を記録
		if(( msg==WM_COMMAND && LOWORD(wp)==1 ) || ((LPOFNOTIFY)lp)->hdr.code==CDN_FILEOK )
		{
			ulong j=0, i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			for(;;++j,--i)
			{
				while( !(pThis->csl_[j].type & 2) ) // !LOAD
					++j;
				if( i==0 )
					break;
			}
			pThis->csIndex_ = j;
		}
	}

	return FALSE;
}



//------------------------------------------------------------------------
// 「保存」ダイアログ
//------------------------------------------------------------------------

SaveFileDlg* SaveFileDlg::pThis;

bool SaveFileDlg::DoModal( HWND wnd, const TCHAR* fltr, const TCHAR* fnm )
{
	CurrentDirRecovery cdr;
	TCHAR filepath_[MAX_PATH];

	if( fnm == NULL )
	{
		filename_[0] = TEXT('\0');
		filepath_[0] = TEXT('\0');
	}
	else
	{
		// Limit to MAX_PATH because fnm can be longer
		// And SHELL API does not handle UNC anyway!
		my_lstrcpyn(filepath_, fnm, MAX_PATH);
		my_lstrcpyn(filename_, fnm, MAX_PATH);

		int i = 0;
		int j = -1;

		while(filepath_[i] != TEXT('\0'))
		{
			if(filepath_[i] == TEXT('\\'))
			{
				j = i;
			}
			i++;
		}

		int x = 0;
		for (i = j+1; filepath_[i] != TEXT('\0'); i++)
		{
			filename_[x++] = filepath_[i];
		}
		filename_[x++] = TEXT('\0');
		filepath_[j+1] = TEXT('\0');
	}

	OPENFILENAME ofn = {sizeof(ofn)};
    ofn.hwndOwner      = wnd;
    ofn.hInstance      = app().hinst();
    ofn.lpstrFilter    = fltr;
    ofn.lpstrFile      = filename_;
    ofn.nMaxFile       = countof(filename_);
    ofn.lpstrInitialDir= filepath_;
	ofn.lpfnHook       = OfnHook;
    ofn.Flags = OFN_HIDEREADONLY    |
				OFN_PATHMUSTEXIST   |
				OFN_ENABLESIZING    |
				OFN_ENABLEHOOK      |
				OFN_ENABLETEMPLATE  |
				OFN_OVERWRITEPROMPT |
				OFN_LONGNAMES;

	if (app().isNewShell())
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEFILEHOOK);
	}
	else
	{
	    ofn.lpstrTitle     = TEXT("Save File As");
		// Running under Windows NT, use the old look template.
		ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);
	}

	pThis = this;
	BOOL ret = ::GetSaveFileName(&ofn);
	if(ret != TRUE) {
		DWORD ErrCode = ::GetLastError();

		if(!ErrCode || ErrCode == ERROR_NO_MORE_FILES) {
			// user pressed Cancel button
		} else if((ErrCode == ERROR_INVALID_PARAMETER || ErrCode == ERROR_CALL_NOT_IMPLEMENTED) && ((ofn.Flags & OFN_EXPLORER) == OFN_EXPLORER)) {
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
		    ofn.lpstrTitle     = TEXT("Save File As");
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			ret = ::GetSaveFileName(&ofn);
		} else {
			TCHAR tmp[128];
			::wsprintf(tmp,TEXT("GetSaveFileName Error #%d."),ErrCode);
			::MessageBox( wnd, tmp, String(IDS_APPNAME).c_str(), MB_OK );
		}
	}
	return ( ret != 0 );
}

UINT_PTR CALLBACK SaveFileDlg::OfnHook( HWND dlg, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg==WM_INITDIALOG )
	{
		// コンボボックスを埋めて、適切なのを選ぶ
		{
			ComboBox cb( dlg, IDC_CODELIST );
			const CharSetList& csl = pThis->csl_;

			for( ulong i=0; i<csl.size(); ++i )
				if( csl[i].type & 1 ) // 1:=SAVE
					cb.Add( csl[i].longName );
			cb.Select( csl[pThis->csIndex_].longName );
		}
		{
			ComboBox cb( dlg, IDC_CRLFLIST );
			static const TCHAR* const lbList[] = {
				TEXT("CR"),
				TEXT("LF"),
				TEXT("CRLF")
			};

			for( ulong i=0; i<countof(lbList); ++i )
				cb.Add( lbList[i] );
			cb.Select( lbList[pThis->lb_] );
		}
	}
	else if( msg==WM_NOTIFY ||( msg==WM_COMMAND && LOWORD(wp)==1 ))
	{
		if(( msg==WM_COMMAND && LOWORD(wp)==1 ) || ((LPOFNOTIFY)lp)->hdr.code==CDN_FILEOK )
		{
			// OKが押されたら、文字コードの選択状況を記録
			ulong j=0, i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			for(;;++j,--i)
			{
				while( !(pThis->csl_[j].type & 1) ) // !SAVE
					++j;
				if( i==0 )
					break;
			}
			pThis->csIndex_ = j;
			// 改行コードも
			pThis->lb_ = ComboBox(dlg,IDC_CRLFLIST).GetCurSel();
		}
	}
	return FALSE;
}



//------------------------------------------------------------------------
// ユーティリティー
//------------------------------------------------------------------------

ki::aarr<TCHAR> OpenFileDlg::ConnectWithNull( String lst[], int num )
{
	int TtlLen = 1;
	for( int i=0; i<num; ++i )
		TtlLen += (lst[i].len() + 1);

	aarr<TCHAR> a( new TCHAR[TtlLen] );

	TCHAR* p = a.get();
	for( int i=0; i<num; ++i )
	{
		my_lstrcpy( p, lst[i].c_str() );
		p += (lst[i].len() + 1);
	}
	*p = TEXT('\0');

	return a;
}




//------------------------------------------------------------------------
// 「開き直す」ダイアログ
//------------------------------------------------------------------------

ReopenDlg::ReopenDlg( const CharSetList& csl, int csi )
	: DlgImpl(IDD_REOPENDLG), csl_(csl), csIndex_(csi)
{
}

void ReopenDlg::on_init()
{
	// コンボボックスを埋めて、「自動選択」を選ぶ
	ComboBox cb( hwnd(), IDC_CODELIST );
	for( ulong i=0; i<csl_.size(); ++i )
		if( csl_[i].type & 1 ) // 2:=SAVE
			cb.Add( csl_[i].longName );
	if(csIndex_ < 0 || csIndex_ > (int)csl_.size())
		csIndex_ = 0;
	cb.Select( csl_[csIndex_].longName );
}

bool ReopenDlg::on_ok()
{
	TCHAR buf[32];
	SendMsgToItem(IDC_CPNUMBER, WM_GETTEXT, countof(buf), (LPARAM)buf);
	// Typed CP has precedence over droplist
	if ( buf[0] )
	{ // Offset of ~1million to avoid covering the index range
		int cp = Clamp(-65535, String::GetInt(buf), +65535); // Clamp cp
		csIndex_ = cp + 0x100000;
		return true;
	}

	// OKが押されたら、文字コードの選択状況を記録
	ulong j=0, i=ComboBox(hwnd(),IDC_CODELIST).GetCurSel();
	for(;;++j,--i)
	{
		while( !(csl_[j].type & 1) ) // !SAVE
			++j;
		if( i==0 )
			break;
	}
	csIndex_ = j;
	return true;
}
