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
//	行バッファ構造体
//
//	UCS-2ベタの形式でテキストデータを保持する。またそれと同時に、
//	キーワードファイルによって指定された強調語を区別するための
//	解析処理結果用バッファも管理する。文字データに終端NULは
//	付けないが、解析作業の高速化のため、終端 U+007f が入る。
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

	//@{ 指定テキストで初期化, Initialize with specified text //@}
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

	//@{ テキスト挿入(指定位置に指定サイズ), Insert text (specified position, specified size)  //@}
	void InsertAt( size_t at, const unicode* buf, size_t siz )
	{
		uchar *flgs = flg(); // str_+alen_+1;
		if( len_+siz > alen_ )
		{
			// バッファ拡張
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
			// コピー
			memmove( tmpS,        str_,             at*2 );
			memmove( tmpS+at+siz, str_+at, (len_-at+1)*2 );
			memmove( tmpF,        flgs,             at   );
			// 古いのを削除
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

	//@{ テキスト挿入(末尾に) //@}
	void InsertToTail( const unicode* buf, size_t siz )
	{
		InsertAt( len_, buf, siz );
	}

	//@{ テキスト削除(指定位置から指定サイズ) //@}
	void RemoveAt( size_t at, size_t siz )
	{
		uchar *flgs = flg();
		memmove( str_+at, str_+at+siz, (len_-siz-at+1)*2 );
		memmove( flgs+at, flgs+at+siz, (len_-siz-at)     );
		len_ -= siz;
	}

	//@{ テキスト削除(指定位置から末尾まで) //@}
	void RemoveToTail( size_t at )
	{
		if( at < len_ )
			str_[ len_=at ] = 0x007f;
	}

	//@{ バッファにコピー(指定位置から指定サイズ) //@}
	size_t CopyAt( size_t at, size_t siz, unicode* buf )
	{
		memmove( buf, str_+at, siz*sizeof(unicode) );
		return siz;
	}

	//@{ バッファにコピー(指定位置から末尾まで) //@}
	size_t CopyToTail( size_t at, unicode* buf )
	{
		return CopyAt( at, len_-at, buf );
	}

	//@{ 長さ //@}
	size_t size() const
		{ return len_; }

	//@{ テキスト //@}
	unicode* str()
		{ return str_; }

	//@{ テキスト(const) //@}
	const unicode* str() const
		{ return str_; }

	//@{ 解析結果 //@}
	uchar* flg()
		{ return reinterpret_cast<uchar*>(str_+alen_+1); }

	//@{ 解析結果(const) //@}
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
//	Unicodeテキスト切り分け君
//
//	行単位で処理を行うことが多いので、その行毎に分ける処理を
//	切り出した。getLine() するたびに、指定したポインタと整数変数へ
//	先頭から順に行のデータを格納して行く。
//@}
//=========================================================================

class UniReader
{
public:

	//@{ 読みとり元バッファを与えて初期化 //@}
	UniReader(
		const unicode*     str, size_t     len,
		const unicode** ansstr, size_t* anslen )
		: ptr_  ( str )
		, end_  ( str+len )
		, ans_  ( ansstr )
		, aln_  ( anslen )
		, empty_( false ) {}

	//@{ 読み終わったかどうかチェック //@}
	bool isEmpty()
		{ return empty_; }

	//@{ 一行取得 //@}
	void A_HOT getLine()
	{
		// 次の改行の位置を取得
		const unicode *p=ptr_, *e=end_;
		for( ; p<e; ++p )
			if( *p == L'\r' || *p == L'\n' )
				break;
		// 記録
		*ans_  = ptr_;
		*aln_  = int(p-ptr_);
		// 改行コードスキップ
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
//	Undo/Redo用に、Commandオブジェクトを保存しておくクラス
//@}
//=========================================================================

class UnReDoChain : public ki::Object
{
public:

	//@{ コンストラクタ //@}
	UnReDoChain();

	//@{ デストラクタ //@}
	~UnReDoChain();

	//@{ Undo実行 //@}
	void Undo( Document& doc );

	//@{ Redo実行 //@}
	void Redo( Document& doc );

	//@{ 新しいコマンドを実行 //@}
	void NewlyExec( const Command& cmd, Document& doc );

	//@{ 初期状態に戻す //@}
	void Clear();

	//@{ 保存位置フラグを現在位置にセット //@}
	void SavedHere();

	//@{ Undo/Redoの回数制限を指定。-1 = Infinite //@}
	void SetLimit( long lim );

public:

	//@{ Undo操作が可能か？ //@}
	bool isUndoAble() const;

	//@{ Redo操作が可能か？ //@}
	bool isRedoAble() const;

	//@{ 保存後、変更されているか？ //@}
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
//	Documentクラスの実装部分
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

	//@{ 操作コマンド実行 //@}
	void Execute( const Command& cmd );

	//@{ キーワード定義切り替え //@}
	void SetKeyword( const unicode* defbuf, size_t siz );

	//@{ イベントハンドラ登録 //@}
	void AddHandler( DocEvHandler* eh );

	//@{ イベントハンドラ解除 //@}
	void DelHandler( const DocEvHandler* eh );

	//@{ ファイルを開く //@}
	void OpenFile( ki::TextFileR& tf );

	//@{ ファイルを保存 //@}
	void SaveFile( ki::TextFileW& tf );

	//@{ 内容破棄 //@}
	void ClearAll();

	//@{ アンドゥ //@]
	void Undo();

	//@{ リドゥ //@]
	void Redo();

	//@{ アンドゥ回数制限 //@]
	void SetUndoLimit( long lim );

	//@{ 変更フラグをクリア //@}
	void ClearModifyFlag();

public:

	//@{ 行数 //@}
	ulong tln() const;

	//@{ 行バッファ //@}
	const unicode* tl( ulong i ) const;

	//@{ 行解析結果バッファ //@}
	const uchar* pl( ulong i ) const;

	//@{ 行文字数 //@}
	ulong len( ulong i ) const;

	//@{ 指定範囲のテキストの長さ //@}
	ulong getRangeLength( const DPos& stt, const DPos& end );

	//@{ 指定範囲のテキスト //@}
	void getText( unicode* buf, const DPos& stt, const DPos& end );

	//@{ 指定位置の単語の先頭を取得 //@}
	DPos wordStartOf( const DPos& dp ) const;

	unicode findNextBrace( DPos &dp, unicode q, unicode p ) const;
	unicode findPrevBrace( DPos &dp, unicode q, unicode p ) const;
	DPos findMatchingBrace( const DPos &dp ) const;

	//@{ 指定位置の一つ左の位置を取得 //@}
	DPos leftOf( const DPos& dp, bool wide=false ) const;

	//@{ 指定位置の一つ右の位置を取得 //@}
	DPos rightOf( const DPos& dp, bool wide=false ) const;

	//@{ アンドゥ可能？ //@}
	bool isUndoAble() const;

	//@{ リドゥ可能？ //@}
	bool isRedoAble() const;

	//@{ 変更済み？ //@}
	bool isModified() const;

	const unicode* getCommentStr() const;

	//@{ ビジーフラグ（マクロコマンド実行中のみ成立） //@}
	void setBusyFlag( bool b ) { busy_ = b; }
	bool isBusy() const { return busy_; }

private:

	ki::uptr<Parser>                   parser_; // 文字列解析役
	unicode                        CommentStr_[8];
	ki::gapbufobjnoref<Line>           text_;   // テキストデータ
	mutable ki::storage<DocEvHandler*> pEvHan_; // イベント通知先
	UnReDoChain                    urdo_;   // アンドゥリドゥ
	editwing::DPos acc_s_, acc_e2_;
	bool busy_;
	bool acc_textupdate_mode_;
	bool acc_reparsed_;
	bool acc_nmlcmd_;

public:
	void acc_Fire_TEXTUPDATE_begin();
	void acc_Fire_TEXTUPDATE_end();
private:

	// 変更通知
	void Fire_KEYWORDCHANGE();
	void Fire_MODIFYFLAGCHANGE();
	void Fire_TEXTUPDATE( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd );

	// ヘルパー関数
	bool ReParse( ulong s, ulong e );
	void SetCommentBit( const Line& x ) const;
	void CorrectPos( DPos& pos );
	void CorrectPos( DPos& stt, DPos& end );

	// 挿入・削除作業
	bool InsertingOperation(
		DPos& stt, const unicode* str, ulong len, DPos& undoend, bool reparse=true );
	bool DeletingOperation(
		DPos& stt, DPos& end, unicode*& undobuf, ulong& undosiz );

	// パラレルリード
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
