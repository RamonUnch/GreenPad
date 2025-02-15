#ifndef _EDITWING_DOC_H_
#define _EDITWING_DOC_H_
#include "ewCommon.h"
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



class Document;
class DocEvHandler;
class Command;
class Insert;
class Delete;
class Replace;


//=========================================================================
//@{
//	イベントハンドラインターフェイス
//
//	ドキュメントから発生するイベント（挿入/削除などなど…）を
//	受け取りたい場合は、このインターフェイスを継承し、適宜ハンドラを
//	書くこと。Viewの再描画処理などもこれを通じて実行されている。
//@}
//=========================================================================

class A_NOVTABLE DocEvHandler
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

class A_NOVTABLE Command : public ki::Object
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
	~Insert() override;

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	const unicode* buf_;
	ulong          len_ : sizeof(ulong)*8-1;
	ulong          del_ : 1;
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
	~Replace() override;

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	DPos           end_;
	const unicode* buf_;
	ulong          len_ : sizeof(ulong)*8-1;
	ulong          del_ : 1;
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
	void Add( Command* cmd ) { if( cmd ) arr_.Add(cmd); } // do not save NULLs

	//@{ コマンド数 //@}
	ulong size() const { return arr_.size(); }

	//@ デストラクタ //@}
	~MacroCommand() override
	{
		for( ulong i=0,e=arr_.size(); i<e; ++i )
			delete arr_[i];
	}

	MacroCommand() : arr_(16) {}

private:
	Command* operator()( Document& doc ) const override;
	ki::storage<Command*> arr_;
};



//=========================================================================

}}     // namespace editwing::document
#endif // _EDITWING_DOC_H_
