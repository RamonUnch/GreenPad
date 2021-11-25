#ifndef _EDITWING_VIEW_H_
#define _EDITWING_VIEW_H_
#include "ewCommon.h"
#include "ewDoc.h"
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif



class Canvas;
class ViewImpl;
class Cursor;
class Caret;



//=========================================================================
//@{ @pkg editwing.View //@}
//@{
//	�`�揈���Ȃ�
//
//	���̃N���X�ł́A���b�Z�[�W�̕��z���s�������ŁA������
//	Canvas/ViewImpl ���ōs���B�̂ŁA�ڂ����͂�������Q�Ƃ̂��ƁB
//@}
//=========================================================================

class View : public ki::WndImpl, public doc::DocEvHandler
{
public:

	//@{ �������Ȃ��R���X�g���N�^ //@}
	View( doc::Document& d, HWND wnd );
	~View();

	//@{ �܂�Ԃ������ؑ� //@}
	void SetWrapType( int wt );

	//@{ �s�ԍ��\��/��\���ؑ� //@}
	void ShowLineNo( bool show );

	//@{ �\���F�E�t�H���g�ؑ� //@}
	void SetFont( const VConfig& vc );

	//@{ �������� //@}
	ViewImpl& impl() { return *impl_; }

	//@{ �J�[�\�� //@}
	Cursor& cur();

private:

	doc::DocImpl&      doc_;
	ki::dptr<ViewImpl> impl_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs );
	void    on_destroy();
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp );
	void    on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur );
	void    on_keyword_change();
};



//=========================================================================
//@{
//	�C�x���g�n���h���C���^�[�t�F�C�X
//
//	�J�[�\�����甭������C�x���g�F�X��
//@}
//=========================================================================

class CurEvHandler
{
	friend class Cursor;
	virtual void on_move( const DPos& c, const DPos& s ) {}
	virtual void on_char( Cursor& cur, unicode wch );
	virtual void on_key( Cursor& cur, int vk, bool sft, bool ctl );
	virtual void on_ime( Cursor& cur, unicode* str, ulong len );
};



//=========================================================================
//@{
//	�\���ʒu���܂Ŋ܂߂�DPos
//@}
//=========================================================================

struct VPos : public DPos
{
	ulong vl; // VLine-Index
	ulong rl; // RLine-Index
	int   vx; // �X�N���[�����l�����Ȃ����z�X�N���[�����x���W(pixel) 
	int   rx; // �����̕��тɍ��E����ĂȂ�x���W(pixel)
		      //   == �����s�̂����ۂ���Z���s�� [��] �ňړ�����
		      //   == ���̌� [��] �Ŗ߂��悤�ȃA���ł��B
	void operator=( const DPos& dp ) { tl=dp.tl, ad=dp.ad; }

	VPos(bool) : DPos(0,0),vl(0),rl(0),vx(0),rx(0) {}
	VPos() {}
};



//=========================================================================
//@{
//	�J�[�\��
//@}
//=========================================================================

class Cursor : public ki::Object
{
public:

	// �������Ƃ�
	Cursor( HWND wnd, ViewImpl& vw, doc::DocImpl& dc );
	~Cursor();
	void AddHandler( CurEvHandler* ev );
	void DelHandler( CurEvHandler* ev );

	// �J�[�\���ړ�
	void MoveCur( const DPos& dp, bool select );

	// �L�[�ɂ��J�[�\���ړ�
	void Left( bool wide, bool select );
	void Right( bool wide, bool select );
	void Up( bool wide, bool select );
	void Down( bool wide, bool select );
	void Home( bool wide, bool select );
	void End( bool wide, bool select );
	void PageUp( bool select );
	void PageDown( bool select );

	// �e�L�X�g��������
	void Input( const unicode* str, ulong len );
	void Input( const char* str, ulong len );
	void InputChar( unicode ch );
	void Del();
	void DelBack();

	// �N���b�v�{�[�h
	void Cut();
	void Copy();
	void Paste();

	// �I���e�L�X�g�擾
	ki::aarr<unicode> getSelectedStr() const;

	// ���[�h�ؑ�
	void SetInsMode( bool bIns );
	void SetROMode( bool bRO );

	// IME
	void Reconv();
	void ToggleIME();

public:

	bool isInsMode() const;
	bool isROMode() const;
	bool isSelected() const;
	bool getCurPos( const VPos** start, const VPos** end ) const;
	void ResetPos();
	void on_scroll_begin();
	void on_scroll_end();
	void on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool mCur );
	void on_setfocus();
	void on_killfocus();
	void on_keydown( int vk, LPARAM flag );
	void on_char( TCHAR ch );
	void on_ime_composition( LPARAM lp );
	void on_lbutton_down( short x, short y, bool shift );
	void on_mouse_move( short x, short y );
	void on_lbutton_up( short x, short y );
	void on_lbutton_dbl( short x, short y );
	bool on_contextmenu( short x, short y );
	void on_timer();
	int  on_ime_reconvertstring( RECONVERTSTRING* rs );
	bool on_ime_confirmreconvertstring( RECONVERTSTRING* rs );

private:

	doc::DocImpl&   doc_;
	ViewImpl&       view_;
	CurEvHandler*   pEvHan_;
	ki::dptr<Caret> caret_;

	VPos cur_;  // �J�[�\���ʒu
	VPos sel_;  // �I�����̎����ʒu
	bool bIns_; // �}�����[�h�H
	bool bRO_;  // �ǎ��p�H

	UINT_PTR timerID_;// �}�E�X�h���b�O����p��
	int  keyRepTime_; // �^�C�}�[�֌W
	int  dragX_;      // �ʒu
	int  dragY_;      // �ʒu
	bool lineSelectMode_; // �s�I�����[�h�H

	CurEvHandler defaultHandler_;

private:

	void MoveByMouse( int x, int y );
	void MoveTo( const VPos& vp, bool sel );
	void Ud( int dy, bool select );
	void UpdateCaretPos();
	void Redraw( const VPos& s, const VPos& e );
};



//-------------------------------------------------------------------------

inline bool Cursor::isSelected() const
	{ return cur_!=sel_; }

inline bool Cursor::isInsMode() const
	{ return bIns_; }

inline bool Cursor::isROMode() const
	{ return bRO_; }



//=========================================================================

}}    // namespace editwing::view
#endif // _EDITWING_VIEW_H_
