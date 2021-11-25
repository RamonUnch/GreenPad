#include "stdafx.h"
#include "find.h"
using namespace ki;



//=========================================================================

namespace {
static inline bool isDots(const TCHAR* p)
{
	return (*p=='.' && (p[1]=='\0' || (p[1]=='.' && p[2]=='\0')));
}
}

bool FindFile::FindFirst( const TCHAR* wild, WIN32_FIND_DATA* pfd )
{
	HANDLE xh = ::FindFirstFile( wild, pfd );
	if( xh==INVALID_HANDLE_VALUE )
		return false;
	while( isDots(pfd->cFileName) )
		if( !::FindNextFile( xh, pfd ) )
		{
			::FindClose( xh );
			return false;
		}
	::FindClose( xh );
	return true;
}

void FindFile::Close()
{
	first_ = true;
	if( han_ != INVALID_HANDLE_VALUE )
		::FindClose( han_ ), han_ = INVALID_HANDLE_VALUE;
}

bool FindFile::Begin( const TCHAR* wild )
{
	Close();

	han_ = ::FindFirstFile( wild, &fd_ );
	if( han_ == INVALID_HANDLE_VALUE )
		return false;

	while( isDots(fd_.cFileName) )
		if( !::FindNextFile( han_, &fd_ ) )
		{
			Close();
			return false;
		}
	return true;
}

bool FindFile::Next( WIN32_FIND_DATA* pfd )
{
	if( han_ == INVALID_HANDLE_VALUE )
		return false;

	if( first_ )
	{
		first_ = false;
		memmove( pfd, &fd_, sizeof(fd_) );
		return true;
	}
	if( !::FindNextFile( han_, pfd ) )
		return false;
	while( isDots(fd_.cFileName) )
		if( !::FindNextFile( han_, pfd ) )
			return false;
	return true;
}

#undef isDots
