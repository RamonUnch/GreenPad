
#include "stdafx.h"
#include "rsrc/resource.h"
#include "Search.h"
#include "NSearch.h"
#include "RSearch.h"
using namespace ki;
using namespace editwing;
using view::VPos;



//-------------------------------------------------------------------------

SearchManager::SearchManager( ki::Window& w, editwing::EwEdit& e )
	: searcher_( NULL )
	, edit_( e )
	, DlgImpl( IDD_FINDREPLACE )
	, bIgnoreCase_( true ) // 1.08 default true
	, bRegExp_( false )
	, bDownSearch_( true )
	, mainWnd_( w )
{
}

SearchManager::~SearchManager()
{
}

void SearchManager::SaveToINI( ki::IniFile& ini )
{
	ini.SetSectionAsUserName();
	ini.PutBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
	ini.PutBool( TEXT("SearchRegExp"), bRegExp_ );
}

void SearchManager::LoadFromINI( ki::IniFile& ini )
{
	ini.SetSectionAsUserName();
	bIgnoreCase_ = ini.GetBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
	bRegExp_     = ini.GetBool( TEXT("SearchRegExp"), bRegExp_ );
}

//-------------------------------------------------------------------------
// �_�C�A���O�֌W
//-------------------------------------------------------------------------

void SearchManager::ShowDlg()
{
//	GoModal( ::GetParent(edit_.hwnd()) );
	if( isAlive() )
	{
		SetFront();
	}
	else
	{
		GoModeless( ::GetParent(edit_.hwnd()) );
		ShowUp();
	}
}

bool SearchManager::TrapMsg(MSG* msg)
{
	if( ! isAlive() || type()==MODAL )
		return false;
	return DlgImpl::PreTranslateMessage(msg);
}

void SearchManager::on_init()
{
	if( bIgnoreCase_ )
		SendMsgToItem( IDC_IGNORECASE, BM_SETCHECK, BST_CHECKED );
	if( bRegExp_ )
		SendMsgToItem( IDC_REGEXP, BM_SETCHECK, BST_CHECKED );

	if( edit_.getCursor().isSelected() )
	{
		// �I������Ă����Ԃł́A��{�I�ɂ�����{�b�N�X�ɕ\��
		ulong dmy;
		aarr<unicode> str = edit_.getCursor().getSelectedStr();

		ulong len=0;
		for( ; str[len]!=L'\0' && str[len]!=L'\n'; ++len );
		str[len] = L'\0';

		if( searcher_.isValid() &&
		    searcher_->Search( str.get(), len, 0, &dmy, &dmy ) )
		{
			SendMsgToItem( IDC_FINDBOX, WM_SETTEXT, 0,
				reinterpret_cast<LPARAM>(findStr_.c_str()) );
		}
		else
		{
		#ifdef _UNICODE
			SendMsgToItem( IDC_FINDBOX, WM_SETTEXT, 0,
				reinterpret_cast<LPARAM>(str.get()) );
		#else
			ki::aarr<char> ab( new TCHAR[(len+1)*3] );
			::WideCharToMultiByte( CP_ACP, 0, str.get(), -1,
				ab.get(), (len+1)*3, NULL, NULL );
			SendMsgToItem( IDC_FINDBOX, WM_SETTEXT, 0,
				reinterpret_cast<LPARAM>(ab.get()) );
		#endif
		}
	}
	else
	{
		SendMsgToItem( IDC_FINDBOX, WM_SETTEXT, 0,
			reinterpret_cast<LPARAM>(findStr_.c_str()) );
	}

	SendMsgToItem( IDC_REPLACEBOX, WM_SETTEXT, 0,
		reinterpret_cast<LPARAM>(replStr_.c_str()) );

	::SetFocus( item(IDC_FINDBOX) );
	SendMsgToItem( IDC_FINDBOX, EM_SETSEL, 0,
		::GetWindowTextLength(item(IDC_FINDBOX)) );
}

void SearchManager::on_destroy()
{
	bChanged_ = false;
}

bool SearchManager::on_command( UINT cmd, UINT id, HWND ctrl )
{
	if( cmd==EN_CHANGE )
	{
		// ������ύX�����������Ƃ��L��
		bChanged_ = true;
	}
	else if( cmd==BN_CLICKED )
	{
		switch( id )
		{
		// �`�F�b�N�{�b�N�X�̕ύX�����������Ƃ��L��
		case IDC_IGNORECASE:
		case IDC_REGEXP:
			bChanged_ = true;
			break;
		// �{�^���������ꂽ�ꍇ
		case ID_FINDNEXT:
			on_findnext();
			break;
		case ID_FINDPREV:
			on_findprev();
			break;
		case ID_REPLACENEXT:
			on_replacenext();
			break;
		case ID_REPLACEALL:
			on_replaceall();
			break;
		}
	}
	else
	{
		return false;
	}
	return true;
}

void SearchManager::on_findnext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
	{
		FindNextImpl();
//		End( IDOK );
	}
}

void SearchManager::on_findprev()
{
	UpdateData();
	ConstructSearcher( false );
	if( isReady() )
		FindPrevImpl();
}

void SearchManager::on_replacenext()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceImpl();
}

void SearchManager::on_replaceall()
{
	UpdateData();
	ConstructSearcher();
	if( isReady() )
		ReplaceAllImpl();
}

void SearchManager::UpdateData()
{
	// �_�C�A���O����ύX�_����荞��
	bIgnoreCase_ =
		(BST_CHECKED==SendMsgToItem( IDC_IGNORECASE, BM_GETCHECK ));
	bRegExp_ =
		(BST_CHECKED==SendMsgToItem( IDC_REGEXP, BM_GETCHECK ));

	TCHAR* str;
	LRESULT n = SendMsgToItem( IDC_FINDBOX, WM_GETTEXTLENGTH );
	str = new TCHAR[n+1];
	SendMsgToItem( IDC_FINDBOX, WM_GETTEXT,
		n+1, reinterpret_cast<LPARAM>(str) );
	findStr_ = str;
	delete [] str;

	n = SendMsgToItem( IDC_REPLACEBOX, WM_GETTEXTLENGTH );
	str = new TCHAR[n+1];
	SendMsgToItem( IDC_REPLACEBOX, WM_GETTEXT,
		n+1, reinterpret_cast<LPARAM>(str) );
	replStr_ = str;
	delete [] str;
}

void SearchManager::ConstructSearcher( bool down )
{
	bChanged_ = (bChanged_ || (bDownSearch_ != down));
	if( (bChanged_ || !isReady()) && findStr_.len()!=0 )
	{
		// �����ҍ쐬
		bDownSearch_ = down;
		const unicode *u = findStr_.ConvToWChar();

		if( bRegExp_ )
			searcher_ = new RSearch( u, !bIgnoreCase_, bDownSearch_ );
		else
			if( bDownSearch_ )
				if( bIgnoreCase_ )
					searcher_ = new NSearch<IgnoreCase>(u);
				else
					searcher_ = new NSearch<CaseSensitive>(u);
			else
				if( bIgnoreCase_ )
					searcher_ = new NSearchRev<IgnoreCase>(u);
				else
					searcher_ = new NSearchRev<CaseSensitive>(u);

		findStr_.FreeWCMem(u);

		// �ύX�I���t���O
		bChanged_ = false;
	}
}



//-------------------------------------------------------------------------

void SearchManager::FindNext()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher();
		if( isReady() )
			FindNextImpl();
	}
}

void SearchManager::FindPrev()
{
	if( !isReady() )
	{
		ShowDlg();
	}
	else
	{
		ConstructSearcher( false );
		if( isReady() )
			FindPrevImpl();
	}
}



//-------------------------------------------------------------------------
// ���ۂ̏����̎���
//-------------------------------------------------------------------------

void SearchManager::FindNextImpl()
{
	// �J�[�\���ʒu�擾
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// �I��͈͂���Ȃ�A�I��͈͐擪�̂P�����悩�猟��
	// �����łȂ���΃J�[�\���ʒu���猟��
	DPos s = *stt;
	if( *stt != *end )
		if( stt->ad == edit_.getDoc().len(stt->tl) )
			s = DPos( stt->tl+1, 0 );
		else
			s = DPos( stt->tl, stt->ad+1 );

	// ����
	DPos b, e;
	if( FindNextFromImpl( s, &b, &e ) )
	{
		// ����������I��
		edit_.getCursor().MoveCur( b, false );
		edit_.getCursor().MoveCur( e, true );
		return;
	}

	// ������Ȃ������ꍇ
	NotFound();
}

void SearchManager::NotFound()
{
	//MsgBox( String(IDS_NOTFOUND).c_str() );
	::MessageBox( NULL, String(IDS_NOTFOUND).c_str(), NULL, MB_OK|MB_TASKMODAL );
}

void SearchManager::FindPrevImpl()
{
	// �J�[�\���ʒu�擾
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	if( stt->ad!=0 || stt->tl!=0 )
	{
		// �I��͈͐擪�̂P�����O���猟��
		DPos s;
		if( stt->ad == 0 )
			s = DPos( stt->tl-1, edit_.getDoc().len(stt->tl-1) );
		else
			s = DPos( stt->tl, stt->ad-1 );

		// ����
		DPos b, e;
		if( FindPrevFromImpl( s, &b, &e ) )
		{
			// ����������I��
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}
	}

	// ������Ȃ������ꍇ
	NotFound();
}

bool SearchManager::FindNextFromImpl( DPos s, DPos* beg, DPos* end )
{
	// �P�s���T�[�`
	doc::Document& d = edit_.getDoc();
	for( ulong mbg,med,e=d.tln(); s.tl<e; ++s.tl, s.ad=0 )
		if( searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // ����
		}
	return false;
}

bool SearchManager::FindPrevFromImpl( DPos s, DPos* beg, DPos* end )
{
	// �P�s���T�[�`
	doc::Document& d = edit_.getDoc();
	for( ulong mbg,med; ; s.ad=d.len(--s.tl) )
	{
		if( searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // ����
		}
		if( s.tl==0 )
			break;
	}
	return false;
}

void SearchManager::ReplaceImpl()
{
	// �J�[�\���ʒu�擾
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// �I��͈͐擪���猟��
	DPos b, e;
	if( FindNextFromImpl( *stt, &b, &e ) )
		if( e == *end )
		{
			const wchar_t* ustr = replStr_.ConvToWChar();
			const ulong ulen = my_lstrlenW( ustr );

			// �u��
			edit_.getDoc().Execute( doc::Replace(
				b, e, ustr, ulen
			) );

			replStr_.FreeWCMem( ustr );

			if( FindNextFromImpl( DPos(b.tl,b.ad+ulen), &b, &e ) )
			{
				// ����I��
				edit_.getCursor().MoveCur( b, false );
				edit_.getCursor().MoveCur( e, true );
				return;
			}
		}
		else
		{
			// �����łȂ���΂Ƃ肠�����I��
			edit_.getCursor().MoveCur( b, false );
			edit_.getCursor().MoveCur( e, true );
			return;
		}

	// ������Ȃ������ꍇ
	NotFound();
}

void SearchManager::ReplaceAllImpl()
{
	// �܂��A���s����u����S�Ă����ɓo�^����
	doc::MacroCommand mcr;

	// �u���㕶����
	const wchar_t* ustr = replStr_.ConvToWChar();
	const ulong ulen = my_lstrlenW( ustr );

	// �����̓����猟��
	int dif=0;
	DPos s(0,0), b, e;
	while( FindNextFromImpl( s, &b, &e ) )
	{
		if( s.tl != b.tl ) dif = 0;
		s = e;

		// �u���R�}���h��o�^
		b.ad += dif, e.ad += dif;
		mcr.Add( new doc::Replace(b,e,ustr,ulen) );
		dif -= e.ad-b.ad-ulen;
	}

	if( mcr.size() > 0 )
	{
		// �����ŘA���u��
		edit_.getDoc().Execute( mcr );
		// �J�[�\���ړ�
		e.ad = b.ad + ulen;
		edit_.getCursor().MoveCur( e, false );
		// ����H
		End( IDOK );
	}

	TCHAR str[255];
	::wsprintf( str, String(IDS_REPLACEALLDONE).c_str(), mcr.size() );
	MsgBox( str, String(IDS_APPNAME).c_str(), MB_ICONINFORMATION );

	replStr_.FreeWCMem( ustr );
}
