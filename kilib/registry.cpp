#include "stdafx.h"
#include "registry.h"
using namespace ki;


#ifdef INI_CACHESECTION
/* Returns 0 if both strings start the same */
static int my_lstrcmpi_samestart(const TCHAR *a, const TCHAR *b)
{
	#define tolower(x) ( x | ('A'<x && x<'Z') << 5 )
	while(*a && tolower(*a) == tolower(*b)) { a++; b++; }
	return (*a != *b) && (*b != '\0');
	#undef tolower
}
static TCHAR *my_lstrcpy_s(TCHAR *dest, const size_t N, const TCHAR *src)
{
	TCHAR *orig=dest;
	TCHAR *dmax=dest+N-1; /* keep space for a terminating NULL */
	for (; dest<dmax && (*dest=*src); ++src,++dest);  /* then append from src */
	*dest='\0'; /* ensure result is NULL terminated */
	return orig;
}

static int my_lstrtoi(const TCHAR *s)
{
	long int v=0;
	int sign=1;
	while (*s == ' ') s++; /*  ||  (unsigned int)(*s - 9) < 5u */

	switch (*s) {
	case '-': sign=-1; /* fall through */
	case '+': ++s;
	}
	while ((unsigned)(*s - '0') < 10u) {
		v = v * 10 + (*s - '0');
		++s;
	}
	return sign*v;
}

/* Get the string inside the section returned by GetPrivateProfileSection */
static DWORD GetSectionOptionStr(const TCHAR *section, const TCHAR * const oname, const TCHAR *def, TCHAR *txt, size_t txtlen)
{
	if (section) {
		TCHAR name[128];
		my_lstrcpy_s(name, countof(name)-1, oname);
		my_lstrcat(name, TEXT("=")); /* Add equal at the end of name */
		const TCHAR *p = section;
		while (p[0] && p[1]) { /* Double NULL treminated string */
			if(!my_lstrcmpi_samestart(p, name)) {
				/* Copy the buffer */
				my_lstrcpy_s(txt, txtlen, p+lstrlen(name));
				return (DWORD) my_lstrlen(txt); /* DONE! */
			} else {
				/* Go to next string... */
				p += lstrlen(p); /* p in on the '\0' */
				p++; /* next string start. */
				if (!*p) break;
			}
		}
	}
	/* Default to the provided def string */
	my_lstrcpy_s(txt, txtlen, def);
	return (DWORD) my_lstrlen(txt); /* DONE! */
}

/* Get the int inside the section returned by GetPrivateProfileSection */
static int GetSectionOptionInt(const TCHAR *section, const TCHAR * const oname, const int def)
{
	if (section) {
		TCHAR name[128];
		my_lstrcpy_s(name, countof(name)-1, oname);
		my_lstrcat(name, TEXT("=")); /* Add equal at the end of name */
		const TCHAR *p = section;
		while (p[0] && p[1]) { /* Double NULL treminated string */
			if(!my_lstrcmpi_samestart(p, name)) {
				/* DONE !*/
				return my_lstrtoi(p+lstrlen(name));
			} else {
				/* Go to next string... */
				p += lstrlen(p); /* p in on the '\0' */
				p++; /* next string start. */
				if (!*p) break;
			}
		}
	}
	/* Default to the provided def value */
	return def;
}
#endif // INI_CACHESECTION

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

void IniFile::CacheSection()
{
#ifdef INI_CACHESECTION
	// Load the whole section inside the fullsection_ buffer.
	fullsection_ = m_StrBuf;
	GetPrivateProfileSection( section_.c_str(), fullsection_, countof(m_StrBuf), iniName_.c_str() );
#endif
}

void IniFile::SetSectionAsUserName()
{
	TCHAR usr[256+1]; // UNLEN+1
	DWORD siz = countof(usr);
	if( ::GetUserName( usr, &siz ) )
		SetSection( usr );
	else
		SetSection( TEXT("Default") );
}

bool IniFile::HasSectionEnabled( const TCHAR* section ) const
{
	return (0!=::GetPrivateProfileInt(
		section, TEXT("Enable"), 0, iniName_.c_str() ));
}

int IniFile::GetInt( const TCHAR* key, int defval ) const
{
#ifdef INI_CACHESECTION
	return fullsection_
		? GetSectionOptionInt( fullsection_, key, defval )
		: ::GetPrivateProfileInt( section_.c_str(), key, defval, iniName_.c_str() );
#else
	return ::GetPrivateProfileInt( section_.c_str(), key, defval, iniName_.c_str() );
#endif
}

bool IniFile::GetBool( const TCHAR* key, bool defval ) const
{
	return !!GetInt( key, !!defval );
}
void IniFile::GetRect ( const TCHAR* key, RECT *rc, const RECT *defrc  ) const
{
	TCHAR rcCN[128];
	my_lstrcpy(rcCN, key);
	TCHAR *lastc = &rcCN[my_lstrlen(rcCN)];
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

String IniFile::GetStr ( const TCHAR* key, const String& defval ) const
{
	RawString str;
	ulong l=128, s;
	for(;;)
	{
		TCHAR* x = str.AllocMem(l);
#ifdef INI_CACHESECTION
		s = fullsection_
			? GetSectionOptionStr( fullsection_, key, defval.c_str(), x, l )
			: ::GetPrivateProfileString( section_.c_str(), key, defval.c_str(), x, l, iniName_.c_str() );
#else
		s = ::GetPrivateProfileString(
			section_.c_str(), key, defval.c_str(), x, l, iniName_.c_str() );
#endif
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


bool IniFile::PutStr( const TCHAR* key, const TCHAR* val )
{
	if( val[0]==TEXT('"') && val[my_lstrlen(val)-1]==TEXT('"') )
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

bool IniFile::PutInt( const TCHAR* key, int val )
{
	TCHAR buf[20];
	//::wsprintf( buf, TEXT("%d"), val );
	return PutStr( key, Inbt2lStr(buf, val) );
}

bool IniFile::PutBool( const TCHAR* key, bool val )
{
	return PutStr( key, val ? TEXT("1") : TEXT("0") );
}

bool IniFile::PutRect ( const TCHAR* key, const RECT *rc  )
{
	TCHAR rcCN[128];
	my_lstrcpy(rcCN, key);
	TCHAR *lastc = &rcCN[my_lstrlen(rcCN)];
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
	BOOL err = FALSE;
	::WideCharToMultiByte( CP_ACP, 0, val.c_str(), -1, NULL, 0, NULL, &err );
	if( !err )
		return PutStr( key , val.c_str() );

	// UTF-encoder
	const TCHAR* hex = TEXT("0123456789abcdef");
	String buf = TEXT("#");
	for(unsigned i=0; i!=val.len(); ++i)
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


