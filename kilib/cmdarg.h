#ifndef _KILIB_CMDARG_H_
#define _KILIB_CMDARG_H_
#include "types.h"
#include "memory.h"
#include "ktlarray.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	�R�}���h���C��������̕���
//
//	�����P�ɃX�y�[�X�Ɠ�d���p�����l�����ċ�؂邾���ł��B
//@}
//=========================================================================

class A_WUNUSED Argv
{
public:

	//@{ �w�肳�ꂽ������𕪊����� //@}
	explicit Argv( const TCHAR* cmd = GetCommandLine() );
	~Argv() { free( buf_ ); }

	//@{ ����Get //@}
	const TCHAR* operator[]( size_t i ) const;

	//@{ �����̌� //@}
	size_t size() const;

private:

	enum { MAX_ARGS = 16 };
	TCHAR                *  buf_;
	size_t        argNum_;
	const TCHAR * arg_[MAX_ARGS];

private:

	NOCOPY(Argv);
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline const TCHAR* Argv::operator []( size_t i ) const
	{ return arg_[i]; }

inline size_t Argv::size() const
	{ return argNum_; }



//=========================================================================

#endif // __ccdoc__
}      // namespace ki
#endif // _KILIB_CMDARG_H_
