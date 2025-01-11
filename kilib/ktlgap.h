#ifndef _KILIB_KTL_GAP_H_
#define _KILIB_KTL_GAP_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	��{�^��p�M���b�v�o�b�t�@
//
//	�����ɋ��񂾃M���b�v�o�b�t�@�Ƃ������́B
//	���̂��������Ⴂ�����ĕʕ����ł��Ă��邩������܂��񂪁A
//	�ׂ������Ƃ͂��܂�C�ɂ��Ȃ��ł��������B
//	�z��̂悤�Ƀ����_���A�N�Z�X�\�ŁA
//	����ӏ��ւ̘A�������}��/�폜�������Ƃ����f�[�^�\���ł��B
//	( ���̐}�Ō����ƁAgap_start�ւ̑}��/�폜�ɂ̓f�[�^�̈ړ����K�v�Ȃ� )
//	<pre>
//@@  D  <--0
//@@  D
//@@  |  <--gap_start
//@@  |
//@@  D  <--gap_end
//@@  D
//@@     <--array_end
//	</pre>
//	�������C���[�W�����̂܂܃R�s�[���Ă�����Ȃ̂ŁA
//	�v���~�e�B�u�^�ȊO�ł͐�΂Ɏg��Ȃ����ƁB
//@}
//=========================================================================

template<class T>
class A_WUNUSED gapbuf
{
public:

	//@{
	//	�R���X�g���N�^
	//	@param alloc_size
	//		�ŏ��Ɋm�ۂ���"��������"�T�C�Y�B
	//		"�z���"�T�C�Y�ł͂Ȃ����Ƃɒ��ӁB
	//@}
	explicit gapbuf( ulong alloc_size=32 )
		: alen_( Max(alloc_size, (ulong)16) )
		, gs_  ( 0 )
		, ge_  ( alen_ )
		, buf_ ( (T*) ::malloc( sizeof(T) * alen_ )  )
		{ if( !buf_ ) alen_ = ge_ = 0; }
	~gapbuf()
		{ ::free( buf_ ); }

	//@{ �v�f�}�� //@}
	bool InsertAt( ulong i, const T& x )
		{
			// Try to get more room if needed.
			if( gs_+1 >= ge_ )
				if( !Reallocate( alen_<<1 ) )
					return false;

			MakeGapAt( i );
			buf_[gs_++] = x;
			return true;
		}

	//@{ �v�f�}��(����) //@}
	void InsertAt( ulong i, const T* x, ulong len )
		{
			if( ge_-gs_ <= len )
				if( !Reallocate( Max(alen_+len+1, alen_<<1) ) )
					return;

			MakeGapAt( size() );
			MakeGapAt( i );

			memmove( (char*)(buf_+gs_), (char*)x, len*sizeof(T) );
			gs_ += len;
		}

	//@{ �����ɗv�f��ǉ� //@}
	void Add( const T& x )
		{ InsertAt( size(), x ); }

	//@{ �����ɗv�f��ǉ�(����) //@}
	void Add( const T* x, ulong len )
		{ InsertAt( size(), x, len ); }

	//@{ �v�f�폜 //@}
	void RemoveAt( ulong i, ulong len=1 )
		{
			if( i <= gs_ && gs_ <= i+len )
			{
				// ���̏ꍇ�̓������ړ��̕K�v���Ȃ�
				// �܂��O�����폜
				len -= (gs_-i);
				gs_ = i;
			}
			else
			{
				MakeGapAt( i );
			}

			// �㔼���폜
			ge_ += len;
		}

	//@{ �v�f�폜(�S��) //@}
	void RemoveAll()
		{ RemoveAt( 0, size() ); }

	//@{ �v�f�폜(�w��index�ȍ~�S��) //@}
	void RemoveToTail( ulong i )
		{ RemoveAt( i, size()-i ); }

	//@{ �v�f�R�s�[(�w��index�ȍ~�S��) //@}
	ulong CopyToTail( ulong i, T* x )
		{ return CopyAt( i, size()-i, x ); }

	//@{ �v�f�R�s�[ //@}
	ulong CopyAt( ulong i, ulong len, T* x )
		{
			ulong copyed=0;
			if( i < gs_ )
			{
				// �O��
				copyed += Min( len, gs_-i );
				memmove( (char*)x, (char*)(buf_+i), copyed*sizeof(T) );
				x   += copyed;
				len -= copyed;
				i   += copyed;
			}

			// �㔼
			memmove( (char*)x, (char*)(buf_+(i-gs_)+ge_), len*sizeof(T) );
			return copyed + len;
		}

public:

	//@{ �v�f�� //@}
	ulong size() const
		{ return alen_ - (ge_-gs_); }

	//@{ �v�f�擾 //@}
	T& operator[]( ulong i )
		{ return buf_[ ( i<gs_ ) ? i : i+(ge_-gs_) ]; }

	//@{ �v�f�擾(const) //@}
	const T& operator[]( ulong i ) const
		{ return buf_[ ( i<gs_ ) ? i : i+(ge_-gs_) ]; }

protected:

	ulong alen_;
	ulong gs_;
	ulong ge_;
	T*    buf_;

protected:

	void MakeGapAt( ulong i )
		{
			if( i<gs_ )
			{
				ge_ -= (gs_-i);
				memmove( (char*)(buf_+ge_), (char*)(buf_+i), (gs_-i)*sizeof(T) );
			}
			else if( i>gs_ )
			{
				int j = i+(ge_-gs_);
				memmove( (char*)(buf_+gs_), (char*)(buf_+ge_), (j-ge_)*sizeof(T) );
				ge_ = j;
			}
			gs_ = i;
		}
	bool Reallocate( ulong newalen )
		{
			T *tmp = (T *)malloc( sizeof(T) * newalen ), *old=buf_;
			if( !tmp ) return false;
			const ulong tail = alen_-ge_;

			memmove( (char*)tmp, (char*)old, gs_*sizeof(T) );
			memmove( (char*)(tmp+newalen-tail), (char*)(old+ge_), tail*sizeof(T) );
			free( old );

			buf_  = tmp;
			ge_   = newalen-tail;
			alen_ = newalen;
			return true;
		}

/*	bool Reallocate( ulong newalen )
		{
			T *tmp = (T *)::realloc( (void *)buf_, sizeof(T) * newalen );
			if( !tmp )
				return false;

			const ulong tail = alen_-ge_;
			memmove( (char*)(tmp+newalen-tail), (char*)(buf_+ge_), tail*sizeof(T) );
			buf_  = tmp;
			ge_   = newalen-tail;
			alen_ = newalen;
			return true;
		}*/

private:

	NOCOPY(gapbuf);
};



//=========================================================================
//@{
//	gapbuf + smartptr �̂ӂ�
//
//	�v�f�폜����delete�����s������������肷��o�[�W�����B
//	�C�ӃI�u�W�F�N�g���M���b�v�o�b�t�@�Ŏg�������Ƃ���
//	����łĂ��Ɓ[�ɑ�p���ׂ��B
//@}
//=========================================================================

//template<class T>
//class A_WUNUSED gapbufobj : public gapbuf<T*>
//{
//public:
//
//	explicit gapbufobj( ulong alloc_size=32 )
//		: gapbuf<T*>( alloc_size )
//		{ }
//
//	void RemoveAt( ulong i, ulong len=1 )
//		{
//			ulong& gs_ = gapbuf<T*>::gs_;
//			ulong& ge_ = gapbuf<T*>::ge_;
//			T**&   buf_= gapbuf<T*>::buf_;
//
//			if( i <= gs_ && gs_ <= i+len )
//			{
//				// �O�����폜
//				for( ulong j=i, ed=gs_; j<ed; ++j )
//					delete buf_[j];
//
//				len -= (gs_-i);
//				gs_  = i;
//			}
//			else
//			{
//				gapbuf<T*>::MakeGapAt( i );
//			}
//
//			// �㔼���폜
//			for( ulong j=ge_, ed=ge_+len; j<ed; ++j )
//				delete buf_[j];
//			ge_ = ge_+len;
//		}
//
//	~gapbufobj()
//		{ RemoveAt( 0, gapbuf<T*>::size() ); }
//
//	void RemoveAll( ulong i )
//		{ RemoveAt( 0, gapbuf<T*>::size() ); }
//
//	void RemoveToTail( ulong i )
//		{ RemoveAt( i, gapbuf<T*>::size()-i ); }
//
//public:
//
//	T& operator[]( ulong i )
//		{ return *gapbuf<T*>::operator[](i); }
//
//	const T& operator[]( ulong i ) const
//		{ return *gapbuf<T*>::operator[](i); }
//
//private:
//
//	NOCOPY(gapbufobj);
//};
//

template<class T>
class A_WUNUSED gapbufobjnoref : public gapbuf<T>
{
public:

	explicit gapbufobjnoref( ulong alloc_size=32 )
		: gapbuf<T>( alloc_size )
		{ }

	void RemoveAt( ulong i, ulong len=1 )
		{
			ulong& gs_ = gapbuf<T>::gs_;
			ulong& ge_ = gapbuf<T>::ge_;
			T*&    buf_= gapbuf<T>::buf_;

			if( i <= gs_ && gs_ <= i+len )
			{
				// �O�����폜
				for( ulong j=i, ed=gs_; j<ed; ++j )
					buf_[j].Clear();

				len -= (gs_-i);
				gs_  = i;
			}
			else
			{
				gapbuf<T>::MakeGapAt( i );
			}

			// �㔼���폜
			for( ulong j=ge_, ed=ge_+len; j<ed; ++j )
				buf_[j].Clear();
			ge_ = ge_+len;
			
			// If the buffer is widely oversized, reduce it.
			if( gapbuf<T>::alen_ > 128 && gapbuf<T>::size()  <= gapbuf<T>::alen_ >> 2 )
				gapbuf<T>::Reallocate( gapbuf<T>::size() );
		}

	~gapbufobjnoref()
		{ RemoveAt( 0, gapbuf<T>::size() ); }

	void RemoveAll( ulong i )
		{ RemoveAt( 0, gapbuf<T>::size() ); }

	void RemoveToTail( ulong i )
		{ RemoveAt( i, gapbuf<T>::size()-i ); }

public:

	T& operator[]( ulong i )
		{ return gapbuf<T>::operator[](i); }

	const T& operator[]( ulong i ) const
		{ return gapbuf<T>::operator[](i); }

private:

	NOCOPY(gapbufobjnoref);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_KTL_GAP_H_
