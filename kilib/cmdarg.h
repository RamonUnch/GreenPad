#ifndef _KILIB_CMDARG_H_
#define _KILIB_CMDARG_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"
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

class Argv : public Object
{
public:

	//@{ �w�肳�ꂽ������𕪊����� //@}
	Argv( const TCHAR* cmd = GetCommandLine() );

	//@{ ����Get //@}
	const TCHAR* operator[]( ulong i ) const;

	//@{ �����̌� //@}
	ulong size() const;

private:

	darr<TCHAR>           buf_;
	storage<const TCHAR*> arg_;

private:

	NOCOPY(Argv);
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline const TCHAR* Argv::operator []( ulong i ) const
	{ return arg_[i]; }

inline ulong Argv::size() const
	{ return arg_.size(); }



//=========================================================================

#endif // __ccdoc__
}      // namespace ki
#endif // _KILIB_CMDARG_H_
