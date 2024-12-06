#include "stdafx.h"
#include "app.h"
#include "winutil.h"
#include "kstring.h"
using namespace ki;

#ifndef NO_OLEDNDTAR
// Alternative version of DragDetect that beter fits my needs:
// 1) it does not removes the button up from message queue.
// 2) Any button can be used to do the drag and a drag can occur
//    in the non-client area.
// 3) Should run on All windows versions, even Win32s beta/Chicago.
bool coolDragDetect( HWND hwnd, LPARAM pt, WORD btup, WORD removebutton )
{
	int cxd = GetSystemMetrics(SM_CXDRAG);
	int cyd = GetSystemMetrics(SM_CYDRAG);
	short x = LOWORD(pt);
	short y = HIWORD(pt);

	MSG msg;
	BOOL mm;
	do
	{
		mm = PeekMessage(&msg, hwnd, btup, btup, removebutton );
		if( mm )
			return false;

		mm = PeekMessage(&msg, hwnd, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE);
		if( mm && msg.message == VK_ESCAPE )
			return false;

		mm = PeekMessage(&msg, hwnd, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_REMOVE);
		if( !mm )
			mm = PeekMessage(&msg, hwnd, WM_NCMOUSEMOVE, WM_NCMOUSEMOVE, PM_REMOVE);
		if( mm && (Abs(x-LOWORD(msg.lParam)) > cxd || Abs(y-HIWORD(msg.lParam)) > cyd) )
		{
			return true;
		}
	}
	while( WaitMessage() );

	return false;
}
#endif // NO_OLEDNDTAR

//=========================================================================

Clipboard::Clipboard( HWND owner, bool read )
	: opened_( false )
{
	if( ::OpenClipboard(owner) )
	{
		if( read || ::EmptyClipboard() )
			opened_ = true;
		else
			::CloseClipboard();
	}
}

Clipboard::~Clipboard()
{
	if( opened_ )
		::CloseClipboard();
}

Clipboard::Text Clipboard::GetUnicodeText() const
{
	// Always try to get the best available clipboard data.
	if( IsClipboardFormatAvailable(CF_UNICODETEXT) )
	{
		// NTÇ»ÇÁíºê⁄UnicodeÇ≈Ç∆ÇÍÇÈ
		// Also on Win9x we can use CF_UNICODETEXT with UNICOWS
		HANDLE h = GetData( CF_UNICODETEXT );
		if( h != NULL )
		{
			unicode* ustr = static_cast<unicode*>( ::GlobalLock( h ) );
			return Text( ustr, Text::GALLOC );
		}
	}

	#ifndef _UNICODE
	// ANSI text, not needed in Unicode/Unicows builds
	else if( IsClipboardFormatAvailable(CF_TEXT) )
	{
		// Fallback to ANSI clipboard data.
		// 9xÇ»ÇÁïœä∑Ç™ïKóv
		HANDLE h = GetData( CF_TEXT );
		if( h != NULL )
		{
			char* cstr = static_cast<char*>( ::GlobalLock( h ) );
			if( cstr )
			{
				int Lu = my_lstrlenA( cstr ) * 3;
				unicode* ustr = (unicode *)malloc( sizeof(unicode) * Lu );
				if( ustr )
				{
					::MultiByteToWideChar( CP_ACP, 0, cstr, -1, ustr, Lu );
					::GlobalUnlock( h );
					return Text( ustr, Text::MALLOC );
				}
			}
		}
	}
	#endif

	else if( IsClipboardFormatAvailable(CF_HDROP) )
	{
		// No "normal" text in the clipboard...
		// Maybe paste a list of files?
		HGLOBAL hg = GetData( CF_HDROP );
		if( hg != NULL )
		{
			HDROP h = (HDROP)::GlobalLock( hg );
			if( h )
			{
				UINT nf = myDragQueryFile(h, 0xFFFFFFFF, NULL, 0);
				size_t totstrlen=0;
				UINT *lenmap = (UINT *)TS.alloc( sizeof(UINT) * nf );
				if (!lenmap) return Text( NULL, Text::MALLOC );
				for( uint i=0; i < nf; i++ )
				{	// On Windows NT3.1 DragQueryFile() does not return
					// The required buffer length hence the Min()...
					lenmap[i] = Min((UINT)MAX_PATH, myDragQueryFile(h, i, NULL, 0));
					totstrlen += lenmap[i];
				}
				unicode* ustr = (unicode *)malloc( sizeof(unicode) * (totstrlen+2*nf+1) );
				if(!ustr) return Text( NULL, Text::MALLOC );
				//mem00( ustr, (totstrlen+2*nf+1) * sizeof(unicode) );
				unicode* ptr=ustr; *ptr = L'\0';
				for( UINT i=0; i < nf; i++ )
				{
					// Return the length without NULL and requires length with NULL
					#ifdef UNICODE
					ptr += myDragQueryFileW(h, i, ptr, Min(lenmap[i]+1, (UINT)MAX_PATH));
					#else
					{
						char buf[MAX_PATH]; // MAX_PATH is the maximum in ANSI mode
						UINT len = DragQueryFileA(h, i, buf, MAX_PATH);
						::MultiByteToWideChar( CP_ACP, 0, buf, len, ptr, len );
						ptr+=len;
					}
					#endif

					*ptr++ = L'\r';
					*ptr++ = L'\n';
				}
				*ptr = L'\0';
				GlobalUnlock( hg );
				TS.freelast( lenmap, sizeof(UINT) * nf );
				return Text( ustr, Text::MALLOC );
			}
		}
	}
	return Text( NULL, Text::MALLOC );
}


//=========================================================================
// IDataObjectTxt: Class for a minimalist Text drag and drop data object
//=========================================================================
#ifndef NO_OLEDNDSRC
size_t IDataObjectTxt::convCRLFtoNULLS(unicode *d, const unicode *s, size_t l)
{
	const unicode *od = d;
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

HRESULT STDMETHODCALLTYPE IDataObjectTxt::GetData(FORMATETC *fmt, STGMEDIUM *pm)
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

HRESULT STDMETHODCALLTYPE IDataObjectTxt::GetDataHere(FORMATETC *fmt, STGMEDIUM *pm)
{
	// Data is already allocated by caller!
	VOID *data;
	if( S_OK == QueryGetData(fmt) && pm->hGlobal != NULL && (data = GlobalLock( pm->hGlobal )) != NULL )
	{
		// Check actual size of allocated mem in case.
		size_t gmemsz = GlobalSize( pm->hGlobal );
		size_t remaining_bytes = 0;

		if( fmt->cfFormat == CF_UNICODETEXT )
		{
			size_t len = Min( len_*sizeof(unicode), gmemsz-sizeof(unicode) );
			memmove( data, str_, len );
			remaining_bytes = gmemsz - len;
		}
		else if( fmt->cfFormat == CF_TEXT )
		{	// Convert unicode string to ANSI.
			size_t destlen = Min( len_*sizeof(unicode), gmemsz-sizeof(char) );
			int len = ::WideCharToMultiByte(CP_ACP, 0, str_, len_, (char*)data, destlen, NULL, NULL);
			remaining_bytes = gmemsz - len;
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

			// Convert multi line in multi file paths
			unicode *flst = (unicode *)TS.alloc( sizeof(unicode) * len_ );
			if( !flst ) return E_OUTOFMEMORY;
			size_t flen = convCRLFtoNULLS(flst, str_, len_);

			// Destination length in BYTES!
			size_t len = Min(flen*sizeof(unicode), gmemsz-sizeof(DROPFILES)-2*sizeof(unicode));
			if( !df->fWide )
			{	// Convert to ANSI and copy to dest!
				len = ::WideCharToMultiByte( CP_ACP, 0, flst, flen, dest, len, NULL, NULL );
			}
			else
			{	// Directly copy unicode data
				memmove( dest, flst, len );
			}
			TS.freelast( flst, sizeof(unicode) * len_ );
			remaining_bytes = gmemsz - len - df->pFiles ;
		}
		// Clear remaining bytes
		if( remaining_bytes )
			mem00((BYTE*)data+gmemsz-remaining_bytes, remaining_bytes);

		GlobalUnlock( pm->hGlobal );
		pm->pUnkForRelease = NULL; // Caller must free!
		pm->tymed = TYMED_HGLOBAL;
		return S_OK;
	}
	return DV_E_FORMATETC;
}

HRESULT STDMETHODCALLTYPE IDataObjectTxt::QueryGetData(FORMATETC *fmt)
{
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

HRESULT STDMETHODCALLTYPE IDataObjectTxt::GetCanonicalFormatEtc(FORMATETC *fmt, FORMATETC *fout)
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
#endif // NO_OLEDNDSRC
