#ifndef _KILIB_APP_H_
#define _KILIB_APP_H_
#include "types.h"
#include "log.h"
#include "memory.h"

HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
HRESULT MyCoLockObjectExternal(IUnknown * pUnk, BOOL fLock, BOOL fLastUnlockReleases);

#if defined(__GNUC__) && defined(_M_IX86) && _M_IX86 == 300
	// In recent GCC versions this is the only way to link to the real
	// Win32 functions (i386 only).
	#undef InterlockedIncrement
	#undef InterlockedDecrement
	extern "C" WINBASEAPI LONG WINAPI InterlockedIncrement(LONG volatile *);
	extern "C" WINBASEAPI LONG WINAPI InterlockedDecrement(LONG volatile *);
#endif

// Use to make a ordered window version ie: MKVER(3,10,511) = 0x030A01FF
#define MKVER(M, m, b) ( (DWORD)( (BYTE)(M)<<24 | (BYTE)(m)<<16 | (WORD)(b) ) )

// roytam1's versioninfo style
typedef struct _MYVERINFO {
	union {
		DWORD dwVer;
		struct {
			WORD wBuild;
			union {
				WORD wVer;
				struct {
					BYTE cMinor;
					BYTE cMajor;
				} u;
			} ver;
		} vb;
	} v;
	WORD wPlatform;
	WORD wFromWhichAPI;
} MYVERINFO;

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
	inline HACCEL LoadAccel( LPCTSTR name )
		{ return ::LoadAccelerators( hInst_, name ); }

	//@{ リソース //@}
	inline HACCEL LoadAccel( UINT id )
		{ return ::LoadAccelerators( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ リソース //@}
	inline HBITMAP LoadBitmap( LPCTSTR name )
		{ return ::LoadBitmap( hInst_, name ); }

	//@{ リソース //@}
	inline HBITMAP LoadBitmap( UINT id )
		{ return ::LoadBitmap( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ リソース(OBM_XXXX) //@}
	inline HBITMAP LoadOemBitmap( LPCTSTR obm )
		{ return ::LoadBitmap( NULL, obm ); }

	//@{ リソース //@}
	inline HCURSOR LoadCursor( LPCTSTR name )
		{ return ::LoadCursor( hInst_, name ); }

	//@{ リソース //@}
	inline HCURSOR LoadCursor( UINT id )
		{ return ::LoadCursor( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ リソース(IDC_XXXX) //@}
	inline HCURSOR LoadOemCursor( LPCTSTR idc )
		{ return ::LoadCursor( NULL, idc ); }

	//@{ リソース //@}
	inline HICON LoadIcon( LPCTSTR name )
		{ return ::LoadIcon( hInst_, name ); }

	//@{ リソース //@}
	inline HICON LoadIcon( UINT id )
		{ return ::LoadIcon( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ リソース(IDI_XXXX) //@}
	inline HICON LoadOemIcon( LPCTSTR idi )
		{ return ::LoadIcon( NULL, idi ); }

	//@{ リソース //@}
	inline HMENU LoadMenu( LPCTSTR name )
		{ return ::LoadMenu( hInst_, name ); }

	//@{ リソース //@}
	inline HMENU LoadMenu( UINT id )
		{ return ::LoadMenu( hInst_, MAKEINTRESOURCE(id) ); }

	//@{ リソース //@}
	inline int LoadString( UINT id, LPTSTR buf, int siz )
		{ return ::LoadString( hInst_, id, buf, siz ); }

public:

	//@{ インスタンスハンドル //@}
	HINSTANCE hinst() const { return hInst_; }
	HINSTANCE hOle32() const { return hOle32_; }
	bool hasSysDLL(const TCHAR *dllname) const;
	//@{ Windowsのバージョン //@}

	DWORD getOOSVer() const A_PURE;
	WORD getOSVer() const A_PURE;
	WORD getOSBuild() const A_PURE;
	bool isOSVerLarger(DWORD ver) const A_PURE;
	bool is9xOSVerLarger(DWORD ver) const A_PURE;
	bool isNTOSVerLarger(DWORD ver) const A_PURE;
	bool isOSVerEqual(DWORD ver) const A_PURE;
	bool is9xOSVerEqual(DWORD ver) const A_PURE;
	bool isNTOSVerEqual(DWORD ver) const A_PURE;
	bool isWin95() const A_PURE;
	bool isNT() const A_PURE;
	bool isWin32s() const A_PURE;
	bool isNewShell() const A_PURE;
	bool isNewOpenSaveDlg() const A_PURE;
	bool isNewTypeWindows() const A_PURE;

private:

	App();
	~App();
	void SetExitCode( int code );
	MYVERINFO init_osver();

private:
	const MYVERINFO osver_;
	int             exitcode_;
	uint            loadedModule_;
	const HINSTANCE hInst_;
	HINSTANCE       hOle32_;
	HINSTANCE       hInstComCtl_;
	static App*     pUniqueInstance_;

private:

	friend void APIENTRY Startup();

	//@{ 唯一のアプリ情報オブジェクトを返す //@}
	friend inline App& app();

	NOCOPY(App);
};

inline ki::App& app()
	{ return *App::pUniqueInstance_; }

//=========================================================================

}      // namespace ki
#endif // _KILIB_APP_H_
