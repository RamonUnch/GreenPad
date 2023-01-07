#ifndef _KILIB_WINUTIL_H_
#define _KILIB_WINUTIL_H_
#include "types.h"
#include "memory.h"
#include "ktlaptr.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.WinUtil //@}
//@{
//	クリップボード管理
//
//	OpenClipboard や CloseClipboard 辺りの呼び出しを適当に自動化します。
//@}
//=========================================================================

class Clipboard : public Object
{
public:

	//@{ 開く //@}
	Clipboard( HWND owner, bool read=true );

	//@{ 閉じる //@}
	~Clipboard();

	//@{ データ読み込み //@}
	HANDLE GetData( UINT uFormat ) const;

	//@{ 指定フォーマットのデータがクリップボード上にあるか？ //@}
	bool IsAvail( UINT uFormat ) const;

	//@{ 指定フォーマットのデータがクリップボード上にあるか？(複数) //@}
	bool IsAvail( UINT uFormats[], int num ) const;

	//@{ テキスト情報保持クラス //@}
	class Text {
		friend class Clipboard;

		mutable unicode*        str_;
		enum Tp { NEW, GALLOC } mem_;

		Text( unicode* s, Tp m ) : str_(s), mem_(m) {}
		void operator=( const Text& );

	public:
		Text( const Text& t )
			: str_(t.str_), mem_(t.mem_) { t.str_=NULL; }
		~Text()
			{
				if( str_ != NULL )
				{
					if( mem_==NEW ) delete [] str_;
					else      GlobalUnlock( str_ );
				}
			}
		const unicode* data() const { return str_; }
	};

	//@{ テキスト読み込み //@}
	Text GetUnicodeText() const;

	//@{ データ書き込み //@}
	bool SetData( UINT uFormat, HANDLE hData );

	//@{ 独自フォーマットの登録 //@}
	static UINT RegisterFormat( const TCHAR* name );

public:

	//@{ 正常に開かれているかチェック //@}
	bool isOpened() const;

private:

	bool opened_;

private:

	NOCOPY(Clipboard);
};



//-------------------------------------------------------------------------

inline bool Clipboard::isOpened() const
	{ return opened_; }

inline HANDLE Clipboard::GetData( UINT uFormat ) const
	{ return ::GetClipboardData( uFormat ); }

inline bool Clipboard::SetData( UINT uFormat, HANDLE hData )
	{ return NULL != ::SetClipboardData( uFormat, hData ); }

inline bool Clipboard::IsAvail( UINT uFormat ) const
	{ return false!=::IsClipboardFormatAvailable(uFormat); }

inline bool Clipboard::IsAvail( UINT uFormats[], int num ) const
	{ return -1!=::GetPriorityClipboardFormat(uFormats,num); }

inline UINT Clipboard::RegisterFormat( const TCHAR* name )
	{ return ::RegisterClipboardFormat(name); }



//=========================================================================
//@{
//	排他制御
//
//	名前付きMutexを扱います
//@}
//=========================================================================

class Mutex : public Object
{
public:
	Mutex( const TCHAR* name );
	~Mutex();
	bool isLocked() const;

private:
	const HANDLE mtx_;
	bool locked_;

private:
	NOCOPY(Mutex);
};



//-------------------------------------------------------------------------

inline Mutex::Mutex( const TCHAR* name )
	: mtx_( ::CreateMutex( NULL, TRUE, name ) )
	, locked_ (false)
	{
		if( mtx_ )
		{
			// Wait for Mutex ownership, in case it was already created.
			if( ::GetLastError() == ERROR_ALREADY_EXISTS )
				// Wait 10 second for ownership of fail.
				locked_ = WAIT_OBJECT_0 == ::WaitForSingleObject(mtx_, 1000);
			else
				locked_ = true; // The mutex is ours.
		}
		else
		{	// In case mutex creation failed, we prented everithing went file
			// This is required for Win32s 1.1 (at least)
			locked_ = true;
		}
	}

inline bool Mutex::isLocked() const
	{ return locked_; }

inline Mutex::~Mutex()
	{
		if( mtx_ != NULL )
		{
			if( locked_ ) ::ReleaseMutex( mtx_ );
			::CloseHandle( mtx_ );
		}
	}



//=========================================================================

}      // namespace ki
#endif // _KILIB_WINUTIL_H_
