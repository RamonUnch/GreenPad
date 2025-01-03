#include "stdafx.h"
#include "registry.h"
using namespace ki;


//=========================================================================

void IniFile::SetFileName( /*const TCHAR* ini, bool exepath*/ )
{
	DWORD len = Path::GetExeName( iniName_ );
	if( len > 3 )
		my_lstrcpy( iniName_+len-3, TEXT("ini") );
}

void IniFile::SetSectionAsUserName()
{
	TCHAR usr[256+1];
	DWORD siz = countof(usr);
	if( ::GetUserName( usr, &siz ) )
		SetSection( usr );
	else
		SetSection( TEXT("Default") );
}

bool IniFile::SetSectionAsUserNameIfNotShared( const TCHAR *section )
{
	bool shared = HasSectionEnabled( section );
	if( shared )
		SetSection( section );
	else
		SetSectionAsUserName();

	return shared;
}

bool IniFile::HasSectionEnabled( const TCHAR* section ) const
{
	return (0!=::GetPrivateProfileInt(
		section, TEXT("Enable"), 0, iniName_ ));
}

int IniFile::GetInt( const TCHAR* key, int defval ) const
{
	return ::GetPrivateProfileInt( section_.c_str(), key, defval, iniName_ );
}

bool IniFile::GetBool( const TCHAR* key, bool defval ) const
{
	return !!GetInt( key, !!defval );
}
void IniFile::GetRect ( const TCHAR* key, RECT *rc, const RECT *defrc  ) const
{
	TCHAR rcCN[128];
	TCHAR *lastc = my_lstrkpy(rcCN, key);
	lastc[1] = TEXT('\0'); // Extra NULL

	*lastc = TEXT('L');
	rc->left  = GetInt(rcCN, defrc->left);

	*lastc = TEXT('T');
	rc->top  = GetInt(rcCN, defrc->top);

	*lastc = TEXT('R');
	rc->right  = GetInt(rcCN, defrc->right);

	*lastc = TEXT('B');
	rc->bottom  = GetInt(rcCN, defrc->bottom);
}

String IniFile::GetStr( const TCHAR* key, const TCHAR *defval ) const
{
	return GetStrinSect( key, section_.c_str(), defval );
}
String IniFile::GetStrinSect( const TCHAR* key, const TCHAR* sect, const TCHAR *defval ) const
{
	RawString str;
	DWORD l=128;
	for(;;)
	{
		TCHAR* x = str.AllocMem(l);
		if( !x ) return String(TEXT(""));
		DWORD s = ::GetPrivateProfileString( sect, key, defval, x, l, iniName_ );
		if( s < l-1 )
			break;
		l <<= 1;
	}
	str.UnlockMem();
	return str;
}

TCHAR *IniFile::GetSStrHere(const TCHAR* key, const TCHAR* sect, const TCHAR *defval, TCHAR buf[MAX_PATH]) const
{
	buf[0] = TEXT('\0');
	::GetPrivateProfileString( sect, key, defval, buf, MAX_PATH, iniName_ );
	return buf;
}

Path IniFile::GetPath( const TCHAR* key, const TCHAR *defval ) const
{
#ifdef _UNICODE
	String s = GetStr( key, defval );
	if( s.len()==0 || s[0]!='#' )
		return s;

	// UTF-decoder
	String buf;
	for(uint i=1; i<s.len(); ++i)
	{
		unsigned short v = s[i];
		// Percent escape sequence %xxxx
		if( v == L'%' && i+4 < s.len() )
		{
			for(int j=1; j<=4; ++j)
			{
				int ch = s[i+j];
				if( '0'<=ch && ch<='9' ) v = 16*v + ch-'0';
				if( 'A'<=ch && ch<='F' ) v = 16*v + ch-'A'+10;
				if( 'a'<=ch && ch<='f' ) v = 16*v + ch-'a'+10;
			}
			i+=4;
		}
		buf += (wchar_t) v;
	}
	return buf;
#else
	return GetStr( key, defval );
#endif
}


bool IniFile::PutStr( const TCHAR* key, const TCHAR* val )
{
	return PutStrinSect( key, section_.c_str(), val );
}

bool IniFile::PutStrinSect( const TCHAR* key, const TCHAR *sect, const TCHAR* val )
{
	if( val[0]==TEXT('"') && val[my_lstrlen(val)-1]==TEXT('"') )
	{
		// 両端に " があると勝手に削られるので対処
		String nval;
		nval += TEXT('"');
		nval += val;
		nval += TEXT('"');
		return FALSE != ::WritePrivateProfileString(sect, key, nval.c_str(), iniName_);
	}
	else
	{
		return FALSE != ::WritePrivateProfileString(sect, key, val, iniName_);
	}
}

bool IniFile::PutInt( const TCHAR* key, int val )
{
	TCHAR buf[INT_DIGITS+1];
	return PutStr( key, Int2lStr(buf, val) );
}

bool IniFile::PutBool( const TCHAR* key, bool val )
{
	return PutStr( key, val ? TEXT("1") : TEXT("0") );
}

bool IniFile::PutRect( const TCHAR* key, const RECT *rc  )
{
	TCHAR rcCN[128];
	TCHAR *lastc = my_lstrkpy(rcCN, key);
	lastc[1] = TEXT('\0'); // Extra NULL

	*lastc = TEXT('L');
	PutInt(rcCN, rc->left);

	*lastc = TEXT('T');
	PutInt(rcCN, rc->top);

	*lastc = TEXT('R');
	PutInt(rcCN, rc->right);

	*lastc = TEXT('B');
	return PutInt(rcCN, rc->bottom);
}

bool IniFile::PutPath( const TCHAR* key, const Path& val )
{
#ifdef _UNICODE
	if( val.isCompatibleWithACP() )
		return PutStr( key , val.c_str() );

	// UTF-encoder
	static const TCHAR hex[] = {
		'0','1','2','3','4','5','6','7',
		'8','9','a','b','c','d','e','f' };
	String buf = TEXT("#");
	for(size_t i=0; i<val.len(); ++i)
	{
		unsigned short u = (unsigned short) val[i];
		if( u > 127 || u == L'%' )
		{	// Unicode, save as %xxxx
			buf += L'%';
			buf += hex[(u>>12) & 0xf];
			buf += hex[(u>> 8) & 0xf];
			buf += hex[(u>> 4) & 0xf];
			buf += hex[(u>> 0) & 0xf];
		}
		else
		{
			buf += u; // ASCII, save as-is
		}
	}
	return PutStr( key, buf.c_str() );
#else
	return PutStr( key, val.c_str() );
#endif
}


