#ifndef _KILIB_WINUTIL_H_
#define _KILIB_WINUTIL_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"

#ifndef NO_OLEDNDTAR
bool coolDragDetect( HWND hwnd, LPARAM pt, WORD btup, WORD removebutton );

const IID myIID_IUnknown =    { 0x00000000, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDataObject = { 0x0000010e, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDropSource = { 0x00000121, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IEnumFORMATETC = { 0x00000103, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
#endif // NO_OLEDNDTAR

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

static void DeepCopyFormatEtc(FORMATETC *dest, const FORMATETC *source)
{
	// copy the source FORMATETC into dest
	*dest = *source;

//  We never use the ptd field!
	dest->ptd = NULL; // In case.
//	if(source->ptd)
//	{
//		// allocate memory for the DVTARGETDEVICE if necessary
//		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
//
//		// copy the contents of the source DVTARGETDEVICE into dest->ptd
//		*(dest->ptd) = *(source->ptd);
//	}
}
static HRESULT CreateEnumFormatEtc(UINT , const FORMATETC *, IEnumFORMATETC **);

class CEnumFormatEtc A_FINAL: public IEnumFORMATETC, public Object
{
private:
	LONG        m_lRefCount;   // Reference count for this COM interface
	ULONG       m_nIndex;      // current enumerator index
	ULONG       m_nNumFormats; // number of FORMATETC members
	FORMATETC  *m_pFormatEtc;  // array of FORMATETC objects

public:
	//
	// IUnknown members
	//
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
	{
		if( memEQ(&riid, &myIID_IUnknown, sizeof(riid) )
		||  memEQ(&riid, &myIID_IEnumFORMATETC, sizeof(riid) ) )
		{
			LOGGER( "CEnumFormatEtc::QueryInterface S_OK" );
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		LOGGER( "CEnumFormatEtc::QueryInterface E_NOINTERFACE" );
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef()  override { return ::InterlockedIncrement(&m_lRefCount); }
	ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG cnt = ::InterlockedDecrement(&m_lRefCount);
		if( cnt == 0 ) delete this;
		return cnt;
	}

	//
	// IEnumFormatEtc members
	//
	HRESULT __stdcall  Next(ULONG celt, FORMATETC * pFormatEtc, ULONG * pceltFetched) override
	{
		ULONG copied  = 0;

		// validate arguments
		if(celt == 0 || pFormatEtc == 0)
			return E_INVALIDARG;

		// copy FORMATETC structures into caller's buffer
		while(m_nIndex < m_nNumFormats && copied < celt)
		{
			DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
			copied++;
			m_nIndex++;
		}

		// store result
		if(pceltFetched != 0)
			*pceltFetched = copied;

		// did we copy all that was requested?
		return (copied == celt) ? S_OK : S_FALSE;
	}

	HRESULT __stdcall  Skip(ULONG celt) override
	{
		m_nIndex += celt;
		return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
	}

	HRESULT __stdcall  Reset(void) override
	{
		m_nIndex = 0;
		return S_OK;
	}

	HRESULT __stdcall  Clone(IEnumFORMATETC ** ppEnumFormatEtc) override
	{
		HRESULT hResult;

		// make a duplicate enumerator
		hResult = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

		if(hResult == S_OK)
		{
			// manually set the index state
			((CEnumFormatEtc *) *ppEnumFormatEtc)->m_nIndex = m_nIndex;
		}
		return hResult;
	}

	//
	// Construction / Destruction
	//
	CEnumFormatEtc(const FORMATETC *pFormatEtc, int nNumFormats)
		: m_lRefCount   ( 1 )
		, m_nIndex      ( 0 )
		, m_nNumFormats ( nNumFormats )
	{
		m_pFormatEtc = new FORMATETC[nNumFormats];

		// copy the FORMATETC structures
		for(int i = 0; i < nNumFormats; i++)
		{
			DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
		}
	}

	~CEnumFormatEtc()
	{
		if(m_pFormatEtc)
		{
//			for(ULONG i = 0; i < m_nNumFormats; i++)
//			{
//				if(m_pFormatEtc[i].ptd)
//					CoTaskMemFree(m_pFormatEtc[i].ptd);
//			}
			delete[] m_pFormatEtc;
		}
	}

};
static HRESULT CreateEnumFormatEtc(UINT nNumFormats, const FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc)
{
	if(nNumFormats == 0 || pFormatEtc == 0 || ppEnumFormatEtc == 0)
		return E_INVALIDARG;

	*ppEnumFormatEtc = new CEnumFormatEtc(pFormatEtc, nNumFormats);

	return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

// Class for a minimalist Text/File drag and drop data object
class IDataObjectTxt A_FINAL: public IDataObject, public Object
{
public:
	IDataObjectTxt(const unicode *str, size_t len)
		: refcnt( 1 )
		, str_  ( str )
		, len_  ( len )
		{
			SetFORMATETC(&m_rgfe[DATA_UNICODETEXT],  CF_UNICODETEXT);
			SetFORMATETC(&m_rgfe[DATA_TEXT],         CF_TEXT);
			SetFORMATETC(&m_rgfe[DATA_HDROP],        CF_HDROP);
		}
	~IDataObjectTxt(){}

private:
	void SetFORMATETC(FORMATETC* pfe, UINT cf, TYMED tymed = TYMED_HGLOBAL, LONG lindex = -1,
		DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE* ptd = NULL) const
	{
	    pfe->cfFormat = (CLIPFORMAT)cf;
	    pfe->tymed = tymed;
	    pfe->lindex = lindex;
	    pfe->dwAspect = dwAspect;
	    pfe->ptd = ptd;
	}

	size_t convCRLFtoNULLS(unicode *d, const unicode *s, size_t l);

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
	{
		if( memEQ(&riid, &myIID_IUnknown, sizeof(riid) )
		||  memEQ(&riid, &myIID_IDataObject, sizeof(riid) ) )
		{
			LOGGER( "IDataObjectTxt::QueryInterface S_OK" );
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		LOGGER( "IDataObjectTxt::QueryInterface E_NOINTERFACE" );
		return E_NOINTERFACE;
	}
	ULONG STDMETHODCALLTYPE AddRef()  override { return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() override
	{
		ULONG cnt = ::InterlockedDecrement(&refcnt);
		if( cnt == 0 ) delete this;
		return cnt;
	}

	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *fmt, STGMEDIUM *pm) override;
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppefe) override
	{
		if( dwDirection == DATADIR_GET )
		{
			LOGGER( "IDataObjectTxt::EnumFormatEtc(DATADIR_GET)" );
			return CreateEnumFormatEtc(countof(m_rgfe), m_rgfe, ppefe);
		}
		*ppefe = NULL;
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *fmt, STGMEDIUM *pm) override;
	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *fmt) override;
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *fmt, FORMATETC *fout) override;

	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pFormatetc, STGMEDIUM *pmedium, BOOL fRelease) override
		{ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pFormatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) override
		{ return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override
		{ return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA ** ppenumAdvise) override
		{ return OLE_E_ADVISENOTSUPPORTED; }

private:
	LONG refcnt;
	const unicode *str_;
	const size_t len_;
	enum {
		DATA_UNICODETEXT,
		DATA_TEXT,
		DATA_HDROP,
		DATA_NUM,
		DATA_INVALID = -1,
	};
	FORMATETC m_rgfe[DATA_NUM];
};

//-------------------------------------------------------------------------
// Implementation of a simple IDropSource for Text/File Drag and drop.
class OleDnDSourceTxt A_FINAL: public IDropSource
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
		#undef FUNK_TYPE

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

			LOGGERF( TEXT("OleDnDSourceTxt DoDragDrop end, ret=%x, effect=%d"), (DWORD)ret, (int)effect );
		}
	}

	DWORD getEffect() const
		{ return dwEffect_; }

	~OleDnDSourceTxt(){}

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
	{
		if( memEQ(&riid, &myIID_IUnknown, sizeof(riid) )
		||  memEQ(&riid, &myIID_IDropSource, sizeof(riid) ) )
		{
			LOGGER( "OleDnDSourceTxt::QueryInterface S_OK" );
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		LOGGER( "OleDnDSourceTxt::QueryInterface E_NOINTERFACE" );
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override  { return ::InterlockedIncrement(&refcnt); }
	ULONG STDMETHODCALLTYPE Release() override { return ::InterlockedDecrement(&refcnt); }

	HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override
		{
			// LOGGER( "OleDnDSourceTxt::QueryContinueDrag" );
			if( fEscapePressed )
				return DRAGDROP_S_CANCEL;
			if (!(grfKeyState & (MK_LBUTTON | MK_RBUTTON)))
				return DRAGDROP_S_DROP; // Do the drop!
			return S_OK;
		}

	HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) override
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

class Mutex
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

inline Mutex::Mutex( const TCHAR* name)
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
