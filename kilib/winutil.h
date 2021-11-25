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
//	�N���b�v�{�[�h�Ǘ�
//
//	OpenClipboard �� CloseClipboard �ӂ�̌Ăяo����K���Ɏ��������܂��B
//@}
//=========================================================================

class Clipboard : public Object
{
public:

	//@{ �J�� //@}
	Clipboard( HWND owner, bool read=true );

	//@{ ���� //@}
	~Clipboard();

	//@{ �f�[�^�ǂݍ��� //@}
	HANDLE GetData( UINT uFormat ) const;

	//@{ �w��t�H�[�}�b�g�̃f�[�^���N���b�v�{�[�h��ɂ��邩�H //@}
	bool IsAvail( UINT uFormat ) const;

	//@{ �w��t�H�[�}�b�g�̃f�[�^���N���b�v�{�[�h��ɂ��邩�H(����) //@}
	bool IsAvail( UINT uFormats[], int num ) const;

	//@{ �e�L�X�g���ێ��N���X //@}
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
					if( mem_==NEW ) delete [] str_;
					else      GlobalUnlock( str_ );
			}
		const unicode* data() const { return str_; }
	};

	//@{ �e�L�X�g�ǂݍ��� //@}
	Text GetUnicodeText() const;

	//@{ �f�[�^�������� //@}
	bool SetData( UINT uFormat, HANDLE hData );

	//@{ �Ǝ��t�H�[�}�b�g�̓o�^ //@}
	static UINT RegisterFormat( const TCHAR* name );

public:

	//@{ ����ɊJ����Ă��邩�`�F�b�N //@}
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
//	�r������
//
//	���O�t��Mutex�������܂�
//@}
//=========================================================================

class Mutex : public Object
{
public:
	Mutex( const TCHAR* name );
	~Mutex();

private:
	const HANDLE mtx_;

private:
	NOCOPY(Mutex);
};



//-------------------------------------------------------------------------

inline Mutex::Mutex( const TCHAR* name )
	: mtx_( ::CreateMutex( NULL, TRUE, name ) ) {}

inline Mutex::~Mutex()
	{ if( mtx_ != NULL ) ::ReleaseMutex( mtx_ ), ::CloseHandle( mtx_ ); }



//=========================================================================

}      // namespace ki
#endif // _KILIB_WINUTIL_H_
