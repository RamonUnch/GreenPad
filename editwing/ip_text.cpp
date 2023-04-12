
#include "../kilib/stdafx.h"
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

void Document::OpenFile( TextFileR& t )
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
	: next_( n )
	, prev_( p )
	, cmd_ ( c )
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
	: savedPos_( &headTail_ )
	, lastOp_  ( &headTail_ )
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
	limit_ = Max( (ulong)1, ulong(lim) );
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
			// Deleted the old one because it exceeded the count limit.
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
// �J�[�\���ړ��w���p�[, Cursor movement helper
//-------------------------------------------------------------------------

DPos DocImpl::leftOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == 0 )
	{
		// �s�̐擪�����A�t�@�C���̐擪�ł͂Ȃ��ꍇ
		// ��O�̍s�̍s����
		// If beginning of a line, but not the beginning of a file
		// Go to the end of the previous line.
		if( dp.tl > 0 )
			return DPos( dp.tl-1, len(dp.tl-1) );
		return dp;
	}
	else if( !wide )
	{
		// �s�̓r���ŁA���ʂɂP�����߂�ꍇ
		// If you want to go back one character "normally", in the middle of a line
		const unicode* l = tl(dp.tl);
		if( dp.ad>=2 && isLowSurrogate(l[dp.ad-1]) && isHighSurrogate(l[dp.ad-2]) )
			return DPos( dp.tl, dp.ad-2 );
		return DPos( dp.tl, dp.ad-1 );
	}
	else
	{
		// �s�̓r���ŁA�P�P�ꕪ�߂�ꍇ, To go back one word in the middle of a line
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad;
		while( --s && (f[s]>>5)==0 );
		return DPos( dp.tl, s );
	}
}

DPos DocImpl::rightOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == len(dp.tl) )
	{
		// �s�������A�t�@�C���̏I���ł͂Ȃ��ꍇ
		// ���̍s�̐擪��
		// At the end of a line, but not at the end of a file
		// Go to the beginning of the next line.
		if( dp.tl < tln()-1 )
			return DPos( dp.tl+1, 0 );
		return dp;
	}
	else if( !wide )
	{
		// �s�̓r���ŁA���ʂɂP�����i�ޏꍇ
		// If you advance one character normally in the middle of a line
		const unicode* l = tl(dp.tl);
		// �ԕ� 0x007f �� l �̖����ɂ���̂Œ����`�F�b�N�͕s�v
		// No need to check the length of l with the #0x007f guard at the end of l.
		if( isHighSurrogate(l[dp.ad]) && isLowSurrogate(l[dp.ad+1]) )
			return DPos( dp.tl, dp.ad+2 );
		return DPos( dp.tl, dp.ad+1 );
	}
	else
	{
		// �s�̓r���ŁA���ʂɂP�P��i�ޏꍇ
		// If you advance one word normally in the middle of a line
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
		// �s�̐擪, Beginning of line
		return dp;
	}
	else
	{
		// �s�̓r��, Midline
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad+1;
		while( --s && (f[s]>>5)==0 );

		return DPos( dp.tl, s );
	}
}



//-------------------------------------------------------------------------
// �}���E�폜���̍�Ɨp�֐��Q, A set of functions for working with ins, del, etc.
//-------------------------------------------------------------------------

ulong DocImpl::getRangeLength( const DPos& s, const DPos& e )
{
	// �Ƃ肠�����S������, Just add it all up.
	ulong ans=0, tl=s.tl, te=e.tl;
	for( ; tl<=te; ++tl )
		ans += len(tl);
	// �擪�s�̕�������, subtract the first line
	ans -= s.ad;
	// �ŏI�s�̕�������, Subtract the last line
	ans -= len(te) - e.ad;
	// ���s�R�[�h(CRLF)�̕���������, Add the portion of the line feed code (CRLF)
	ans += (e.tl-s.tl) * 2;
	// �����܂�, The end
	return ans;
}

void DocImpl::getText( unicode* buf, const DPos& s, const DPos& e )
{
	if( !buf ) return;
	if( s.tl == e.tl )
	{
		// ��s�����̏ꍇ, If you have only one line
		text_[s.tl].CopyAt( s.ad, e.ad-s.ad, buf );
		buf[e.ad-s.ad] = L'\0';
	}
	else
	{
		// �擪�s�̌����R�s�[, Copy the end of the first line
		buf += text_[s.tl].CopyToTail( s.ad, buf );
		*buf++ = L'\r', *buf++ = L'\n';
		// �r�����R�s�[, Copy the middle
		for( ulong i=s.tl+1; i<e.tl; i++ )
		{
			buf += text_[i].CopyToTail( 0, buf );
			*buf++ = L'\r', *buf++ = L'\n';
		}
		// �I���s�̐擪���R�s�[, Copy the beginning of the end line
		buf += text_[e.tl].CopyAt( 0, e.ad, buf );
		*buf = L'\0';
	}
}

void DocImpl::CorrectPos( DPos& pos )
{
	// ����͈͂Ɏ��܂�悤�ɏC��, Fix to fall within normal range.
	pos.tl = Min( pos.tl, tln()-1 );
	pos.ad = Min( pos.ad, len(pos.tl) );
}

void DocImpl::CorrectPos( DPos& s, DPos& e )
{
	// �K��s<=e�ɂȂ�悤�ɏC��,  Fix it so that s<=e.
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

	// �ʒu�␳, Position correction
	CorrectPos( s );
	CorrectPos( e );
	CorrectPos( s, e );

	// �폜�����ʂ��J�E���g, Amount to be deleted
	undosiz = getRangeLength( s, e );

	// Undo����p�o�b�t�@�m��, Allocate buffer for Undo operation
	undobuf = new unicode[undosiz+1];
	if(!undobuf)
	{ // Settext to "" if we are unable to allocate memory.
		undobuf = new unicode[1];
		undobuf[0] = 0;
		undosiz=0;
	}
	else
	{ // We got enough memory...
		getText( undobuf, s, e );// get text to del for undo
	}

	// �폜��, delete
	if( s.tl == e.tl )
	{
		// ��s���폜, line-by-line deletion
		text_[s.tl].RemoveAt( s.ad, e.ad-s.ad );
	}
	else
	{
		// �擪�s�̌����폜, Remove the end of the first line
		text_[s.tl].RemoveToTail( s.ad );
		// �I���s�̎c�蕔������������, Attach the rest of the end line
		text_[s.tl].InsertToTail( tl(e.tl)+e.ad, len(e.tl)-e.ad );
		// �����s���폜, delete a line that doesn't belong
		text_.RemoveAt( s.tl+1, e.tl-s.tl );
	}

	// �ĉ��, Reanalysis
	return ReParse( s.tl, s.tl );
}

bool DocImpl::InsertingOperation
	( DPos& s, const unicode* str, ulong len, DPos& e )
{
	AutoLock lk( this );

	// �ʒu�␳, Position correction
	CorrectPos( s );

	// ��[���A�ǂ�I, All right, Go!
	e.ad = s.ad;
	e.tl = s.tl;

	// �w�蕶��������s�Ő؂蕪���鏀��
	// Prepare to separate the specified string with a newline.
	const unicode* lineStr;
	ulong lineLen;
	UniReader r( str, len, &lineStr, &lineLen );

	// ��s�ځc, The first line...
	r.getLine();
	text_[e.tl].InsertAt( e.ad, lineStr, lineLen );
	e.ad += lineLen;

	if( !r.isEmpty() )
	{
		// ��s�ځ`�ŏI�s, Second to last line
		do
		{
			r.getLine();
			text_.InsertAt( ++e.tl, new Line(lineStr,lineLen) );
		} while( !r.isEmpty() );

		// ��s�ڂ̍Ō���Ɏc���Ă���������ŏI�s��
		// Move the remaining string from the end
		// of the first line to the last line.
		Line& fl = text_[s.tl];
		Line& ed = text_[e.tl];
		const ulong ln = fl.size()-e.ad;
		if( ln )
		{
			ed.InsertToTail( fl.str()+e.ad, ln );
			fl.RemoveToTail( e.ad );
		}

		// �I���ʒu�L�^, Record end position
		e.ad = lineLen;
	}

	// �ĉ��, Reanalysis
	return ReParse( s.tl, e.tl );
}



//-------------------------------------------------------------------------
// �}���R�}���h, Insert command
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

	// �}��, Insertion
	DPos s=stt_, e;
	bool aa = di.InsertingOperation( s, buf_, len_, e );

	// �C�x���g����, Event firing
	di.Fire_TEXTUPDATE( s, s, e, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�, Return the reverse operation object
	return new Delete( s, e );
}



//-------------------------------------------------------------------------
// �폜�R�}���h, delete command
//-------------------------------------------------------------------------

Delete::Delete( const DPos& s, const DPos& e )
	: stt_( s )
	, end_( e )
{
}

Command* Delete::operator()( Document& d ) const
{
	DocImpl& di = d.impl();

	// �폜, Deletion
	unicode* buf;
	ulong    siz;
	DPos s = stt_, e=end_;
	bool aa = di.DeletingOperation( s, e, buf, siz );

	// �C�x���g����, Event firing
	di.Fire_TEXTUPDATE( s, e, s, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�, Return the reverse operation object
	return new Insert( s, buf, siz, true );
}



//-------------------------------------------------------------------------
// �u���R�}���h, Replace command
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

	// �폜, Deletion
	unicode* buf;
	ulong    siz;
	DPos s=stt_, e=end_;
	bool aa = di.DeletingOperation( s, e, buf, siz );

	// �}��, Insertion
	DPos e2;
	aa = (di.InsertingOperation( s, buf_, len_, e2 ) || aa);

	// �C�x���g����, event firing
	di.Fire_TEXTUPDATE( s, e, e2, aa, true );

	// �t����I�u�W�F�N�g��Ԃ�, Return the reverse operation object
	return new Replace( s, e2, buf, siz, true );
}



//-------------------------------------------------------------------------
// �}�N���R�}���h, macro command
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
// �t�@�C���ɕۑ�, Save to file
//-------------------------------------------------------------------------

void DocImpl::SaveFile( TextFileW& tf )
{
	urdo_.SavedHere();
	for( ulong i=0,iLast=tln()-1; i<=iLast; ++i )
		tf.WriteLine( tl(i), len(i), i==iLast );
}



//-------------------------------------------------------------------------
// �o�b�t�@�̓��e��S���j���i�b��j, Destroy all buffer contents (temporary)
//-------------------------------------------------------------------------

void DocImpl::ClearAll()
{
	// �S���폜, delete everything
	Execute( Delete( DPos(0,0), DPos(0xffffffff,0xffffffff) ) );

	// Undo-Redo�`�F�C�����N���A
	// Clear the Undo-Redo chain
	urdo_.Clear();
	urdo_.SavedHere();
	Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// �t�@�C�����J���i�b��j, Open a file (temporary)
//-------------------------------------------------------------------------

void DocImpl::OpenFile( TextFileR& tf )
{
	// ToDo: �}���`�X���b�h��, ToDo: multi-threaded
	//currentOpeningFile_ = tf;
	//thd().Run( *this );
	// �}��, Insertion
	DPos e(0,0);

	// Super small stack buffer in case the malloc fails
	#define SBUF_SZ 1800
	unicode sbuf[SBUF_SZ];

	// Use big buffer (much faster on long lines)
#ifdef WIN64
	size_t buf_sz = 2097152;
	// static unicode buf[2097152]; // 4MB on x64
#else
	// static unicode buf[131072]; // 256KB on i386
	size_t buf_sz = 131072;
#endif
	// Do not allocate more mem than twice the file size in bytes.
	// Should help with loaing small files on Win32s.
	buf_sz = Min( buf_sz, (size_t)(tf.size()+16)<<1 );
	unicode *buf=NULL;
	if( buf_sz > SBUF_SZ )
		buf = new unicode[buf_sz];
	if( !buf )
	{
		buf = sbuf;
		buf_sz = SBUF_SZ;
	}
	for( ulong i=0; tf.state(); )
	{
		if( size_t L = tf.ReadBuf( buf, buf_sz ) )
		{
			DPos p(i,0xffffffff);
			InsertingOperation( p, buf, (ulong)L, e );
			i = tln() - 1;
		}
		if( tf.state() == 1 )
		{
			DPos p(i++,0xffffffff);
			InsertingOperation( p, L"\n", 1, e );
		}
	}

	if( buf != sbuf )
		delete [] buf;

	// �C�x���g����, Event firing
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

