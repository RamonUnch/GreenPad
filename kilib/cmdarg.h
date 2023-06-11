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
//	コマンドライン文字列の分割
//
//	ただ単にスペースと二重引用符を考慮して区切るだけです。
//@}
//=========================================================================

class Argv : public Object
{
public:

	//@{ 指定された文字列を分割する //@}
	Argv( const TCHAR* cmd = GetCommandLine() );
	~Argv() { delete [] buf_; }

	//@{ 引数Get //@}
	const TCHAR* operator[]( ulong i ) const;

	//@{ 引数の個数 //@}
	ulong size() const;

private:

	TCHAR              *  buf_;
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
