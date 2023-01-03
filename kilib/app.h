#ifndef _KILIB_APP_H_
#define _KILIB_APP_H_
#include "types.h"
#include "log.h"

HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	アプリケーション全体の統括
//
//	アプリ起動/終了用処理を担当します。
//	旧kilibと違って、ユーザー側のアプリケーションクラスを
//	ここから派生させることは出来ません。ユーザーのコードは、
//	必ず kmain() というグローバル関数から実行開始されます。
//	このAppクラス自体は、主にHINSTANCEの管理を行うだけ。
//@}
//=========================================================================

class App
{
public:

	enum imflag { CTL=1, COM=2, OLE=4, OLEDLL=8 };

	//@{
	//	種々のモジュールを初期化する
	//
	//	これで初期化しておくと、App終了時に自動で
	//	終了処理が行われるので簡単便利でございます。
	//	@param what CTL(コモンコントロール)、COM、OLE
	//@}
	void InitModule( imflag what );

	//@{ プロセス強制終了 //@}
	void Exit( int code );

	//@{ リソース //@}
	HACCEL LoadAccel( LPCTSTR name );

	//@{ リソース //@}
	HACCEL LoadAccel( UINT id );

	//@{ リソース //@}
	HBITMAP LoadBitmap( LPCTSTR name );

	//@{ リソース //@}
	HBITMAP LoadBitmap( UINT id );

	//@{ リソース(OBM_XXXX) //@}
	HBITMAP LoadOemBitmap( LPCTSTR obm );

	//@{ リソース //@}
	HCURSOR LoadCursor( LPCTSTR name );

	//@{ リソース //@}
	HCURSOR LoadCursor( UINT id );

	//@{ リソース(IDC_XXXX) //@}
	HCURSOR LoadOemCursor( LPCTSTR idc );

	//@{ リソース //@}
	HICON LoadIcon( LPCTSTR name );

	//@{ リソース //@}
	HICON LoadIcon( UINT id );

	//@{ リソース(IDI_XXXX) //@}
	HICON LoadOemIcon( LPCTSTR idi );

	//@{ リソース //@}
	HMENU LoadMenu( LPCTSTR name );

	//@{ リソース //@}
	HMENU LoadMenu( UINT id );

	//@{ リソース //@}
	int LoadString( UINT id, LPTSTR buf, int siz );

public:

	//@{ インスタンスハンドル //@}
	HINSTANCE hinst() const;
	HINSTANCE       hOle32_;

	//@{ Windowsのバージョン //@}
	static const OSVERSIONINFOA& osver();
	static DWORD getOSVer();
	static DWORD getOSBuild();
	static bool isOSVerLarger(DWORD ver, DWORD build);
	static bool is9xOSVerLarger(DWORD ver, DWORD build);
	static bool isNTOSVerLarger(DWORD ver, DWORD build);
	static bool isOSVerEqual(DWORD ver, DWORD build);
	static bool is9xOSVerEqual(DWORD ver, DWORD build);
	static bool isNTOSVerEqual(DWORD ver, DWORD build);
	static bool isWin95();
	static bool isNT();
	static bool isWin32s();
	static bool is351p();
	static bool isNT31();
	static bool isNewShell();
	static bool isNewTypeWindows();

private:

	App();
	~App();
	void SetExitCode( int code );

private:

	int             exitcode_;
	ulong           loadedModule_;
	const HINSTANCE hInst_;
	HINSTANCE       hInstComCtl_;
	static App*     pUniqueInstance_;

private:

	friend void APIENTRY Startup();
	friend inline App& app();
	NOCOPY(App);
};



//-------------------------------------------------------------------------

//@{ 唯一のアプリ情報オブジェクトを返す //@}
inline App& app()
	{ return *App::pUniqueInstance_; }

inline HACCEL App::LoadAccel( LPCTSTR name )
	{ return ::LoadAccelerators( hInst_, name ); }

inline HACCEL App::LoadAccel( UINT id )
	{ return ::LoadAccelerators( hInst_, MAKEINTRESOURCE(id) ); }

inline HBITMAP App::LoadBitmap( LPCTSTR name )
	{ return ::LoadBitmap( hInst_, name ); }

inline HBITMAP App::LoadBitmap( UINT id )
	{ return ::LoadBitmap( hInst_, MAKEINTRESOURCE(id) ); }

inline HBITMAP App::LoadOemBitmap( LPCTSTR obm )
	{ return ::LoadBitmap( NULL, obm ); }

inline HCURSOR App::LoadCursor( LPCTSTR name )
	{ return ::LoadCursor( hInst_, name ); }

inline HCURSOR App::LoadCursor( UINT id )
	{ return ::LoadCursor( hInst_, MAKEINTRESOURCE(id) ); }

inline HCURSOR App::LoadOemCursor( LPCTSTR idc )
	{ return ::LoadCursor( NULL, idc ); }

inline HICON App::LoadIcon( LPCTSTR name )
	{ return ::LoadIcon( hInst_, name ); }

inline HICON App::LoadIcon( UINT id )
	{ return ::LoadIcon( hInst_, MAKEINTRESOURCE(id) ); }

inline HICON App::LoadOemIcon( LPCTSTR idi )
	{ return ::LoadIcon( NULL, idi ); }

inline HMENU App::LoadMenu( LPCTSTR name )
	{ return ::LoadMenu( hInst_, name ); }

inline HMENU App::LoadMenu( UINT id )
	{ return ::LoadMenu( hInst_, MAKEINTRESOURCE(id) ); }

inline int App::LoadString( UINT id, LPTSTR buf, int siz )
	{ return ::LoadString( hInst_, id, buf, siz ); }

inline HINSTANCE App::hinst() const
	{ return hInst_; }



//=========================================================================

}      // namespace ki
#endif // _KILIB_APP_H_
