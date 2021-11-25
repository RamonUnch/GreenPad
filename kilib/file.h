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

class FileR : public Object
{
public:

	FileR();
	~FileR();

	//@{
	//	�J��
	//	@param fname �t�@�C����
	//	@return �J�������ǂ���
	//@}
	bool Open( const TCHAR* fname );

	//@{
	//	����
	//@}
	void Close();

public:

	//@{ �t�@�C���T�C�Y //@}
	ulong size() const;

	//@{ �t�@�C�����e���}�b�v�����A�h���X�擾 //@}
	const uchar* base() const;

private:

	HANDLE      handle_;
	HANDLE      fmo_;
	ulong       size_;
	const void* basePtr_;

private:

	NOCOPY(FileR);
};



//-------------------------------------------------------------------------

inline ulong FileR::size() const
	{ return size_; }

inline const uchar* FileR::base() const
	{ return static_cast<const uchar*>(basePtr_); }



//=========================================================================
//@{
//	�ȈՃt�@�C����������
//
//	�Ă��Ɓ[�Ƀo�b�t�@�����O���B
//@}
//=========================================================================

class FileW : public Object
{
public:

	FileW();
	~FileW();

	//@{ �J�� //@}
	bool Open( const TCHAR* fname, bool creat=true );

	//@{ ���� //@}
	void Close();

	//@{ ���� //@}
	void Write( const void* buf, ulong siz );

	//@{ �ꕶ������ //@}
	void WriteC( uchar ch );

public:

	void Flush();

private:

	const int    BUFSIZE;
	HANDLE       handle_;
	uchar* const buf_;
	ulong        bPos_;

private:

	NOCOPY(FileW);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_FILE_H_
