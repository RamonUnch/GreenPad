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
		func = (funkk)GetProcAddress(GetModuleHandleA("OLE32.DLL"),"CoTaskMemFree");
	if (func) func(mem);
}
typedef HRESULT (WINAPI * funkk2)(HWND h, int i, LPITEMIDLIST *idl);
HRESULT MySHGetSpecialFolderLocation(HWND h, int i, LPITEMIDLIST *idl)
{
	static funkk2 func = (funkk2) (-1);
	if (func == (funkk2)(-1))
		func = (funkk2)GetProcAddress(GetModuleHandleA("SHELL32.DLL"),"SHGetSpecialFolderLocation");
	if (func)
		return func(h, i, idl);
	return 666;
}
#endif
static const TCHAR *GetFNinPath(const TCHAR *p)
{
    int i=0;

    while(p[++i] != '\0');
    while(i >= 0 && p[i] != '\\' && p[i] != '/') i--;
    i++;
    i += (p[i] == '\\' || p[i] == '/');
    return &p[i]; // first char of the filename
}

//=========================================================================

Path& Path::BeSpecialPath( int nPATH, bool bs )
{
	TCHAR* buf = AllocMem( MAX_PATH + 1 );

	switch( nPATH )
	{
	case Win:     ::GetWindowsDirectory( buf, MAX_PATH );     break;
	case Sys:     ::GetSystemDirectory( buf, MAX_PATH );      break;
	case Tmp:     ::GetTempPath( MAX_PATH, buf );             break;
	case Cur:     ::GetCurrentDirectory( MAX_PATH, buf );     break;
	case Exe:
	case ExeName: ::GetModuleFileName( NULL, buf, MAX_PATH ); break;
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
#if defined(UNICOWS) || !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>300)
// In UNICOWS mode the A/W functions are imported dynamically anyway.
// GetShortPathName needs at least 95/NT4 but there is a stub in NT3.5
	if(app().isNewShell()) // 95/NT4+
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
	WIN32_FIND_DATA fd;
	HANDLE h = ::FindFirstFile( c_str(), &fd );
	if( h == INVALID_HANDLE_VALUE )
		return *this;
	::FindClose( h );

	TCHAR  t;
	TCHAR* buf = ReallocMem( MAX_PATH*2 );
	TCHAR* nam = const_cast<TCHAR*>(name(buf));

	if( nam != buf )
		t = *(nam-1), *(nam-1) = TEXT('\0');
	my_lstrcpy( nam, fd.cFileName );
	if( nam != buf )
		*(nam-1) = t;

	UnlockMem();
	return *this;
}

String Path::CompactIfPossible( unsigned Mx )
{
	if(this->len() <= Mx)
		return *this; // Nothiing to do

	TCHAR* buf = new TCHAR[Mx+2];
	const TCHAR *fn = GetFNinPath(c_str())-1; // includes the '\'
	int fnlen = my_lstrlen(fn);
	int remaining = Mx - fnlen; // what remains
	int premaining = Max(remaining-3, 3); // what w will use for the path.
	my_lstrcpyn(buf, c_str(), premaining);
	my_lstrcpyn(&buf[premaining], TEXT("..."), 3); // Add ... after the truncated path
	if (remaining >= 3) {
		my_lstrcpy(&buf[remaining], fn); // copy fn to the end of buf
	} else { // remaining < 3
		my_lstrcpy(&buf[6], &fn[-remaining]); // copy fn to the end of buf
	}
	String ans = buf;
	delete [] buf;
	return ans;
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

