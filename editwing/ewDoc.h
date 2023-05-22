#ifndef _EDITWING_DOC_H_
#define _EDITWING_DOC_H_
#include "ewCommon.h"
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



class DocImpl;
class DocEvHandler;
class Command;
class Insert;
class Delete;
class Replace;



//=========================================================================
//@{ @pkg editwing.Doc //@}
//@{
//	文書データ
//
//	このクラスは単なるインターフェイスで、内部実装は
//	class DocImpl で行う。ので、詳しくはそちらを参照のこと。
//@}
//=========================================================================

class Document : public ki::Object
{
public:

	//@{ 何もしないコンストラクタ //@}
	Document();
	~Document();

	//@{ ファイルを開く //@}
	void OpenFile( ki::TextFileR& tf );

	//@{ ファイルを保存 //@}
	void SaveFile( ki::TextFileW& tf );

	//@{ 内容破棄 //@}
	void ClearAll();

	//@{ 操作コマンド実行 //@}
	void Execute( const Command& cmd );

	//@{ アンドゥ //@]
	void Undo();

	//@{ リドゥ //@]
	void Redo();

	//@{ アンドゥ回数制限 //@]
	void SetUndoLimit( long lim );

	//@{ 変更フラグをクリア //@}
	void ClearModifyFlag();

	//@{ イベントハンドラ登録 //@}
	void AddHandler( DocEvHandler* eh );

	//@{ イベントハンドラ解除 //@}
	void DelHandler( DocEvHandler* eh );

	//@{ キーワード定義切り替え //@}
	void SetKeyword( const unicode* defbuf, ulong siz=0 );

public:

	//@{ 内部実装クラス //@}
	DocImpl& impl() { return *impl_; }

	//@{ 行数, number of lines //@}
	ulong tln() const;

	//@{ 行バッファ, line beuufe //@}
	const unicode* tl( ulong i ) const;

	//@{ 行文字数 line character count //@}
	ulong len( ulong i ) const;

	//@{ 指定範囲のテキストの長さ, Length of the specified range of text //@}
	ulong getRangeLength( const DPos& stt, const DPos& end ) const;

	//@{ 指定範囲のテキスト, Text in the specified range //@}
	void getText( unicode* buf, const DPos& stt, const DPos& end ) const;

	//@{ アンドゥ可能？ //@}
	bool isUndoAble() const;

	//@{ リドゥ可能？ //@}
	bool isRedoAble() const;

	//@{ 変更済み？ //@}
	bool isModified() const;

	//@{ ビジーフラグ（マクロコマンド実行中のみ成立） //@}
	void setBusyFlag( bool b=true ) { busy_ = b; }
	bool isBusy() const { return busy_; }

private:

	// 実装
	ki::dptr<DocImpl> impl_;
	bool busy_;

private:

	NOCOPY(Document);
};



//=========================================================================
//@{
//	イベントハンドラインターフェイス
//
//	ドキュメントから発生するイベント（挿入/削除などなど…）を
//	受け取りたい場合は、このインターフェイスを継承し、適宜ハンドラを
//	書くこと。Viewの再描画処理などもこれを通じて実行されている。
//@}
//=========================================================================

class DocEvHandler
{
public:
	//@{
	//	テキスト内容が変更されたときに発生
	//	@param s        変更範囲の先頭
	//	@param e        変更範囲の終端(前)
	//	@param e2       変更範囲の終端(後)
	//	@param reparsed e2より後ろのコメントアウト状態が変化していたらtrue
	//	@param nmlcmd   挿入/削除/置換ならtrue、ファイル開き/全置換ならfalse
	//@}
	virtual void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd ) {}

	//@{
	//	キーワードが変更されたときに発生
	//@}
	virtual void on_keyword_change() {}

	//@{
	//	ダーティフラグが変更されたときに発生
	//@}
	virtual void on_dirtyflag_change( bool dirty ) {}
};



//=========================================================================
//@{
//	操作コマンドインターフェイス
//
//	ドキュメントは、Command から派生したクラスのインスタンスの
//	operator() を呼び出すことで、色々な操作を実行する。とりあえず
//	具体的には Insert/Delete/Replace の３つだけ。あとでマクロコマンド用
//	クラスも作るつもりだけど、とりあえずは保留。
//@}
//=========================================================================

class Command : public ki::Object
{
protected:
	friend class UnReDoChain;
	friend class MacroCommand;
	virtual Command* operator()( Document& doc ) const = 0;
public:
	virtual ~Command() {};
};



//=========================================================================
//@{
//	挿入コマンド
//@}
//=========================================================================

class Insert A_FINAL: public Command
{
public:

	//@{
	//	@param s 挿入位置
	//	@param str 挿入文字列
	//	@param len 文字列の長さ
	//	@param del コマンド終了時にdelete [] strしてよいか？
	//@}
	Insert( const DPos& s, const unicode* str, ulong len, bool del=false );
	~Insert();

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	const unicode* buf_;
	ulong          len_;
	bool           del_;
};



//=========================================================================
//@{
//	削除コマンド
//@}
//=========================================================================

class Delete A_FINAL: public Command
{
public:

	//@{
	//	@param s 開始位置
	//	@param e 終端位置
	//@}
	Delete( const DPos& s, const DPos& e );

private:

	Command* operator()( Document& doc ) const override;
	DPos stt_;
	DPos end_;
};




//=========================================================================
//@{
//	置換コマンド
//@}
//=========================================================================

class Replace A_FINAL: public Command
{
public:

	//@{
	//	@param s 開始位置
	//	@param e 終端位置
	//	@param str 挿入文字列
	//	@param len 文字列の長さ
	//	@param del コマンド終了時にdelete [] strしてよいか？
	//@}
	Replace( const DPos& s, const DPos& e,
		const unicode* str, ulong len, bool del=false );
	~Replace();

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	DPos           end_;
	const unicode* buf_;
	ulong          len_;
	bool           del_;
};



//=========================================================================
//@{
//	マクロコマンド
//
//	複数のコマンドを一つのコマンドとして連続実行する。
//	ただし、Insert/Delete/Replaceを一回行うたびに当然
//	文字列の位置は変化するのだが、それに関する変換処理は
//	行わない。すなわち、Insert->Delete->Insert みたいな
//	連続処理を書くときは、行数や文字数の変化を考慮しながら
//	値を定めていくことが必要になる。ので、あんまり使えない(^^;
//@}
//=========================================================================

class MacroCommand A_FINAL: public Command
{
public:
	//@{ コマンドの追加 //@}
	void Add( Command* cmd ) { arr_.Add(cmd); }

	//@{ コマンド数 //@}
	ulong size() const { return arr_.size(); }

	//@ デストラクタ //@}
	~MacroCommand()
	{
		for( ulong i=0,e=arr_.size(); i<e; ++i )
			delete arr_[i];
	}

private:
	Command* operator()( Document& doc ) const override;
	ki::storage<Command*> arr_;
};



//=========================================================================

}}     // namespace editwing::document
#endif // _EDITWING_DOC_H_
