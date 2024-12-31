#ifndef _EDITWING_IP_DOC_H_
#define _EDITWING_IP_DOC_H_
#include "ewDoc.h"

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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Row buffer structure
//
// Holds text data in the form of UCS-2 betas. And at the same time.
// buffer for parsing results to distinguish emphasized words specified by the keyword file.
// It also maintains a buffer for the result of parsing. Text data is not terminated with NULs.
// The character data does not have a terminator NUL, but the terminator
// U+007f is included to speed up the parsing process
//@}
//=========================================================================
#ifdef USE_ORIGINAL_MEMMAN
	// be sure to align alloc evenly because unicode is 2 bytes long
	// and we append bytes, but alignement should remain 2bytes
	// Otherwise some functions might fail such as TextOutW
	#define EVEN(x) ( ((x)+1)&(~1ul) )
#else
	#define EVEN(x) (x)
#endif
class Line //: public ki::Object
{
public:

	//@{ �w��e�L�X�g�ŏ�����, Initialize with specified text //@}
	Line( const unicode* str, size_t len )
		: commentBitReady_( 0 )
		, isLineHeadCommented_( 0 )
		, alen_( len )
		, commentTransition_( 0 )
		, len_ ( len )
		, str_ ( len==0? empty_buf() :static_cast<unicode*>( ki::mem().Alloc(EVEN((alen_+1)*2+alen_)) ) )
		{
			if( !str_ )
			{	// Allocation failed!
				len_ = 0;
				alen_ = 0;
				str_ = empty_buf();
				return;
			}
			memmove( str_, str, len*2 );
			str_[ len ] = 0x007f;
		}

	void Clear() // Manually destroy line
	{
		if( str_ != empty_buf() )
			ki::mem().DeAlloc( str_, EVEN((alen_+1)*2+alen_) );
	}

	//@{ �e�L�X�g�}��(�w��ʒu�Ɏw��T�C�Y), Insert text (specified position, specified size)  //@}
	void InsertAt( size_t at, const unicode* buf, size_t siz )
	{
		uchar *flgs = flg(); // str_+alen_+1;
		if( len_+siz > alen_ )
		{
			// �o�b�t�@�g��
			size_t psiz = (alen_+1)*2+alen_;
			size_t nalen = Max( (size_t)(alen_+(alen_>>1)), len_+siz ); // len_+siz;
			unicode* tmpS;
			TRYAGAIN:
				tmpS = static_cast<unicode*>( ki::mem().Alloc( EVEN((nalen+1)*2+nalen) ) );
			if( !tmpS )
			{
				if( nalen > len_+siz )
				{	// Try again with minimal buffer size...
					nalen = len_+siz;
					goto TRYAGAIN;
				}
				return;
			}
			uchar*   tmpF =
				reinterpret_cast<uchar*>(tmpS+nalen+1);
			alen_ = nalen;
			// �R�s�[
			memmove( tmpS,        str_,             at*2 );
			memmove( tmpS+at+siz, str_+at, (len_-at+1)*2 );
			memmove( tmpF,        flgs,             at   );
			// �Â��̂��폜
			if( str_ != empty_buf() )
				ki::mem().DeAlloc( str_, EVEN(psiz) );
			str_ = tmpS;
		}
		else
		{
			memmove( str_+at+siz, str_+at, (len_-at+1)*2 );
			memmove( flgs+at+siz, flgs+at, (len_-at)     );
		}
		memmove( str_+at, buf, siz*sizeof(unicode) );
		len_ += siz;
	}

	//@{ �e�L�X�g�}��(������) //@}
	void InsertToTail( const unicode* buf, size_t siz )
	{
		InsertAt( len_, buf, siz );
	}

	//@{ �e�L�X�g�폜(�w��ʒu����w��T�C�Y) //@}
	void RemoveAt( size_t at, size_t siz )
	{
		uchar *flgs = flg();
		memmove( str_+at, str_+at+siz, (len_-siz-at+1)*2 );
		memmove( flgs+at, flgs+at+siz, (len_-siz-at)     );
		len_ -= siz;
	}

	//@{ �e�L�X�g�폜(�w��ʒu���疖���܂�) //@}
	void RemoveToTail( size_t at )
	{
		if( at < len_ )
			str_[ len_=at ] = 0x007f;
	}

	//@{ �o�b�t�@�ɃR�s�[(�w��ʒu����w��T�C�Y) //@}
	size_t CopyAt( size_t at, size_t siz, unicode* buf )
	{
		memmove( buf, str_+at, siz*sizeof(unicode) );
		return siz;
	}

	//@{ �o�b�t�@�ɃR�s�[(�w��ʒu���疖���܂�) //@}
	size_t CopyToTail( size_t at, unicode* buf )
	{
		return CopyAt( at, len_-at, buf );
	}

	//@{ ���� //@}
	size_t size() const
		{ return len_; }

	//@{ �e�L�X�g //@}
	unicode* str()
		{ return str_; }

	//@{ �e�L�X�g(const) //@}
	const unicode* str() const
		{ return str_; }

	//@{ ��͌��� //@}
	uchar* flg()
		{ return reinterpret_cast<uchar*>(str_+alen_+1); }

	//@{ ��͌���(const) //@}
	const uchar* flg() const
		{ return reinterpret_cast<const uchar*>(str_+alen_+1); }

	// ask
	uchar isCmtBitReady() const
		{ return commentBitReady_; }
	uchar isLineHeadCmt() const
		{ return isLineHeadCommented_; }
	// for doc
	uchar TransitCmt( uchar start )
	{
		isLineHeadCommented_ = start;
		commentBitReady_     = false;
		return ((uchar)commentTransition_>>start)&1;
	}
	// for parser
	void SetTransitFlag( uchar flag )
		{ commentTransition_ = flag; }
	void CommentBitUpdated()
		{ commentBitReady_   = true; }

private:
	// 32 bit mode: max_line_length = 2^30 * 3 = 3GB
	// Which is larger than the max 2GB adress space.
	size_t   commentBitReady_:      1;
	size_t   isLineHeadCommented_:  1;
	size_t   alen_: sizeof(size_t)*8-2;
	size_t   commentTransition_:    2;
	size_t   len_:  sizeof(size_t)*8-2;
	unicode* str_;

	static unicode* empty_buf()
		{ static unicode empty[2] = { 0x7F, 0 }; return empty; }
};

#undef EVEN


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
		const unicode*     str, size_t     len,
		const unicode** ansstr, size_t* anslen )
		: ptr_  ( str )
		, end_  ( str+len )
		, ans_  ( ansstr )
		, aln_  ( anslen )
		, empty_( false ) {}

	//@{ �ǂݏI��������ǂ����`�F�b�N //@}
	bool isEmpty()
		{ return empty_; }

	//@{ ��s�擾 //@}
	void A_HOT getLine()
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
	const unicode *ptr_, *end_, **ans_;
	size_t *aln_;
	bool  empty_;
};



//=========================================================================
//@{
//	Undo/Redo�p�ɁACommand�I�u�W�F�N�g��ۑ����Ă����N���X
//@}
//=========================================================================

class UnReDoChain : public ki::Object
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

	struct Node : public ki::Object
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
	size_t num_;
	size_t limit_;

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

class Document A_FINAL : public ki::Object
#ifdef USE_THREADS
              , private ki::EzLockable
              , private ki::Runnable
#else
              , private ki::NoLockable
#endif
{
public:

	Document();
	~Document();

	//@{ ����R�}���h���s //@}
	void Execute( const Command& cmd );

	//@{ �L�[���[�h��`�؂�ւ� //@}
	void SetKeyword( const unicode* defbuf, size_t siz );

	//@{ �C�x���g�n���h���o�^ //@}
	void AddHandler( DocEvHandler* eh );

	//@{ �C�x���g�n���h������ //@}
	void DelHandler( const DocEvHandler* eh );

	//@{ �t�@�C�����J�� //@}
	void OpenFile( ki::TextFileR& tf );

	//@{ �t�@�C����ۑ� //@}
	void SaveFile( ki::TextFileW& tf );

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

	unicode findNextBrace( DPos &dp, unicode q, unicode p ) const;
	unicode findPrevBrace( DPos &dp, unicode q, unicode p ) const;
	DPos findMatchingBrace( const DPos &dp ) const;

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

	const unicode* getCommentStr() const;

	//@{ �r�W�[�t���O�i�}�N���R�}���h���s���̂ݐ����j //@}
	void setBusyFlag( bool b ) { busy_ = b; }
	bool isBusy() const { return busy_; }

private:

	ki::uptr<Parser>                   parser_; // �������͖�
	unicode                        CommentStr_[8];
	ki::gapbufobjnoref<Line>           text_;   // �e�L�X�g�f�[�^
	mutable ki::storage<DocEvHandler*> pEvHan_; // �C�x���g�ʒm��
	UnReDoChain                    urdo_;   // �A���h�D���h�D
	editwing::DPos acc_s_, acc_e2_;
	bool busy_;
	bool acc_textupdate_mode_;
	bool acc_reparsed_;
	bool acc_nmlcmd_;

public:
	void acc_Fire_TEXTUPDATE_begin();
	void acc_Fire_TEXTUPDATE_end();
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
		DPos& stt, const unicode* str, ulong len, DPos& undoend, bool reparse=true );
	bool DeletingOperation(
		DPos& stt, DPos& end, unicode*& undobuf, ulong& undosiz );

	// �p���������[�h
	//virtual void StartThread();

private:

	NOCOPY(Document);
	friend class Insert;
	friend class Delete;
	friend class Replace;
};



//-------------------------------------------------------------------------

inline ulong Document::tln() const
	{ return text_.size(); }

inline const unicode* Document::tl( ulong i ) const
	{ return text_[i].str(); }

inline ulong Document::len( ulong i ) const
	{ return text_[i].size(); }

inline const uchar* Document::pl( ulong i ) const
{
	const Line& x = text_[i];
	if( !x.isCmtBitReady() )
		SetCommentBit( x );
	return x.flg();
}
inline const unicode* Document::getCommentStr() const
	{ return CommentStr_; }



//=========================================================================

}}     // namespace editwing::doc
#endif // _EDITWING_IP_DOC_H_
