#ifndef _KILIB_FIND_H_
#define _KILIB_FIND_H_
#include "types.h"
#include "memory.h"
#include "path.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	�t�@�C������
//
//	Win32 API��FindFirstFile����p���āA�f�B���N�g������
//	�t�@�C�����X�g�A�b�v���s���܂��B
//@}
//=========================================================================

class FindFile : public Object
{
public:

	//@{ �R���X�g���N�^ //@}
	FindFile();

	//@{ �f�X�g���N�^ //@}
	~FindFile();

	//@{ �����I������ //@}
	void Close();

public: //-- �O�����C���^�[�t�F�C�X --------------------------

	bool Begin( const TCHAR* wild );
	bool Next( WIN32_FIND_DATA* pfd );

public:

	//@{ static�ŁB�}�b�`����ŏ��̃t�@�C�����擾 //@}
	static bool FindFirst( const TCHAR* wild, WIN32_FIND_DATA* pfd );

private:

	HANDLE         han_;
	bool         first_;
	WIN32_FIND_DATA fd_;

private:

	NOCOPY(FindFile);
};



//-------------------------------------------------------------------------

inline FindFile::FindFile()
	: han_( INVALID_HANDLE_VALUE ) {}

inline FindFile::~FindFile()
	{ Close(); }



//=========================================================================

}      // namespace ki
#endif // _KILIB_FIND_H_
