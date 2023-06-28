
#include "kilib/stdafx.h"
#include "rsrc/resource.h"
#include "Search.h"
#include "NSearch.h"
#include "RSearch.h"
using namespace ki;
using namespace editwing;
using view::VPos;



//-------------------------------------------------------------------------

SearchManager::SearchManager( ki::Window& w, editwing::EwEdit& e )
	: DlgImpl( IDD_FINDREPLACE )
	, edit_( e )
	, searcher_( NULL )
	, mainWnd_( w )
	, bIgnoreCase_( true ) // 1.08 default true
	, bRegExp_( false )
	, bDownSearch_( true )
	, inichanged_( false )
{
}

SearchManager::~SearchManager()
{
	if(searcher_)
		delete searcher_;
}

void SearchManager::SaveToINI()
{
	if (inichanged_)
	{
		ki::IniFile ini;
		inichanged_ = false;
		ini.SetSectionAsUserName();
		ini.PutBool( TEXT("SearchIgnoreCase"), bIgnoreCase_ );
		ini.PutBool( TEXT("SearchRegExp"), bRegExp_ );
	}
}

void SearchManager::LoadFromINI()
{
	ki::IniFile ini;
	inichanged_ = false;
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
		SetActive();
	}
	else
	{
		GoModeless( ::GetParent(edit_.hwnd()) );
		SetCenter(hwnd(), edit_.hwnd());
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
	if( bIgnoreCase_ ) CheckItem( IDC_IGNORECASE );
	if( bRegExp_ )     CheckItem( IDC_REGEXP );

	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	// Set non multiline selection as find string.
	if( edit_.getCursor().isSelected() && stt->tl == end->tl )
	{
		// �I������Ă����Ԃł́A��{�I�ɂ�����{�b�N�X�ɕ\��
		ulong dmy;
		aarr<unicode> str = edit_.getCursor().getSelectedStr();

		ulong len=0;
		for( ; str[len]!=L'\0' && str[len]!=L'\n'; ++len );
		str[len] = L'\0';

		if( searcher_ &&
		    searcher_->Search( str.get(), len, 0, &dmy, &dmy ) )
		{
			SetItemText( IDC_FINDBOX, findStr_.c_str() );
		}
		else
		{
		#ifdef _UNICODE
			SetItemText( IDC_FINDBOX, str.get() );
		#else
			char *ab = new TCHAR[(len+1)*3];
			::WideCharToMultiByte( CP_ACP, 0, str.get(), -1,
				ab, (len+1)*3, NULL, NULL );
			SetItemText( IDC_FINDBOX, ab );
			delete [] ab;
		#endif
		}
	}
	else
	{
		SetItemText( IDC_FINDBOX, findStr_.c_str() );
	}

	SetItemText( IDC_REPLACEBOX, replStr_.c_str() );

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
	bool IgnoreCase = isItemChecked( IDC_IGNORECASE );
	bool RegExp     = isItemChecked( IDC_REGEXP );

	// Must we save save to ini?
	inichanged_ = bIgnoreCase_ != IgnoreCase || RegExp != bRegExp_;
	bIgnoreCase_ = IgnoreCase;
	bRegExp_ = RegExp;

	TCHAR* str;
	LRESULT n = SendMsgToItem( IDC_FINDBOX, WM_GETTEXTLENGTH );
	str = new TCHAR[n+1];
	GetItemText( IDC_FINDBOX, n+1, str );
	findStr_ = str;
	delete [] str;

	n = SendMsgToItem( IDC_REPLACEBOX, WM_GETTEXTLENGTH );
	str = new TCHAR[n+1];
	GetItemText( IDC_REPLACEBOX, n+1, str );
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

		if( searcher_ )
			delete searcher_;
		searcher_ = NULL;
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

void SearchManager::FindNextImpl(bool redo)
{
	// �J�[�\���ʒu�擾
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );

	// �I��͈͂���Ȃ�A�I��͈͐擪�̂P�����悩�猟��
	// �����łȂ���΃J�[�\���ʒu���猟��
	DPos s = *stt;
	if( *stt != *end )
	{
		if( stt->ad == edit_.getDoc().len(stt->tl) )
			s = DPos( stt->tl+1, 0 );
		else
			s = DPos( stt->tl, stt->ad+1 );
	}
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
	NotFound(!redo);
}

void SearchManager::NotFound(bool GoingDown)
{
	//MsgBox( RzsString(IDS_NOTFOUND).c_str() );
	if (GoingDown) {
		if (IDOK == MsgBox( RzsString(IDS_NOTFOUNDDOWN).c_str(), NULL, MB_OKCANCEL )) {
			edit_.getCursor().MoveCur( DPos(0,0), false );
			FindNextImpl(true);
		}
	} else {
	    MsgBox(RzsString(IDS_NOTFOUND).c_str(), NULL, MB_OK);
	}
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
	// �P�s���T�[, Search one line at a time
	doc::Document& d = edit_.getDoc();
	for( ulong mbg,med,e=d.tln(); s.tl<e; ++s.tl, s.ad=0 )
		if( searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // ����, Found!
		}
	return false;
}

bool SearchManager::FindPrevFromImpl( DPos s, DPos* beg, DPos* end )
{
	// �P�s���T�[�`, Search one line at a time
	doc::Document& d = edit_.getDoc();
	for( ulong mbg,med; ; s.ad=d.len(--s.tl) )
	{
		if( searcher_->Search(
			d.tl(s.tl), d.len(s.tl), s.ad, &mbg, &med ) )
		{
			beg->tl = end->tl = s.tl;
			beg->ad = mbg;
			end->ad = med;
			return true; // ����, Found!
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
	{
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

	// Get selection position
	const VPos *stt, *end;
	edit_.getCursor().getCurPos( &stt, &end );
	DPos oristt = *stt;

	// Set begining and end for replace all
	// if multi-line selection
	DPos s(0,0), dend(0, 0);
	bool noselection = true;
	if(stt->tl != end->tl)
	{ // Multi-line selection
		noselection = false;
		s = *stt; // Start of selection
		dend = *end; // End of selection
	}

	// �����̓����猟��, Search from the beginning of the document (or selection)
	int dif=0;
	DPos b, e;
	while( FindNextFromImpl( s, &b, &e ) && (noselection || e <= dend) )
	{ // search until the end of selection if any
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
		if (noselection)
		{
			edit_.getCursor().MoveCur( e, false );
		}
		else
		{ // Re-select the text that was modified if needed.
			edit_.getCursor().MoveCur( oristt, false );
			edit_.getCursor().MoveCur( DPos(dend.tl, dend.ad+dif), true );
		}
		// ����H
		End( IDOK );
	}

	TCHAR str[256+INT_DIGITS+1];
	::wsprintf( str, RzsString(IDS_REPLACEALLDONE).c_str(), mcr.size() );
	MsgBox( str, RzsString(IDS_APPNAME).c_str(), MB_ICONINFORMATION );

	replStr_.FreeWCMem( ustr );
}
