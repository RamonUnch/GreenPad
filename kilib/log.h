#ifndef _KILIB_LOG_H_
#define _KILIB_LOG_H_
#include "types.h"
#ifndef __ccdoc__
namespace ki {
#endif



class String;

//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	���O���@�\�i�f�o�b�O�p�j
//
//	�A�v���N��/�I���p������S�����܂��B
//	��kilib�ƈ���āA���[�U�[���̃A�v���P�[�V�����N���X��
//	��������h�������邱�Ƃ͏o���܂���B���[�U�[�̃R�[�h�́A
//	�K�� kmain() �Ƃ����O���[�o���֐�������s�J�n����܂��B
//	����App�N���X���̂́A���HINSTANCE�̊Ǘ����s�������B
//@}
//=========================================================================

class Logger
{
public:

	Logger() {}
	void WriteLine( const String& str );
	void WriteLine( const TCHAR* str );
	void WriteLine( const TCHAR* str, int siz );
	void __cdecl WriteLineFmtErr(const TCHAR *fmt, ...);
private:

	NOCOPY(Logger);
};

#ifdef DO_LOGGING
	#define LOGGER(str) Logger().WriteLine(TEXT(str))
	#define LOGGERS(str) Logger().WriteLine(str)
	#define LOGGERF Logger().WriteLineFmtErr
#else
	#define LOGGER(x)
	#define LOGGERS(x)
	#define LOGGERF Logger().WriteLineFmtErr
//	static void __cdecl LOGGERF(const TCHAR *fmt, ...){}
#endif

//=========================================================================

}      // namespace ki
#endif // _KILIB_LOG_H_
