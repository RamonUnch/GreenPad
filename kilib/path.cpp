#include "stdafx.h"
#include "path.h"
using namespace ki;


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
		{
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>350)
			LPITEMIDLIST il;
			if( NOERROR==::SHGetSpecialFolderLocation( NULL, nPATH, &il ) )
			{
				::SHGetPathFromIDList( il, buf );
				::CoTaskMemFree( il );
			}
#endif
		}
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
#if !defined(TARGET_VER) || (defined(TARGET_VER) && TARGET_VER>310)
	TCHAR* buf = ReallocMem( len()+1 );
	::GetShortPathName( buf, buf, len()+1 );
	UnlockMem();
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
	::lstrcpy( nam, fd.cFileName );
	if( nam != buf )
		*(nam-1) = t;

	UnlockMem();
	return *this;
}

String Path::CompactIfPossible( int Mx )
{
	HMODULE hshl = ::LoadLibrary( TEXT("shlwapi.dll") );
	if( !hshl ) return *this;

	typedef BOOL (STDAPICALLTYPE *PCPE_t)( LPTSTR, LPCTSTR, UINT, DWORD );
#ifdef _UNICODE
	PCPE_t MyPathCompactPathEx = (PCPE_t)::GetProcAddress( hshl, "PathCompactPathExW" );
#else
	PCPE_t MyPathCompactPathEx = (PCPE_t)::GetProcAddress( hshl, "PathCompactPathExA" );
#endif
	if( !MyPathCompactPathEx ) return *this;

	TCHAR* buf = new TCHAR[Mx+2];
	MyPathCompactPathEx( buf, c_str(), Mx+1, 0 );
	::FreeLibrary( hshl );

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

