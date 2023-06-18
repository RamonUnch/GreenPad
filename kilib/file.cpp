#include "stdafx.h"
#include "file.h"
#include "app.h"
using namespace ki;

#ifdef UNICODE
static TCHAR *GetUNCPath(const TCHAR *ifn)
{
	if(!ifn) return NULL;
	// Actually for GetFullPathName it is not needed to
	// add the funny \\?\ prefix to get the full path name.
	// However we add it in to the buffer for future uses...
	// also we should not add it if it is alrady an UNC
	// and we should prefix with \\?\UNC\ in case of Network path
	ULONG len = GetFullPathName(ifn, 0, 0, 0);
	// Note that it seems that UNC are not working on NT 3.1
	// So we only convert them if the path is longer than MAX_PATH
	// this also mean we do not have to check for Win9x/NT versions.
	if (len > MAX_PATH) // len includes the terminating '\0'.
	{
		TCHAR *buf = new TCHAR [(len + 16) * sizeof(TCHAR)];
		if (!buf) return (TCHAR* )ifn;
		int buffstart = 0;
		if (ifn[0] == '\\' && ifn[1] == '\\')
		{
			if (ifn[2] == '?') {
				// Already an UNC...
				buffstart = 0;
			} else {
				// Network path "\\server\share" style...
				buf[0] = '\\'; buf[1] = '\\';  buf[2] = '?'; buf[3] = '\\';
				buf[4] = 'U'; buf[5] = 'N';
				buffstart = 6;
			}
		}
		else
		{
			// Relative or non UNC path.
			buf[0] = '\\'; buf[1] = '\\';  buf[2] = '?'; buf[3] = '\\';
			buffstart = 4;
		}
		// Get the real pathname this time...
		ULONG nlen = GetFullPathName(ifn, len, &buf[buffstart], NULL);
		if (nlen)
		{
			// We got the FullPathName
			if (buffstart == 6) {
				buf[6] = 'C'; // Network path set again the C from \\?\UNC
			} else if (buffstart == 4 && buf[4] == '\\' && buf[5] == '\\') {
				// it was a relative network path,
				// so now it is in the \\?\\\server\share format (BAD).
				// shift the full path two char to the right.
				int i = nlen + buffstart;
				for (; i > 4; i--) { buf[i+2] = buf[i]; }
				// Add UNC so that we have \\?\UNC\server\share
				buf[4] = 'U'; buf[5] = 'N'; buf[6] = 'C';
			}
			return buf;
		}
		else
		{
			// Unable to get full path name for ifn
			delete [] buf;
		}
	}
	return (TCHAR *)ifn;
}
#endif

static HANDLE CreateFileUNC(
	LPCTSTR fname,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	TCHAR *UNCPath = (TCHAR *)fname;
#ifdef UNICODE
	// UNC are supported only un Unicode mode on Windows NT
	if( app().isNT() )
		UNCPath = GetUNCPath(fname); // may return fname!
#endif

	// ファイルを読みとり専用で開く
	HANDLE hFile = ::CreateFile(
		UNCPath,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
#ifdef UNICODE
	if( UNCPath != fname ) // Was allocated...
		delete [] UNCPath;
#endif
	return hFile;
}

DWORD GetFileAttributesUNC(LPCTSTR fname)
{
	TCHAR *UNCPath = (TCHAR *)fname;
#ifdef UNICODE
	// UNC are supported only un Unicode mode on Windows NT
	if( app().isNT() )
		UNCPath = GetUNCPath(fname); // may return fname!
#endif

	DWORD ret = ::GetFileAttributes(UNCPath);

#ifdef UNICODE
	if( UNCPath != fname ) // Was allocated...
		delete [] UNCPath;
#endif
	return ret;
}

//=========================================================================

FileR::FileR()
	: handle_ ( INVALID_HANDLE_VALUE )
	, fmo_    ( NULL )
	, basePtr_( NULL )
{
}

FileR::~FileR()
{
	Close();
}

bool FileR::Open( const TCHAR* fname, bool always)
{
//	MessageBox(NULL, fname, fname, MB_OK);
	Close();

	// ファイルを読みとり専用で開く
	handle_ = ::CreateFileUNC(fname, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, always? OPEN_ALWAYS: OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_NO_BUFFERING,
		NULL
	);
	if( handle_ == INVALID_HANDLE_VALUE )
	{
		#ifdef _DEBUG
		TCHAR msg[300];
		::wsprintf( msg, TEXT("CreateFile(%s) failed #%d"), fname, ::GetLastError() );
		::MessageBox( NULL,msg,TEXT("Debug"),0 );
		#endif
		return false;
	}

	// サイズを取得
	size_ = ::GetFileSize( handle_, NULL );

	if( size_==0 )
	{
		// 0バイトのファイルはマッピング出来ないので適当に回避
		basePtr_ = &size_;
	}
	else
	{
		// マッピングオブジェクトを作る
		fmo_ = ::CreateFileMapping(
			handle_, NULL, PAGE_READONLY, 0, 0, NULL );
		if( fmo_ == NULL )
		{
		#ifdef OLDWIN32S
			// We cannot use CreateFileMapping() on old Win32s beta
			// So we allocate a buffer for the whole file and use ReadFile().
			basePtr_ = new BYTE[size_];
			DWORD nBytesRead=0;
			BOOL ret = ReadFile( handle_, (void*)basePtr_, size_, &nBytesRead,  NULL);
			::CloseHandle( handle_ ); // We can already close the handle
			handle_ = INVALID_HANDLE_VALUE;
			size_ = nBytesRead; // Update size with what was actually read.
			return nBytesRead && ret;
		#else
			// Just close the file handle and exit with error.
			#ifdef _DEBUG
			TCHAR msg[300];
			::wsprintf( msg, TEXT("CreateFileMapping(%s) failed #%d"), fname, ::GetLastError() );
			::MessageBox( NULL,msg,TEXT("Debug"),0 );
			#endif
			::CloseHandle( handle_ );
			handle_ = INVALID_HANDLE_VALUE;
			return false;
		#endif
		}

		// ビュー
		basePtr_ = ::MapViewOfFile( fmo_, FILE_MAP_READ, 0, 0, 0 );
		if( basePtr_ == NULL )
		{
			#ifdef _DEBUG
			TCHAR msg[300];
			::wsprintf( msg, TEXT("MapViewOfFile(%s) failed #%d"), fname, ::GetLastError() );
			::MessageBox( NULL,msg,TEXT("Debug"),0 );
			#endif
			::CloseHandle( fmo_ );
			fmo_ = NULL;
			::CloseHandle( handle_ );
			handle_ = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void FileR::Close()
{
	if( handle_ != INVALID_HANDLE_VALUE )
	{
		// ヘンテコマッピングをしてなければここで解放
		if( basePtr_ != &size_ )
			::UnmapViewOfFile( const_cast<void*>(basePtr_) );
		basePtr_ = NULL;

		if( fmo_ != NULL )
			::CloseHandle( fmo_ );
		fmo_ = NULL;

		::CloseHandle( handle_ );
		handle_ = INVALID_HANDLE_VALUE;
	}
#ifdef OLDWIN32S
	else
	{
		// If basePtr_ is non-NULL it means we allocated file
		// Via ReadFile (Win32s beta), so we must free the memory.
		// File handle is already closed.
		if( basePtr_ != NULL && basePtr_ != &size_ )
			delete [] (void*)basePtr_;
		basePtr_ = NULL;
	}
#endif

}



//=========================================================================

FileW::FileW()
	: BUFSIZE( 32768 )
	, handle_( INVALID_HANDLE_VALUE )
	, buf_   ( new uchar[BUFSIZE] )
{
}

FileW::~FileW()
{
	Close();
	delete [] buf_;
}

inline void FileW::Flush()
{
	DWORD dummy;
	::WriteFile( handle_, buf_, bPos_, &dummy, NULL );
	bPos_ = 0;
}

bool FileW::Open( const TCHAR* fname, bool creat )
{
	Close();

	TCHAR *UNCPath = (TCHAR *)fname;
#ifdef UNICODE
	// UNC are supported only un Unicode mode on Windows NT
	if( app().isNT() )
		UNCPath = GetUNCPath(fname); // may return fname!
#endif

	// Check for readonly flag
	DWORD fattr = GetFileAttributes(UNCPath);
	if( fattr == 0xFFFFFFFF )
	{ // File does not exist!
		fattr = FILE_ATTRIBUTE_NORMAL;
	}
	else if( fattr&FILE_ATTRIBUTE_READONLY )
	{
		if( IDYES==::MessageBox(NULL, TEXT("Read-Only file!\nRemove attribute to allow Writting?")
				, NULL, MB_YESNO|MB_TASKMODAL|MB_TOPMOST) )
		{
			SetFileAttributes(UNCPath, fattr&~FILE_ATTRIBUTE_READONLY);
			fattr = GetFileAttributes(UNCPath); // To be sure...
		}
	}
	// If file does not exist, use normal attributes.

	// ファイルを書き込み専用で開く
	handle_ = ::CreateFile( UNCPath,
		GENERIC_WRITE, FILE_SHARE_READ, NULL,
		creat ? CREATE_ALWAYS : OPEN_EXISTING,
		fattr | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

#ifdef UNICODE
	if( UNCPath != fname ) // Was allocated...
		delete [] UNCPath;
#endif

	if( handle_ == INVALID_HANDLE_VALUE )
	{
		// MessageBox(NULL, TEXT("Error opening file for write"), NULL, MB_OK);
		return false;
	}

	bPos_ = 0;
	return true;
}

void FileW::Close()
{
	if( handle_ != INVALID_HANDLE_VALUE )
	{
		Flush();
		::CloseHandle( handle_ );
		handle_ = INVALID_HANDLE_VALUE;
	}
}

void FileW::Write( const void* dat, ulong siz )
{
	const uchar* udat = static_cast<const uchar*>(dat);

	while( (BUFSIZE-bPos_) <= siz )
	{
		memmove( buf_+bPos_, udat, BUFSIZE-bPos_ );
		siz  -= (BUFSIZE-bPos_);
		udat += (BUFSIZE-bPos_);
		bPos_ = BUFSIZE;
		Flush();
	}

	memmove( buf_+bPos_, udat, siz );
	bPos_ += siz;
}

void FileW::WriteC( const uchar ch )
{
	if( (BUFSIZE-bPos_) <= 1 )
		Flush();

	buf_[bPos_++] = ch;
}

void FileW::WriteInCodepageFromUnicode( int cp, const unicode* str, ulong len )
{
	ulong bremain;
	while( (bremain = BUFSIZE-bPos_) <= len<<2 )
	{	// While the remaining number of bytes is less than 4x len

		ulong uniwrite = (bremain>>2) - (0xD800 <= str[(bremain>>2)-1] && str[(bremain>>2)-1] <= 0xDBFF);

		// Fill the buffer to the end (kinda).
		bPos_ += ::WideCharToMultiByte( cp, 0, str, uniwrite, (char*)(buf_+bPos_), bremain, NULL, NULL ) ;
		len  -= uniwrite;
		str  += uniwrite;

		Flush();
	}

	// We have room to write len bytes directly to the tail of the buffer.
	bPos_ += ::WideCharToMultiByte( cp, 0, str, len, (char*)(buf_+bPos_), bremain, NULL, NULL ) ;
}
