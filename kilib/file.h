#ifndef _KILIB_FILE_H_
#define _KILIB_FILE_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	�ȈՃt�@�C���Ǎ�
//
//	�t�@�C���}�b�s���O��p����̂ŁA�������ȒP�ł��ƍ����ł��B
//	���������������ƂɁA4GB�܂ł����J���܂���B
//@}
//=========================================================================

class FileR
{
public:

	FileR()
		: handle_ ( INVALID_HANDLE_VALUE )
		, fmo_    ( NULL )
		, size_   ( 0 )
		, basePtr_( NULL ) {}
	~FileR() { Close(); }

	//@{
	//	�J��
	//	@param fname �t�@�C����
	//	@return �J�������ǂ���
	//@}
	bool Open( const TCHAR* fname, bool always=false );

	//@{
	//	����
	//@}
	void Close();

public:

	//@{ �t�@�C���T�C�Y //@}
	size_t size() const
		{ return size_; };

	//@{ �t�@�C�����e���}�b�v�����A�h���X�擾 //@}
	const uchar* base() const
		{ return static_cast<const uchar*>(basePtr_); }

private:

	HANDLE      handle_;
	HANDLE      fmo_;
	size_t      size_;
	const void* basePtr_;

private:

	NOCOPY(FileR);
};



//=========================================================================
//@{
//	�ȈՃt�@�C����������
//
//	�Ă��Ɓ[�Ƀo�b�t�@�����O���B
//@}
//=========================================================================

class FileW
{
public:

	FileW();
	~FileW();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname, bool creat=true );

	//@{ ���� //@}
	void Close();

	//@{ ���� //@}
	void Write( const void* buf, size_t siz );

	//@{ �ꕶ������ //@}
	void WriteC( const uchar ch );

	//@{ Write a character without checking bufer //@}
	inline void WriteCN( const uchar ch )
		{ buf_[bPos_++] = ch; }

	//@{ Flush if needed to get the specified space //@}
	inline void NeedSpace( const size_t sz )
		{ if( (BUFSIZE-bPos_) <= sz ) Flush(); }

	//@{ Writes to the file using a specific output codepage //@}
	void WriteInCodepageFromUnicode( int cp, const unicode* str, size_t len );

public:

	void Flush();

private:

	enum { BUFSIZE = 32768 };
	HANDLE       handle_;
	uchar* const buf_;
	size_t       bPos_;

private:

	NOCOPY(FileW);
};

//=========================================================================

}      // namespace ki
#endif // _KILIB_FILE_H_
