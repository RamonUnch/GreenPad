
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "kilib/kilib.h"
#include "OpenSaveDlg.h"
using namespace ki;

//------------------------------------------------------------------------
// �����R�[�h���X�g
//------------------------------------------------------------------------

// merge 3 lists into 1 #define for easier management
//  format: CHARSET_VALUE("jp-text", "en-text", "short-name")
//  keep forward slash at line-end for non-last-line of list
//  ordering are important as Enroll/EnrollS/EnrollL uses list in same order
#define CHARSETS_LIST \
	CHARSET_VALUE("��������",			"AutoDetect",			"") \
	CHARSET_VALUE("���{��(ShiftJIS)",	"Japanese(ShiftJIS)",	"SJIS") \
	CHARSET_VALUE("���{��(EUC)",		"Japanese(EUC)",		"EUC") \
	CHARSET_VALUE("���{��(ISO-2022-JP)","Japanese(ISO-2022-JP)","JIS") \
	CHARSET_VALUE("�؍���(EUC-KR)",		"Korean(EUC-KR)",		"UHC") \
	CHARSET_VALUE("�؍���(ISO-2022-KR)","Korean(ISO-2022-KR)",	"I2022KR") \
	CHARSET_VALUE("�؍���(Johab)",		"Korean(Johab)",		"Johab") \
	CHARSET_VALUE("������(GB18030)",	"Chinese(GB18030)",		"GB18030") \
	CHARSET_VALUE("������(GB2312)",		"Chinese(GB2312)",		"GBK") \
	CHARSET_VALUE("������(ISO-2022-CN)","Chinese(ISO-2022-CN)",	"I2022CN") \
	CHARSET_VALUE("������(HZ)",			"Chinese(HZ)",			"HZ") \
	CHARSET_VALUE("������(Big5)",		"Chinese(Big5)",		"BIG5") \
	CHARSET_VALUE("������(EUC-TW/CNS)",	"Chinese(EUC-TW/CNS)",	"CNS") \
	CHARSET_VALUE("������(TCA)",		"Chinese(TCA)",			"TCA") \
	CHARSET_VALUE("������(ETen)",		"Chinese(ETen)",		"ETEN") \
	CHARSET_VALUE("������(IBM 5550)",	"Chinese(IBM 5550)",	"IBM5550") \
	CHARSET_VALUE("������(Teletext)",	"Chinese(Teletext)",	"TLTEXT") \
	CHARSET_VALUE("������(Wang)",		"Chinese(Wang)",		"WANG") \
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
	CHARSET_VALUE("����(DOS)",			"Latin-1(DOS)",			"LN1DOS") \
	CHARSET_VALUE("����",				"Latin-1",				"LTN1") \
	CHARSET_VALUE("����(DOS)",			"Latin-2(DOS)",			"LN2DOS") \
	CHARSET_VALUE("����",				"Latin-2",				"LTN2") \
	CHARSET_VALUE("�L������(IBM)",		"Cyrillic(IBM)",		"CYRIBM") \
	CHARSET_VALUE("�L������(MS-DOS)",	"Cyrillic(MS-DOS)",		"CYRDOS") \
	CHARSET_VALUE("�L������(Windows)",	"Cyrillic(Windows)",	"CYRL") \
	CHARSET_VALUE("�L������(KOI8-R)",	"Cyrillic(KOI8-R)",		"KO8R") \
	CHARSET_VALUE("�L������(KOI8-U)",	"Cyrillic(KOI8-U)",		"KO8U") \
	CHARSET_VALUE("�^�C��",				"Thai",					"THAI") \
	CHARSET_VALUE("�g���R��(DOS)",		"Turkish(DOS)",			"TRKDOS") \
	CHARSET_VALUE("�g���R��",			"Turkish",				"TRK") \
	CHARSET_VALUE("�o���g��(IBM)",		"Baltic(IBM)",			"BALIBM") \
	CHARSET_VALUE("�o���g��",			"Baltic",				"BALT") \
	CHARSET_VALUE("�x�g�i����",			"Vietnamese",			"VTNM") \
	CHARSET_VALUE("�M���V����(IBM)",	"Greek(IBM)",			"GRKIBM") \
	CHARSET_VALUE("�M���V����(MS-DOS)",	"Greek(MS-DOS)",		"GRKDOS") \
	CHARSET_VALUE("�M���V����",			"Greek",				"GRK") \
	CHARSET_VALUE("�A���r�A��(IBM)",	"Arabic(IBM)",			"ARAIBM") \
	CHARSET_VALUE("�A���r�A��(MS-DOS)",	"Arabic(MS-DOS)",		"ARADOS") \
	CHARSET_VALUE("�A���r�A��",			"Arabic",				"ARA") \
	CHARSET_VALUE("�w�u���C��(DOS)",	"Hebrew(DOS)",			"HEBDOS") \
	CHARSET_VALUE("�w�u���C��",			"Hebrew",				"HEB") \
	CHARSET_VALUE("�|���g�K����(DOS)",	"Portuguese(DOS)",		"PRT") \
	CHARSET_VALUE("�A�C�X�����h��(DOS)","Icelandic(DOS)",		"ICE") \
	CHARSET_VALUE("�t�����X��(�J�i�_)(DOS)","Canadian French(DOS)","CFR") \
	CHARSET_VALUE("MSDOS(�k��)",		"MSDOS(Nodic)",			"NODIC") \
	CHARSET_VALUE("MSDOS(us)",			"MSDOS(us)",			"DOS")

CharSetList::CharSetList()
	: list_( 30 )
{
	#define Enroll(_id,_nm)  EnrollCs( _id, _nm|(LOAD|SAVE)<<8 )
	#define EnrollS(_id,_nm) EnrollCs( _id, _nm|SAVE<<8 )
	#define EnrollL(_id,_nm) EnrollCs( _id, _nm|LOAD<<8 )
	// �K�X�o�^
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
	/* if( always ) */             Enroll(  Western,        39 );
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
	/* if( always ) */             Enroll(  DOSUS,          65 );

	// �I��
	#undef Enroll
	#undef EnrollS
	#undef EnrollL
}

void CharSetList::EnrollCs(int _id, ushort nmtype)
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

	// ���{����Ȃ���{��\����I��
	#if !defined(TARGET_VER) || TARGET_VER >= 350
	const TCHAR* const * lnm = (::GetACP()==932 ? lnmJp : lnmEn);
	#else
	// On Windows 3.1 we cannot have the japaneese UI so for
	// Consistancy sake we remove also this string table.
	const TCHAR* const * lnm = lnmEn;
	#endif

	CsInfo cs;
	uchar type = nmtype>>8;
	uchar nm = (uchar)nmtype;
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

ulong CharSetList::defaultCsi() const
{
	return findCsi( defaultCs() );
}

ulong CharSetList::findCsi( int cs ) const
{
	for( ulong i=0,ie=list_.size(); i<ie; ++i )
		if( list_[i].ID == cs )
			return i;

	// If we could not find the index, then store the cs with a mask
	return 0xf0f00000 | (cs & 0xfffff);
}

ulong CharSetList::GetCSIfromNumStr( const TCHAR *buf ) const
{
	// Clamp cs
	int cs = Clamp(-65535, String::GetInt(buf), +65535);
	// Try to find value in the charset list
	ulong csi = findCsi( cs );

	return csi;
}



//------------------------------------------------------------------------
// �u�J���v�_�C�A���O
//------------------------------------------------------------------------
#if defined(__MINGW32__) && !defined(_WIN64)
// Mingw has bad headers!
typedef struct mytagOFNA {
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
} myOPENFILENAMEA, *myLPOPENFILENAMEA;

typedef struct mytagOFNW {
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
} myOPENFILENAMEW, *myLPOPENFILENAMEW;
#ifdef UNICODE
typedef myOPENFILENAMEW myOPENFILENAME;
typedef myLPOPENFILENAMEW myLPOPENFILENAME;
#else
typedef myOPENFILENAMEA myOPENFILENAME;
typedef myLPOPENFILENAMEA myLPOPENFILENAME;
#endif // UNICODE
#else
#define myOPENFILENAME OPENFILENAME
#endif // __MINGW32__

namespace
{
	// �֐��I�����ɁA�J�����g�f�B���N�g�������ɖ߂�
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
	LOGGER( "OpenFileDlg::DoModal begin" );
	CurrentDirRecovery cdr;
	TCHAR filepath_[MAX_PATH];

	if( fnm == NULL )
	{
		filepath_[0] = TEXT('\0');
	}
	else
	{
		// Limit to MAX_PATH because fnm can be longer
		// And SHELL API does not handle UNC anyway!
		my_lstrcpyn(filepath_, fnm, MAX_PATH);
		filepath_[MAX_PATH-1] = TEXT('\0'); // in case

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
		filepath_[j+1] = TEXT('\0');
	}

	filename_[0] = TEXT('\0');
	myOPENFILENAME ofn = {sizeof(ofn)};
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
				OFN_CREATEPROMPT;

	// On Windows 95 4.00.116 we cannot add the cs droplist.
	// Only use the New style dialog on Win95 347+/NT4 RTM.
	if( app().isNewOpenSaveDlg() )
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPENFILEHOOK);
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
	::SetLastError(0);
	BOOL ret = ::GetOpenFileName((LPOPENFILENAME)&ofn);
	if( !ret )
	{
		DWORD ErrCode = ::GetLastError();

		if( !ErrCode
		|| ErrCode == ERROR_NO_MORE_FILES
		|| ErrCode == ERROR_INVALID_PARAMETER
		|| ErrCode == ERROR_CLASS_DOES_NOT_EXIST ) // On XP I sometime get this!!!!
		{
			// user pressed Cancel button
			LOGGER( "OpenFileDlg::DoModal CANCEL end" );
		}
		else if(( ErrCode == ERROR_INVALID_PARAMETER
		       || ErrCode == ERROR_CALL_NOT_IMPLEMENTED
		       || ErrCode == ERROR_INVALID_ACCEL_HANDLE )
		&&   ( ofn.Flags&OFN_EXPLORER == OFN_EXPLORER) )
		{
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			goto TryAgain;
		}
		else
		{	// Failed, display LastError.
			//TCHAR tmp[64]; tmp[0] = TEXT('\0');
			//::wsprintf(tmp,TEXT("GetOpenFileName LastError #%d"), ErrCode);
			//::MessageBox( NULL, tmp, String(IDS_APPNAME).c_str(), MB_OK );
			LOGGER( "OpenFileDlg::DoModal FAILED end" );
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
		// �R���{�{�b�N�X�𖄂߂āA�u�����I���v��I��
		ComboBox cb( dlg, IDC_CODELIST );
		const CharSetList& csl = pThis->csl_;
		for( ulong i=0; i<csl.size(); ++i )
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
		// older NT wants OfnHook returning TRUE in WM_INITDIALOG
		return TRUE;
	}
	else if( msg==WM_NOTIFY ||( msg==WM_COMMAND && LOWORD(wp)==1 ))
	{
		// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
		if(( msg==WM_COMMAND && LOWORD(wp)==1 ) || ((LPOFNOTIFY)lp)->hdr.code==CDN_FILEOK )
		{
			TCHAR buf[32];
			buf[0] = TEXT('\0');
			::SendDlgItemMessage(dlg, IDC_CODELIST, WM_GETTEXT, countof(buf), (LPARAM)buf);
			// Typed CP has precedence over droplist
			if ( isSDigit(buf[0]) )
			{
				// Try to find value in the charset list
				pThis->csIndex_ = pThis->csl_.GetCSIfromNumStr( buf );
				return FALSE;
			}

			pThis->csIndex_ = 0;
			int i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			if( 0 <= i && i < (int)pThis->csl_.size() )
			{
				ulong j=0;
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
	}

	return FALSE;
}



//------------------------------------------------------------------------
// �u�ۑ��v�_�C�A���O
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

	myOPENFILENAME ofn = {sizeof(ofn)};
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
				OFN_OVERWRITEPROMPT;


	if( app().isNewOpenSaveDlg() )
	{
		// Include the OFN_EXPLORER flag to get the new look.
		ofn.Flags |= OFN_EXPLORER;
		// Use the new template sans the Open File controls.
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_SAVEFILEHOOK);
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
	::SetLastError(0);
	BOOL ret = ::GetSaveFileName((LPOPENFILENAME)&ofn);
	if( !ret )
	{
		DWORD ErrCode = ::GetLastError();

		if( !ErrCode
		|| ErrCode == ERROR_NO_MORE_FILES
		|| ErrCode == ERROR_INVALID_PARAMETER
		|| ErrCode == ERROR_CLASS_DOES_NOT_EXIST ) // On XP I sometime get this!!!!
		{
			// user pressed Cancel button
		}
		else if(( ErrCode == ERROR_INVALID_PARAMETER
		       || ErrCode == ERROR_CALL_NOT_IMPLEMENTED
		       || ErrCode == ERROR_INVALID_ACCEL_HANDLE )
		&&   ( ofn.Flags&OFN_EXPLORER == OFN_EXPLORER) )
		{
			// maybe Common Dialog DLL doesn't like OFN_EXPLORER, try again without it
			ofn.Flags &= ~OFN_EXPLORER;
			ofn.lpstrTitle     = TEXT("Save File As");
			ofn.lpTemplateName = (LPTSTR)MAKEINTRESOURCE(FILEOPENORD);

			// try again!
			goto TryAgain;
		}
		else
		{	// Failed, display LastError.
			TCHAR tmp[64]; tmp[0] = TEXT('\0');
			::wsprintf(tmp,TEXT("GetSaveFileName LastError #%d"), ErrCode);
			::MessageBox( wnd, tmp, String(IDS_APPNAME).c_str(), MB_OK );
		}
	}
	return ( ret != 0 );
}

UINT_PTR CALLBACK SaveFileDlg::OfnHook( HWND dlg, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg==WM_INITDIALOG )
	{
		// �R���{�{�b�N�X�𖄂߂āA�K�؂Ȃ̂�I��
		{
			ComboBox cb( dlg, IDC_CODELIST );
			const CharSetList& csl = pThis->csl_;

			for( ulong i=0; i<csl.size(); ++i )
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
				TCHAR *cpnum = (TCHAR*)Int2lStr(tmp, csi&0xfffff);
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

			for( ulong i=0; i<countof(lbList); ++i )
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
			// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
			// ���s�R�[�h��
			int lb = ComboBox(dlg,IDC_CRLFLIST).GetCurSel();
			pThis->lb_ = lb == CB_ERR? 2 :lb; // Default to CRLF;

			TCHAR buf[64];
			::SendDlgItemMessage( dlg, IDC_CODELIST, WM_GETTEXT, countof(buf), (LPARAM)buf);
			// Typed CP has precedence over droplist
			if ( isSDigit(buf[0]) )
			{
				pThis->csIndex_ = pThis->csl_.GetCSIfromNumStr(buf);
				return FALSE;
			}

			int i = ComboBox(dlg,IDC_CODELIST).GetCurSel();
			if( 0 <= i && i < (int)pThis->csl_.size() )
			{
				ulong j;
				for(j=0; ;++j,--i)
				{
					while( !(pThis->csl_[j].type & 1) ) // !SAVE
						++j;
					if( i==0 )
						break;
				}
				pThis->csIndex_ = j;
			}
			else
			{
				pThis->csIndex_ = 0;
			}
		}
	}
	return FALSE;
}



//------------------------------------------------------------------------
// ���[�e�B���e�B�[
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
// �u�J�������v�_�C�A���O
//------------------------------------------------------------------------

ReopenDlg::ReopenDlg( const CharSetList& csl, int csi )
	: DlgImpl(IDD_REOPENDLG), csl_(csl), csIndex_(csi)
{
}

void ReopenDlg::on_init()
{
	// �R���{�{�b�N�X�𖄂߂āA�u�����I���v��I��
	ComboBox cb( hwnd(), IDC_CODELIST );
	for( ulong i=0; i<csl_.size(); ++i )
		if( csl_[i].type & 1 ) // 2:=SAVE
			cb.Add( csl_[i].longName );

	int csi = csIndex_;
	if( 0 <= csi && csi < (int)csl_.size() )
	{
		// Select combobox item
		cb.Select( csl_[csi].longName );
	}
	else
	{	// Show CP number in the reopen dialog
		// If selection failed.
		TCHAR tmp[INT_DIGITS+1];
		SendMsgToItem( IDC_CODELIST, WM_SETTEXT, 0, (LPARAM)Int2lStr(tmp, csIndex_&0xfffff) );
	}
}

bool ReopenDlg::on_ok()
{
	TCHAR buf[32];
	SendMsgToItem( IDC_CODELIST, WM_GETTEXT, countof(buf), (LPARAM)buf );
	// Typed CP has precedence over droplist
	if ( isSDigit(buf[0]) )
	{
		csIndex_ = csl_.GetCSIfromNumStr( buf );
		return true;
	}

	// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
	ulong j=0;
	int i=ComboBox(hwnd(),IDC_CODELIST).GetCurSel();
	if( 0 <= i && i < (int)csl_.size() )
	{ // Only if i is in correct range.
		for(;;++j,--i)
		{
			while( !(csl_[j].type & 1) ) // !SAVE
				++j;
			if( i==0 )
				break;
		}
		csIndex_ = j;
	}
	return true;
}
