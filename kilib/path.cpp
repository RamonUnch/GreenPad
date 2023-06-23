#include "stdafx.h"
#include "path.h"
#include "app.h"
using namespace ki;

#if 0
typedef void (WINAPI * funkk)(LPVOID mem);
static void MyCoTaskMemFree(void *mem)
{
	static funkk func = (funkk) (-1);
	if (func == (funkk)(-1))
		func = (funkk)GetProcAddress(GetModuleHandle(TEXT("OLE32.DLL")),"CoTaskMemFree");
	if (func) func(mem);
}
typedef HRESULT (WINAPI * funkk2)(HWND h, int i, LPITEMIDLIST *idl);
HRESULT MySHGetSpecialFolderLocation(HWND h, int i, LPITEMIDLIST *idl)
{
	static funkk2 func = (funkk2) (-1);
	if (func == (funkk2)(-1))
		func = (funkk2)GetProcAddress(GetModuleHandle(TEXT("SHELL32.DLL")),"SHGetSpecialFolderLocation");
	if (func)
		return func(h, i, idl);
	return 666;
}
#endif

//=========================================================================

Path& Path::BeSpecialPath( int nPATH, bool bs )
{
	TCHAR* buf = AllocMem( MAX_PATH + 1 );

	switch( nPATH )
	{
//	case Win:     ::GetWindowsDirectory( buf, MAX_PATH );     break;
//	case Sys:     ::GetSystemDirectory( buf, MAX_PATH );      break;
//	case Tmp:     ::GetTempPath( MAX_PATH, buf );             break;
	case Cur:     ::GetCurrentDirectory( MAX_PATH, buf );     break;
	case Exe:
	case ExeName: GetExeName( buf ); break;
	default:
		*buf = TEXT('\0');
// This part seems to never be used for now...
//#if (defined(UNICODE) && defined(UNICOWS)) || !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
#if 0
		if(app().isNewShell())
		{
			// MessageBoxA(NULL, "SHGetSpecialFolderLocation","",MB_OK);
			app().InitModule(App::OLEDLL); // Load the dll to be sure...
			LPITEMIDLIST il;
			if( NOERROR==MySHGetSpecialFolderLocation( NULL, nPATH, &il ) )
			{
				::SHGetPathFromIDList( il, buf ); // Dynamic in UNICOWS mode
				MyCoTaskMemFree( il );
			}
		}
#endif
	}

	UnlockMem();
	if( nPATH != ExeName )
	{
		if( nPATH == Exe )
			BeDirOnly();
		BeBackSlash( bs );
	}
	return *this;
}

Path& Path::BeBackSlash( bool add )
{
	// ÅŒã‚Ìˆê•¶Žš‚ð“¾‚é
	const TCHAR* last=c_str();
	for( const TCHAR *p=last; *p!=TEXT('\0'); p=next(p) )
		last = p;

	if( *last==TEXT('\\') || *last==TEXT('/') )
	{
		if( !add )
			TrimRight( 1 );
	}
	else if( add && *last!=TEXT('\0') )
	{
		*this += TEXT('\\');
	}

	return *this;
}

Path& Path::BeDirOnly()
{
	const TCHAR* lastslash = c_str()-1;
	for( const TCHAR* p=lastslash+1; *p!=TEXT('\0'); p=next(p) )
		if( *p==TEXT('\\') || *p==TEXT('/') )
			lastslash = p;

	TrimRight( len() - ulong((lastslash+1)-c_str()) );
	return *this;
}

Path& Path::BeDriveOnly()
{
	if( len() > 2 )
	{
		const TCHAR* p = c_str()+2;
		for( ; *p!=TEXT('\0'); p=next(p) )
			if( *p==TEXT('\\') || *p==TEXT('/') )
				break;
		if( *p!=TEXT('\0') )
			TrimRight( len() - ulong((p+1)-c_str()) );
	}
	return *this;
}

Path& Path::BeShortStyle()
{
#if defined(UNICOWS) || !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>=350)
// In UNICOWS mode the A/W functions are imported dynamically anyway.
// GetShortPathName needs at least 95/NT4 but there is a stub in NT3.5
	if( app().isNewShell() ) // 95/NT4+
	{
		TCHAR* buf = ReallocMem( len()+1 );
		::GetShortPathName( buf, buf, len()+1 );
		UnlockMem();
	}
#endif
	return *this;
}

Path& Path::BeShortLongStyle()
{
#ifndef WIN32S
	WIN32_FIND_DATA fd;
	HANDLE h = ::FindFirstFile( c_str(), &fd );
	if( h == INVALID_HANDLE_VALUE)
		return *this;
	::FindClose( h );

#ifndef _UNICODE
    // Avoid using the long file name if it contains invalid chars.
    // ie: the short path name is always ASCII on NT.
    // Someone migh be using the short file name for a reason!
	TCHAR fp[MAX_PATH];
	if( my_lstrchr( fd.cFileName, TEXT('?') )
	|| !my_lstrcpy( fp, c_str() )
	|| !my_lstrcpy( (char*)name(fp), fd.cFileName)
	|| ::GetFileAttributes( fp ) == 0xFFFFFFFF )
	{ // Unable to find long file name.
		return *this;
	}
#endif
	TCHAR  t;
	TCHAR* buf = ReallocMem( MAX_PATH*2 );
	TCHAR* nam = const_cast<TCHAR*>(name(buf));

	if( nam != buf )
		t = *(nam-1), *(nam-1) = TEXT('\0');
	my_lstrcpy( nam, fd.cFileName );
	if( nam != buf )
		*(nam-1) = t;

	UnlockMem();
#endif // not WIN32S
	return *this;
}

const TCHAR *Path::CompactIfPossible( TCHAR *buf, unsigned Mx )
{
	if(this->len() <= Mx)
		return this->c_str(); // Nothing to do

	const TCHAR *fn = name();
	int fnlen = my_lstrlen(fn);
	int remaining = Mx - fnlen; // what remains
	int premaining = Max(remaining-3, 3); // what w will use for the path.
	my_lstrcpyn(buf, c_str(), premaining);
	my_lstrcpyn( buf+premaining, TEXT("..."), 3); // Add ... after the truncated path
	if (remaining >= 3)
		my_lstrcpy(buf+remaining, fn); // copy fn to the end of buf
	else // remaining < 4
		my_lstrcpyn( buf+6, fn-remaining+6, Mx-6 ); // copy end fn to the end of buf

	buf[Mx] = '\0'; // In case

	return buf;
}

const TCHAR* Path::name( const TCHAR* str )
{
	const TCHAR* ans = str - 1;
	for( const TCHAR* p=str; *p!=TEXT('\0'); p=next(p) )
		if( *p==TEXT('\\') || *p==TEXT('/') )
			ans = p;
	return (ans+1);
}

const TCHAR* Path::ext( const TCHAR* str )
{
	const TCHAR *ans = NULL, *p;
	for( p=name(str); *p!=TEXT('\0'); p=next(p) )
		if( *p==TEXT('.') )
			ans = p;
	return ans ? (ans+1) : p;
}

const TCHAR* Path::ext_all( const TCHAR* str )
{
	const TCHAR* p;
	for( p=name(str); *p!=TEXT('\0'); p=next(p) )
		if( *p==TEXT('.') )
			return (p+1);
	return p;
}

Path Path::body() const
{
	const TCHAR* nm = name();
	const TCHAR* ex = ext() - 1;
	TCHAR t = *ex;
	*const_cast<TCHAR*>(ex) = TEXT('\0');
	Path ans = nm;
	*const_cast<TCHAR*>(ex) = t;
	return ans;
}

Path Path::body_all() const
{
	const TCHAR* nm = name();
	const TCHAR* ex = ext_all() - 1;
	TCHAR t = *ex;
	*const_cast<TCHAR*>(ex) = TEXT('\0');
	Path ans = nm;
	*const_cast<TCHAR*>(ex) = t;
	return ans;
}

DWORD Path::GetExeName( TCHAR buf[MAX_PATH] )
{
	buf[0] = TEXT('\0');
	// In Win32s 1.2 GetModuleFileName fails with NULL hinstance
	DWORD len = ::GetModuleFileName( ::GetModuleHandle(NULL), buf, MAX_PATH );
	// In Win32s 1.1 the return value includes the terminating NULL!!!
	len -=  len > 0 && buf[len-1] == TEXT('\0');
	return len;
}
