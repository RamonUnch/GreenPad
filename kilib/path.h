#ifndef _KILIB_PATH_H_
#define _KILIB_PATH_H_
#include "types.h"
#include "string.h"
#ifndef __ccdoc__
namespace ki {
#endif


	
//=========================================================================
//@{ @pkg ki.StdLib //@}
//@{
//	�t�@�C��������
//
//	��{�I�ɂ͕�����ł����A���Ƀt�@�C�����Ƃ��Ĉ����Ƃ��ɕ֗���
//	�N���X�ł��B���O�����̂ݎ��o���Ƃ��g���q�̂ݎ��o���Ƃ��A
//	����������Ă���Ƃ��F�X�B
//@}
//=========================================================================

class Path : public String
{
public:

	Path() {}

	//@{ �ʂ�Path�̃R�s�[ //@}
	Path( const Path&   s )              : String( s ){}
	Path( const String& s )              : String( s ){}
	Path( const TCHAR*  s, long siz=-1 ) : String( s, siz ){}

	//@{ �P����� //@}
	Path& operator=( const Path&   s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const String& s )
		{ return static_cast<Path&>(String::operator=(s)); }
	Path& operator=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator=(s)); }

	//@{ ���Z��� //@}
	Path& operator+=( const Path&   s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const String& s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( const TCHAR*  s )
		{ return static_cast<Path&>(String::operator+=(s)); }
	Path& operator+=( TCHAR c )
		{ return static_cast<Path&>(String::operator+=(c)); }

	//@{ ����p�X�擾���ď����� //@}
	explicit Path( int nPATH, bool bs=true )
		{ BeSpecialPath( nPATH, bs ); }

	//@{ ����p�X�擾 //@}
	Path& BeSpecialPath( int nPATH, bool bs=true );

	//@{ ����p�X�w��p�萔 //@}
	enum { Win=0x1787, Sys, Tmp, Exe, Cur, ExeName,
			Snd=CSIDL_SENDTO, Dsk=CSIDL_DESKTOPDIRECTORY };

	//@{ �Ō�Ƀo�b�N�X���b�V��������(true)/����Ȃ�(false) //@}
	Path& BeBackSlash( bool add );

	//@{ �h���C�u���Ȃ������[�g�̂� //@}
	Path& BeDriveOnly();

	//@{ �f�B���N�g�����̂� //@}
	Path& BeDirOnly();

	//@{ �Z���p�X�� //@}
	Path& BeShortStyle();

	//@{ �t�@�C���������͊m���ɒ��� //@}
	Path& BeShortLongStyle();

	//@{ ...�Ƃ������ĒZ�� //@}
	String CompactIfPossible(int Mx);

	//@{ �f�B���N�g�����ȊO //@}
	const TCHAR* name() const;

	//@{ �Ō�̊g���q //@}
	const TCHAR* ext() const;

	//@{ �ŏ���.�ȍ~�S���ƌ��Ȃ����g���q //@}
	const TCHAR* ext_all() const;

	//@{ �f�B���N�g�����ƍŌ�̊g���q�����������O���� //@}
	Path body() const;

	//@{ �f�B���N�g�����ƍŏ���.�ȍ~�S�������������O���� //@}
	Path body_all() const;

public:

	//@{ �t�@�C�����ǂ��� //@}
	bool isFile() const;

	//@{ �f�B���N�g�����ǂ��� //@}
	bool isDirectory() const;

	//@{ ���݂��邩�ǂ����BisFile() || isDirectory() //@}
	bool exist() const;

	//@{ �ǂݎ���p���ǂ��� //@}
	bool isReadOnly() const;

public:

	static const TCHAR* name( const TCHAR* str );
	static const TCHAR* ext( const TCHAR* str );
	static const TCHAR* ext_all( const TCHAR* str );
};



//-------------------------------------------------------------------------

inline bool Path::isFile() const
	{ return 0==(::GetFileAttributes(c_str())&FILE_ATTRIBUTE_DIRECTORY); }

inline bool Path::isDirectory() const
	{ DWORD x=::GetFileAttributes(c_str());
	  return x!=0xffffffff && (x&FILE_ATTRIBUTE_DIRECTORY)!=0; }

inline bool Path::exist() const
	{ return 0xffffffff != ::GetFileAttributes(c_str()); }

inline bool Path::isReadOnly() const
	{ DWORD x=::GetFileAttributes(c_str());
	  return x!=0xffffffff && (x&FILE_ATTRIBUTE_READONLY)!=0; }

inline const TCHAR* Path::name() const
	{ return name(c_str()); }

inline const TCHAR* Path::ext() const
	{ return ext(c_str()); }

inline const TCHAR* Path::ext_all() const
	{ return ext_all(c_str()); }



//=========================================================================

}      // namespace ki
#endif // _KILIB_PATH_H_
