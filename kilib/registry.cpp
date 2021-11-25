#include "stdafx.h"
#include "registry.h"
using namespace ki;



//=========================================================================

void IniFile::SetFileName( const TCHAR* ini, bool exepath )
{
	iniName_ =
		(exepath  ? iniName_.BeSpecialPath(Path::Exe) : Path(TEXT("")));
	if( ini != NULL )
		iniName_ += ini;
	else
		iniName_ += (Path(Path::ExeName).body()+=TEXT(".ini"));
}

void IniFile::SetSectionAsUserName()
{
	TCHAR usr[256];
	DWORD siz = countof(usr);
	if( !::GetUserName( usr, &siz ) )
		::lstrcpy( usr, TEXT("Default") );
	SetSection( usr );
}

bool IniFile::HasSectionEnabled( const TCHAR* section ) const
{
	return (0!=::GetPrivateProfileInt(
		section, TEXT("Enable"), 0, iniName_.c_str() ));
}

int IniFile::GetInt ( const TCHAR* key, int defval ) const
{
	return ::GetPrivateProfileInt(
		section_.c_str(), key, defval, iniName_.c_str() );
}

bool IniFile::GetBool( const TCHAR* key, bool defval ) const
{
	return (0!=::GetPrivateProfileInt(
		section_.c_str(), key, defval?1:0, iniName_.c_str() ));
}

String IniFile::GetStr ( const TCHAR* key, const String& defval ) const
{
	RawString str;
	ulong l=256, s;
	for(;;)
	{
		TCHAR* x = str.AllocMem(l);
		s = ::GetPrivateProfileString( section_.c_str(), key,
			defval.c_str(), x, l, iniName_.c_str() );
		if( s < l-1 )
			break;
		l <<= 1;
	}
	str.UnlockMem();
	return str;
}

Path IniFile::GetPath( const TCHAR* key, const Path& defval ) const
{
#ifdef _UNICODE
	String s = GetStr( key, defval );
	if( s.len()==0 || s[0]!='#' )
		return s;

	// UTF-decoder
	String buf;
	for(uint i=0; 4*i+4<s.len(); ++i)
	{
		unsigned short v = 0;
		for(int j=1; j<=4; ++j)
		{
			int ch = s[4*i+j];
			if( '0'<=ch && ch<='9' ) v = 16*v + ch-'0';
			if( 'a'<=ch && ch<='z' ) v = 16*v + ch-'a'+10;
			if( 'A'<=ch && ch<='Z' ) v = 16*v + ch-'A'+10;
		}
		buf += (wchar_t) v;
	}
	return buf;
#else
	return GetStr( key, defval );
#endif
}


bool IniFile::PutStr ( const TCHAR* key, const TCHAR* val )
{
	if( val[0]==TEXT('"') && val[::lstrlen(val)-1]==TEXT('"') )
	{
		// —¼’[‚É " ‚ª‚ ‚é‚ÆŸŽè‚Éí‚ç‚ê‚é‚Ì‚Å‘Îˆ
		String nval;
		nval += TEXT('"');
		nval += val;
		nval += TEXT('"');
		return (FALSE != ::WritePrivateProfileString(
			section_.c_str(), key, nval.c_str(), iniName_.c_str() ) );
	}
	else
	{
		return (FALSE != ::WritePrivateProfileString(
			section_.c_str(), key, val, iniName_.c_str() ) );
	}
}

bool IniFile::PutInt ( const TCHAR* key, int val )
{
	TCHAR buf[20];
	::wsprintf( buf, TEXT("%d"), val );
	return PutStr( key, buf );
}

bool IniFile::PutBool( const TCHAR* key, bool val )
{
	return PutStr( key, val ? TEXT("1") : TEXT("0") );
}

bool IniFile::PutPath( const TCHAR* key, const Path& val )
{
#ifdef _UNICODE
	BOOL err = FALSE;
	::WideCharToMultiByte( CP_ACP, 0, val.c_str(), -1, NULL, 0, NULL, &err );
	if( !err )
		return PutStr( key , val.c_str() );

	// UTF-encoder
	const TCHAR* hex = TEXT("0123456789abcdef");
	String buf = TEXT("#");
	for(int i=0; i!=val.len(); ++i)
	{
		unsigned short u = (unsigned short) val[i];
		buf += hex[(u>>12) & 0xf];
		buf += hex[(u>> 8) & 0xf];
		buf += hex[(u>> 4) & 0xf];
		buf += hex[(u>> 0) & 0xf];
	}
	return PutStr( key, buf.c_str() );
#else
	return PutStr( key, val.c_str() );
#endif
}


