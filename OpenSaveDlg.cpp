
#include "kilib/stdafx.h"
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
	CHARSET_VALUE("中国語(GB18030,BOM)",	"Chinese(GB18030,BOM)",	"GB18030b") \
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
	CHARSET_VALUE("UTF-1(BOM)",			"UTF-1(BOM)",			"UTF1b") \
	CHARSET_VALUE("UTF-5",				"UTF-5",				"UTF5") \
	CHARSET_VALUE("UTF-5(BOM)",			"UTF-5(BOM)",			"UTF5b") \
	CHARSET_VALUE("UTF-7",				"UTF-7",				"UTF7") \
	CHARSET_VALUE("UTF-8",				"UTF-8",				"UTF8") \
	CHARSET_VALUE("UTF-8(BOM)",			"UTF-8(BOM)",			"UTF8b") \
	CHARSET_VALUE("UTF-9(1997)",		"UTF-9(1997)",			"UTF9") \
	CHARSET_VALUE("UTF-9(1997,BOM)",	"UTF-9(1997,BOM)",		"UTF9b") \
	CHARSET_VALUE("UTF-16BE(BOM)",		"UTF-16BE(BOM)",		"U16BEb") \
	CHARSET_VALUE("UTF-16LE(BOM)",		"UTF-16LE(BOM)",		"U16LEb") \
	CHARSET_VALUE("UTF-16BE",			"UTF-16BE",				"U16BE") \
	CHARSET_VALUE("UTF-16LE",			"UTF-16LE",				"U16LE") \
	CHARSET_VALUE("UTF-32BE(BOM)",		"UTF-32BE(BOM)",		"U32BEb") \
	CHARSET_VALUE("UTF-32LE(BOM)",		"UTF-32LE(BOM)",		"U32LEb") \
	CHARSET_VALUE("UTF-32BE",			"UTF-32BE",				"U32BE") \
	CHARSET_VALUE("UTF-32LE",			"UTF-32LE",				"U32LE") \
	CHARSET_VALUE("SCSU",				"SCSU",					"SCSU") \
	CHARSET_VALUE("SCSU(BOM)",			"SCSU(BOM)",			"SCSUb") \
	CHARSET_VALUE("BOCU",				"BOCU",					"BOCU") \
	CHARSET_VALUE("BOCU(BOM)",			"BOCU(BOM)",			"BOCUb") \
	CHARSET_VALUE("FSS-UTF(19920902)",	"FSS-UTF(19920902)",	"FSSUTF") \
	CHARSET_VALUE("FSS-UTF(19920902,BOM)","FSS-UTF(19920902,BOM)","FSSUTFb") \
	CHARSET_VALUE("UTF-EBCDIC",			"UTF-EBCDIC",			"UEBCDIC") \
	CHARSET_VALUE("UTF-EBCDIC(BOM)",	"UTF-EBCDIC(BOM)",		"UEBCDICb") \
	CHARSET_VALUE("欧米",				"Latin-1",				"LTN1") \
	CHARSET_VALUE("欧米(DOS)",			"Latin-1(DOS)",			"LN1DOS") \
	CHARSET_VALUE("中欧",				"Latin-2",				"LTN2") \
	CHARSET_VALUE("中欧(DOS)",			"Latin-2(DOS)",			"LN2DOS") \
	CHARSET_VALUE("キリル語",				"Cyrillic",			"CYRL") \
	CHARSET_VALUE("キリル語(IBM)",		"Cyrillic(IBM)",		"CYRIBM") \
	CHARSET_VALUE("キリル語(MS-DOS)",	"Cyrillic(MS-DOS)",		"CYRDOS") \
	CHARSET_VALUE("キリル語(KOI8-R)",	"Cyrillic(KOI8-R)",		"KO8R") \
	CHARSET_VALUE("キリル語(KOI8-U)",	"Cyrillic(KOI8-U)",		"KO8U") \
	CHARSET_VALUE("タイ語",				"Thai",					"THAI") \
	CHARSET_VALUE("トルコ語",			"Turkish",				"TRK") \
	CHARSET_VALUE("トルコ語(DOS)",		"Turkish(DOS)",			"TRKDOS") \
	CHARSET_VALUE("バルト語",			"Baltic",				"BALT") \
	CHARSET_VALUE("バルト語(IBM)",		"Baltic(IBM)",			"BALIBM") \
	CHARSET_VALUE("ベトナム語",			"Vietnamese",			"VTNM") \
	CHARSET_VALUE("ギリシャ語",			"Greek",				"GRK") \
	CHARSET_VALUE("ギリシャ語(IBM)",	"Greek(IBM)",			"GRKIBM") \
	CHARSET_VALUE("ギリシャ語(MS-DOS)",	"Greek(MS-DOS)",		"GRKDOS") \
	CHARSET_VALUE("アラビア語",			"Arabic",				"ARA") \
	CHARSET_VALUE("アラビア語(IBM)",	"Arabic(IBM)",			"ARAIBM") \
	CHARSET_VALUE("アラビア語(MS-DOS)",	"Arabic(MS-DOS)",		"ARADOS") \
	CHARSET_VALUE("ヘブライ語",			"Hebrew",				"HEB") \
	CHARSET_VALUE("ヘブライ語(DOS)",	"Hebrew(DOS)",			"HEBDOS") \
	CHARSET_VALUE("ポルトガル語(DOS)",	"Portuguese(DOS)",		"PRT") \
	CHARSET_VALUE("アイスランド語(DOS)","Icelandic(DOS)",		"ICE") \
	CHARSET_VALUE("フランス語(カナダ)(DOS)","Canadian French(DOS)","CFR") \
	CHARSET_VALUE("MSDOS(北欧)",		"MSDOS(Nodic)",			"NODIC") \
	CHARSET_VALUE("MSDOS(us)",			"MSDOS(us)",			"DOS")

CharSetList::CharSetList()
	: list_( 72 )
{
	#if !defined(TARGET_VER) || TARGET_VER >= 350
	short useJP = GetACP() == 932;
	#else
	#define useJP 0
	#endif
	#define Enroll(_id,_nm)  EnrollCs( _id, _nm | BOTH<<8 | useJP<<16 )
	#define EnrollS(_id,_nm) EnrollCs( _id, _nm | SAVE<<8 | useJP<<16 )
	#define EnrollL(_id,_nm) EnrollCs( _id, _nm | LOAD<<8 | useJP<<16 )
	// 適宜登録
	                               EnrollL( AutoDetect,      0 );
	if( ::IsValidCodePage(932) )   Enroll(  SJIS,            1 )
	                             , Enroll(  EucJP,           2 )
	                             , Enroll(  IsoJP,           3 );
	if( ::IsValidCodePage(949) )   Enroll(  UHC,             4 )
	                             , Enroll(  IsoKR,           5 );
	if( ::IsValidCodePage(1361) )  Enroll(  Johab,           6 );
	if( ::IsValidCodePage(54936) ) Enroll(  GB18030,         7 )
	                             , EnrollS( GB18030Y,        8 );
	if( ::IsValidCodePage(936) )   Enroll(  GBK,             9 )
	                             , Enroll(  IsoCN,          10 )
	                             , Enroll(  HZ   ,          11 );
	if( ::IsValidCodePage(950) )   Enroll(  Big5 ,          12 );
	if( ::IsValidCodePage(20000) ) Enroll(  CNS  ,          13 );
	if( ::IsValidCodePage(20001) ) Enroll(  TCA  ,          14 );
	if( ::IsValidCodePage(20002) ) Enroll(  ETen  ,         15 );
	if( ::IsValidCodePage(20003) ) Enroll(  IBM5550,        16 );
	if( ::IsValidCodePage(20004) ) Enroll( Teletext,        17 );
	if( ::IsValidCodePage(20005) ) Enroll(  Wang  ,         18 );
	/* if( always ) */             Enroll(  UTF1,           19 );
	                               EnrollS( UTF1Y,          20 );
	                               Enroll(  UTF5,           21 );
	                               EnrollS( UTF5Y,          22 );
	                               Enroll(  UTF7,           23 );
	                               Enroll(  UTF8N,          24 );
	                               EnrollS( UTF8,           25 );
	                               Enroll(  UTF9,           26 );
	                               EnrollS( UTF9Y,          27 );
	                               EnrollS( UTF16b,         28 );
	                               EnrollS( UTF16l,         29 );
	                               Enroll(  UTF16BE,        30 );
	                               Enroll(  UTF16LE,        31 );
	                               EnrollS( UTF32b,         32 );
	                               EnrollS( UTF32l,         33 );
	                               Enroll(  UTF32BE,        34 );
	                               Enroll(  UTF32LE,        35 );
	                               Enroll(  SCSU,           36 );
	                               EnrollS( SCSUY,          37 );
	                               Enroll(  BOCU1,          38 );
	                               EnrollS( BOCU1Y,         39 );
	                               Enroll(  OFSSUTF,        40 );
	                               EnrollS( OFSSUTFY,       41 );
	                               Enroll(  UTFEBCDIC,      42 );
	                               EnrollS( UTFEBCDICY,     43 );
	/* if( always ) */             Enroll(  Western,        44 );
	if( ::IsValidCodePage(850) )   Enroll(  WesternDOS,     45 );
	if( ::IsValidCodePage(1250) )  Enroll(  Central,        46 );
	if( ::IsValidCodePage(852) )   Enroll(  CentralDOS,     47 );
	if( ::IsValidCodePage(1251) )  Enroll(  Cyrillic,       48 );
	if( ::IsValidCodePage(855) )   Enroll(  CyrillicIBM,    49 );
	if( ::IsValidCodePage(866) )   Enroll(  CyrillicDOS,    50 );
	if( ::IsValidCodePage(20866) ) Enroll(  Koi8R,          51 );
	if( ::IsValidCodePage(21866) ) Enroll(  Koi8U,          52 );
	if( ::IsValidCodePage(874) )   Enroll(  Thai,           53 );
	if( ::IsValidCodePage(1254) )  Enroll(  Turkish,        54 );
	if( ::IsValidCodePage(857) )   Enroll(  TurkishDOS,     55 );
	if( ::IsValidCodePage(1257) )  Enroll(  Baltic,         56 );
	if( ::IsValidCodePage(775) )   Enroll(  BalticIBM,      57 );
	if( ::IsValidCodePage(1258) )  Enroll( Vietnamese,      58 );
	if( ::IsValidCodePage(1253) )  Enroll(  Greek,          59 );
	if( ::IsValidCodePage(737) )   Enroll(  GreekIBM,       60 );
	if( ::IsValidCodePage(869) )   Enroll(  GreekMSDOS,     61 );
	if( ::IsValidCodePage(1256) )  Enroll(  Arabic,         62 );
	if( ::IsValidCodePage(720) )   Enroll(  ArabicIBM,      63 );
	if( ::IsValidCodePage(864) )   Enroll(  ArabicMSDOS,    64 );
	if( ::IsValidCodePage(1255) )  Enroll(  Hebrew,         65 );
	if( ::IsValidCodePage(862) )   Enroll(  HebrewDOS,      66 );
	if( ::IsValidCodePage(860) )   Enroll(  Portuguese,     67 );
	if( ::IsValidCodePage(861) )   Enroll(  Icelandic,      68 );
	if( ::IsValidCodePage(863) )   Enroll(  CanadianFrench, 69 );
	if( ::IsValidCodePage(865) )   Enroll(  Nordic,         70 );
	/* if( always ) */             Enroll(  DOSUS,          71 );


	// 終了
	#undef Enroll
	#undef EnrollS
	#undef EnrollL
}

void CharSetList::EnrollCs(int _id, uint nmtype)
{
	#if !defined(TARGET_VER) || TARGET_VER >= 350
	static const TCHAR* const lnmJp[] = {
		#define CHARSET_VALUE(a,b,c) TEXT(a),
		CHARSETS_LIST
		#undef CHARSET_VALUE
	};
	#endif

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
	#if !defined(TARGET_VER) || TARGET_VER >= 350
	bool useJP = HIWORD(nmtype) != 0;
	const TCHAR* const * lnm = (useJP ? lnmJp : lnmEn);
	#else
	// On Windows 3.1 we cannot have the japaneese UI so for
	// Consistancy sake we remove also this string table.
	const TCHAR* const * lnm = lnmEn;
	#endif

	CsInfo cs;
	uchar type = LOWORD(nmtype)>>8;
	uchar nm = (uchar)LOWORD(nmtype);
	cs.ID=_id;
	cs.longName=lnm[nm];
	cs.shortName=snm[nm];
	cs.type=type;
	list_.Add( cs );
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

size_t CharSetList::defaultCsi() const
{
	return findCsi( defaultCs() );
}

size_t CharSetList::findCsi( int cs ) const
{
	for( size_t i=0,ie=list_.size(); i<ie; ++i )
		if( list_[i].ID == cs )
			return i;

	// If we could not find the index, then store the cs with a mask
	return 0xf0f00000 | (cs & 0xfffff);
}

size_t CharSetList::GetCSIfromNumStr( const TCHAR *buf ) const
{
	// Clamp cs
	int cs = Clamp(-65535, String::GetInt(buf), +65535);
	// Try to find value in the charset list
	size_t csi = findCsi( cs );

	return csi;
}

// Find the best cs index based on the selection in the combobox.
int CharSetList::GetCSIFromComboBox( HWND dlg, const CharSetList& csl, uint OpenSaveMask )
{

	int i = ComboBox(dlg,IDC_CODELIST).GetCurSel();
	if( 0 <= i && i < (int)csl.size() )
	{
		size_t j;
		for(j=0; ;++j,--i)
		{
			while( !(csl[j].type & OpenSaveMask) ) // 1:Open, 2:Save, 3:Both
				++j;
			if( i==0 )
				break;
		}
		return j;
	}

	// Text was typed in the ComboBox
	TCHAR buf[32];
	buf[0] = TEXT('\0');
	int blen = ::SendDlgItemMessage( dlg, IDC_CODELIST, WM_GETTEXT, countof(buf), reinterpret_cast<LPARAM>(buf));

	if( blen > 0 && buf[0] )
	{
		// If a number was typed, convert it to cs index.
		if ( isSDigit(buf[0]) )
			return csl.GetCSIfromNumStr(buf);

		// Last resort, Try to find the string in the whole csl list...
		size_t j;
		for(j=0; j<csl.size() ;++j)
		{
			if( !my_lstrcmpiAscii(csl[j].shortName, buf)
				||  !looseStrCmp(csl[j].longName, buf) )
				return j; // We found
		}

		// Unable to find a match, try with some canonical names
		if( !my_lstrcmpiAscii(buf, TEXT("UTF16")) )
			return csl.findCsi( UTF16LE ); // BOM-free UTF-16 LE
		if( !my_lstrcmpiAscii(buf, TEXT("UTF32")) )
			return csl.findCsi( UTF32LE ); // BOM-free UTF-32 LE
		if( !my_lstrcmpiAscii(buf, TEXT("LATIN")) )
			return csl.findCsi( Western ); // Windows 1252 for Latin
	}
	// Failed to select any cs, return -1
	return -1;
}



//------------------------------------------------------------------------
// 「開く」ダイアログ
//------------------------------------------------------------------------

// Mingw has bad headers!
typedef struct oldtagOFNA {
	DWORD        lStructSize;
	HWND         hwndOwner;
	HINSTANCE    hInstance;
	LPCSTR       lpstrFilter;
	LPSTR        lpstrCustomFilter;
	DWORD        nMaxCustFilter;
	DWORD        nFilterIndex;
	LPSTR        lpstrFile;
	DWORD        nMaxFile;
	LPSTR        lpstrFileTitle;
	DWORD        nMaxFileTitle;
	LPCSTR       lpstrInitialDir;
	LPCSTR       lpstrTitle;
	DWORD        Flags;
	WORD         nFileOffset;
	WORD         nFileExtension;
	LPCSTR       lpstrDefExt;
	LPARAM       lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCSTR       lpTemplateName;
} oldOPENFILENAMEA, *oldLPOPENFILENAMEA;

typedef struct oldtagOFNW {
	DWORD        lStructSize;
	HWND         hwndOwner;
	HINSTANCE    hInstance;
	LPCWSTR      lpstrFilter;
	LPWSTR       lpstrCustomFilter;
	DWORD        nMaxCustFilter;
	DWORD        nFilterIndex;
	LPWSTR       lpstrFile;
	DWORD        nMaxFile;
	LPWSTR       lpstrFileTitle;
	DWORD        nMaxFileTitle;
	LPCWSTR      lpstrInitialDir;
	LPCWSTR      lpstrTitle;
	DWORD        Flags;
	WORD         nFileOffset;
	WORD         nFileExtension;
	LPCWSTR      lpstrDefExt;
	LPARAM       lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCWSTR      lpTemplateName;
} oldOPENFILENAMEW, *oldLPOPENFILENAMEW;

typedef struct newtagOFNA {
	DWORD lStructSize;
	HWND hwndOwner;
	HINSTANCE hInstance;
	LPCSTR lpstrFilter;
	LPSTR lpstrCustomFilter;
	DWORD nMaxCustFilter;
	DWORD nFilterIndex;
	LPSTR lpstrFile;
	DWORD nMaxFile;
	LPSTR lpstrFileTitle;
	DWORD nMaxFileTitle;
	LPCSTR lpstrInitialDir;
	LPCSTR lpstrTitle;
	DWORD Flags;
	WORD nFileOffset;
	WORD nFileExtension;
	LPCSTR lpstrDefExt;
	LPARAM lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCSTR lpTemplateName;
	void *pvReserved;
	DWORD dwReserved;
	DWORD FlagsEx;
} newOPENFILENAMEA,*newLPOPENFILENAMEA;

typedef struct newtagOFNW {
	DWORD lStructSize;
	HWND hwndOwner;
	HINSTANCE hInstance;
	LPCWSTR lpstrFilter;
	LPWSTR lpstrCustomFilter;
	DWORD nMaxCustFilter;
	DWORD nFilterIndex;
	LPWSTR lpstrFile;
	DWORD nMaxFile;
	LPWSTR lpstrFileTitle;
	DWORD nMaxFileTitle;
	LPCWSTR lpstrInitialDir;
	LPCWSTR lpstrTitle;
	DWORD Flags;
	WORD nFileOffset;
	WORD nFileExtension;
	LPCWSTR lpstrDefExt;
	LPARAM lCustData;
	LPOFNHOOKPROC lpfnHook;
	LPCWSTR lpTemplateName;
	void *pvReserved;
	DWORD dwReserved;
	DWORD FlagsEx;
} newOPENFILENAMEW,*newLPOPENFILENAMEW;


#ifdef UNICODE
typedef oldOPENFILENAMEW oldOPENFILENAME;
typedef oldLPOPENFILENAMEW oldLPOPENFILENAME;
typedef newOPENFILENAMEW newOPENFILENAME;
typedef newLPOPENFILENAMEW newLPOPENFILENAME;
#else
typedef oldOPENFILENAMEA oldOPENFILENAME;
typedef oldLPOPENFILENAMEA oldLPOPENFILENAME;
typedef newOPENFILENAMEA newOPENFILENAME;
typedef newLPOPENFILENAMEA newLPOPENFILENAME;
#endif // UNICODE

namespace
{
	// 関数終了時に、カレントディレクトリを元に戻す
	class CurrentDirRecovery
	{
		TCHAR cur_[MAX_PATH];
	public:
		CurrentDirRecovery()  { cur_[0] = TEXT('\0'); ::GetCurrentDirectory( countof(cur_), cur_ ); }
		~CurrentDirRecovery() { ::SetCurrentDirectory( cur_ ); }
	};
}

static void CommonDialogPrepareBuffers( const TCHAR* fnm, TCHAR* filepath, TCHAR* filename )
{
	mem00( filepath, MAX_PATH*sizeof(TCHAR) );
	mem00( filename, MAX_PATH*sizeof(TCHAR) );

	if( fnm != NULL )
	{
		// Limit to MAX_PATH because fnm can be longer
		// And SHELL API does not handle UNC anyway!
		my_lstrcpys( filepath, MAX_PATH, fnm );
		my_lstrcpys( filename, MAX_PATH, fnm );

		int i = 0;
		int j = -1;

		while( filepath[i] != TEXT('\0') )
		{
			if( filepath[i] == TEXT('\\') )
			{
				j = i;
			}
			i++;
		}

		int x = 0;
		for( i = j+1; filepath[i] != TEXT('\0'); i++ )
		{
			filename[x++] = filepath[i];
		}
		filename[x++] = TEXT('\0');
		filepath[j+1] = TEXT('\0');
	}
}

OpenFileDlg* OpenFileDlg::pThis;

bool OpenFileDlg::DoModal( HWND wnd, const TCHAR* fltr, const TCHAR* fnm )
{
	LOGGER( "OpenFileDlg::DoModal begin" );
	CurrentDirRecovery cdr;
	TCHAR filepath[MAX_PATH];

	CommonDialogPrepareBuffers(fnm, filepath, filename_);

	newOPENFILENAME ofn = {0};
	ofn.lStructSize = app().getOSVer() >= 0x500? sizeof(newOPENFILENAME): sizeof(oldOPENFILENAME);
	ofn.hwndOwner      = wnd;
	ofn.hInstance      = app().hinst();
	ofn.lpstrFilter    = fltr;
	ofn.lpstrFile      = filename_;
	ofn.nMaxFile       = countof(filename_);
	ofn.lpstrInitialDir= filepath;
	ofn.lpfnHook       = OfnHook;
	ofn.Flags = OFN_FILEMUSTEXIST |
				OFN_HIDEREADONLY  |
				OFN_ENABLEHOOK    |
				OFN_ENABLESIZING  |
				OFN_ENABLETEMPLATE|
				OFN_CREATEPROMPT;

	// On Windows 95 4.00.116 we cannot add the cs droplist.
	// Only use the New style dialog on Win95 347+/NT4 RTM.
	if(  !oldstyleDlg_ && app().isNewOpenSaveDlg() )
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENSAVEFILEHOOK);
	}
	else
	{
		// WinNT 3.x
		// Win32s all versions.
		// Win95 pre-4.00.180
		// Use the old look template.
		ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);
	}

	// clear last error
	pThis = this;
	TryAgain:
	pThis->dlgEverOpened_ = false;
	::SetLastError(0);
	BOOL ret = ::GetOpenFileName((LPOPENFILENAME)&ofn);
	if( !ret )
	{
		DWORD ErrCode = ::GetLastError();

		if( !pThis->dlgEverOpened_ && ofn.Flags&OFN_EXPLORER )
		{
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			goto TryAgain;
		}
		else if( !ErrCode
			|| ErrCode == ERROR_NO_MORE_FILES
			|| ErrCode == ERROR_INVALID_PARAMETER
			|| ErrCode == ERROR_CLASS_DOES_NOT_EXIST ) // On XP I sometime get this!!!!
		{
			// user pressed Cancel button
			LOGGER( "OpenFileDlg::DoModal CANCEL end" );
		}
		else
		{	// Failed, display LastError.
			//TCHAR tmp[64]; tmp[0] = TEXT('\0');
			//::wsprintf(tmp,TEXT("GetOpenFileName LastError #%d, dlgEverOpened_=%d"), ErrCode, (int)pThis->dlgEverOpened_);
			//::MessageBox( NULL, tmp, RzsString(IDS_APPNAME).c_str(), MB_OK );
			LOGGERF( TEXT("OpenFileDlg::DoModal FAILED end, dlgEverOpened_=%d"), (int)pThis->dlgEverOpened_ );
		}
	}
	else
	{
		LOGGER( "OpenFileDlg::DoModal SUCCESS end with file" );
		LOGGERS( filename_ );
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
		for( size_t i=0; i<csl.size(); ++i )
			if( csl[i].type & 2 ) // 2:=LOAD
				cb.Add( csl[i].longName );
		cb.Select( csl[0].longName );

		HWND hCRLFCombo = ::GetDlgItem( dlg, IDC_CRLFLIST );
		if( hCRLFCombo )
		{
			::ShowWindow( hCRLFCombo, SW_HIDE );
			HWND hCRLFlbl = ::GetDlgItem( dlg, IDC_CRLFLBL );
			if( hCRLFlbl ) ::ShowWindow( hCRLFlbl, SW_HIDE );
		}
		// Older NT wants OfnHook returning TRUE in WM_INITDIALOG
		return TRUE;
	}
	else if( msg==WM_NOTIFY ||( msg==WM_COMMAND && LOWORD(wp)==1 ))
	{
		// OKが押されたら、文字コードの選択状況を記録
		if(( msg==WM_COMMAND && LOWORD(wp)==1 ) || ((LPOFNOTIFY)lp)->hdr.code==CDN_FILEOK )
		{
			int csi = CharSetList::GetCSIFromComboBox( dlg, pThis->csl_, 2 ); // LOAD
			if(csi != -1)
				pThis->csIndex_ = csi;
		}
	}
	else if (msg == WM_PAINT)
	{
		pThis->dlgEverOpened_ = true;
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
	TCHAR filepath[MAX_PATH];

	CommonDialogPrepareBuffers(fnm, filepath, filename_);

	newOPENFILENAME ofn = {0};
	ofn.lStructSize = app().getOSVer() >= 0x500? sizeof(newOPENFILENAME): sizeof(oldOPENFILENAME);
	ofn.hwndOwner      = wnd;
	ofn.hInstance      = app().hinst();
	ofn.lpstrFilter    = fltr;
	ofn.lpstrFile      = filename_;
	ofn.nMaxFile       = countof(filename_);
	ofn.lpstrInitialDir= filepath;
	ofn.lpfnHook       = OfnHook;
	ofn.Flags = OFN_HIDEREADONLY    |
				OFN_PATHMUSTEXIST   |
				OFN_ENABLESIZING    |
				OFN_ENABLEHOOK      |
				OFN_ENABLETEMPLATE  |
				OFN_OVERWRITEPROMPT;


	if( !oldstyleDlg_ && app().isNewOpenSaveDlg() )
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENSAVEFILEHOOK);
	}
	else
	{	// WinNT 3.x
		// Win32s all versions.
		// Win95 pre-4.00.180
	    ofn.lpstrTitle     = TEXT("Save File As");
		// Use the old look template.
		ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);
	}

	pThis = this;
	TryAgain:
	pThis->dlgEverOpened_ = false;
	::SetLastError(0);
	BOOL ret = ::GetSaveFileName((LPOPENFILENAME)&ofn);
	if( !ret )
	{
		DWORD ErrCode = ::GetLastError();

		if( !pThis->dlgEverOpened_ && ofn.Flags&OFN_EXPLORER )
		{
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
			ofn.lpstrTitle     = TEXT("Save File As");
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			goto TryAgain;
		}
		else if( !ErrCode
			|| ErrCode == ERROR_NO_MORE_FILES
			|| ErrCode == ERROR_INVALID_PARAMETER
			|| ErrCode == ERROR_CLASS_DOES_NOT_EXIST ) // On XP I sometime get this!!!!
		{
			// user pressed Cancel button
		}
		else
		{	// Failed, display LastError.
			//TCHAR tmp[64]; tmp[0] = TEXT('\0');
			//::wsprintf(tmp,TEXT("GetSaveFileName LastError #%d"), ErrCode);
			//::MessageBox( wnd, tmp, RzsString(IDS_APPNAME).c_str(), MB_OK );
			LOGGERF( TEXT("SaveFileDlg::DoModal FAILED end, dlgEverOpened_=%d"), (int)pThis->dlgEverOpened_ );
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

			for( size_t i=0; i<csl.size(); ++i )
				if( csl[i].type & 1 ) // 1:=SAVE
					cb.Add( csl[i].longName );

			int csi = pThis->csIndex_;
			if( 0 <= csi && csi < (int)pThis->csl_.size() )
			{
				// Select combobox item
				cb.Select( pThis->csl_[csi].longName );
			}
			else
			{	// Show CP number If selection failed.
				TCHAR tmp[INT_DIGITS+1];
				const TCHAR *cpnum = Int2lStr(tmp, csi&0xfffff);
				cb.Add( cpnum );
				cb.Select( cpnum );
			}
		}
		{
			ComboBox cb( dlg, IDC_CRLFLIST );
			static const TCHAR* const lbList[] = {
				TEXT("CR"),
				TEXT("LF"),
				TEXT("CRLF")
			};

			for( size_t i=0; i<countof(lbList); ++i )
				cb.Add( lbList[i] );
			cb.Select( lbList[Clamp(0, pThis->lb_, 2)] );
		}
		// Older NT wants OfnHook returning TRUE in WM_INITDIALOG
		return TRUE;
	}
	else if( msg==WM_NOTIFY || msg==WM_COMMAND )
	{
		if(( msg==WM_COMMAND && LOWORD(wp) == 1 )
		|| ( msg==WM_NOTIFY && ((LPOFNOTIFY)lp)->hdr.code==CDN_FILEOK) )
		{
			// OKが押されたら、文字コードの選択状況を記録
			// 改行コードも
			int lb = ComboBox(dlg,IDC_CRLFLIST).GetCurSel();
			pThis->lb_ = lb == CB_ERR? 2 :lb; // Default to CRLF;

			pThis->csIndex_ = CharSetList::GetCSIFromComboBox( dlg, pThis->csl_, 1 ); //SAVE
		}
	}
	else if (msg == WM_PAINT)
	{
		pThis->dlgEverOpened_ = true;
	}

	return FALSE;
}



//------------------------------------------------------------------------
// ユーティリティー
//------------------------------------------------------------------------

ki::aarr<TCHAR> OpenFileDlg::ConnectWithNull( const TCHAR *lst[], size_t num )
{
	size_t TtlLen = 1;
	for( size_t i=0; i<num; ++i )
		TtlLen += (my_lstrlen(lst[i]) + 1);

	aarr<TCHAR> a( TtlLen );

	TCHAR* p = a.get();
	if( p )
	{
		for( size_t i=0; i<num; ++i )
		{
			my_lstrcpy( p, lst[i] );
			p += (my_lstrlen(lst[i]) + 1);
		}
		*p = TEXT('\0');
	}

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
	for( size_t i=0; i<csl_.size(); ++i )
		if( csl_[i].type & 2 ) // 2:=LOAD
			cb.Add( csl_[i].longName );

	int csi = csIndex_;
	if( 0 <= csi && csi < (int)csl_.size() )
	{
		// Select combobox item
		if ( !(csl_[csi].type & 2) ) // Append if needed.
			cb.Add( csl_[csi].longName );

		cb.Select( csl_[csi].longName );
	}
	else
	{	// Show CP number in the reopen dialog
		// If selection failed.
		TCHAR tmp[INT_DIGITS+1];
		const TCHAR *numstr = Int2lStr(tmp, csIndex_&0xfffff);
		cb.Add( numstr );
		cb.Select( numstr );
	}
}

bool ReopenDlg::on_ok()
{
	int csi = CharSetList::GetCSIFromComboBox( hwnd(), csl_, 2 );
	if(csi != -1)
		csIndex_ = csi;

	return true;
}
