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
//	�ȒP�ȃG�f�B�b�g�R���g���[��
//
//	�Ƃ肠���������\���ł��ĐF���ς����ăJ�[�\�����������āc
//	�Ƃ����ADoc/View�̊�{�@�\�����̂܂܎g�����`�̂��́B
//	�E�C���h�E�����Ή��łƂ������̂�����邩������Ȃ��B
//@}
//=========================================================================

class EwEdit A_FINAL: public ki::WndImpl
{
public:

	EwEdit();
	~EwEdit();

public:

	//@{ �����f�[�^���� //@}
	doc::Document& getDoc() { return *doc_; }

	//@{ �\���@�\���� //@}
	view::View& getView() { return *view_; }

	//@{ �J�[�\���@�\���� //@}
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
