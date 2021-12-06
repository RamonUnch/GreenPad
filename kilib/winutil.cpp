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
		if( read || ::EmptyClipboard() )
			opened_ = true;
		else
			::CloseClipboard();
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

	return Text( NULL, Text::NEW );
}
