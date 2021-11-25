#include "stdafx.h"
#include "file.h"
using namespace ki;



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

bool FileR::Open( const TCHAR* fname )
{
	Close();

	// �t�@�C����ǂ݂Ƃ��p�ŊJ��
	handle_ = ::CreateFile(
		fname, GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, NULL
	);
	if( handle_ == INVALID_HANDLE_VALUE )
		return false;

	// �T�C�Y���擾
	size_ = ::GetFileSize( handle_, NULL );

	if( size_==0 )
	{
		// 0�o�C�g�̃t�@�C���̓}�b�s���O�o���Ȃ��̂œK���ɉ��
		basePtr_ = &size_;
	}
	else
	{
		// �}�b�s���O�I�u�W�F�N�g�����
		fmo_ = ::CreateFileMapping(
			handle_, NULL, PAGE_READONLY, 0, 0, NULL );
		if( fmo_ == NULL )
		{
			::CloseHandle( handle_ );
			handle_ = INVALID_HANDLE_VALUE;
			return false;
		}

		// �r���[
		basePtr_ = ::MapViewOfFile( fmo_, FILE_MAP_READ, 0, 0, 0 );
		if( basePtr_ == NULL )
		{
			::CloseHandle( fmo_ );
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
}



//=========================================================================

FileW::FileW()
	: BUFSIZE( 65536 )
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

	// �t�@�C�����������ݐ�p�ŊJ��
	handle_ = ::CreateFile( fname,
		GENERIC_WRITE, FILE_SHARE_READ, NULL,
		creat ? CREATE_ALWAYS : OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( handle_ == INVALID_HANDLE_VALUE )
		return false;

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

void FileW::WriteC( uchar ch )
{
	if( (BUFSIZE-bPos_) <= 1 )
		Flush();

	buf_[bPos_++] = ch;
}

