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
//	ファイル検索
//
//	Win32 APIのFindFirstFile等を用いて、ディレクトリ内の
//	ファイルリストアップを行います。
//@}
//=========================================================================

class FindFile : public Object
{
public:

	//@{ コンストラクタ //@}
	FindFile() : han_( INVALID_HANDLE_VALUE ), first_(false) {}

	//@{ デストラクタ //@}
	~FindFile() { Close(); }

	//@{ 検索終了処理 //@}
	void Close();

public: //-- 外向きインターフェイス --------------------------

	bool Begin( const TCHAR* wild );
	bool Next( WIN32_FIND_DATA* pfd );

public:

	//@{ static版。マッチする最初のファイルを取得 //@}
	static bool FindFirst( const TCHAR* wild, WIN32_FIND_DATA* pfd );

private:

	HANDLE         han_;
	bool         first_;
	WIN32_FIND_DATA fd_;

private:

	NOCOPY(FindFile);
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_FIND_H_
