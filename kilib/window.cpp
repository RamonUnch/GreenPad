#include "stdafx.h"
#include "app.h"
#include "window.h"
using namespace ki;

#if defined(TARGET_VER) && TARGET_VER<=350
#pragma comment(lib, "DelayImp.lib")
#pragma comment(linker, "/DelayLoad:IMM32.DLL")

// Dynalmically import GetKeyboardLayout for NT3.1 (NT3.5 has a stub).
typedef HKL(WINAPI *funkk)(DWORD dwLayout);
static HKL MyGetKeyboardLayout(DWORD dwLayout)
{
	static funkk func = (funkk)(-1);
	if(func == (funkk)(-1)) {
		func = (funkk) GetProcAddress(GetModuleHandleA("USER32.DLL"), "GetKeyboardLayout");
	}
	if (func) return func(dwLayout);

	return NULL;
}
#else 
  #define MyGetKeyboardLayout GetKeyboardLayout
#endif // Target_VER

#ifdef UNICOWS //Use A or W version at runtime...
static BOOL MyImmSetCompositionFont(HIMC hIMC, LPLOGFONTW plf)
{
	// Unicode support on Windows NT...
	if (app().isNT()) return ImmSetCompositionFontW(hIMC, plf);

	// Convert LPLOGFONTW --> LPLOGFONTA
	LOGFONTA lfa;
	// Copy lf struct in lfa.
	memmove((void*)&lfa, (void*)plf, sizeof(lfa));
	// Convert lfFaceName from W to A.
	
	::WideCharToMultiByte(CP_ACP, 0, plf->lfFaceName, -1 , lfa.lfFaceName, LF_FACESIZE, NULL, NULL);
	return ImmSetCompositionFontA(hIMC, &lfa);
}
#else
#define MyImmSetCompositionFont ImmSetCompositionFont
#endif
//=========================================================================
// IME�Ɋւ��邠�ꂱ��
//=========================================================================
static GUID myIID_IActiveIMMMessagePumpOwner = {0xb5cf2cfa,0x8aeb,0x11d1,{0x93,0x64,0x00,0x60,0xb0,0x67,0xb8,0x6e}};
IMEManager* IMEManager::pUniqueInstance_;
IMEManager::IMEManager()
#ifdef USEGLOBALIME
	: immApp_( NULL )
	, immMsg_( NULL )
#endif
{
	#ifdef USEGLOBALIME
		// �F�X�ʓ|�Ȃ̂�Win95�ł�GlobalIME����
		// No global IME on Win95 because it is buggy...
		if( !app().isWin95() )
		{
			app().InitModule( App::OLE );
			if( S_OK == ::MyCoCreateInstance(
					CLSID_CActiveIMM, NULL, CLSCTX_INPROC_SERVER,
					IID_IActiveIMMApp, (void**)&immApp_ ) )
			{
				immApp_->QueryInterface(
					myIID_IActiveIMMMessagePumpOwner, (void**)&immMsg_ );
			}
		}
	#endif //USEGLOBALIME

	// check if IMM32.DLL can be loaded...
	hasIMM32_ = !!LoadLibraryA("IMM32.DLL");

	// �B��̃C���X�^���X�͎��ł�
	pUniqueInstance_ = this;
}

IMEManager::~IMEManager()
{
	#ifdef USEGLOBALIME
		if( immMsg_ != NULL )
		{
			immMsg_->Release();
			immMsg_ = NULL;
		}
		if( immApp_ != NULL )
		{
			immApp_->Deactivate();
			immApp_->Release();
			immApp_ = NULL;
		}
	#endif
}

void IMEManager::EnableGlobalIME( bool enable )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
			if( enable ) immApp_->Activate( TRUE );
			else         immApp_->Deactivate();
	#endif
}

BOOL IMEManager::IsIME()
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HKL hKL = MyGetKeyboardLayout(GetCurrentThreadId());
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			return immApp_->IsIME( hKL );
		}
		else
	#endif // USEGLOBALIME
		if (hasIMM32_) 
		{
			return ::ImmIsIME( hKL );
		}
#endif // TARGET_VER
	return FALSE;
}

BOOL IMEManager::CanReconv()
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HKL hKL = MyGetKeyboardLayout(GetCurrentThreadId());
	DWORD nImeProps = ImmGetProperty( MyGetKeyboardLayout( 0 ), IGP_SETCOMPSTR );
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			immApp_->GetProperty( hKL, IGP_SETCOMPSTR, &nImeProps );
		}
		else
	#endif
		if (hasIMM32_) 
		{
			nImeProps = ::ImmGetProperty( hKL, IGP_SETCOMPSTR );
		}
		return (nImeProps & SCS_CAP_SETRECONVERTSTRING) != 0;
#else
	return FALSE;
#endif // TARGET_VER
}

BOOL IMEManager::GetState( HWND wnd )
{
	BOOL imeStatus = FALSE;
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HIMC ime;
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			immApp_->GetContext( wnd, &ime );
			imeStatus = immApp_->GetOpenStatus( ime );
			immApp_->ReleaseContext( wnd, ime );
		}
		else
	#endif
		if (hasIMM32_) 
		{
			ime = ::ImmGetContext( wnd );
			imeStatus = ::ImmGetOpenStatus(ime );
			::ImmReleaseContext( wnd, ime );
		}
#endif // TARGET_VER
	return imeStatus;
}

void IMEManager::SetState( HWND wnd, bool enable )
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HIMC ime;
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			immApp_->GetContext( wnd, &ime );
			immApp_->SetOpenStatus( ime, (enable ? TRUE : FALSE) );
			immApp_->ReleaseContext( wnd, ime );
		}
		else
	#endif // USEGLOBALIME
		if (hasIMM32_) 
		{
			ime = ::ImmGetContext( wnd );
			::ImmSetOpenStatus(ime, (enable ? TRUE : FALSE) );
			::ImmReleaseContext( wnd, ime );
		}
#endif // TARGET_VER
}

void IMEManager::FilterWindows( ATOM* lst, UINT siz )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
			immApp_->FilterClientWindows( lst, siz );
	#endif
}

inline void IMEManager::TranslateMsg( MSG* msg )
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			if( S_OK == immMsg_->OnTranslateMessage( msg ) )
				return;
	#endif
	::TranslateMessage( msg );
}

inline LRESULT IMEManager::DefProc( HWND h, UINT m, WPARAM w, LPARAM l )
{
	#ifdef USEGLOBALIME
		if( immApp_ )
		{
			LRESULT res;
			if( S_OK == immApp_->OnDefWindowProc( h,m,w,l,&res ) )
				return res;
		}
	#endif
	return ::DefWindowProc( h, m, w, l );
}

inline void IMEManager::MsgLoopBegin()
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			immMsg_->Start();
	#endif
}

inline void IMEManager::MsgLoopEnd()
{
	#ifdef USEGLOBALIME
		if( immMsg_ )
			immMsg_->End();
	#endif
}

void IMEManager::SetFont( HWND wnd, const LOGFONT& lf )
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HIMC ime;
	LOGFONT* plf = const_cast<LOGFONT*>(&lf);

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		immApp_->GetContext( wnd, &ime );
		#ifdef _UNICODE
			immApp_->SetCompositionFontW( ime, plf );
		#else
			immApp_->SetCompositionFontA( ime, plf );
		#endif
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if (hasIMM32_) 
	{
		ime = ::ImmGetContext( wnd );
		MyImmSetCompositionFont( ime, plf ); // A/W
		
		::ImmReleaseContext( wnd, ime );
	}
#endif // TARGET_VER
}

void IMEManager::SetPos( HWND wnd, int x, int y )
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HIMC ime;
	COMPOSITIONFORM cf;
	cf.dwStyle = CFS_POINT;
	cf.ptCurrentPos.x  = x;
	cf.ptCurrentPos.y  = y;

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		immApp_->GetContext( wnd, &ime );
		immApp_->SetCompositionWindow( ime, &cf );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif // USEGLOBALIME
	if (hasIMM32_) 
	{
		ime = ::ImmGetContext( wnd );
		::ImmSetCompositionWindow( ime, &cf );
		::ImmReleaseContext( wnd, ime );
	}
#endif // TARGET_VER
}

void IMEManager::GetString( HWND wnd, unicode** str, ulong* len )
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	*str = NULL;
	HIMC ime;

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		long s=0;
		immApp_->GetContext( wnd, &ime );
		immApp_->GetCompositionStringW( ime, GCS_RESULTSTR, 0, &s, NULL );
		*str = new unicode[ (*len=s/2)+1 ];
		immApp_->GetCompositionStringW( ime, GCS_RESULTSTR, s, &s, *str );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if (hasIMM32_) 
	{
		ime = ::ImmGetContext( wnd );
		long s = ::ImmGetCompositionStringW( ime,GCS_RESULTSTR,NULL,0 );

		#if  !defined(_UNICODE) || defined(UNICOWS)
			if( s <= 0 )
			{
				s = ::ImmGetCompositionStringA(ime,GCS_RESULTSTR,NULL,0);
				if( s > 0 )
				{
					char* tmp = new char[s];
					*str = new unicode[*len=s*2];
					::ImmGetCompositionStringA( ime,GCS_RESULTSTR,tmp,s );
					*len = ::MultiByteToWideChar(
						CP_ACP, MB_PRECOMPOSED, tmp, s, *str, *len );
					delete [] tmp;
				}
			}
			else
		#endif
			{
				*str = new unicode[ (*len=s/2)+1 ];
				::ImmGetCompositionStringW( ime, GCS_RESULTSTR, *str, s );
			}

		::ImmReleaseContext( wnd, ime );
	} // endif (hasIMM32_)
#endif //TARGET_VER
}

void IMEManager::SetString( HWND wnd, unicode* str, ulong len )
{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
	HIMC ime;

	#ifdef USEGLOBALIME
	if( immApp_ )
	{
		long s=0;
		immApp_->GetContext( wnd, &ime );
		immApp_->SetCompositionStringW( ime, SCS_SETSTR, str, len*sizeof(unicode), NULL, 0 );
		immApp_->NotifyIME( ime, NI_COMPOSITIONSTR, CPS_CONVERT, 0 );
		immApp_->NotifyIME( ime, NI_OPENCANDIDATE, 0, 0 );
		immApp_->ReleaseContext( wnd, ime );
	}
	else
	#endif //USEGLOBALIME
	if (hasIMM32_) 
	{
		ime = ::ImmGetContext( wnd );
		long s = ::ImmSetCompositionStringW( ime,SCS_SETSTR,str,len*sizeof(unicode),NULL,0 );

		#if  !defined(_UNICODE) || defined(UNICOWS)
			if( s == 0 )
			{
				BOOL defchr = TRUE;
				len = ::WideCharToMultiByte( CP_ACP,MB_PRECOMPOSED,str,-1,NULL,NULL,"?",&defchr );
				char* tmp = new char[len];
				
				::WideCharToMultiByte( CP_ACP,MB_PRECOMPOSED,str,-1,tmp,len,"?",&defchr );
				s = ::ImmSetCompositionStringA(ime,SCS_SETSTR,tmp,len,NULL,0);
				delete [] tmp;
			}
			else
		#endif

		::ImmNotifyIME( ime, NI_COMPOSITIONSTR, CPS_CONVERT, 0); // �ϊ����s
		::ImmNotifyIME( ime, NI_OPENCANDIDATE, 0, 0 ); // �ϊ���⃊�X�g�\��
		::ImmReleaseContext( wnd, ime );
	}// endif (hasIMM32_) 
#endif //TARGET_VER
}



//=========================================================================
// Window�Ɋւ��邠�ꂱ��
//=========================================================================

Window::Window()
	: wnd_      (NULL)
	, isLooping_(false)
{
}

void Window::SetHwnd( HWND wnd )
{
	wnd_ = wnd;
}

void Window::MsgLoop()
{
	// this�����C���E�C���h�E�Ƃ��āA
	// ���C�����b�Z�[�W���[�v����
	isLooping_ = true;
	ime().MsgLoopBegin();

	for( MSG msg; ::GetMessage( &msg, NULL, 0, 0 ); )
		if( !PreTranslateMessage( &msg ) )
		{
			ime().TranslateMsg( &msg );
			::DispatchMessage( &msg );
		}

	ime().MsgLoopEnd();
	isLooping_ = false;
}

void Window::ProcessMsg()
{
	// �������̓O���[�o���֐��B
	// ���������b�Z�[�W����|

	ime().MsgLoopBegin();
	for( MSG msg; ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ); )
	{
		ime().TranslateMsg( &msg );
		::DispatchMessage( &msg );
	}
	ime().MsgLoopEnd();
}



//-------------------------------------------------------------------------

void Window::SetCenter( HWND hwnd, HWND rel )
{
	// �����̃T�C�Y���擾
	RECT rc,pr;
	::GetWindowRect( hwnd, &rc );

	// �e�̈ʒu�A�Ȃ����͑S��ʂ̈ʒu���擾
	if( rel != NULL )
		::GetWindowRect( rel, &pr );
	else
		::SystemParametersInfo( SPI_GETWORKAREA, 0, &pr, 0 );

	// �������v�Z
	::SetWindowPos( hwnd, 0,
		pr.left + ( (pr.right-pr.left)-(rc.right-rc.left) )/2,
		pr.top  + ( (pr.bottom-pr.top)-(rc.bottom-rc.top) )/2,
		0, 0, SWP_NOSIZE|SWP_NOZORDER );
}

void Window::SetFront( HWND hwnd )
{
	// kazubon���� TClock �̃\�[�X���Q�l�ɂ��܂����B���ӁI

	if( app().isNewTypeWindows() )
	{
		DWORD pid;
		HWND  fore= ::GetForegroundWindow();
		DWORD th1 = ::GetWindowThreadProcessId( fore, &pid );
		DWORD th2 = ::GetCurrentThreadId();
		::AttachThreadInput( th2, th1, TRUE );
		::SetForegroundWindow( hwnd );
		::AttachThreadInput( th2, th1, FALSE );
		::BringWindowToTop( hwnd );
	}
	else
	{
		::SetForegroundWindow( hwnd );
	}
}

//=========================================================================

WndImpl::WndImpl( LPCTSTR className, DWORD style, DWORD styleEx )
	: className_( className )
	, style_    ( style )
	, styleEx_  ( styleEx )
	, thunk_    ( static_cast<byte*>(
	                ::VirtualAlloc( NULL, THUNK_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE )) )
{
}

WndImpl::~WndImpl()
{
	// �E�C���h�E��j�����Y��Ă������
	// �c���A���̎��_�Ŋ��� vtable �͔j�����ꂩ�����Ă���̂�
	// ������ on_destroy ���Ă΂��ۏ؂͑S���Ȃ��B�����܂�
	// �ً}�E�o�p(^^; �ƍl���邱�ƁB
	Destroy();
	::VirtualFree( thunk_, 0, MEM_RELEASE );
}

void WndImpl::Destroy()
{
	if( hwnd() != NULL )
		::DestroyWindow( hwnd() );
}

ATOM WndImpl::Register( WNDCLASS* cls )
{
	// WndImpl�h���N���X�Ŏg��WndClass��o�^�B
	// �v���V�[�W����kilib�ސ��̂��̂ɏ����������Ⴂ�܂��B
	cls->hInstance   = app().hinst();
	cls->lpfnWndProc = StartProc;
	return ::RegisterClass( cls );
}

struct ThisAndParam
{
	// ���[�U�w��̃p�����[�^�ȊO�ɂ��n���������m�����X�c
	WndImpl* pThis;
	void*   pParam;
};

bool WndImpl::Create(
	LPCTSTR wndName, HWND parent, int x, int y, int w, int h, void* param )
{
	// ������this�|�C���^��E�э��܂��Ă���
	ThisAndParam z = { this, param };

	LOGGER("WndImpl::Create before CreateWindowEx API call");

	return (NULL != ::CreateWindowEx(
		styleEx_, className_, wndName, style_,
		x, y, w, h, parent, NULL, app().hinst(), &z
	));
}



//-------------------------------------------------------------------------

LRESULT CALLBACK WndImpl::StartProc(
	HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
{
	// WM_CREATE�ȊO�̓X���[�̕��j��
	if( msg != WM_CREATE )
		return ::DefWindowProc( wnd, msg, wp, lp );

	LOGGER("WndImpl::StartProc WM_CREATE kitaaaaa!!");

	// �E�΂��Ēu����this�|�C���^�����o��
	CREATESTRUCT* cs   = reinterpret_cast<CREATESTRUCT*>(lp);
	ThisAndParam* pz   = static_cast<ThisAndParam*>(cs->lpCreateParams);
	WndImpl*   pThis   = pz->pThis;
	cs->lpCreateParams = pz->pParam;

	// �T���N
	pThis->SetUpThunk( wnd );

	// WM_CREATE�p���b�Z�[�W���Ă�
	pThis->on_create( cs );
	return 0;
}

void WndImpl::SetUpThunk( HWND wnd )
{
	SetHwnd( wnd );

	// �����œ��I��x86�̖��ߗ�
	//   | mov dword ptr [esp+4] this
	//   | jmp MainProc
	// ���邢��AMD64�̖��ߗ�
	//   | mov rcx this
	//   | mov rax MainProc
	//   | jmp rax
	// �𐶐����A���b�Z�[�W�v���V�[�W���Ƃ��č����ւ���B
	//
	// ����Ŏ��񂩂�́A�������� hwnd �̂�����
	// this�|�C���^�ɂȂ�����Ԃ�MainProc���Ă΂��
	// �c�ƌ��Ȃ����v���O������������B
	//
	// �Q�l�����FATL�̃\�[�X

#ifdef _M_AMD64
	*reinterpret_cast<dbyte*>   (thunk_+ 0) = 0xb948;
	*reinterpret_cast<WndImpl**>(thunk_+ 2) = this;
	*reinterpret_cast<dbyte*>   (thunk_+10) = 0xb848;
	*reinterpret_cast<void**>   (thunk_+12) = MainProc;
	*reinterpret_cast<dbyte*>   (thunk_+20) = 0xe0ff;
#else
	*reinterpret_cast<qbyte*>   (thunk_+0) = 0x042444C7;
	*reinterpret_cast<WndImpl**>(thunk_+4) = this;
	*reinterpret_cast< byte*>   (thunk_+8) = 0xE9;
	*reinterpret_cast<qbyte*>   (thunk_+9) =
		reinterpret_cast<byte*>((void*)MainProc)-(thunk_+13);
#endif

	::FlushInstructionCache( ::GetCurrentProcess(), thunk_, THUNK_SIZE );
	::SetWindowLongPtr( wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&thunk_[0]) );
}

LRESULT CALLBACK WndImpl::MainProc(
	WndImpl* ptr, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg == WM_COMMAND )
	{
		if( !ptr->on_command( LOWORD(wp), (HWND)lp ) )
			return ::DefWindowProc( ptr->hwnd(), msg, wp, lp );
	}
	else if( msg == WM_DESTROY )
	{
		ptr->on_destroy();
		::SetWindowLongPtr( ptr->hwnd(), GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(StartProc) );
		if( ptr->isMainWnd() )
			::PostQuitMessage( 0 );
		ptr->SetHwnd(NULL);
	}
	else
	{
		return ptr->on_message( msg, wp, lp );
	}

	return 0;
}



//-------------------------------------------------------------------------

void WndImpl::on_create( CREATESTRUCT* cs )
{
	// �������Ȃ�
}

void WndImpl::on_destroy()
{
	// �������Ȃ�
}

bool WndImpl::on_command( UINT, HWND )
{
	// �������Ȃ�
	return false;
}

LRESULT WndImpl::on_message( UINT msg, WPARAM wp, LPARAM lp )
{
	// �������Ȃ�
	return ime().DefProc( hwnd(), msg, wp, lp );
}

bool WndImpl::PreTranslateMessage( MSG* )
{
	// �������Ȃ�
	return false;
}



//=========================================================================

DlgImpl::DlgImpl( UINT id )
	: rsrcID_( id )
{
}

DlgImpl::~DlgImpl()
{
	// �E�C���h�E��j�����Y��Ă������
	if( hwnd() != NULL )
		End( IDCANCEL );
}

void DlgImpl::End( UINT code )
{
	endCode_ = code;

	if( type() == MODAL )
		::EndDialog( hwnd(), code );
	else
		::DestroyWindow( hwnd() );
}

void DlgImpl::GoModal( HWND parent )
{
	type_ = MODAL;
	::DialogBoxParam( app().hinst(), MAKEINTRESOURCE(rsrcID_), parent,
		(DLGPROC)MainProc, reinterpret_cast<LPARAM>(this) );
}

void DlgImpl::GoModeless( HWND parent )
{
	type_ = MODELESS;
	::CreateDialogParam( app().hinst(), MAKEINTRESOURCE(rsrcID_), parent,
		(DLGPROC)MainProc, reinterpret_cast<LPARAM>(this) );
}



//-------------------------------------------------------------------------

BOOL CALLBACK DlgImpl::MainProc(
	HWND dlg, UINT msg, WPARAM wp, LPARAM lp )
{
	if( msg == WM_INITDIALOG )
	{
		::SetWindowLongPtr( dlg, GWLP_USERDATA, lp );

		DlgImpl* ptr = reinterpret_cast<DlgImpl*>(lp);
		ptr->SetHwnd( dlg );
		ptr->on_init();
		return FALSE;
	}

	DlgImpl* ptr =
		reinterpret_cast<DlgImpl*>(::GetWindowLongPtr(dlg,GWLP_USERDATA));

	if( ptr != NULL )
		switch( msg )
		{
		case WM_COMMAND:
			switch( LOWORD(wp) )
			{
			case IDOK:
				if( ptr->on_ok() )
					ptr->End( IDOK );
				return TRUE;

			case IDCANCEL:
				if( ptr->on_cancel() )
					ptr->End( IDCANCEL );
				return TRUE;

			default:
				return ptr->on_command( HIWORD(wp), LOWORD(wp),
					reinterpret_cast<HWND>(lp) ) ? TRUE : FALSE;
			}

		case WM_DESTROY:
			ptr->on_destroy();
			if( ptr->isMainWnd() )
				::PostQuitMessage( 0 );
			ptr->SetHwnd(NULL);
			break;

		default:
			return ptr->on_message( msg, wp, lp ) ? TRUE : FALSE;
		}

	return FALSE;
}



//-------------------------------------------------------------------------

void DlgImpl::on_init()
{
	// �������Ȃ�
}

void DlgImpl::on_destroy()
{
	// �������Ȃ�
}

bool DlgImpl::on_ok()
{
	// �������Ȃ�
	return true;
}

bool DlgImpl::on_cancel()
{
	// �������Ȃ�
	return true;
}

bool DlgImpl::on_command( UINT, UINT, HWND )
{
	// �������Ȃ�
	return false;
}

bool DlgImpl::on_message( UINT, WPARAM, LPARAM )
{
	// �������Ȃ�
	return false;
}

bool DlgImpl::PreTranslateMessage( MSG* msg )
{
	// ���[�h���X�̎��p�B�_�C�A���O���b�Z�[�W�����B
	return (FALSE != ::IsDialogMessage( hwnd(), msg ));
}
