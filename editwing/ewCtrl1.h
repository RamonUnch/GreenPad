#ifndef _EDITWING_CTRL1_H_
#define _EDITWING_CTRL1_H_
#include "ewDoc.h"
#include "ewView.h"
#ifndef __ccdoc__
namespace editwing {
#endif



//=========================================================================
//@{ @pkg editwing.Ctrl //@}
//@{
//	簡単なエディットコントロール
//
//	とりあえず字が表示できて色が変えられてカーソルが動かせて…
//	という、Doc/Viewの基本機能をそのまま使った形のもの。
//	ウインドウ分割対応版とかもそのうち作るかもしれない。
//@}
//=========================================================================

class EwEdit A_FINAL: public ki::WndImpl
{
public:

	EwEdit();
	~EwEdit();

public:

	//@{ 文書データ操作 //@}
	doc::Document& getDoc() { return *doc_; }

	//@{ 表示機能操作 //@}
	view::View& getView() { return *view_; }

	//@{ カーソル機能操作 //@}
	view::Cursor& getCursor() { return view_->cur(); }

private:

	doc::Document*     doc_;
	view::View*        view_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs ) override;
	void    on_destroy() override;
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp ) override;
};



//=========================================================================

}      // namespace editwing
#endif // _EDITWING_CTRL1_H_
