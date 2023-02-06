#ifndef _KILIB_WINUTIL_H_
#define _KILIB_WINUTIL_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"

bool coolDragDetect( HWND hwnd, LPARAM pt, WORD btup, WORD removebutton );

const IID myIID_IUnknown =    { 0x00000000, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDataObject = { 0x0000010e, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };
const IID myIID_IDropSource = { 0x00000121, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	�N���b�v�{�[�h�Ǘ�
//
//	OpenClipboard �� CloseClipboard �ӂ�̌Ăяo����K���Ɏ��������܂��B
//@}
//=========================================================================

class Clipboard : public Object
{
public:

	//@{ �J�� //@}
	Clipboard( HWND owner, bool read=true );

	//@{ ���� //@}
	~Clipboard();

	//@{ �f�[�^�ǂݍ��� //@}
	HANDLE GetData( UINT uFormat ) const;

	//@{ �w��t�H�[�}�b�g�̃f�[�^���N���b�v�{�[�h��ɂ��邩�H //@}
	bool IsAvail( UINT uFormat ) const;

	//@{ �w��t�H�[�}�b�g�̃f�[�^���N���b�v�{�[�h��ɂ��邩�H(����) //@}
	bool IsAvail( UINT uFormats[], int num ) const;

	//@{ �e�L�X�g���ێ��N���X //@}
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

	//@{ �e�L�X�g�ǂݍ��� //@}
	Text GetUnicodeText() const;

	//@{ �f�[�^�������� //@}
	bool SetData( UINT uFormat, HANDLE hData );

	//@{ �Ǝ��t�H�[�}�b�g�̓o�^ //@}
	static UINT RegisterFormat( const TCHAR* name );

public:

	//@{ ����ɊJ����Ă��邩�`�F�b�N //@}
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
// Class for a minimalist Text/File drag and drop data object
class IDataObjectTxt : public IDataObject, public Object
{
public:
	IDataObjectTxt(const unicode *str, size_t len)
		: refcnt( 1 )
		, str_  ( str )
		, len_  ( len )
		{
			SetFORMATETC(&m_rgfe[DATA_TEXT],         CF_TEXT);
			SetFORMATETC(&m_rgfe[DATA_UNICODETEXT],  CF_UNICODETEXT);
			SetFORMATETC(&m_rgfe[DATA_HDROP],        CF_HDROP);
		}

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

	size_t convCRLFtoNULLS(unicode *d, const unicode *s, size_t l)
	{
		unicode *od = d;
		while( l-- )
		{
			if( *s == L'\r' || *s == L'\n' )
			{	// Replace any sequence of CR or LF by a single NULL.
				*d++ = L'\0';
				s++;
				while( *s == L'\r' || *s == L'\n' )
					s++; // skip LF
			}
			else
			{	// Copy
				*d++ = *s++;
			}
		}
		return d - od;
	}

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

	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *fmt, STGMEDIUM *pm)
	{
		if( S_OK == QueryGetData(fmt) )
		{
			mem00( pm, sizeof(*pm) ); // In case...
			if( fmt->cfFormat == CF_HDROP )
				pm->hGlobal = GlobalAlloc( GMEM_MOVEABLE, sizeof(DROPFILES) + Max((size_t)MAX_PATH+2, (size_t)(len_+4)*sizeof(unicode) ) );
			else
				pm->hGlobal = GlobalAlloc( GMEM_MOVEABLE, (len_+1)*sizeof(unicode) );
			if( !pm->hGlobal )
				return E_OUTOFMEMORY;
			// Copy the data into pm
			return GetDataHere(fmt, pm);
		}
		return DV_E_FORMATETC;
	}
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppefe)
	{
		if( dwDirection == DATADIR_GET )
		{
			// SHCreateStdEnumFmtEtc first appear in win2000, or you need to implement whole IEnumFORMATETC,
			// for example https://github.com/mirror/sevenzip/blob/master/CPP/7zip/UI/FileManager/EnumFormatEtc.cpp
			#define FUNK_TYPE ( HRESULT (WINAPI *)(UINT cfmt, const FORMATETC *afmt, IEnumFORMATETC **ppefe) )
			static HRESULT (WINAPI *dyn_SHCreateStdEnumFmtEtc)(UINT cfmt, const FORMATETC *afmt, IEnumFORMATETC **ppefe) = FUNK_TYPE(-1);
			if( dyn_SHCreateStdEnumFmtEtc == FUNK_TYPE(-1) )
				dyn_SHCreateStdEnumFmtEtc = FUNK_TYPE GetProcAddress( GetModuleHandle(TEXT("SHELL32.DLL")), "SHCreateStdEnumFmtEtc" );
			if( dyn_SHCreateStdEnumFmtEtc )
				return dyn_SHCreateStdEnumFmtEtc(countof(m_rgfe), m_rgfe, ppefe);
			#undef FUNK_TYPE
		}
		*ppefe = NULL;
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *fmt, STGMEDIUM *pm)
	{
		// Data is already allocated by caller!
		VOID *data;
		if( S_OK == QueryGetData(fmt) && pm->hGlobal != NULL && (data = GlobalLock(pm->hGlobal)) != NULL )
		{
			// Check actual size of allocated mem in case.
			size_t gmemsz = GlobalSize( pm->hGlobal );

			if( fmt->cfFormat == CF_UNICODETEXT )
			{
				size_t len = Min( len_*sizeof(unicode), gmemsz );
				memmove( data, str_, len );
				((unicode*)data)[len/sizeof(unicode)] = L'\0'; // NULL Terminate
			}
			else if( fmt->cfFormat == CF_TEXT )
			{	// Convert unicode string to ANSI.
				size_t destlen = Min( len_*sizeof(unicode), gmemsz );
				char *dest = (char*)data;
				int len = ::WideCharToMultiByte(CP_ACP, 0, str_, len_, dest, destlen, NULL, NULL);
				dest[len/sizeof(char)] = '\0'; // NULL Terminate
			}
			else if( fmt->cfFormat == CF_HDROP )
			{
				DROPFILES *df = (DROPFILES *)data;
				df->pFiles = sizeof(DROPFILES); // File path starts just after the end of struct.
				df->fWide = app().isNT(); // Use unicode on NT!
				df->fNC = 1;
				df->pt.x = df->pt.y = 0;
				// The string starts just at the end of the structure
				char *dest = (char *)( ((BYTE*)df) + df->pFiles );
				// convert multi line in multi file paths
				unicode *flst = new unicode[len_];
				size_t flen = convCRLFtoNULLS(flst, str_, len_);
				// Destination length in BYTES!
				size_t len = Min(flen*sizeof(unicode), gmemsz-sizeof(DROPFILES)-2*sizeof(unicode));
				if( !df->fWide )
				{	// Convert to ANSI and copy to dest!
					::WideCharToMultiByte( CP_ACP, 0, flst, flen, dest, len, NULL, NULL);
					dest[len+1] = '\0'; // Double NULL Terminate
				}
				else
				{
					unicode *uni = (unicode *)dest;
					memmove(dest, flst, len);
					len = len/sizeof(unicode);
					uni[len  ] = L'\0';
					uni[len+1] = L'\0'; // Double NULL Terminate
				}
				delete flst;
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
		if( fmt->cfFormat == CF_UNICODETEXT
		||  fmt->cfFormat == CF_TEXT
		||  fmt->cfFormat == CF_HDROP
		)
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
		if( fmt )
		{
			if( fmt->cfFormat == CF_UNICODETEXT
			||  fmt->cfFormat == CF_TEXT
			||  fmt->cfFormat == CF_HDROP
			){
				if( fmt->dwAspect == DVASPECT_CONTENT
				&&  fmt->lindex == -1 )
				{
					if( fout )
					{
						*fout = *fmt;
						fout->ptd = NULL;
					}
					return DATA_S_SAMEFORMATETC;
				}
				if( fout ) {
					*fout = *fmt;
					fout->ptd = NULL;
					fout->dwAspect = DVASPECT_CONTENT;
					fout->lindex = -1;
					return S_OK;
				}
			}
		}
		static const FORMATETC canon = { CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		if( fout ) *fout = canon;
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
				return DRAGDROP_S_DROP; // Do the drop!
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
//	�r������
//
//	���O�t��Mutex�������܂�
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
