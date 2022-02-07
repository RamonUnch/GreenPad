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
//	簡易ファイル読込
//
//	ファイルマッピングを用いるので、扱いが簡単でわりと高速です。
//	ただし困ったことに、4GBまでしか開けません。
//@}
//=========================================================================

class FileR : public Object
{
public:

	FileR();
	~FileR();

	//@{
	//	開く
	//	@param fname ファイル名
	//	@return 開けたかどうか
	//@}
	bool Open( const TCHAR* fname, bool always=false );

	//@{
	//	閉じる
	//@}
	void Close();

public:

	//@{ ファイルサイズ //@}
	ulong size() const;

	//@{ ファイル内容をマップしたアドレス取得 //@}
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
//	簡易ファイル書き込み
//
//	てきとーにバッファリングしつつ。
//@}
//=========================================================================

class FileW : public Object
{
public:

	FileW();
	~FileW();

	//@{ 開く //@}
	bool Open( const TCHAR* fname, bool creat=true );

	//@{ 閉じる //@}
	void Close();

	//@{ 書く //@}
	void Write( const void* buf, ulong siz );

	//@{ 一文字書く //@}
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
