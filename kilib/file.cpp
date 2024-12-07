#include "stdafx.h"
#include "file.h"
#include "app.h"
#include "path.h"
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
		TCHAR *buf = (TCHAR *)malloc((len + 16) * sizeof(TCHAR));
		if (!buf) return const_cast<TCHAR*>(ifn);
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
			free( buf );
		}
	}
	return (TCHAR *)ifn;
}
#endif

HANDLE CreateFileUNC(
	LPCTSTR fname,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	TCHAR *UNCPath = const_cast<TCHAR*>(fname);
#ifdef UNICODE
	// UNC are supported only un Unicode mode on Windows NT
	if( app().isNT() )
		UNCPath = GetUNCPath(fname); // may return fname!
#endif

	// �t�@�C����ǂ݂Ƃ��p�ŊJ��
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
		free( UNCPath );
#endif
	return hFile;
}

DWORD GetFileAttributesUNC(LPCTSTR fname)
{
	TCHAR *UNCPath = const_cast<TCHAR*>(fname);
#ifdef UNICODE
	// UNC are supported only un Unicode mode on Windows NT
	if( app().isNT() )
		UNCPath = GetUNCPath(fname); // may return fname!
#endif

	DWORD ret = ::GetFileAttributes(UNCPath);

#ifdef UNICODE
	if( UNCPath != fname ) // Was allocated...
		free( UNCPath );
#endif
	return ret;
}

//=========================================================================

bool FileR::Open( const TCHAR* fname, bool always)
{
//	MessageBox(NULL, fname, fname, MB_OK);
	Close();

	// �t�@�C����ǂ݂Ƃ��p�ŊJ��
	// |FILE_FLAG_NO_BUFFERING
	handle_ = ::CreateFileUNC(fname, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, always? OPEN_ALWAYS: OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
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

	// �T�C�Y���擾
	size_t bytesToMap=0; // 0 => map all
	DWORD hisize=0;
	size_ = ::GetFileSize( handle_, &hisize );
#ifdef WIN64
	// in 64 bit mode, we can try to Map a huge file.
	// size_ must combine lo and hi size
	size_ = size_ | (((size_t)hisize) << 32);
#else
	// in 32 bit mode we cannot map a file larger than 2GB
	if( hisize || size_ >= 0x80000000 )
	{
		int yesno = MessageBox(
			GetActiveWindow(),
			TEXT("File is too large to be loaded!\nRetry loading the 1st 64KB only?\nBE CAREFUL NOT TO ERASE THE FILE!!!!"),
			Path::name(fname),
			MB_ABORTRETRYIGNORE|MB_TASKMODAL);
		if( yesno == IDRETRY )
			size_ = bytesToMap = 1<<16;
		else if (yesno == IDABORT)
			return false;
	}
#endif


	if( size_==0 )
	{
		// 0�o�C�g�̃t�@�C���̓}�b�s���O�o���Ȃ��̂œK���ɉ��
		basePtr_ = &size_;
	}
	else
	{
		// �}�b�s���O�I�u�W�F�N�g�����
		fmo_ = ::CreateFileMapping( handle_, NULL, PAGE_READONLY, 0, 0, NULL );
		if( fmo_ == NULL )
		{
		#ifdef OLDWIN32S
			// We cannot use CreateFileMapping() on old Win32s beta
			// So we allocate a buffer for the whole file and use ReadFile().
			basePtr_ = (BYTE *)malloc( size_ );
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

		// �r���[
		basePtr_ = ::MapViewOfFile( fmo_, FILE_MAP_READ, 0, 0, bytesToMap );
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
		// �w���e�R�}�b�s���O�����ĂȂ���΂����ŉ��
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
		{
		#ifdef _DEBUG
			// Zero out in debug mode to detect use after free!
			mem00( (void*)basePtr_, size_ );
		#endif
			free( (void*)basePtr_ );
		}
		basePtr_ = NULL;
	}
#endif

}



//=========================================================================

FileW::FileW()
	: handle_( INVALID_HANDLE_VALUE )
	, buf_   ( (uchar *)malloc( sizeof(uchar) * BUFSIZE ) )
	, bPos_  ( 0 )
{
}

FileW::~FileW()
{
	Close();
	free( buf_ );
}

void FileW::Flush()
{
	DWORD dummy;
	::WriteFile( handle_, buf_, bPos_, &dummy, NULL );
	bPos_ = 0;
}

bool FileW::Open( const TCHAR* fname, bool creat )
{
	if( buf_==NULL ) // Internal buffer allocation failed!
		return false;
	Close();

	TCHAR *UNCPath = const_cast<TCHAR*>(fname);
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

	// �t�@�C�����������ݐ�p�ŊJ��
	handle_ = ::CreateFile( UNCPath,
		GENERIC_WRITE, FILE_SHARE_READ, NULL,
		creat ? CREATE_ALWAYS : OPEN_EXISTING,
		fattr | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

#ifdef UNICODE
	if( UNCPath != fname ) // Was allocated...
		free( UNCPath );
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

void FileW::Write( const void* dat, size_t siz )
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

void FileW::WriteInCodepageFromUnicode( int cp, const unicode* str, size_t len )
{
	size_t bremain;
	while( (bremain = BUFSIZE-bPos_) <= len<<2 )
	{	// While the remaining number of bytes is less than 4x len

		size_t uniwrite = (bremain>>2) - (0xD800 <= str[(bremain>>2)-1] && str[(bremain>>2)-1] <= 0xDBFF);

		// Fill the buffer to the end (kinda).
		bPos_ += ::WideCharToMultiByte( cp, 0, str, uniwrite, reinterpret_cast<char*>(buf_+bPos_), bremain, NULL, NULL ) ;
		len  -= uniwrite;
		str  += uniwrite;

		Flush();
	}

	// We have room to write len bytes directly to the tail of the buffer.
	bPos_ += ::WideCharToMultiByte( cp, 0, str, len, reinterpret_cast<char*>(buf_+bPos_), bremain, NULL, NULL ) ;
}
