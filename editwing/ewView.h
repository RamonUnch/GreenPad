#ifndef _EDITWING_VIEW_H_
#define _EDITWING_VIEW_H_
#include "ewCommon.h"
#include "ewDoc.h"
#ifndef __ccdoc__
namespace editwing {
namespace view {
#endif

using namespace ki;

class Canvas;
class ViewImpl;
class Cursor;
class Caret;
class OleDnDTarget;


//=========================================================================
//@{ @pkg editwing.View //@}
//@{
//	�`�揈���Ȃ�
//
//	���̃N���X�ł́A���b�Z�[�W�̕��z���s�������ŁA������
//	Canvas/ViewImpl ���ōs���B�̂ŁA�ڂ����͂�������Q�Ƃ̂��ƁB
//@}
//=========================================================================

class View A_FINAL: public ki::WndImpl, public doc::DocEvHandler
{
public:

	//@{ �������Ȃ��R���X�g���N�^ //@}
	View( doc::Document& d, HWND wnd );
	~View();

	//@{ �܂�Ԃ������ؑ� //@}
	void SetWrapType( int wt );

	void SetWrapSmart( bool ws);

	//@{ �s�ԍ��\��/��\���ؑ� //@}
	void ShowLineNo( bool show );

	//@{ �\���F�E�t�H���g�ؑ� //@}
	void SetFont( const VConfig& vc );

	//@{ Set all canva stuff at once (faster) //@}
	void SetWrapLNandFont( int wt, bool ws, bool showLN, const VConfig& vc );

	//@{ �������� //@}
	ViewImpl& impl() { return *impl_; }

	//@{ �J�[�\�� //@}
	Cursor& cur();

private:

	doc::DocImpl&      doc_;
	ki::dptr<ViewImpl> impl_;
	static ClsName     className_;

private:

	void    on_create( CREATESTRUCT* cs ) override;
	void    on_destroy() override;
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp ) override;
	void    on_text_update( const DPos& s, const DPos& e, const DPos& e2, bool bAft, bool mCur ) override;
	void    on_keyword_change() override;
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
// OLE Drag and Drop handler.
//@}
//=========================================================================
#ifndef NO_OLEDNDTAR
class OleDnDTarget A_FINAL: public IDropTarget
{
	friend class Cursor;
	OleDnDTarget( HWND hwnd, ViewImpl& vw );
	~OleDnDTarget();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

	ULONG STDMETHODCALLTYPE AddRef()  override { return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() override { return ::InterlockedDecrement(&refcnt); }

	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override
	{
		// Only accept DragEnter if we can get some text.
		comes_from_center_ = false;
		FORMATETC fmt = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if( S_OK == pDataObj->QueryGetData(&fmt) )
		{
			LOGGER( "OleDnDTarget::DragEnter(CF_UNICODETEXT)" );
			return S_OK;
		}

		fmt.cfFormat = CF_TEXT;
		if( S_OK == pDataObj->QueryGetData(&fmt) )
		{
			LOGGER( "OleDnDTarget::DragEnter(CF_TEXT)" );
			return S_OK;
		}

		fmt.cfFormat = CF_HDROP;
		if( app().isWin32s() && S_OK == pDataObj->QueryGetData(&fmt) )
		{
			LOGGER( "OleDnDTarget::DragEnter(CF_HDROP)" );
			return S_OK;
		}

		LOGGER( "OleDnDTarget::DragEnter(No supported IDataObject format)" );
		return E_UNEXPECTED;
	}

	HRESULT STDMETHODCALLTYPE DragLeave() override
		{ LOGGER( "OleDnDTarget::DragLeave()" ); return S_OK; }

	HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

	HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

private:
	void setDropEffect(DWORD grfKeyState, DWORD *pdwEffect) const;

private:
	LONG refcnt;
	HWND hwnd_;
	ViewImpl& view_;

	bool comes_from_center_;
};

#endif // NO_OLEDNDTAR

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
typedef unicode *(WINAPI *ModProc)(unicode* str);
class Cursor A_FINAL: public ki::Object
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
	void InputAt( const unicode *str, ulong len, int x, int y );
	void InputAt( const char *str, ulong len, int x, int y );
	void InputChar( unicode ch );
	void Del(bool wide);
	void DelBack(bool wide);
	void DelToEndline( bool wide );
	void DelToStartline( bool wide );
	void Return();
	void Tabulation(bool shift);
	void QuoteSelectionW(const unicode *qs, bool shift);
	void QuoteSelection(bool unquote);

	// �N���b�v�{�[�h
	void Cut();
	void Copy();
	void Paste();

	// �I���e�L�X�g�擾
	ki::aarr<unicode> getSelectedStr() const;

	// ���[�h�ؑ�
	void SetInsMode( bool bIns );
	void SetROMode( bool bRO );

	// More Edit
	void ModSelection(ModProc funk);
	void UpperCaseSel();
	void LowerCaseSel();
	static unicode* WINAPI InvertCaseW(unicode *);
	static unicode* WINAPI TrimTrailingSpacesW(unicode *);
	static unicode* WINAPI StripFirstCharsW(unicode *);
	static unicode* WINAPI StripLastCharsW(unicode *);
	void InvertCaseSel();
	void TTSpacesSel();
	void StripFirstChar();
	void StripLastChar();
	// IME
	void Reconv();
	void ToggleIME();

public:

	bool isInsMode() const;
	bool isROMode() const;
	bool isSelected() const;
	bool isInSelection(const VPos &vp) const;
	bool getCurPos( const VPos** start, const VPos** end ) const;
	bool getCurPosUnordered( const VPos** cur, const VPos** sel ) const;
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
	bool on_drag_start( short x, short y );
	void on_mouse_move( short x, short y, WPARAM fwKeys );
	void on_lbutton_up( short x, short y );
	void on_lbutton_dbl( short x, short y );
	bool on_contextmenu( short x, short y );
	void on_timer();
	int  on_ime_reconvertstring( RECONVERTSTRING* rs );
	bool on_ime_confirmreconvertstring( RECONVERTSTRING* rs );

private:

	ViewImpl&       view_;
	doc::DocImpl&   doc_;
	CurEvHandler*   pEvHan_;
	ki::dptr<Caret> caret_;
#ifndef NO_OLEDNDTAR
	OleDnDTarget    dndtg_;
#endif
	VPos cur_;  // �J�[�\���ʒu
	VPos sel_;  // �I�����̎����ʒu
	bool bIns_; // �}�����[�h�H, Insertion mode?
	bool bRO_;  // �ǎ��p�H, Read Only?
	bool lineSelectMode_; // �s�I�����[�h�H
//	bool squareSelect_; // Select in square AA mode

	UINT_PTR timerID_;// �}�E�X�h���b�O����p��
	int  keyRepTime_; // �^�C�}�[�֌W
	int  dragX_;      // �ʒu
	int  dragY_;      // �ʒu

	CurEvHandler defaultHandler_;

private:

	void MoveTo( const VPos& vp, bool sel );
	void MoveByMouse( int x, int y );
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

inline bool Cursor::isInSelection(const VPos &vp) const
	{ return Min(cur_, sel_) <= vp && vp < Max(cur_,sel_); }


//inline void Cursor::on_input_lang_change(int cp, HKL hkl)
//	{ kbcp_ = cp; hkl_ = hkl; }

//=========================================================================

}}    // namespace editwing::view
#endif // _EDITWING_VIEW_H_
