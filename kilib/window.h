#ifndef _KILIB_WINDOW_H_
#define _KILIB_WINDOW_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



// タイムアウト付きMsgBoxの返値
// #define IDTIMEOUT 0



//=========================================================================
//@{ @pkg ki.Window //@}
//@{
//	窓操作用クラス
//
//	外側から、ダイアログやコントロールやプロパティシートや
//	普通のウインドウを全部まとめて共通に扱うためのインターフェイス。
//	実際の実装は、下位クラス XxxImpl で行われている。
//@}
//=========================================================================

class A_NOVTABLE Window : public Object
{
public:

	//@{ メインメッセージループ //@}
	void MsgLoop();

	//@{ メッセージを送って処理されるまで待機 //@}
	LRESULT SendMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{ メッセージを送ってすぐ帰る //@}
	BOOL PostMsg( UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{
	//	自動消滅機能付きメッセージボックス
	//	@param msg 表示する文字列
	//	@param caption ダイアログの題名
	//	@param type Win32SDKの説明を見てね
	//@}
	int MsgBox( LPCTSTR msg, LPCTSTR caption=NULL, UINT type=MB_OK ) const;

	static int __cdecl MsgBoxf( HWND hwnd,  LPCTSTR title, LPCTSTR fmt, ... );

	//@{ テキスト設定 //@}
	void SetText( const TCHAR* str );

	//@{ 表示 //@}
	void ShowUp( int sw=SW_SHOW );

	//@{ 移動 //@}
	void MoveTo( int l, int t, int r, int b );

	//@{ フォーカス //@}
	void SetFocus();

	//@{ 最前面へGo! //@}
	void SetFront();

	void SetActive();

	//@{ 画面中央へGo! //@}
	void SetCenter();

public:

	//@{ ウインドウハンドル //@}
	HWND hwnd() const;

	//@{ 位置・サイズ //@}
	void getPos( RECT* rc ) const;

	//@{ サイズ //@}
	void getClientRect( RECT* rc ) const;

	//@{ メインループを回してるウインドウかどうか //@}
	bool isMainWnd() const;

	//@{ 生きてる？ //@}
	bool isAlive() const;

	//@{ Is the window visible? //@}
	bool isVisible() const;

public:

	//@{ 未処理メッセージを適当に処理 //@}
	static void ProcessMsg();

	//@{ 最前面へGo! //@}
	static void SetFront( HWND hwnd );

	//@{
	//	画面中央へGo!
	//	@param hwnd 動かすウインドウ
	//	@param rel 基準にするウインドウ
	//@}
	static void SetCenter( HWND hwnd, HWND rel=NULL );

protected:

	// 何もしないコンストラクタ
	Window();

	// Hwndをセット
	void SetHwnd( HWND wnd );

	// アクセラレータを通すとかダイアログメッセージの処理とか
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
//	IME制御マネージャ
//
//	Global IME をサポートするには、ウインドウメッセージの処理を
//	根本的に入れ替える必要がある。そこで、処理をこのクラスにまとめ
//	Windowクラスと連携処理を行うことで、ライブラリの外からは一切
//	気にせず処理をできるようにしておく。なお、Global IMEに対応
//	するにはバージョンの新しいPlatform SDKが必要なため
//	マクロ USEGLOBALIME が定義されていなければその辺は処理しない。
//@}
//=========================================================================
class IMEManager
{
public:

	//@{ フォント指定 //@}
	void SetFont( HWND wnd, const LOGFONT& lf );

	//@{ 位置指定 //@}
	void SetPos( HWND wnd, int x, int y );

	//@{ 確定文字列ゲット。受け取ったら delete すること。 //@}
	void GetString( HWND wnd, unicode** str, ulong* len );

	//@{ 再変換 //@}
	void SetString( HWND wnd, unicode* str, ulong len );

	//@{ GlobalIMEを利用可能状態にする //@}
	void EnableGlobalIME( bool enable );

	//@{ IMEをON/OFFにする //@}
	void SetState( HWND wnd, bool enable );

	//@{ IME ON/OFF判定 //@}
	BOOL GetState( HWND wnd );

	//@{ IMEを持つかどうかを調べる //@}
	BOOL IsIME();

	//@{ 逆変換をサポートを調べる //@}
	BOOL CanReconv();

	//@{ GlobalIMEを使えるWindowのリストを登録 //@}
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

//@{ 唯一のIME管理オブジェクトを返す //@}
inline IMEManager& ime()
	{ return *IMEManager::pUniqueInstance_; }



//=========================================================================
//@{
//	普通のウインドウ実装
//
//	派生クラスを定義して、コンストラクタの引数で WndImpl に
//	WNDCLASS名やスタイルを渡し、初回なら WNDCLASS を Register
//	して、あとは適当なタイミングで Create() という使い方を想定。
//
//	HWNDからWndImpl*への変換はx86/x86-64専用のサンクで行っています。
//	ので、他のアーキテクチャではこのままでは動作しません。移植
//	する場合は、GWL_USERDATA を使うなり clsExtra に入れるなりで
//	もう少し汎用性のある方法に適宜変えてください。
//@}
//=========================================================================

class A_NOVTABLE WndImpl : public Window
{
	enum { THUNK_SIZE = 22 };

public:

	//@{ ウインドウ作成 //@}
	bool Create( LPCTSTR wndName=NULL,      HWND parent=NULL,
	             int  x     =CW_USEDEFAULT, int y      =CW_USEDEFAULT,
	             int  width =CW_USEDEFAULT, int height =CW_USEDEFAULT,
	             void* param=NULL );

	//@{ ウインドウ破棄 //@}
	void Destroy();

protected:

	//@{
	//	コンストラクタ
	//	@param className ウインドウクラス名
	//	@param style 標準スタイル
	//	@param styleEx 標準拡張スタイル
	//@}
	WndImpl( LPCTSTR className, DWORD style, DWORD styleEx=0 );
	~WndImpl();

	//@{ クラス名用の型 //@}
	typedef const TCHAR* const ClsName;

	//@{ ウインドウクラス登録 //@}
	static ATOM Register( WNDCLASS* cls );

	// てけとーに実装して反応してください。
	// on_commandは、処理しなかったらfalseを返すこと。
	// on_messageは、処理しなかったらWndImpl::on_messageを呼び出すこと。
	// PreTranslateMessageは、処理してもしなくても中で呼び出すこと。
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
//	ダイアログ実装
//
//	派生クラスを定義して、コンストラクタの引数で DlgImpl に
//	リソースIDを渡し、後は適当なタイミングで GoModal なり
//	Modeless なり、という使い方を想定。
//
//	HWNDからDlgImpl*への変換は GWL_USERDATA で行っています。
//	ので、それは使わないようにしましょう。
//@}
//=========================================================================

class A_NOVTABLE DlgImpl : public Window
{
public:

	enum dlgtype { MODAL, MODELESS, UNDEFINED };

	//@{ モーダルで実行 //@}
	void GoModal( HWND parent=NULL );

	//@{ モードレスで作成 //@}
	void GoModeless( HWND parent=NULL );

	//@{ 強制的に終わらせる //@}
	void End( UINT code );

public:

	//@{ モーダルかモードレスか //@}
	dlgtype type() const;

	//@{ 終了コード取得 //@}
	UINT endcode() const;

protected:

	//@{ コンストラクタ //@}
	explicit DlgImpl( UINT id )
		: type_(UNDEFINED), rsrcID_( id ) { }
	~DlgImpl();

	//@{ 子アイテムID→HWND変換 //@}
	HWND item( UINT id ) const;

	//@{ アイテムに対してメッセージ送信 //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, WPARAM wp=0, LPARAM lp=0 );

	//@{ アイテムに対してメッセージ送信（ポインタ送る版） //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, void* lp );

	//@{ アイテムに対してメッセージ送信（文字列送る版） //@}
	LRESULT SendMsgToItem( UINT id, UINT msg, const TCHAR* lp );

	bool isItemChecked( UINT id ) const;
	void setItemCheck( UINT id, WPARAM state);
	void CheckItem( UINT id);
	void UncheckItem( UINT id);
	void GrayCheckItem( UINT id);

	LRESULT SetItemText( UINT id, const TCHAR *str );
	LRESULT GetItemText( UINT id, size_t len, TCHAR *str ) const;

	// てけとーに実装して反応してください。
	// on_ok/on_cancelは、終了して良いならtrueを返すこと。
	// on_cmd/on_msgは、処理済みならtrueを返すこと。
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
