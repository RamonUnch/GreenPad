#include "stdafx.h"
#include "rsrc/resource.h"
#include "kilib/kilib.h"
#include "OpenSaveDlg.h"
using namespace ki;



//------------------------------------------------------------------------
// �����R�[�h���X�g
//------------------------------------------------------------------------

CharSetList::CharSetList()
	: list_( 30 )
{
	static const TCHAR* const lnmJp[] = {
		TEXT("��������"),
		TEXT("���{��(ShiftJIS)"),
		TEXT("���{��(EUC)"),
		TEXT("���{��(ISO-2022-JP)"),
		TEXT("�؍���(EUC-KR)"),
		TEXT("�؍���(ISO-2022-KR)"),
		TEXT("�؍���(Johab)"),
		TEXT("������(GB18030)"),
		TEXT("������(GB2312)"),
		TEXT("������(ISO-2022-CN)"),
		TEXT("������(HZ)"),
		TEXT("������(Big5)"),
		TEXT("������(EUC-TW/CNS)"),
		TEXT("������(TCA)"),
		TEXT("������(ETen)"),
		TEXT("������(IBM 5550)"),
		TEXT("������(Teletext)"),
		TEXT("������(Wang)"),
		TEXT("UTF-1"),
		TEXT("UTF-1(BOM)"),
		TEXT("UTF-5"),
		TEXT("UTF-7"),
		TEXT("UTF-8"),
		TEXT("UTF-8N"),
		TEXT("UTF-9(1997)"),
		TEXT("UTF-9(1997,BOM)"),
		TEXT("UTF-16BE(BOM)"),
		TEXT("UTF-16LE(BOM)"),
		TEXT("UTF-16BE"),
		TEXT("UTF-16LE"),
		TEXT("UTF-32BE(BOM)"),
		TEXT("UTF-32LE(BOM)"),
		TEXT("UTF-32BE"),
		TEXT("UTF-32LE"),
		TEXT("SCSU"),
		TEXT("BOCU"),
		TEXT("FSS-UTF(19920902)"),
		TEXT("FSS-UTF(19920902,BOM)"),
		TEXT("����(DOS)"),
		TEXT("����"),
		TEXT("����(DOS)"),
		TEXT("����"),
		TEXT("�L������(IBM)"),
		TEXT("�L������(MS-DOS)"),
		TEXT("�L������(Windows)"),
		TEXT("�L������(KOI8-R)"),
		TEXT("�L������(KOI8-U)"),
		TEXT("�^�C��"),
		TEXT("�g���R��(DOS)"),
		TEXT("�g���R��"),
		TEXT("�o���g��(IBM)"),
		TEXT("�o���g��"),
		TEXT("�x�g�i����"),
		TEXT("�M���V����(IBM)"),
		TEXT("�M���V����(MS-DOS)"),
		TEXT("�M���V����"),
		TEXT("�A���r�A��(IBM)"),
		TEXT("�A���r�A��(MS-DOS)"),
		TEXT("�A���r�A��"),
		TEXT("�w�u���C��(DOS)"),
		TEXT("�w�u���C��"),
		TEXT("�|���g�K����(DOS)"),
		TEXT("�A�C�X�����h��(DOS)"),
		TEXT("�t�����X��(�J�i�_)(DOS)"),
		TEXT("MSDOS(�k��)"),
		TEXT("MSDOS(us)")
	};
	static const TCHAR* const lnmEn[] = {
		TEXT("AutoDetect"),
		TEXT("Japanese(ShiftJIS)"),
		TEXT("Japanese(EUC)"),
		TEXT("Japanese(ISO-2022-JP)"),
		TEXT("Korean(EUC-KR)"),
		TEXT("Korean(ISO-2022-KR)"),
		TEXT("Korean(Johab)"),
		TEXT("Chinese(GB18030)"),
		TEXT("Chinese(GB2312)"),
		TEXT("Chinese(ISO-2022-CN)"),
		TEXT("Chinese(HZ)"),
		TEXT("Chinese(Big5)"),
		TEXT("Chinese(EUC-TW/CNS)"),
		TEXT("Chinese(TCA)"),
		TEXT("Chinese(ETen)"),
		TEXT("Chinese(IBM 5550)"),
		TEXT("Chinese(Teletext)"),
		TEXT("Chinese(Wang)"),
		TEXT("UTF-1"),
		TEXT("UTF-1(BOM)"),
		TEXT("UTF-5"),
		TEXT("UTF-7"),
		TEXT("UTF-8"),
		TEXT("UTF-8N"),
		TEXT("UTF-9(1997)"),
		TEXT("UTF-9(1997,BOM)"),
		TEXT("UTF-16BE(BOM)"),
		TEXT("UTF-16LE(BOM)"),
		TEXT("UTF-16BE"),
		TEXT("UTF-16LE"),
		TEXT("UTF-32BE(BOM)"),
		TEXT("UTF-32LE(BOM)"),
		TEXT("UTF-32BE"),
		TEXT("UTF-32LE"),
		TEXT("SCSU"),
		TEXT("BOCU"),
		TEXT("FSS-UTF(19920902)"),
		TEXT("FSS-UTF(19920902,BOM)"),
		TEXT("Latin-1(DOS)"),
		TEXT("Latin-1"),
		TEXT("Latin-2(DOS)"),
		TEXT("Latin-2"),
		TEXT("Cyrillic(IBM)"),
		TEXT("Cyrillic(MS-DOS)"),
		TEXT("Cyrillic(Windows)"),
		TEXT("Cyrillic(KOI8-R)"),
		TEXT("Cyrillic(KOI8-U)"),
		TEXT("Thai"),
		TEXT("Turkish(DOS)"),
		TEXT("Turkish"),
		TEXT("Baltic(IBM)"),
		TEXT("Baltic"),
		TEXT("Vietnamese"),
		TEXT("Greek(IBM)"),
		TEXT("Greek(MS-DOS)"),
		TEXT("Greek"),
		TEXT("Arabic(IBM)"),
		TEXT("Arabic(MS-DOS)"),
		TEXT("Arabic"),
		TEXT("Hebrew(DOS)"),
		TEXT("Hebrew"),
		TEXT("Portuguese(DOS)"),
		TEXT("Icelandic(DOS)"),
		TEXT("Canadian French(DOS)"),
		TEXT("MSDOS(Nodic)"),
		TEXT("MSDOS(us)")
	};
	static const TCHAR* const snm[] = {
		TEXT(""),
		TEXT("SJIS"),
		TEXT("EUC"),
		TEXT("JIS"),
		TEXT("UHC"),
		TEXT("I2022KR"),
		TEXT("Johab"),
		TEXT("GB18030"),
		TEXT("GBK"),
		TEXT("I2022CN"),
		TEXT("HZ"),
		TEXT("BIG5"),
		TEXT("CNS"),
		TEXT("TCA"),
		TEXT("ETEN"),
		TEXT("IBM5550"),
		TEXT("TLTEXT"),
		TEXT("WANG"),
		TEXT("UTF1"),
		TEXT("UTF1"),
		TEXT("UTF5"),
		TEXT("UTF7"),
		TEXT("UTF8"),
		TEXT("UTF8"),
		TEXT("UTF9"),
		TEXT("UTF9"),
		TEXT("U16BE"),
		TEXT("U16LE"),
		TEXT("U16BE"),
		TEXT("U16LE"),
		TEXT("U32BE"),
		TEXT("U32LE"),
		TEXT("U32BE"),
		TEXT("U32LE"),
		TEXT("SCSU"),
		TEXT("BOCU"),
		TEXT("FSSUTF"),
		TEXT("FSSUTF"),
		TEXT("LN1DOS"),
		TEXT("LTN1"),
		TEXT("LN2DOS"),
		TEXT("LTN2"),
		TEXT("CYRIBM"),
		TEXT("CYRDOS"),
		TEXT("CYRL"),
		TEXT("KO8R"),
		TEXT("KO8U"),
		TEXT("THAI"),
		TEXT("TRKDOS"),
		TEXT("TRK"),
		TEXT("BALIBM"),
		TEXT("BALT"),
		TEXT("VTNM"),
		TEXT("GRKIBM"),
		TEXT("GRKDOS"),
		TEXT("GRK"),
		TEXT("ARAIBM"),
		TEXT("ARADOS"),
		TEXT("ARA"),
		TEXT("HEBDOS"),
		TEXT("HEB"),
		TEXT("PRT"),
		TEXT("ICE"),
		TEXT("CFR"),
		TEXT("NODIC"),
		TEXT("DOS")
	};

	// ���{����Ȃ���{��\����I��
	const TCHAR* const * lnm = (::GetACP()==932 ? lnmJp : lnmEn);

	// �������������̖ʓ|�Ȃ̂ŒZ�k�\�L(^^;
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

	// �K�X�o�^
	                               EnrollL( AutoDetect,      0 );
	if( ::IsValidCodePage(932) )   Enroll(  SJIS,            1 ),
	                               Enroll(  EucJP,           2 ),
	                               Enroll(  IsoJP,           3 );
	if( ::IsValidCodePage(949) )   Enroll(  UHC,             4 ),
	                               Enroll(  IsoKR,           5 );
	if( ::IsValidCodePage(1361) )  Enroll(  Johab,           6 );
	if( ::IsValidCodePage(54936) ) Enroll(  GB18030,         7 );
	if( ::IsValidCodePage(936) )   Enroll(  GBK,             8 ),
	                               Enroll(  IsoCN,           9 ),
	                               Enroll(  HZ   ,          10 );
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

	// �I��
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
// �u�J���v�_�C�A���O
//------------------------------------------------------------------------

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
	CurrentDirRecovery cdr;
	TCHAR filepath_[MAX_PATH];

	if( fnm == NULL )
	{
		filename_[0] = TEXT('\0');
		filepath_[0] = TEXT('\0');
	}
	else
	{
		::lstrcpy(filepath_, fnm);
		::lstrcpy(filename_, fnm);
		
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
				OFN_ENABLETEMPLATE;

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
	return ( ::GetOpenFileName(&ofn) != 0 );
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
	}
	else if( msg==WM_NOTIFY ||( msg==WM_COMMAND && LOWORD(wp)==1 ))
	{
		// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
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
		::lstrcpy(filepath_, fnm);
		::lstrcpy(filename_, fnm);
		
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
				OFN_OVERWRITEPROMPT;

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

	pThis        = this;
	return ( ::GetSaveFileName(&ofn) != 0 );
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
			// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
			ulong j=0, i=ComboBox(dlg,IDC_CODELIST).GetCurSel();
			for(;;++j,--i)
			{
				while( !(pThis->csl_[j].type & 1) ) // !SAVE
					++j;
				if( i==0 )
					break;
			}
			pThis->csIndex_ = j;
			// ���s�R�[�h��
			pThis->lb_ = ComboBox(dlg,IDC_CRLFLIST).GetCurSel();
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
		::lstrcpy( p, lst[i].c_str() );
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
	cb.Select( csl_[csIndex_].longName );
}

bool ReopenDlg::on_ok()
{
	// OK�������ꂽ��A�����R�[�h�̑I���󋵂��L�^
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
