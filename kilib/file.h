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
	size_t size() const
		{ return size_; };

	//@{ ファイル内容をマップしたアドレス取得 //@}
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
//	簡易ファイル書き込み
//
//	てきとーにバッファリングしつつ。
//@}
//=========================================================================

class FileW
{
public:

	FileW();
	~FileW();

	//@{ 開く //@}
	bool Open( const TCHAR* fname, bool creat=true );

	//@{ 閉じる //@}
	void Close();

	//@{ 書く //@}
	void Write( const void* buf, size_t siz );

	//@{ 一文字書く //@}
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
