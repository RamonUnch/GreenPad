#include "stdafx.h"
#include "ip_doc.h"
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_text.cpp   �����񑀍�E��
//
//		�������}��������폜������c�Ƃ����ӂ�̏�����
//		���̃t�@�C���ɂ܂Ƃ߂Ă���B�O�����̃C���^�[�t�F�C�X��
//		���������łɂ����ŁB
//
//---- ip_parse.cpp  �L�[���[�h���
//---- ip_wrap.cpp   �܂�Ԃ�
//---- ip_scroll.cpp �X�N���[��
//---- ip_draw.cpp   �`��E��
//---- ip_cursor.cpp �J�[�\���R���g���[��
//=========================================================================



//-------------------------------------------------------------------------
// ���J�C���^�[�t�F�C�X
//-------------------------------------------------------------------------

Document::Document() : busy_(false)
	{ impl_ = new DocImpl( *this ); }

Document::~Document()
	{}

void Document::Execute( const Command& c )
	{ impl_->Execute( c ); }

void Document::SetKeyword( const unicode* b, ulong s )
	{ impl_->SetKeyword( b, s ); }

void Document::AddHandler( DocEvHandler* h )
	{ impl_->AddHandler( h ); }

void Document::DelHandler( DocEvHandler* h )
	{ impl_->DelHandler( h ); }

void Document::OpenFile( aptr<TextFileR> t )
	{ impl_->OpenFile( t ); }

void Document::SaveFile( TextFileW& t )
	{ impl_->SaveFile( t ); }

void Document::ClearAll()
	{ impl_->ClearAll(); }

ulong Document::tln() const
	{ return impl_->tln(); }

const unicode* Document::tl( ulong i ) const
	{ return impl_->tl( i ); }

ulong Document::len( ulong i ) const
	{ return impl_->len( i ); }

ulong Document::getRangeLength( const DPos& s, const DPos& e ) const
	{ return impl_->getRangeLength( s, e ); }

void Document::getText( unicode* b, const DPos& s, const DPos& e ) const
	{ impl_->getText( b, s, e ); }

bool Document::isUndoAble() const
	{ return impl_->isUndoAble(); }

bool Document::isRedoAble() const
	{ return impl_->isRedoAble(); }

bool Document::isModified() const
	{ return impl_->isModified(); }

void Document::ClearModifyFlag()
	{ impl_->ClearModifyFlag(); }

void Document::Undo()
	{ impl_->Undo(); }

void Document::Redo()
	{ impl_->Redo(); }

void Document::SetUndoLimit( long lim )
	{ impl_->SetUndoLimit( lim ); }



//-------------------------------------------------------------------------
// �C�x���g�n���h������
//-------------------------------------------------------------------------

void DocImpl::AddHandler( DocEvHandler* eh ) 
{
	// �n���h���ǉ�
	pEvHan_.Add( eh );
}

void DocImpl::DelHandler( DocEvHandler* eh )
{
	// ��납�猩�čs���āc
	const int last = pEvHan_.size() - 1;

	// �c��������폜
	for( int i=last; i>=0; --i )
		if( pEvHan_[i] == eh )
		{
			pEvHan_[i] = pEvHan_[last];
			pEvHan_.ForceSize( last );
			break;
		}
}

void DocImpl::Fire_TEXTUPDATE
	( const DPos& s, const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd )
{
	AutoLock lk(this);

	// �S���ɃC�x���g�ʒm
	for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
		pEvHan_[i]->on_text_update( s, e, e2, reparsed, nmlcmd );
}

void DocImpl::Fire_KEYWORDCHANGE()
{
	AutoLock lk(this);

	// �S���ɃC�x���g�ʒm
	for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
		pEvHan_[i]->on_keyword_change();
}

void DocImpl::Fire_MODIFYFLAGCHANGE()
{
	AutoLock lk(this);

	// �S���ɃC�x���g�ʒm
	bool b = urdo_.isModified();
	for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
		pEvHan_[i]->on_dirtyflag_change( b );
}



//-------------------------------------------------------------------------
// UnDo,ReDo ����
//-------------------------------------------------------------------------

UnReDoChain::Node::Node( Command* c, Node* p, Node* n )
	: cmd_ ( c )
	, prev_( p )
	, next_( n )
{
}

UnReDoChain::Node::Node()
{
	next_ = prev_ = this;
	cmd_  = NULL;
}

UnReDoChain::Node::~Node()
{
	delete cmd_;
}

void UnReDoChain::Node::ResetCommand( Command* cmd )
{
	delete cmd_;
	cmd_ = cmd;
}

UnReDoChain::UnReDoChain()
	: lastOp_  ( &headTail_ )
	, savedPos_( &headTail_ )
	, num_     ( 0 )
	, limit_   ( static_cast<ulong>(-1) )
{
}

UnReDoChain::~UnReDoChain()
{
	Clear();
}

ulong UnReDoChain::Node::ChainDelete(Node*& savedPos_ref)
{
	if( cmd_ == NULL )
		return 0;
	if( savedPos_ref == this )
		savedPos_ref = NULL;
	dptr<Node> d(this);
	return 1 + next_->ChainDelete(savedPos_ref);
}

void UnReDoChain::Clear()
{
	headTail_.next_->ChainDelete(savedPos_);
	headTail_.next_ = headTail_.prev_ = lastOp_  = savedPos_ = &headTail_;
	num_ = 0;
}

void UnReDoChain::SetLimit( long lim )
{
	limit_ = Max( 1UL, ulong(lim) );
}

inline void UnReDoChain::Undo( Document& doc )
{
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
	lastOp_ = lastOp_->prev_;
}

inline void UnReDoChain::Redo( Document& doc )
{
	lastOp_ = lastOp_->next_;
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
}

void UnReDoChain::NewlyExec( const Command& cmd, Document& doc )
{
	Command* nCmd = cmd(doc);
	if( nCmd != NULL )
	{
		num_   -= (lastOp_->next_->ChainDelete(savedPos_) - 1);
		lastOp_ = lastOp_->next_ = new Node(nCmd,lastOp_,&headTail_);

		while( limit_ < num_ )
		{
			// �񐔐������z�����̂ŁA�Â������폜
			Node* old = headTail_.next_;
			headTail_.next_   = old->next_;
			old->next_->prev_ = &headTail_;
			if( old != &headTail_ )
				delete old;
			if( savedPos_ == &headTail_ )
				savedPos_ = NULL;
			else if( savedPos_ == old )
				savedPos_ = &headTail_;
			--num_;
		}
	}
}



bool DocImpl::isUndoAble() const
	{ return urdo_.isUndoAble(); }

bool DocImpl::isRedoAble() const
	{ return urdo_.isRedoAble(); }

bool DocImpl::isModified() const
	{ return urdo_.isModified(); }

void DocImpl::SetUndoLimit( long lim )
	{ urdo_.SetLimit( lim ); }

void DocImpl::ClearModifyFlag()
{
	bool b = urdo_.isModified();
	urdo_.SavedHere();
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}

void DocImpl::Undo()
{
	if( urdo_.isUndoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Undo(doc_);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void DocImpl::Redo()
{
	if( urdo_.isRedoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Redo(doc_);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void DocImpl::Execute( const Command& cmd )
{
	bool b = urdo_.isModified();
	urdo_.NewlyExec( cmd, doc_ );
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// �J�[�\���ړ��w���p�[
//-------------------------------------------------------------------------

DPos DocImpl::leftOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == 0 )
	{
		// �s�̐擪�����A�t�@�C���̐擪�ł͂Ȃ��ꍇ
		// ��O�̍s�̍s����
		if( dp.tl > 0 )
			return DPos( dp.tl-1, len(dp.tl-1) );
		return dp;
	}
	else if( !wide )
	{
		// �s�̓r���ŁA���ʂɂP�����߂�ꍇ
		const unicode* l = tl(dp.tl);
		if( dp.ad>=2 && isLowSurrogate(l[dp.ad-1]) && isHighSurrogate(l[dp.ad-2]) )
			return DPos( dp.tl, dp.ad-2 );
		return DPos( dp.tl, dp.ad-1 );
	}
	else
	{
		// �s�̓r���ŁA�P�P�ꕪ�߂�ꍇ
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad-1;
		while( (f[s]>>5)==0 && 0<=s )
			--s;
		return DPos( dp.tl, s );
	}
}

DPos DocImpl::rightOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == len(dp.tl) )
	{
		// �s�������A�t�@�C���̏I���ł͂Ȃ��ꍇ
		// ���̍s�̐擪��
		if( dp.tl < tln()-1 )
			return DPos( dp.tl+1, 0 );
		return dp;
	}
	else if( !wide )
	{
		// �s�̓r���ŁA���ʂɂP�����i�ޏꍇ
		const unicode* l = tl(dp.tl);
		// �ԕ� 0x007f �� l �̖����ɂ���̂Œ����`�F�b�N�͕s�v
		if( isHighSurrogate(l[dp.ad]) && isLowSurrogate(l[dp.ad+1]) )
			return DPos( dp.tl, dp.ad+2 );
		return DPos( dp.tl, dp.ad+1 );
	}
	else
	{
		// �s�̓r���ŁA���ʂɂP�P��i�ޏꍇ
		const uchar* f = pl(dp.tl);
		const ulong  e = len(dp.tl);
			  ulong  s = dp.ad;
		const ulong  t = (f[s]>>5);
		s += t;
		if( s >= e )
			s = e;
		else if( t==7 || t==0 )
			while( (f[s]>>5)==0 && s<e )
				++s;
		return DPos( dp.tl, s );
	}
}

DPos DocImpl::wordStartOf( const DPos& dp ) const
{
	if( dp.ad == 0 )
	{
		// �s�̐擪
		return dp;
	}
	else
	{
		// �s�̓r��
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad;
		while( (f[s]>>5)==0 && 0<=s )
			--s;
		return DPos( dp.tl, s );
	}
}



//-------------------------------------------------------------------------
// �}���E�폜���̍�Ɨp�֐��Q
//-------------------------------------------------------------------------

ulong DocImpl::getRangeLength( const DPos& s, const DPos& e )
{
	// �Ƃ肠�����S������
	ulong ans=0, tl=s.tl, te=e.tl;
	for( ; tl<=te; ++tl )
		ans += len(tl);
	// �擪�s�̕�������
	ans -= s.ad;
	// �ŏI�s�̕�������
	ans -= len(te) - e.ad;
	// ���s�R�[�h(CRLF)�̕���������
	ans += (e.tl-s.tl) * 2;
	// �����܂�
	return ans;
}

void DocImpl::getText( unicode* buf, const DPos& s, const DPos& e )
{
	if( s.tl == e.tl )
	{
		// ��s�����̏ꍇ
		text_[s.tl].CopyAt( s.ad, e.ad-s.ad, buf );
		buf[e.ad-s.ad] = L'\0';
	}
	else
	{
		// �擪�s�̌����R�s�[
		buf += text_[s.tl].CopyToTail( s.ad, buf );
		*buf++ = L'\r', *buf++ = L'\n';
		// �r�����R�s�[
		for( ulong i=s.tl+1; i<e.tl; i++ )
		{
			buf += text_[i].CopyToTail( 0, buf );
			*buf++ = L'\r', *buf++ = L'\n';
		}
		// �I���s�̐擪���R�s�[
		buf += text_[e.tl].CopyAt( 0, e.ad, buf );
		*buf = L'\0';
	}
}

void DocImpl::CorrectPos( DPos& pos )
{
	// ����͈͂Ɏ��܂�悤�ɏC��
	pos.tl = Min( pos.tl, tln()-1 );
	pos.ad = Min( pos.ad, len(pos.tl) );
}

void DocImpl::CorrectPos( DPos& s, DPos& e )
{
	// �K��s<=e�ɂȂ�悤�ɏC��
	if( s > e )
	{
		ulong t;
		t=s.ad, s.ad=e.ad, e.ad=t;
		t=s.tl, s.tl=e.tl, e.tl=t;
	}
}

bool DocImpl::DeletingOperation
	( DPos& s, DPos& e, unicode*& undobuf, ulong& undosiz )
{
	AutoLock lk( this );

	// �ʒu�␳
	CorrectPos( s );
	CorrectPos( e );
	CorrectPos( s, e );

	// �폜�����ʂ��J�E���g
	undosiz = getRangeLength( s, e );

	// Undo����p�o�b�t�@�m��
	undobuf = new unicode[undosiz+1];
	getText( undobuf, s, e );

	// �폜��
	if( s.tl == e.tl )
	{
		// ��s���폜
		text_[s.tl].RemoveAt( s.ad, e.ad-s.ad );
	}
	else
	{
		// �擪�s�̌����폜
		text_[s.tl].RemoveToTail( s.ad );
		// �I���s�̎c�蕔������������
		text_[s.tl].InsertToTail( tl(e.tl)+e.ad, len(e.tl)-e.ad );
		// �����s���폜
		text_.RemoveAt( s.tl+1, e.tl-s.tl );
	}

	// �ĉ��
	return ReParse( s.tl, s.tl );
}

bool DocImpl::InsertingOperation
	( DPos& s, const unicode* str, ulong len, DPos& e )
{
	AutoLock lk( this );

	// �ʒu�␳
	CorrectPos( s );

	// ��[���A�ǂ�I
	e.ad = s.ad;
	e.tl = s.tl;

	// �w�蕶��������s�Ő؂蕪���鏀��
	const unicode* lineStr;
	ulong lineLen;
	UniReader r( str, len, &lineStr, &lineLen );

	// ��s�ځc
	r.getLine();
	text_[e.tl].InsertAt( e.ad, lineStr, lineLen );
	e.ad += lineLen;

	if( !r.isEmpty() )
	{
		// ��s�ځ`�ŏI�s
		do
		{
			r.getLine();
			text_.InsertAt( ++e.tl, new Line(lineStr,lineLen) );
		} while( !r.isEmpty() );

		// ��s�ڂ̍Ō���Ɏc���Ă���������ŏI�s��
		Line& fl = text_[s.tl];
		Line& ed = text_[e.tl];
		const ulong ln = fl.size()-e.ad;
		if( ln )
		{
			ed.InsertToTail( fl.str()+e.ad, ln );
			fl.RemoveToTail( e.ad );
		}

		// �I���ʒu�L�^
		e.ad = lineLen;
	}

	// �ĉ��
	return ReParse( s.tl, e.tl );
}



//-------------------------------------------------------------------------
// �}���R�}���h
//-------------------------------------------------------------------------

Insert::Insert( const DPos& s, const unicode* str, ulong len, bool del )
	: stt_( s )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Insert::~Insert()
{
	if( del_ )
		delete const_cast<unicode*>(buf_);
}

Command* Insert::operator()( Document& d ) const
{
	DocImpl& di = d.impl();

	// �}��
	DPos s=stt_, e;
	bool aa = di.InsertingOperation( s, buf_, len_, e );

	// �C�x���g����
	di.Fire_TEXTUPDATE( s, s, e, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�
	return new Delete( s, e );
}



//-------------------------------------------------------------------------
// �폜�R�}���h
//-------------------------------------------------------------------------

Delete::Delete( const DPos& s, const DPos& e )
	: stt_( s )
	, end_( e )
{
}

Command* Delete::operator()( Document& d ) const
{
	DocImpl& di = d.impl();

	// �폜
	unicode* buf;
	ulong    siz;
	DPos s = stt_, e=end_;
	bool aa = di.DeletingOperation( s, e, buf, siz );

	// �C�x���g����
	di.Fire_TEXTUPDATE( s, e, s, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�
	return new Insert( s, buf, siz, true );
}



//-------------------------------------------------------------------------
// �u���R�}���h
//-------------------------------------------------------------------------

Replace::Replace(
	const DPos& s, const DPos& e, const unicode* str, ulong len, bool del )
	: stt_( s )
	, end_( e )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Replace::~Replace()
{
	if( del_ )
		delete const_cast<unicode*>(buf_);
}

Command* Replace::operator()( Document& d ) const
{
	DocImpl& di = d.impl();

	// �폜
	unicode* buf;
	ulong    siz;
	DPos s=stt_, e=end_;
	bool aa = di.DeletingOperation( s, e, buf, siz );

	// �}��
	DPos e2;
	aa = (di.InsertingOperation( s, buf_, len_, e2 ) || aa);

	// �C�x���g����
	di.Fire_TEXTUPDATE( s, e, e2, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�
	return new Replace( s, e2, buf, siz, true );
}



//-------------------------------------------------------------------------
// �}�N���R�}���h
//-------------------------------------------------------------------------

Command* MacroCommand::operator()( Document& doc ) const
{
	doc.setBusyFlag();

	MacroCommand* undo = new MacroCommand;
	undo->arr_.ForceSize( size() );
	for( ulong i=0,e=arr_.size(); i<e; ++i )
		undo->arr_[e-i-1] = (*arr_[i])(doc);

	doc.setBusyFlag(false);
	return undo;
}



//-------------------------------------------------------------------------
// �t�@�C���ɕۑ�
//-------------------------------------------------------------------------

void DocImpl::SaveFile( TextFileW& tf )
{
	urdo_.SavedHere();
	for( ulong i=0,iLast=tln()-1; i<=iLast; ++i )
		tf.WriteLine( tl(i), len(i), i==iLast );
}



//-------------------------------------------------------------------------
// �o�b�t�@�̓��e��S���j���i�b��j
//-------------------------------------------------------------------------

void DocImpl::ClearAll()
{
	// �S���폜
	Execute( Delete( DPos(0,0), DPos(0xffffffff,0xffffffff) ) );

	// Undo-Redo�`�F�C�����N���A
	urdo_.Clear();
	urdo_.SavedHere();
	Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// �t�@�C�����J���i�b��j
//-------------------------------------------------------------------------

void DocImpl::OpenFile( aptr<TextFileR> tf )
{
	// ToDo: �}���`�X���b�h��
	//currentOpeningFile_ = tf;
	//thd().Run( *this );

	// �}��
	DPos e(0,0);

	unicode buf[1024];
	for( ulong i=0; tf->state(); )
	{
		if( size_t L = tf->ReadLine( buf, countof(buf) ) )
		{
			DPos p(i,0xffffffff);
			InsertingOperation( p, buf, (ulong)L, e );
			i = tln() - 1;
		}
		if( tf->state() == 1 )
		{
			DPos p(i++,0xffffffff);
			InsertingOperation( p, L"\n", 1, e );
		}
	}

	// �C�x���g����
	Fire_TEXTUPDATE( DPos(0,0), DPos(0,0), e, true, false );
}


void DocImpl::StartThread()
{
/*
	// ToDo:
	aptr<TextFileR> tf = currentOpeningFile_;

	// �}��
	unicode buf[1024];
	while( !isExitRequested() && tf->state() )
	{
		ulong i, j;
		DPos s(i=tln()-1, j=len(i)), e;
		if( ulong L = tf->ReadLine( buf, countof(buf) ) )
			InsertingOperation( s, buf, L, e );
		if( tf->state() == 1 )
			InsertingOperation( DPos(i,0xffffffff), L"\n", 1, e );

		// �C�x���g����
		Fire_TEXTUPDATE( s, s, e, true, false );
	}
*/
}

