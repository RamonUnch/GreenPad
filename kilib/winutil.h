#ifndef _KILIB_WINUTIL_H_
#define _KILIB_WINUTIL_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"

bool coolDragDetect( HWND hwnd, LPARAM pt, WORD btup, WORD removebutton );

const IID myIID_IUnknown = { 0x00000000, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDataObject = { 0x0000010e, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDropSource = { 0x00000121, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	クリップボード管理
//
//	OpenClipboard や CloseClipboard 辺りの呼び出しを適当に自動化します。
//@}
//=========================================================================

class Clipboard : public Object
{
public:

	//@{ 開く //@}
	Clipboard( HWND owner, bool read=true );

	//@{ 閉じる //@}
	~Clipboard();

	//@{ データ読み込み //@}
	HANDLE GetData( UINT uFormat ) const;

	//@{ 指定フォーマットのデータがクリップボード上にあるか？ //@}
	bool IsAvail( UINT uFormat ) const;

	//@{ 指定フォーマットのデータがクリップボード上にあるか？(複数) //@}
	bool IsAvail( UINT uFormats[], int num ) const;

	//@{ テキスト情報保持クラス //@}
	class Text {
		friend class Clipboard;

		mutable unicode*        str_;
		enum Tp { NEW, GALLOC } mem_;

		Text( unicode* s, Tp m ) : str_(s), mem_(m) {}
		void operator=( const Text& );

	public:
		Text( const Text& t )
			: str_(t.str_), mem_(t.mem_) { t.str_=NULL; }
		~Text()
			{
				if( str_ != NULL )
				{
					if( mem_==NEW ) delete [] str_;
					else      GlobalUnlock( str_ );
				}
			}
		const unicode* data() const { return str_; }
	};

	//@{ テキスト読み込み //@}
	Text GetUnicodeText() const;

	//@{ データ書き込み //@}
	bool SetData( UINT uFormat, HANDLE hData );

	//@{ 独自フォーマットの登録 //@}
	static UINT RegisterFormat( const TCHAR* name );

public:

	//@{ 正常に開かれているかチェック //@}
	bool isOpened() const;

private:

	bool opened_;

private:

	NOCOPY(Clipboard);
};



//-------------------------------------------------------------------------

inline bool Clipboard::isOpened() const
	{ return opened_; }

inline HANDLE Clipboard::GetData( UINT uFormat ) const
	{ return ::GetClipboardData( uFormat ); }

inline bool Clipboard::SetData( UINT uFormat, HANDLE hData )
	{ return NULL != ::SetClipboardData( uFormat, hData ); }

inline bool Clipboard::IsAvail( UINT uFormat ) const
	{ return false!=::IsClipboardFormatAvailable(uFormat); }

inline bool Clipboard::IsAvail( UINT uFormats[], int num ) const
	{ return -1!=::GetPriorityClipboardFormat(uFormats,num); }

inline UINT Clipboard::RegisterFormat( const TCHAR* name )
	{ return ::RegisterClipboardFormat(name); }


//=========================================================================
//@{
//	IDataObjectTxt: Class for a minimalist Text drag and drop data object
//
//	OleDnDSourceTxt(str, len) to do the drag and drop
//@}
//=========================================================================

#ifndef NO_OLEDNDSRC
// Class for a minimalist Text drag and drop data object
class IDataObjectTxt : public IDataObject, public Object
{
public:
	IDataObjectTxt(const unicode *str, size_t len)
		: refcnt( 1 )
		, str_  ( str )
		, len_  ( len )
		{ }
private:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if( memEQ(&riid, &myIID_IUnknown, sizeof(riid) )
		||  memEQ(&riid, &myIID_IDataObject, sizeof(riid) ) )
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef()  { return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() { return ::InterlockedDecrement(&refcnt); }

	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *fmt,STGMEDIUM *pm)
	{
		if( S_OK == QueryGetData(fmt) )
		{
			mem00( pm, sizeof(*pm) ); // In case...
			pm->hGlobal = GlobalAlloc( GMEM_MOVEABLE, (len_+1)*sizeof(unicode) );
			if( !pm->hGlobal )
				return E_OUTOFMEMORY;
			// Copy the data into pm
			return GetDataHere(fmt, pm);
		}
		return DV_E_FORMATETC;
	}
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
		{ return E_NOTIMPL; }

	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *fmt, STGMEDIUM *pm)
	{
		// Data is already allocated by caller!
		VOID *data;
		if( S_OK == QueryGetData(fmt) && pm->hGlobal != NULL && (data = GlobalLock(pm->hGlobal)) != NULL )
		{
			if( fmt->cfFormat == CF_UNICODETEXT )
			{
				size_t len = Min(len_*sizeof(unicode), (size_t)GlobalSize(pm->hGlobal));
				memmove( data, str_, len );
				((unicode*)data)[len/sizeof(unicode)] = L'\0'; // NULL Terminate
			}
			else // if( fmt->cfFormat == CF_TEXT)
			{	// Convert unicode string to ANSI.
				int len = ::WideCharToMultiByte(CP_ACP, 0, str_, len_, NULL, 0, NULL, NULL);
				char* ansi = new char[len];
				len = ::WideCharToMultiByte(CP_ACP, 0, str_, len_, ansi, len, NULL, NULL);
				len = Min(len*sizeof(char), (size_t)GlobalSize(pm->hGlobal));
				memmove( data, (void*)ansi, len );
				((char*)data)[len/sizeof(char)] = '\0'; // NULL Terminate
				delete [] ansi;
			}
			GlobalUnlock(pm->hGlobal);
			pm->pUnkForRelease = NULL; // Caller must free!
			pm->tymed = TYMED_HGLOBAL;
			return S_OK;
		}
		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *fmt)
	{
		// { CF_(UNI)TEXT, NULL, DVASPECT_CONTENT, -1, |TYMED_HGLOBAL } Only!
		if( fmt->cfFormat == CF_UNICODETEXT || fmt->cfFormat == CF_TEXT )
			if( fmt->ptd == NULL
			&&  fmt->dwAspect == DVASPECT_CONTENT
		//	&&  fmt->lindex == -1 // Skip this one?
			&&  fmt->tymed & TYMED_HGLOBAL )
				return S_OK;

		// Invalid or unsupported format
		return DV_E_FORMATETC;
	}

	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *fmt, FORMATETC *fout)
	{
		if(fmt)
		{
			if(fmt->cfFormat == CF_UNICODETEXT || fmt->cfFormat == CF_TEXT )
			{
				if( fmt->dwAspect == DVASPECT_CONTENT
				&&  fmt->lindex == -1 )
				{
					if (fout)
					{
						*fout = *fmt;
						fout->ptd = NULL;
					}
					return DATA_S_SAMEFORMATETC;
				}
				if (fout) {
					*fout = *fmt;
					fout->ptd = NULL;
					fout->dwAspect = DVASPECT_CONTENT;
					fout->lindex = -1;
					return S_OK;
				}
			}
		}
		const FORMATETC canon = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		*fout = canon;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pFormatetc, STGMEDIUM *pmedium, BOOL fRelease)
		{ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pFormatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
		{ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection)
		{ return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA ** ppenumAdvise)
		{ return OLE_E_ADVISENOTSUPPORTED; }

private:
	LONG refcnt;
	const unicode *str_;
	const size_t len_;
};
class OleDnDSourceTxt : public IDropSource
{
public:
	OleDnDSourceTxt(const unicode *str, size_t len, DWORD adEffect = DROPEFFECT_MOVE|DROPEFFECT_COPY)
	: refcnt    ( 1 )
	, dwEffect_ ( 0 )
	{
		// Dynamically load DoDragDrop() from OLE32.DLL
		#define FUNK_TYPE ( HRESULT (WINAPI *)(IDataObject *, IDropSource *, DWORD, DWORD *) )
		ki::app().InitModule( App::OLE );
		static HRESULT (WINAPI *dyn_DoDragDrop)(IDataObject *, IDropSource *, DWORD, DWORD *)=FUNK_TYPE(-1);
		if( dyn_DoDragDrop == FUNK_TYPE(-1) )
		{
			dyn_DoDragDrop = NULL;
			if( app().hOle32() )
				dyn_DoDragDrop = FUNK_TYPE GetProcAddress( app().hOle32(), "DoDragDrop" );
		}

		if( dyn_DoDragDrop )
		{
			// Create IData object from the string
			IDataObjectTxt data(str, len);
			LOGGER( "OleDnDSourceTxt IDataObjectTxt created" );
			// Do the drag and drop and set dwEffect_ accordingly.
			DWORD effect=0;
			HRESULT ret = dyn_DoDragDrop( &data, this, adEffect, &effect );
			// Only set the resulting effect if the drop was actually performed
			if( ret == DRAGDROP_S_DROP )
				dwEffect_ = effect;
			LOGGER( "OleDnDSourceTxt DoDragDrop end" );
		}
	}

	DWORD getEffect() const
		{ return dwEffect_; }

	~OleDnDSourceTxt(){}
private:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if( memEQ(&riid, &myIID_IUnknown, sizeof(riid) )
		||  memEQ(&riid, &myIID_IDropSource, sizeof(riid) ) )
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef()  { return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() { return ::InterlockedDecrement(&refcnt); }

	HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
		{
			if( fEscapePressed )
				return DRAGDROP_S_CANCEL;
			if (!(grfKeyState & (MK_LBUTTON | MK_RBUTTON)))
				return DRAGDROP_S_DROP;			// Do the drop!
			return S_OK;
		}

	HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect)
		{ return DRAGDROP_S_USEDEFAULTCURSORS; }

private:
	LONG refcnt;
	DWORD dwEffect_;
};
#endif // NO_OLEDNDSRC

//=========================================================================
//@{
//	排他制御
//
//	名前付きMutexを扱います
//@}
//=========================================================================

class Mutex : public Object
{
public:
	Mutex( const TCHAR* name );
	~Mutex();
	bool isLocked() const;

private:
	const HANDLE mtx_;
	bool locked_;

private:
	NOCOPY(Mutex);
};



//-------------------------------------------------------------------------

inline Mutex::Mutex( const TCHAR* name )
	: mtx_( ::CreateMutex( NULL, TRUE, name ) )
	, locked_ (false)
	{
		DWORD err = ::GetLastError();
		if( mtx_ )
		{
			// Wait for Mutex ownership, in case it was already created.
			if( err == ERROR_ALREADY_EXISTS )
				// Wait 10 second for ownership of fail.
				locked_ = WAIT_OBJECT_0 == ::WaitForSingleObject(mtx_, 10000);
			else
				locked_ = true; // The mutex is ours.
		}
		else if (err == ERROR_CALL_NOT_IMPLEMENTED)
		{	// In case Mutex are not implemented
			// This is required for Win32s 1.1.
			// On Win32s 1.15a CreateMutexA is smartly implemented as
			// mov eax, 1; retn Ch;
			locked_ = true;
		}
	}

inline bool Mutex::isLocked() const
	{ return locked_; }

inline Mutex::~Mutex()
	{
		if( mtx_ != NULL )
		{
			if( locked_ ) ::ReleaseMutex( mtx_ );
			::CloseHandle( mtx_ );
		}
	}



//=========================================================================

}      // namespace ki
#endif // _KILIB_WINUTIL_H_
