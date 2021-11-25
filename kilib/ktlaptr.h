#ifndef _KILIB_KTL_APTR_H_
#define _KILIB_KTL_APTR_H_
#include "types.h"
#ifdef _MSC_VER
#pragma warning( disable : 4284 ) // �x���F->�̃��^�[���^�����ɂႤ�ɂ�
#pragma warning( disable : 4150 ) // �x���Fdelete�̒�`�����ɂ傤�ɂ�
#endif
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	�����|�C���^
//
//	���̊��҂���͈͂ł͊T�� std::auto_ptr �Ɠ������������Ǝv���c�B
//	�ԗւ̍Ŕ����΂񂴁[���I
//@}
//=========================================================================

template<class T>
class aptr
{
public:

	//@{ �R���X�g���N�^ //@}
	explicit aptr( T* p = NULL )
		: obj_( p ) {}

	//@{ �f�X�g���N�^ //@}
	~aptr()
		{ delete obj_; }

	//@{ ���L���ړ� //@}
	aptr( aptr<T>& r )
		: obj_ ( r.release() ) {}

	//@{ ���L���ړ� //@}
	aptr<T>& operator=( aptr<T>& r )
		{
			if( obj_ != r.obj_ )
			{
				delete obj_;
				obj_ = r.release();
			}
			return *this;
		}

public:

	//@{ �Ԑډ��Z�q //@}
	T& operator*() const
		{ return *obj_; }

	//@{ �����o�Q�� //@}
	T* operator->() const
		{ return obj_; }

	//@{ �|�C���^�擾 //@}
	T* get() const
		{ return obj_; }

	//@{ ���L����� //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ �L�����ǂ��� //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

private:

	mutable T* obj_;
};



//=========================================================================
//@{
//	�����|�C���^�i�z��Łj
//@}
//=========================================================================

template<class T>
class aarr
{
public:

	//@{ �R���X�g���N�^ //@}
	explicit aarr( T* p = NULL )
		: obj_( p ) {}

	//@{ �f�X�g���N�^ //@}
	~aarr()
		{ delete [] obj_; }

	//@{ ���L���ړ� ���̂�bcc�ŏ�肭�s���Ȃ�����������̂�const�t�� //@}
	aarr( const aarr<T>& r )
		: obj_ ( const_cast<aarr<T>&>(r).release() ) {}

	//@{ ���L���ړ� //@}
	aarr<T>& operator=( aarr<T>& r )
		{
			if( obj_ != r.obj_ )
			{
				delete [] obj_;
				obj_ = r.release();
			}
			return *this;
		}

public:

	//@{ �|�C���^�擾 //@}
	T* get() const
		{ return obj_; }

	//@{ ���L����� //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ �L�����ǂ��� //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ �z��v�f�A�N�Z�X //@}
	T& operator[]( int i ) const
		{ return obj_[i]; }

private:

	mutable T* obj_;
};



//=========================================================================
//@{
//	�폜����L�|�C���^
//
//	�u���\�[�X�̊l���̓R���X�g���N�^�ŁE����̓f�X�g���N�^�Łv��
//	�O��ł���Ȃ炱��Ȃ̎g�킸�ɁA���킸 const auto_ptr ��p����
//	�ׂ��ł��B�ׂ��ł����A�����o���������X�g�� this ���g����VC++��
//	�R���p�C���ɓ{����̂��C�������̂ŁA�����������g���ăR���X�g���N�^
//	�֐����ŏ��������Ă��܂��̂łӁc(^^;
//@}
//=========================================================================

template<class T>
class dptr
{
public:

	//@{ �R���X�g���N�^ //@}
	explicit dptr( T* p = NULL )
		: obj_( p ) {}

	//@{ �f�X�g���N�^ //@}
	~dptr()
		{ delete obj_; }

	//@{ �V�����I�u�W�F�N�g�����L�B�Â��͍̂폜 //@}
	void operator=( T* p )
		{
			delete obj_; // �Â��͍̂폜
			obj_ = p;
		}

	//@{ �L�����ǂ��� //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ �Ԑډ��Z�q //@}
	T& operator*() const
		{ return *obj_; }

	//@{ �����o�Q�� //@}
	T* operator->() const
		{ return obj_; }

	//@{ �|�C���^�擾 //@}
	T* get() const
		{ return obj_; }

private:

	T* obj_;

private:

	NOCOPY(dptr<T>);
};



//=========================================================================
//@{
//	�폜����L�|�C���^�i�z��Łj
//@}
//=========================================================================

template<class T>
class darr
{
public:

	//@{ �R���X�g���N�^ //@}
	explicit darr( T* p = NULL )
		: obj_( p ) {}

	//@{ �f�X�g���N�^ //@}
	~darr()
		{ delete [] obj_; }

	//@{ �V�����I�u�W�F�N�g�����L�B�Â��͍̂폜 //@}
	void operator=( T* p )
		{
			delete [] obj_; // �Â��͍̂폜
			obj_ = p;
		}

	//@{ �L�����ǂ��� //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ �z��v�f�A�N�Z�X //@}
	T& operator[]( int i ) const
		{ return obj_[i]; }

private:

	T* obj_;

private:

	NOCOPY(darr<T>);
};



//=========================================================================

#ifdef _MSC_VER
#pragma warning( default : 4150 )
#pragma warning( default : 4284 )
#endif
}      // namespace ki
#endif // _KILIB_KTL_APTR_H_
