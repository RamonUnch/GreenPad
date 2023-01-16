#ifndef _KILIB_APP_H_
#define _KILIB_APP_H_
#include "types.h"
#include "log.h"

HRESULT MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);

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
//	�A�v���P�[�V�����S�̂̓���
//
//	�A�v���N��/�I���p������S�����܂��B
//	��kilib�ƈ���āA���[�U�[���̃A�v���P�[�V�����N���X��
//	��������h�������邱�Ƃ͏o���܂���B���[�U�[�̃R�[�h�́A
//	�K�� kmain() �Ƃ����O���[�o���֐�������s�J�n����܂��B
//	����App�N���X���̂́A���HINSTANCE�̊Ǘ����s�������B
//@}
//=========================================================================

class App
{
public:

	enum imflag { CTL=1, COM=2, OLE=4, OLEDLL=8 };

	//@{
	//	��X�̃��W���[��������������
	//
	//	����ŏ��������Ă����ƁAApp�I�����Ɏ�����
	//	�I���������s����̂ŊȒP�֗��ł������܂��B
	//	@param what CTL(�R�����R���g���[��)�ACOM�AOLE
	//@}
	void InitModule( imflag what );

	//@{ �v���Z�X�����I�� //@}
	void Exit( int code );

	//@{ ���\�[�X //@}
	HACCEL LoadAccel( LPCTSTR name );

	//@{ ���\�[�X //@}
	HACCEL LoadAccel( UINT id );

	//@{ ���\�[�X //@}
	HBITMAP LoadBitmap( LPCTSTR name );

	//@{ ���\�[�X //@}
	HBITMAP LoadBitmap( UINT id );

	//@{ ���\�[�X(OBM_XXXX) //@}
	HBITMAP LoadOemBitmap( LPCTSTR obm );

	//@{ ���\�[�X //@}
	HCURSOR LoadCursor( LPCTSTR name );

	//@{ ���\�[�X //@}
	HCURSOR LoadCursor( UINT id );

	//@{ ���\�[�X(IDC_XXXX) //@}
	HCURSOR LoadOemCursor( LPCTSTR idc );

	//@{ ���\�[�X //@}
	HICON LoadIcon( LPCTSTR name );

	//@{ ���\�[�X //@}
	HICON LoadIcon( UINT id );

	//@{ ���\�[�X(IDI_XXXX) //@}
	HICON LoadOemIcon( LPCTSTR idi );

	//@{ ���\�[�X //@}
	HMENU LoadMenu( LPCTSTR name );

	//@{ ���\�[�X //@}
	HMENU LoadMenu( UINT id );

	//@{ ���\�[�X //@}
	int LoadString( UINT id, LPTSTR buf, int siz );

public:

	//@{ �C���X�^���X�n���h�� //@}
	HINSTANCE hinst() const;
	HINSTANCE       hOle32_;

	//@{ Windows�̃o�[�W���� //@}
	DWORD getOOSVer() const;
	WORD getOSVer() const;
	WORD getOSBuild() const;
	bool isOSVerLarger(DWORD ver) const;
	bool is9xOSVerLarger(DWORD ver) const;
	bool isNTOSVerLarger(DWORD ver) const;
	bool isOSVerEqual(DWORD ver) const;
	bool is9xOSVerEqual(DWORD ver) const;
	bool isNTOSVerEqual(DWORD ver) const;
	bool isWin95() const;
	bool isNT() const;
	bool isWin32s() const;
	bool isNewShell() const;
	bool isNewOpenSaveDlg() const;
	bool isNewTypeWindows() const;

private:

	App();
	~App();
	void SetExitCode( int code );
	MYVERINFO init_osver();

private:
	const MYVERINFO osver_;
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

//@{ �B��̃A�v�����I�u�W�F�N�g��Ԃ� //@}
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
