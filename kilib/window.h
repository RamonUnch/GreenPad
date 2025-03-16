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

class A_NOVTABLE Window
{
public:

	//@{ ���C�����b�Z�[�W���[�v //@}
	void MsgLoop();

	//@{ ���b�Z�[�W�𑗂��ď��������܂őҋ@ //@}
	inline LRESULT SendMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 ) const
		{ return ::SendMessage( wnd_, msg, wp, lp ); }

	//@{ ���b�Z�[�W�𑗂��Ă����A�� //@}
	inline BOOL PostMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 ) const
		{ return ::PostMessage( wnd_, msg, wp, lp ); }

	//@{
	//	�������ŋ@�\�t�����b�Z�[�W�{�b�N�X
	//	@param msg �\�����镶����
	//	@param caption �_�C�A���O�̑薼
	//	@param type Win32SDK�̐��������Ă�
	//@}
	inline int MsgBox( LPCTSTR msg, LPCTSTR caption=NULL, UINT type=MB_OK ) const
		{ return ::MessageBox( wnd_?wnd_:GetActiveWindow(), msg, caption, type ); }

	static int __cdecl MsgBoxf( HWND hwnd,  LPCTSTR title, LPCTSTR fmt, ... );

	//@{ �e�L�X�g�ݒ� //@}
	inline void SetText( const TCHAR* str )
		{ ::SetWindowText( wnd_, str ); }

	//@{ �\�� //@}
	inline void ShowUp( int sw=SW_SHOW )
		{ ::ShowWindow( wnd_, sw ), ::UpdateWindow( wnd_ ); }

	//@{ �ړ� //@}
	inline void MoveTo( int l, int t, int r, int b )
		{ ::MoveWindow( wnd_, l, t, r-l, b-t, TRUE ); }

	//@{ �t�H�[�J�X //@}
	inline void SetFocus() { ::SetFocus( wnd_ ); }

	//@{ �őO�ʂ�Go! //@}
	inline void SetFront() { SetFront( wnd_ ); }

	inline void SetActive() { ::SetActiveWindow( wnd_ ); }

	//@{ ��ʒ�����Go! //@}
	inline void SetCenter() { SetCenter( wnd_ ); }

public:

	//@{ �E�C���h�E�n���h�� //@}
	inline HWND hwnd() const { return wnd_; }

	//@{ �ʒu�E�T�C�Y //@}
	inline void getPos( RECT* rc ) const { ::GetWindowRect( wnd_, rc ); }

	//@{ �T�C�Y //@}
	inline void getClientRect( RECT* rc ) const { ::GetClientRect( wnd_, rc ); }

	//@{ ���C�����[�v���񂵂Ă�E�C���h�E���ǂ��� //@}
	inline bool isMainWnd() const { return isLooping_; }

	//@{ �����Ă�H //@}
	inline bool isAlive() const { return FALSE != ::IsWindow( wnd_ ); }

	//@{ Is the window visible? //@}
	inline bool isVisible() const { return FALSE != ::IsWindowVisible( wnd_ ); }

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
	void SetFont( HWND wnd, const LOGFONT& lf ) A_COLD;

	//@{ �ʒu�w�� //@}
	void SetPos( HWND wnd, int x, int y )  A_COLD;

	//@{ �m�蕶����Q�b�g�B�󂯎������ FreeString ���邱�ƁB //@}
	void GetString( HWND wnd, unicode** str, size_t* len ) A_COLD;

	static void FreeString( unicode *str ) { free( str ); }

	//@{ �ĕϊ� //@}
	void SetString( HWND wnd, unicode* str, size_t len ) A_COLD;

	//@{ GlobalIME�𗘗p�\��Ԃɂ��� //@}
	void EnableGlobalIME( bool enable ) A_COLD;

	//@{ IME��ON/OFF�ɂ��� //@}
	void SetState( HWND wnd, bool enable ) A_COLD;

	//@{ IME ON/OFF���� //@}
	BOOL GetState( HWND wnd ) A_COLD;

	//@{ IME�������ǂ����𒲂ׂ� //@}
	BOOL IsIME() A_COLD;

	//@{ �t�ϊ����T�|�[�g�𒲂ׂ� //@}
	BOOL CanReconv() A_COLD;

	//@{ GlobalIME���g����Window�̃��X�g��o�^ //@}
	void FilterWindows( ATOM* lst, UINT siz ) A_COLD;

private:

	IMEManager() A_COLD;
	~IMEManager() A_COLD;
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

	//@{ �B���IME�Ǘ��I�u�W�F�N�g��Ԃ� //@}
	friend inline IMEManager& ime();
	NOCOPY(IMEManager);
};

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

	static LRESULT CALLBACK StartProc( HWND, UINT, WPARAM, LPARAM ) A_COLD;
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
	inline dlgtype type() const { return type_; }

	//@{ �I���R�[�h�擾 //@}
	inline UINT endcode() const { return endCode_; }

protected:

	//@{ �R���X�g���N�^ //@}
	explicit DlgImpl( UINT id )
		: type_(UNDEFINED), rsrcID_( id ) { }
	~DlgImpl();

	//@{ �q�A�C�e��ID��HWND�ϊ� //@}
	inline HWND item( UINT id ) const { return ::GetDlgItem( hwnd(), id ); }

	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, WPARAM wp=0, LPARAM lp=0 )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, wp, lp ); }

	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M�i�|�C���^����Łj //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, void* lp )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, 0, reinterpret_cast<LPARAM>(lp) ); }


	//@{ �A�C�e���ɑ΂��ă��b�Z�[�W���M�i�����񑗂�Łj //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, const TCHAR* lp )
		{ return ::SendDlgItemMessage( hwnd(), id, msg, 0, reinterpret_cast<LPARAM>(lp) ); }

	bool isItemChecked( UINT id ) const
		{ return BST_CHECKED == ::SendDlgItemMessage(hwnd(), id, BM_GETCHECK, 0, 0); }
	void setItemCheck( UINT id, WPARAM state)
		{ ::SendDlgItemMessage(hwnd(), id, BM_SETCHECK, state, 0); }
	inline void CheckItem( UINT id)     { setItemCheck(id, BST_CHECKED); }
	inline void UncheckItem( UINT id)   { setItemCheck(id, BST_UNCHECKED); }
	inline void GrayCheckItem( UINT id) { setItemCheck(id, BST_INDETERMINATE); }

	LRESULT SetItemText( UINT id, const TCHAR *str )
		{ return ::SendDlgItemMessage(hwnd(), id, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(str) ); }
	LRESULT GetItemText( UINT id, size_t len, TCHAR *str ) const
		{ return ::SendDlgItemMessage(hwnd(), id, WM_GETTEXT, static_cast<WPARAM>(len), reinterpret_cast<LPARAM>(str) ); }

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


//=========================================================================

}      // namespace ki
#endif // _KILIB_WINDOW_H_
