#ifndef _KILIB_WINDOW_H_
#define _KILIB_WINDOW_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



// �^�C���A�E�g�t��MsgBox�̕Ԓl
// #define IDTIMEOUT 0



//=========================================================================
//@{ @pkg ki.Window //@}
//@{
//	������p�N���X
//
//	�O������A�_�C�A���O��R���g���[����v���p�e�B�V�[�g��
//	���ʂ̃E�C���h�E��S���܂Ƃ߂ċ��ʂɈ������߂̃C���^�[�t�F�C�X�B
//	���ۂ̎����́A���ʃN���X XxxImpl �ōs���Ă���B
//@}
//=========================================================================

class A_NOVTABLE Window : public Object
{
public:

	//@{ ���C�����b�Z�[�W���[�v //@}
	void MsgLoop();

	//@{ ���b�Z�[�W�𑗂��ď��������܂őҋ@ //@}
	LRESULT SendMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{ ���b�Z�[�W�𑗂��Ă����A�� //@}
	BOOL PostMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{
	//	�������ŋ@�\�t�����b�Z�[�W�{�b�N�X
	//	@param msg �\�����镶����
	//	@param caption �_�C�A���O�̑薼
	//	@param type Win32SDK�̐��������Ă�
	//@}
	int MsgBox( LPCTSTR msg, LPCTSTR caption=NULL, UINT type=MB_OK ) const;

	static int __cdecl MsgBoxf( HWND hwnd,  LPCTSTR title, LPCTSTR fmt, ... );

	//@{ �e�L�X�g�ݒ� //@}
	void SetText( const TCHAR* str );

	//@{ �\�� //@}
	void ShowUp( int sw=SW_SHOW );

	//@{ �ړ� //@}
	void MoveTo( int l, int t, int r, int b );

	//@{ �t�H�[�J�X //@}
	void SetFocus();

	//@{ �őO�ʂ�Go! //@}
	void SetFront();

	void SetActive();

	//@{ ��ʒ�����Go! //@}
	void SetCenter();

public:

	//@{ �E�C���h�E�n���h�� //@}
	HWND hwnd() const;

	//@{ �ʒu�E�T�C�Y //@}
	void getPos( RECT* rc ) const;

	//@{ �T�C�Y //@}
	void getClientRect( RECT* rc ) const;

	//@{ ���C�����[�v���񂵂Ă�E�C���h�E���ǂ��� //@}
	bool isMainWnd() const;

	//@{ �����Ă�H //@}
	bool isAlive() const;

	//@{ Is the window visible? //@}
	bool isVisible() const;

public:

	//@{ ���������b�Z�[�W��K���ɏ��� //@}
	static void ProcessMsg();

	//@{ �őO�ʂ�Go! //@}
	static void SetFront( HWND hwnd );

	//@{
	//	��ʒ�����Go!
	//	@param hwnd �������E�C���h�E
	//	@param rel ��ɂ���E�C���h�E
	//@}
	static void SetCenter( HWND hwnd, HWND rel=NULL );

protected:

	// �������Ȃ��R���X�g���N�^
	Window();

	// Hwnd���Z�b�g
	void SetHwnd( HWND wnd );

	// �A�N�Z�����[�^��ʂ��Ƃ��_�C�A���O���b�Z�[�W�̏����Ƃ�
	virtual bool PreTranslateMessage( MSG* ) = 0;

private:

	HWND wnd_;
	bool isLooping_;

private:

	NOCOPY(Window);
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline LRESULT Window::SendMsg( UINT msg, WPARAM wp, LPARAM lp )
	{ return ::SendMessage( wnd_, msg, wp, lp ); }

inline BOOL Window::PostMsg( UINT msg, WPARAM wp, LPARAM lp )
	{ return ::PostMessage( wnd_, msg, wp, lp ); }

inline int Window::MsgBox( LPCTSTR m, LPCTSTR c, UINT y ) const
	{ return ::MessageBox( wnd_?wnd_:GetActiveWindow(), m, c, y ); }

inline void Window::ShowUp( int sw )
	{ ::ShowWindow( wnd_, sw ), ::UpdateWindow( wnd_ ); }

inline void Window::SetText( const TCHAR* str )
	{ ::SetWindowText( wnd_, str ); }

inline void Window::MoveTo( int l, int t, int r, int b )
	{ ::MoveWindow( wnd_, l, t, r-l, b-t, TRUE ); }

inline void Window::SetFocus()
	{ ::SetFocus( wnd_ ); }

inline void Window::SetFront()
	{ SetFront( wnd_ ); }

inline void Window::SetActive()
	{ ::SetActiveWindow( wnd_ ); }

inline void Window::SetCenter()
	{ SetCenter( wnd_ ); }

inline HWND Window::hwnd() const
	{ return wnd_; }

inline bool Window::isMainWnd() const
	{ return isLooping_; }

inline void Window::getPos( RECT* rc ) const
	{ ::GetWindowRect( wnd_, rc ); }

inline void Window::getClientRect( RECT* rc ) const
	{ ::GetClientRect( wnd_, rc ); }

inline bool Window::isAlive() const
	{ return FALSE != ::IsWindow( wnd_ ); }

inline bool Window::isVisible() const
	{ return FALSE != ::IsWindowVisible( wnd_ ); }


#endif // __ccdoc__
//=========================================================================
//@{
//	IME����}�l�[�W��
//
//	Global IME ���T�|�[�g����ɂ́A�E�C���h�E���b�Z�[�W�̏�����
//	���{�I�ɓ���ւ���K�v������B�����ŁA���������̃N���X�ɂ܂Ƃ�
//	Window�N���X�ƘA�g�������s�����ƂŁA���C�u�����̊O����͈��
//	�C�ɂ����������ł���悤�ɂ��Ă����B�Ȃ��AGlobal IME�ɑΉ�
//	����ɂ̓o�[�W�����̐V����Platform SDK���K�v�Ȃ���
//	�}�N�� USEGLOBALIME ����`����Ă��Ȃ���΂��̕ӂ͏������Ȃ��B
//@}
//=========================================================================
class IMEManager
{
public:

	//@{ �t�H���g�w�� //@}
	void SetFont( HWND wnd, const LOGFONT& lf );

	//@{ �ʒu�w�� //@}
	void SetPos( HWND wnd, int x, int y );

	//@{ �m�蕶����Q�b�g�B�󂯎������ delete ���邱�ƁB //@}
	void GetString( HWND wnd, unicode** str, ulong* len );

	//@{ �ĕϊ� //@}
	void SetString( HWND wnd, unicode* str, ulong len );

	//@{ GlobalIME�𗘗p�\��Ԃɂ��� //@}
	void EnableGlobalIME( bool enable );

	//@{ IME��ON/OFF�ɂ��� //@}
	void SetState( HWND wnd, bool enable );

	//@{ IME ON/OFF���� //@}
	BOOL GetState( HWND wnd );

	//@{ IME�������ǂ����𒲂ׂ� //@}
	BOOL IsIME();

	//@{ �t�ϊ����T�|�[�g�𒲂ׂ� //@}
	BOOL CanReconv();

	//@{ GlobalIME���g����Window�̃��X�g��o�^ //@}
	void FilterWindows( ATOM* lst, UINT siz );

private:

	IMEManager();
	~IMEManager();
	void    TranslateMsg( MSG* msg );
	LRESULT DefProc( HWND wnd, UINT msg, WPARAM wp, LPARAM lp );
	void    MsgLoopBegin();
	void    MsgLoopEnd();

private:

	#ifdef USEGLOBALIME
		IActiveIMMApp*              immApp_;
		IActiveIMMMessagePumpOwner* immMsg_;
	#endif
	#ifdef NO_IME
		#define hIMM32_ = 0;
	#elif defined(TARGET_VER) && TARGET_VER<=350
		const HINSTANCE hIMM32_;
	#else
		#define hIMM32_ 1
	#endif
	static IMEManager* pUniqueInstance_;

private:

	friend class Window;
	friend class WndImpl;
	friend void APIENTRY  Startup();
	friend inline IMEManager& ime();
	NOCOPY(IMEManager);
};



//-------------------------------------------------------------------------

//@{ �B���IME�Ǘ��I�u�W�F�N�g��Ԃ� //@}
inline IMEManager& ime()
	{ return *IMEManager::pUniqueInstance_; }



//=========================================================================
//@{
//	���ʂ̃E�C���h�E����
//
//	�h���N���X���`���āA�R���X�g���N�^�̈����� WndImpl ��
//	WNDCLASS����X�^�C����n���A����Ȃ� WNDCLASS �� Register
//	���āA���Ƃ͓K���ȃ^�C�~���O�� Create() �Ƃ����g������z��B
//
//	HWND����WndImpl*�ւ̕ϊ���x86/x86-64��p�̃T���N�ōs���Ă��܂��B
//	�̂ŁA���̃A�[�L�e�N�`���ł͂��̂܂܂ł͓��삵�܂���B�ڐA
//	����ꍇ�́AGWL_USERDATA ���g���Ȃ� clsExtra �ɓ����Ȃ��
//	���������ėp���̂�����@�ɓK�X�ς��Ă��������B
//@}
//=========================================================================

class A_NOVTABLE WndImpl : public Window
{
	enum { THUNK_SIZE = 22 };

public:

	//@{ �E�C���h�E�쐬 //@}
	bool Create( LPCTSTR wndName=NULL,      HWND parent=NULL,
	             int  x     =CW_USEDEFAULT, int y      =CW_USEDEFAULT,
	             int  width =CW_USEDEFAULT, int height =CW_USEDEFAULT,
	             void* param=NULL );

	//@{ �E�C���h�E�j�� //@}
	void Destroy();

protected:

	//@{
	//	�R���X�g���N�^
	//	@param className �E�C���h�E�N���X��
	//	@param style �W���X�^�C��
	//	@param styleEx �W���g���X�^�C��
	//@}
	WndImpl( LPCTSTR className, DWORD style, DWORD styleEx=0 );
	~WndImpl();

	//@{ �N���X���p�̌^ //@}
	typedef const TCHAR* const ClsName;

	//@{ �E�C���h�E�N���X�o�^ //@}
	static ATOM Register( WNDCLASS* cls );

	// �Ă��Ɓ[�Ɏ������Ĕ������Ă��������B
	// on_command�́A�������Ȃ�������false��Ԃ����ƁB
	// on_message�́A�������Ȃ�������WndImpl::on_message���Ăяo�����ƁB
	// PreTranslateMessage�́A�������Ă����Ȃ��Ă����ŌĂяo�����ƁB
	virtual void    on_create( CREATESTRUCT* cs );
	virtual void    on_destroy();
	virtual bool    on_command( UINT id, HWND ctrl );
	virtual LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp );
	bool    PreTranslateMessage( MSG* msg ) override;

private:

	static LRESULT CALLBACK StartProc( HWND, UINT, WPARAM, LPARAM );
	static LRESULT CALLBACK MainProc( WndImpl*, UINT, WPARAM, LPARAM );
	#ifdef NO_ASMTHUNK
	static LRESULT CALLBACK TrunkMainProc( HWND, UINT, WPARAM, LPARAM );
	#endif
	void SetUpThunk( HWND wnd );

private:

	LPCTSTR     className_;
	const DWORD style_, styleEx_;
#ifndef NO_ASMTHUNK
	byte*       thunk_;
#endif
};


//=========================================================================
//@{
//	�_�C�A���O����
//
//	�h���N���X���`���āA�R���X�g���N�^�̈����� DlgImpl ��
//	���\�[�XID��n���A��͓K���ȃ^�C�~���O�� GoModal �Ȃ�
//	Modeless �Ȃ�A�Ƃ����g������z��B
//
//	HWND����DlgImpl*�ւ̕ϊ��� GWL_USERDATA �ōs���Ă��܂��B
//	�̂ŁA����͎g��Ȃ��悤�ɂ��܂��傤�B
//@}
//=========================================================================

class A_NOVTABLE DlgImpl : public Window
{
public:

	enum dlgtype { MODAL, MODELESS, UNDEFINED };

	//@{ ���[�_���Ŏ��s //@}
	void GoModal( HWND parent=NULL );

	//@{ ���[�h���X�ō쐬 //@}
	void GoModeless( HWND parent=NULL );

	//@{ �����I�ɏI��点�� //@}
	void End( UINT code );

public:

	//@{ ���[�_�������[�h���X�� //@}
	dlgtype type() const;

	//@{ �I���R�[�h�擾 //@}
	UINT endcode() const;

protected:

	//@{ �R���X�g���N�^ //@}
	explicit DlgImpl( UINT id )
		: type_(UNDEFINED), rsrcID_( id ) { }
	~DlgImpl();

	//@{ �q�A�C�e��ID��HWND�ϊ� //@}
	HWND item( UINT id ) const;

	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M�i�|�C���^����Łj //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, void* lp );

	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M�i�����񑗂�Łj //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, const TCHAR* lp );

	bool isItemChecked( UINT id ) const;
	void setItemCheck( UINT id, WPARAM state);
	void CheckItem( UINT id);
	void UncheckItem( UINT id);
	void GrayCheckItem( UINT id);

	LRESULT SetItemText( UINT id, const TCHAR *str );
	LRESULT GetItemText( UINT id, size_t len, TCHAR *str ) const;

	// �Ă��Ɓ[�Ɏ������Ĕ������Ă��������B
	// on_ok/on_cancel�́A�I�����ėǂ��Ȃ�true��Ԃ����ƁB
	// on_cmd/on_msg�́A�����ς݂Ȃ�true��Ԃ����ƁB
	virtual void on_init();
	virtual void on_destroy();
	virtual bool on_ok();
	virtual bool on_cancel();
	virtual bool on_command( UINT cmd, UINT id, HWND ctrl );
	virtual bool on_message( UINT msg, WPARAM wp, LPARAM lp );
	bool PreTranslateMessage( MSG* msg ) override;

private:

	static INT_PTR CALLBACK MainProc( HWND, UINT, WPARAM, LPARAM );

private:

	dlgtype    type_;
	UINT       endCode_;
	const UINT rsrcID_;
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline DlgImpl::dlgtype DlgImpl::type() const
	{ return type_; }

inline UINT DlgImpl::endcode() const
	{ return endCode_; }

inline HWND DlgImpl::item( UINT id ) const
	{ return ::GetDlgItem( hwnd(), id ); }

inline LRESULT DlgImpl::SendMsgToItem
	( UINT id, UINT msg, WPARAM wp, LPARAM lp )
	{ return ::SendDlgItemMessage( hwnd(), id, msg, wp, lp ); }

inline LRESULT DlgImpl::SendMsgToItem( UINT id, UINT msg, void* lp )
	{ return ::SendDlgItemMessage( hwnd(), id, msg, 0,
	                            reinterpret_cast<LPARAM>(lp) ); }

inline LRESULT DlgImpl::SendMsgToItem( UINT id, UINT msg, const TCHAR* lp )
	{ return ::SendDlgItemMessage( hwnd(), id, msg, 0,
	                            reinterpret_cast<LPARAM>(lp) ); }
inline bool DlgImpl::isItemChecked( UINT id ) const
	{ return BST_CHECKED == ::SendDlgItemMessage(hwnd(), id, BM_GETCHECK, 0, 0); }

inline void DlgImpl::setItemCheck( UINT id, WPARAM state )
	{ ::SendDlgItemMessage(hwnd(), id, BM_SETCHECK, state, 0); }

inline void DlgImpl::CheckItem( UINT id)
	{ setItemCheck(id, BST_CHECKED); }

inline void DlgImpl::UncheckItem( UINT id)
	{ setItemCheck(id, BST_UNCHECKED); }

inline void DlgImpl::GrayCheckItem( UINT id)
	{ setItemCheck(id, BST_INDETERMINATE); }

inline LRESULT DlgImpl::SetItemText( UINT id, const TCHAR *str )
	{ return ::SendDlgItemMessage(hwnd(), id, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str) ); }

inline LRESULT DlgImpl::GetItemText( UINT id, size_t len, TCHAR *str ) const
	{ return ::SendDlgItemMessage(hwnd(), id, WM_GETTEXT, static_cast<WPARAM>(len), reinterpret_cast<LPARAM>(str) ); }



//=========================================================================

#endif // __ccdoc__
}      // namespace ki
#endif // _KILIB_WINDOW_H_
