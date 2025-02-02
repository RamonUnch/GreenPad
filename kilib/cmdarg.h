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
//	コマンドライン文字列の分割
//
//	ただ単にスペースと二重引用符を考慮して区切るだけです。
//@}
//=========================================================================

class A_WUNUSED Argv
{
public:

	//@{ 指定された文字列を分割する //@}
	explicit Argv( const TCHAR* cmd = GetCommandLine() );
	~Argv() { free( buf_ ); }

	//@{ 引数Get //@}
	const TCHAR* operator[]( size_t i ) const;

	//@{ 引数の個数 //@}
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
