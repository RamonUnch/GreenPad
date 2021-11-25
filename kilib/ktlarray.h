#ifndef _KILIB_KTL_ARRAY_H_
#define _KILIB_KTL_ARRAY_H_
#include "types.h"
#include "memory.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	��{�^��p�̉ϒ��z��
//
//	���Ɋȑf�ȍ��ł��B�A�N�Z�X�͍D���ȂƂ���֎��R�ɉ\�ł����A
//	�v�f�̒ǉ��E�폜�͖����ɑ΂��Ă̂݁B�r�b�g���̃R�s�[���Ƃ��A
//	�F�X�G�L�Z���g���b�N�ȍ�Ƃ����Ă܂��̂ŁA�����������Ƃ����ėǂ�
//	�^�ȊO�ɂ͎g��Ȃ��ł��������B
//@}
//=========================================================================

template <typename T>
class storage : public Object
{
public:

	//@{
	//	�R���X�g���N�^
	//
	//	@param alloc_size
	//		�ŏ��Ɋm�ۂ���"��������"�T�C�Y�B
	//		"�z���"�T�C�Y�ł͂Ȃ����Ƃɒ��ӁB
	//@}
	explicit storage( ulong allocSize=20 )
		: alen_( Max( allocSize, 1UL ) )
		, len_ ( 0 )
		, buf_ ( static_cast<T*>(mem().Alloc(alen_*sizeof(T))) )
		{}

	~storage()
		{ mem().DeAlloc( buf_, alen_*sizeof(T) ); }

	//@{ �����ɗv�f��ǉ� //@}
	void Add( const T& obj )
		{
			if( len_ >= alen_ )
				ReAllocate( alen_<<2 );
			buf_[ len_++ ] = obj;
		}

	//@{
	//	�z��T�C�Y�������ύX
	//
	//	�k��/�g��̂ǂ�����\�B�R���X�g���N�^�ƈႢ�A
	//	�w�肵���l�Ɋ�Â��ő�index���ω����܂��B
	//	@param new_size �V�����T�C�Y�B
	//@}
	void ForceSize( ulong newSize )
		{
			if( newSize > alen_ )
				ReAllocate( newSize );
			len_ = newSize;
		}

public:

	//@{ �v�f��	//@}
	ulong size() const
		{ return len_; }

	//@{ �v�f�擾 //@}
	T& operator[]( size_t i )
		{ return buf_[i]; }

	//@{ �v�f�擾(const) //@}
	const T& operator[]( size_t i ) const
		{ return buf_[i]; }

	//@{ �z��擪�̃|�C���^��Ԃ� //@}
	const T* head() const
		{ return buf_; }

private:

	void ReAllocate( ulong siz )
		{
			ulong p = alen_*sizeof(T);
			T* newbuf = static_cast<T*>
				(mem().Alloc( (alen_=siz)*sizeof(T) ));
			memmove( newbuf, buf_, len_*sizeof(T) );
			mem().DeAlloc( buf_, p );
			buf_ = newbuf;
		}

private:

	ulong alen_;
	ulong len_;
	T*    buf_;

private:

	NOCOPY(storage<T>);
};



//=========================================================================
//@{
//	�I�u�W�F�N�g�^�ɂ��g����P�������X�g
//
//	�قƂ�ǉ����o���܂���B�o����͖̂����ւ̒ǉ��ƁA�Ǝ���
//	iterator�ɂ��V�[�P���V�����ȃA�N�Z�X�̂݁B
//@}
//=========================================================================

template <class T>
class olist : public Object
{
private:

	struct Node {
		Node( const T& obj )
			: obj_ ( obj ), next_( NULL ) {}
		~Node()
			{ delete next_; }
		Node* Add( Node* pN )
			{ return next_==NULL ? next_=pN : next_->Add(pN); }
		T     obj_;
		Node* next_;
	};

public:

	struct iterator {
		iterator( Node* p=NULL ) : ptr_(p)   {}
		T& operator*()                       { return ptr_->obj_; }
		T* operator->() const                { return &ptr_->obj_; }
		bool operator==( const iterator& i ) { return i.ptr_==ptr_; }
		bool operator!=( const iterator& i ) { return i.ptr_!=ptr_; }
		iterator& operator++()    { ptr_=ptr_->next_; return *this; }
	private:
		Node* ptr_;
	};

public:

	//@{ �R���X�g���N�^ //@}
	olist()
		: top_( NULL ) {}

	//@{ �f�X�g���N�^ //@}
	~olist()
		{ empty(); }

	//@{ ��ɂ��� //@}
	void empty()
		{ delete top_; top_ = NULL; }

	//@{ �擪 //@}
	iterator begin()
		{ return iterator(top_); }

	//@{ ���� //@}
	iterator end()
		{ return iterator(); }

	//@{ �����ɗv�f��ǉ� //@}
	void Add( const T& obj )
		{
			Node* pN = new Node( obj );
			(top_ == NULL) ? top_=pN : top_->Add( pN );
		}

	//@{ �w��v�f���폜 //@}
	void Del( iterator d )
		{
			if( d != end() )
			{
				Node *p=top_, *q=NULL;
				for( ; p!=NULL; q=p,p=p->next_ )
					if( &p->obj_ == &*d )
						break;
				if( q != NULL )
					q->next_ = p->next_;
				else
					top_ = p->next_;
				p->next_ = NULL;
				delete p;
			}
		}

	//@{ �w��v�f�ȍ~�S�Ă��폜 //@}
	void DelAfter( iterator d )
		{
			if( d != end() )
				if( d == begin() )
				{
					empty();
				}
				else
				{
					Node *p=top_, *q=NULL;
					for( ; p!=NULL; q=p,p=p->next_ )
						if( &p->obj_ == &*d )
							break;
					delete p;
					q->next_ = NULL;
				}
		}

private:

	Node* top_;
	NOCOPY(olist<T>);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_KTL_ARRAY_H_
