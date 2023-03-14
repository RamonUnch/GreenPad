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

class Line : public Object
{
public:

	//@{ 指定テキストで初期化, Initialize with specified text //@}
	Line( const unicode* str, ulong len )
		: alen_( Max(len, (ulong)1) )
		, len_ ( len )
		, str_ ( static_cast<unicode*>( mem().Alloc((alen_+1)*2+alen_) ) )
		, flg_ ( reinterpret_cast<uchar*>(str_+alen_+1) )
		, commentBitReady_( 0 )
		, isLineHeadCommented_( 0 )
		{
			memmove( str_, str, len*2 );
			str_[ len ] = 0x007f;
		}

	~Line()
		{
			mem().DeAlloc( str_, (alen_+1)*2+alen_ );
		}

	//@{ テキスト挿入(指定位置に指定サイズ), Insert text (specified position, specified size)  //@}
	void InsertAt( ulong at, const unicode* buf, ulong siz )
		{
			if( len_+siz > alen_ )
			{
				// バッファ拡張
				ulong psiz = (alen_+1)*2+alen_;
				alen_ = len_+siz; // Max( alen_<<1, len_+siz );
				unicode* tmpS =
					static_cast<unicode*>( mem().Alloc((alen_+1)*2+alen_) );
				uchar*   tmpF =
					reinterpret_cast<uchar*>(tmpS+alen_+1);
				// コピー
				memmove( tmpS,        str_,             at*2 );
				memmove( tmpS+at+siz, str_+at, (len_-at+1)*2 );
				memmove( tmpF,        flg_,             at   );
				// 古いのを削除
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

	//@{ テキスト挿入(末尾に) //@}
	void InsertToTail( const unicode* buf, ulong siz )
		{
			InsertAt( len_, buf, siz );
		}

	//@{ テキスト削除(指定位置から指定サイズ) //@}
	void RemoveAt( ulong at, ulong siz )
		{
			memmove( str_+at, str_+at+siz, (len_-siz-at+1)*2 );
			memmove( flg_+at, flg_+at+siz, (len_-siz-at)     );
			len_ -= siz;
		}

	//@{ テキスト削除(指定位置から末尾まで) //@}
	void RemoveToTail( ulong at )
		{
			if( at < len_ )
				str_[ len_=at ] = 0x007f;
		}

	//@{ バッファにコピー(指定位置から指定サイズ) //@}
	ulong CopyAt( ulong at, ulong siz, unicode* buf )
		{
			memmove( buf, str_+at, siz*sizeof(unicode) );
			return siz;
		}

	//@{ バッファにコピー(指定位置から末尾まで) //@}
	ulong CopyToTail( ulong at, unicode* buf )
		{
			return CopyAt( at, len_-at, buf );
		}

	//@{ 長さ //@}
	ulong size() const
		{ return len_; }

	//@{ テキスト //@}
	unicode* str()
		{ return str_; }

	//@{ テキスト(const) //@}
	const unicode* str() const
		{ return str_; }

	//@{ 解析結果 //@}
	uchar* flg()
		{ return flg_; }

	//@{ 解析結果(const) //@}
	const uchar* flg() const
		{ return flg_; }

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

	uchar commentBitReady_;
	uchar isLineHeadCommented_;
	uchar commentTransition_;
};



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
		const unicode*     str, ulong     len,
		const unicode** ansstr, ulong* anslen )
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
	ulong *aln_;
	bool  empty_;
};



//=========================================================================
//@{
//	Undo/Redo用に、Commandオブジェクトを保存しておくクラス
//@}
//=========================================================================

class UnReDoChain : public Object
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
//	Documentクラスの実装部分
//@}
//=========================================================================

class DocImpl : public Object
#ifdef USE_THREADS
              , private EzLockable
              , private Runnable
#else
              , private NoLockable
#endif
{
public:

	DocImpl( Document& theDoc );
	~DocImpl();

	//@{ 操作コマンド実行 //@}
	void Execute( const Command& cmd );

	//@{ キーワード定義切り替え //@}
	void SetKeyword( const unicode* defbuf, ulong siz );

	//@{ イベントハンドラ登録 //@}
	void AddHandler( DocEvHandler* eh );

	//@{ イベントハンドラ解除 //@}
	void DelHandler( DocEvHandler* eh );

	//@{ ファイルを開く //@}
	void OpenFile( aptr<TextFileR> tf );

	//@{ ファイルを保存 //@}
	void SaveFile( TextFileW& tf );

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

private:

	Document&                      doc_;    // 自分
	aptr<Parser>                   parser_; // 文字列解析役
	unicode                        CommentStr_[8];
	gapbufobj<Line>                text_;   // テキストデータ
	mutable storage<DocEvHandler*> pEvHan_; // イベント通知先
	UnReDoChain                    urdo_;   // アンドゥリドゥ

	aptr<TextFileR> currentOpeningFile_;

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
		DPos& stt, const unicode* str, ulong len, DPos& undoend );
	bool DeletingOperation(
		DPos& stt, DPos& end, unicode*& undobuf, ulong& undosiz );

	// パラレルリード
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
inline const unicode* DocImpl::getCommentStr() const
	{ return CommentStr_; }



//=========================================================================

}}     // namespace editwing::doc
#endif // _EDITWING_IP_DOC_H_
