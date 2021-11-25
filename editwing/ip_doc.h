#ifndef _EDITWING_IP_DOC_H_
#define _EDITWING_IP_DOC_H_
#include "ewDoc.h"
using namespace ki;
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



//@{ @pkg editwing.Doc.Impl //@}
class Parser;



//=========================================================================
//@{
//	�s�o�b�t�@�\����
//
//	UCS-2�x�^�̌`���Ńe�L�X�g�f�[�^��ێ�����B�܂�����Ɠ����ɁA
//	�L�[���[�h�t�@�C���ɂ���Ďw�肳�ꂽ���������ʂ��邽�߂�
//	��͏������ʗp�o�b�t�@���Ǘ�����B�����f�[�^�ɏI�[NUL��
//	�t���Ȃ����A��͍�Ƃ̍������̂��߁A�I�[ U+007f ������B
//@}
//=========================================================================

class Line : public Object
{
public:

	//@{ �w��e�L�X�g�ŏ����� //@}
	Line( const unicode* str, ulong len )
		: alen_( 10>len ? 10 : len )
		, len_ ( len )
		, str_ ( static_cast<unicode*>( mem().Alloc((alen_+1)*2+alen_) ) )
		, flg_ ( reinterpret_cast<uchar*>(str_+alen_+1) )
		, commentBitReady_( false )
		, isLineHeadCommented_( 0 )
		{
			memmove( str_, str, len*2 );
			str_[ len ] = 0x007f;
		}

	~Line()
		{
			mem().DeAlloc( str_, (alen_+1)*2+alen_ );
		}

	//@{ �e�L�X�g�}��(�w��ʒu�Ɏw��T�C�Y) //@}
	void InsertAt( ulong at, const unicode* buf, ulong siz )
		{
			if( len_+siz > alen_ )
			{
				// �o�b�t�@�g��
				ulong psiz = (alen_+1)*2+alen_;
				alen_ = Max( alen_<<1, len_+siz );
				unicode* tmpS =
					static_cast<unicode*>( mem().Alloc((alen_+1)*2+alen_) );
				uchar*   tmpF =
					reinterpret_cast<uchar*>(tmpS+alen_+1);
				// �R�s�[
				memmove( tmpS,        str_,             at*2 );
				memmove( tmpS+at+siz, str_+at, (len_-at+1)*2 );
				memmove( tmpF,        flg_,             at   );
				// �Â��̂��폜
				mem().DeAlloc( str_, psiz );
				str_ = tmpS;
				flg_ = tmpF;
			}
			else
			{
				memmove( str_+at+siz, str_+at, (len_-at+1)*2 );
				memmove( flg_+at+siz, flg_+at, (len_-at)     );
			}
			memmove( str_+at, buf, siz*sizeof(unicode) );
			len_ += siz;
		}

	//@{ �e�L�X�g�}��(������) //@}
	void InsertToTail( const unicode* buf, ulong siz )
		{
			InsertAt( len_, buf, siz );
		}

	//@{ �e�L�X�g�폜(�w��ʒu����w��T�C�Y) //@}
	void RemoveAt( ulong at, ulong siz )
		{
			memmove( str_+at, str_+at+siz, (len_-siz-at+1)*2 );
			memmove( flg_+at, flg_+at+siz, (len_-siz-at)     );
			len_ -= siz;
		}

	//@{ �e�L�X�g�폜(�w��ʒu���疖���܂�) //@}
	void RemoveToTail( ulong at )
		{
			if( at < len_ )
				str_[ len_=at ] = 0x007f;
		}

	//@{ �o�b�t�@�ɃR�s�[(�w��ʒu����w��T�C�Y) //@}
	ulong CopyAt( ulong at, ulong siz, unicode* buf )
		{
			memmove( buf, str_+at, siz*sizeof(unicode) );
			return siz;
		}

	//@{ �o�b�t�@�ɃR�s�[(�w��ʒu���疖���܂�) //@}
	ulong CopyToTail( ulong at, unicode* buf )
		{
			return CopyAt( at, len_-at, buf );
		}

	//@{ ���� //@}
	ulong size() const
		{ return len_; }

	//@{ �e�L�X�g //@}
	unicode* str()
		{ return str_; }

	//@{ �e�L�X�g(const) //@}
	const unicode* str() const
		{ return str_; }

	//@{ ��͌��� //@}
	uchar* flg()
		{ return flg_; }

	//@{ ��͌���(const) //@}
	const uchar* flg() const
		{ return flg_; }

	// ask
	bool isCmtBitReady() const
		{ return commentBitReady_; }
	uchar isLineHeadCmt() const
		{ return isLineHeadCommented_; }
	// for doc
	uchar TransitCmt( uchar start )
		{
			isLineHeadCommented_ = start;
			commentBitReady_     = false;
			return (commentTransition_>>start)&1;
		}
	// for parser
	void SetTransitFlag( uchar flag )
		{ commentTransition_ = flag; }
	void CommentBitUpdated()
		{ commentBitReady_   = true; }

private:
	ulong    alen_;
	ulong    len_;
	unicode* str_;
	uchar*   flg_;

	uchar isLineHeadCommented_;
	uchar commentTransition_;
	bool  commentBitReady_;
};



//=========================================================================
//@{
//	Unicode�e�L�X�g�؂蕪���N
//
//	�s�P�ʂŏ������s�����Ƃ������̂ŁA���̍s���ɕ����鏈����
//	�؂�o�����BgetLine() ���邽�тɁA�w�肵���|�C���^�Ɛ����ϐ���
//	�擪���珇�ɍs�̃f�[�^���i�[���čs���B
//@}
//=========================================================================

class UniReader
{
public:

	//@{ �ǂ݂Ƃ茳�o�b�t�@��^���ď����� //@}
	UniReader(
		const unicode*     str, ulong     len,
		const unicode** ansstr, ulong* anslen )
		: ptr_  ( str )
		, end_  ( str+len )
		, ans_  ( ansstr )
		, aln_  ( anslen )
		, empty_( false ) {}

	//@{ �ǂݏI��������ǂ����`�F�b�N //@}
	bool isEmpty()
		{ return empty_; }

	//@{ ��s�擾 //@}
	void getLine()
		{
			// ���̉��s�̈ʒu���擾
			const unicode *p=ptr_, *e=end_;
			for( ; p<e; ++p )
				if( *p == L'\r' || *p == L'\n' )
					break;
			// �L�^
			*ans_  = ptr_;
			*aln_  = int(p-ptr_);
			// ���s�R�[�h�X�L�b�v
			if( p == e )
				empty_ = true;
			else
				if( *(p++)==L'\r'&& p<e && *p==L'\n' )
					++p;
			ptr_  = p;
		}

private:
	bool  empty_;
	const unicode *ptr_, *end_, **ans_;
	ulong *aln_;
};



//=========================================================================
//@{
//	Undo/Redo�p�ɁACommand�I�u�W�F�N�g��ۑ����Ă����N���X
//@}
//=========================================================================

class UnReDoChain : public Object
{
public:

	//@{ �R���X�g���N�^ //@}
	UnReDoChain();

	//@{ �f�X�g���N�^ //@}
	~UnReDoChain();

	//@{ Undo���s //@}
	void Undo( Document& doc );

	//@{ Redo���s //@}
	void Redo( Document& doc );

	//@{ �V�����R�}���h�����s //@}
	void NewlyExec( const Command& cmd, Document& doc );

	//@{ ������Ԃɖ߂� //@}
	void Clear();

	//@{ �ۑ��ʒu�t���O�����݈ʒu�ɃZ�b�g //@}
	void SavedHere();

	//@{ Undo/Redo�̉񐔐������w��B-1 = Infinite //@}
	void SetLimit( long lim );

public:

	//@{ Undo���삪�\���H //@}
	bool isUndoAble() const;

	//@{ Redo���삪�\���H //@}
	bool isRedoAble() const;

	//@{ �ۑ���A�ύX����Ă��邩�H //@}
	bool isModified() const;

private:

	struct Node : public Object
	{
		Node();
		Node( Command*, Node*, Node* );
		~Node();
		void  ResetCommand( Command* cmd );
		ulong ChainDelete(Node*& savedPos_ref);
		Node    *next_, *prev_;
		Command *cmd_;
	};

private:

	Node  headTail_;
	Node* savedPos_;
	Node* lastOp_;
	ulong num_;
	ulong limit_;

private:

	NOCOPY(UnReDoChain);
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline void UnReDoChain::SavedHere()
	{ savedPos_ = lastOp_; }

inline bool UnReDoChain::isUndoAble() const
	{ return (lastOp_ != &headTail_); }

inline bool UnReDoChain::isRedoAble() const
	{ return (lastOp_->next_ != &headTail_); }

inline bool UnReDoChain::isModified() const
	{ return (lastOp_ != savedPos_); }



#endif // __ccdoc__
//=========================================================================
//@{
//	Document�N���X�̎�������
//@}
//=========================================================================

class DocImpl : public Object, EzLockable, Runnable
{
public:

	DocImpl( Document& theDoc );
	~DocImpl();

	//@{ ����R�}���h���s //@}
	void Execute( const Command& cmd );

	//@{ �L�[���[�h��`�؂�ւ� //@}
	void SetKeyword( const unicode* defbuf, ulong siz );

	//@{ �C�x���g�n���h���o�^ //@}
	void AddHandler( DocEvHandler* eh );

	//@{ �C�x���g�n���h������ //@}
	void DelHandler( DocEvHandler* eh );

	//@{ �t�@�C�����J�� //@}
	void OpenFile( aptr<TextFileR> tf );

	//@{ �t�@�C����ۑ� //@}
	void SaveFile( TextFileW& tf );

	//@{ ���e�j�� //@}
	void ClearAll();

	//@{ �A���h�D //@]
	void Undo();

	//@{ ���h�D //@]
	void Redo();

	//@{ �A���h�D�񐔐��� //@]
	void SetUndoLimit( long lim );

	//@{ �ύX�t���O���N���A //@}
	void ClearModifyFlag();

public:

	//@{ �s�� //@}
	ulong tln() const;

	//@{ �s�o�b�t�@ //@}
	const unicode* tl( ulong i ) const;

	//@{ �s��͌��ʃo�b�t�@ //@}
	const uchar* pl( ulong i ) const;

	//@{ �s������ //@}
	ulong len( ulong i ) const;

	//@{ �w��͈͂̃e�L�X�g�̒��� //@}
	ulong getRangeLength( const DPos& stt, const DPos& end );

	//@{ �w��͈͂̃e�L�X�g //@}
	void getText( unicode* buf, const DPos& stt, const DPos& end );

	//@{ �w��ʒu�̒P��̐擪���擾 //@}
	DPos wordStartOf( const DPos& dp ) const;

	//@{ �w��ʒu�̈���̈ʒu���擾 //@}
	DPos leftOf( const DPos& dp, bool wide=false ) const;

	//@{ �w��ʒu�̈�E�̈ʒu���擾 //@}
	DPos rightOf( const DPos& dp, bool wide=false ) const;

	//@{ �A���h�D�\�H //@}
	bool isUndoAble() const;

	//@{ ���h�D�\�H //@}
	bool isRedoAble() const;

	//@{ �ύX�ς݁H //@}
	bool isModified() const;

private:

	Document&                      doc_;    // ����
	aptr<Parser>                   parser_; // �������͖�
	gapbufobj<Line>                text_;   // �e�L�X�g�f�[�^
	mutable storage<DocEvHandler*> pEvHan_; // �C�x���g�ʒm��
	UnReDoChain                    urdo_;   // �A���h�D���h�D

	aptr<TextFileR> currentOpeningFile_;

private:

	// �ύX�ʒm
	void Fire_KEYWORDCHANGE();
	void Fire_MODIFYFLAGCHANGE();
	void Fire_TEXTUPDATE( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd );

	// �w���p�[�֐�
	bool ReParse( ulong s, ulong e );
	void SetCommentBit( const Line& x ) const;
	void CorrectPos( DPos& pos );
	void CorrectPos( DPos& stt, DPos& end );

	// �}���E�폜���
	bool InsertingOperation(
		DPos& stt, const unicode* str, ulong len, DPos& undoend );
	bool DeletingOperation(
		DPos& stt, DPos& end, unicode*& undobuf, ulong& undosiz );

	// �p���������[�h
	virtual void StartThread();

private:

	NOCOPY(DocImpl);
	friend class Insert;
	friend class Delete;
	friend class Replace;
};



//-------------------------------------------------------------------------

inline ulong DocImpl::tln() const
	{ return text_.size(); }

inline const unicode* DocImpl::tl( ulong i ) const
	{ return text_[i].str(); }

inline ulong DocImpl::len( ulong i ) const
	{ return text_[i].size(); }

inline const uchar* DocImpl::pl( ulong i ) const
	{
		const Line& x = text_[i];
		if( !x.isCmtBitReady() )
			SetCommentBit( x );
		return x.flg();
	}



//=========================================================================

}}     // namespace editwing::doc
#endif // _EDITWING_IP_DOC_H_
