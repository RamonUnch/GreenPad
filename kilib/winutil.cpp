#include "stdafx.h"
#include "app.h"
#include "winutil.h"
#include "string.h"
using namespace ki;



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
	if( app().isNT() )
	{
		// NTÇ»ÇÁíºê⁄UnicodeÇ≈Ç∆ÇÍÇÈ
		HANDLE h = GetData( CF_UNICODETEXT );
		if( h != NULL )
		{
			unicode* ustr = static_cast<unicode*>( ::GlobalLock( h ) );
			return Text( ustr, Text::GALLOC );
		}
	}
	else
	{
		// 9xÇ»ÇÁïœä∑Ç™ïKóv
		HANDLE h = GetData( CF_TEXT );
		if( h != NULL )
		{
			char* cstr = static_cast<char*>( ::GlobalLock( h ) );
			int Lu = my_lstrlenA( cstr ) * 3;
			unicode* ustr = new unicode[Lu];
			::MultiByteToWideChar( CP_ACP, 0, cstr, -1, ustr, Lu );
			::GlobalUnlock( h );
			return Text( ustr, Text::NEW );
		}
	}
	// No "normal" text in the clipboard...
	// Maybe paste a list of files?
	HANDLE h = GetData( CF_HDROP );
	if( h != NULL )
	{
		UINT nf = DragQueryFile((HDROP)h, 0xFFFFFFFF, NULL, 0);
		size_t totstrlen=0;
		UINT *lenmap = new UINT[nf];
		for (uint i=0; i < nf; i++)
		{	// On Windows NT3.1 DragQueryFile() does not return
			// The required buffer length hence the Min()...
			lenmap[i] = Min((UINT)MAX_PATH, DragQueryFile((HDROP)h, i, NULL, 0));
			totstrlen += lenmap[i];
		}
		unicode* ustr = new unicode[totstrlen+2*nf+1];
		unicode* ptr=ustr; *ptr = L'\0';
		for (UINT i=0; i < nf; i++)
		{
		    // Return the length without NULL and requires length with NULL
		    #if UNICODE
			ptr += DragQueryFileW((HDROP)h, i, ptr, Min(lenmap[i]+1, (UINT)MAX_PATH));
			#else
			{
				char buf[MAX_PATH]; // MAX_PATH is the maximum in ANSI mode
				UINT len = DragQueryFileA((HDROP)h, i, buf, MAX_PATH);
				::MultiByteToWideChar( CP_ACP, 0, buf, -1, ptr, len );
				ptr+=len;
			}
			#endif

			*ptr++ = L'\r';
			*ptr++ = L'\n';
		}
		*ptr++ = L'\0';
		delete [] lenmap;

		return Text( ustr, Text::NEW );
	}

	return Text( NULL, Text::NEW );
}
