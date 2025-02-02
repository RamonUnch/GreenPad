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
//	基本型専用の可変長配列
//
//	非常に簡素な作りです。アクセスは好きなところへ自由に可能ですが、
//	要素の追加・削除は末尾に対してのみ。ビット毎のコピーだとか、
//	色々エキセントリックな作業をしてますので、そういうことをして良い
//	型以外には使わないでください。
//@}
//=========================================================================

template <typename T, bool clean_on_destroy=true>
class A_WUNUSED storage
{
public:

	//@{
	//	コンストラクタ
	//
	//	@param alloc_size
	//		最初に確保する"メモリの"サイズ。
	//		"配列の"サイズではないことに注意。
	//@}
	explicit storage( size_t allocSize )
		: alen_( Max( allocSize, (size_t)1 ) )
		, len_ ( 0 )
		, buf_ ( static_cast<T*>(mem().Alloc(alen_*sizeof(T))) )
		{
			if( !buf_ )
				alen_=0;
		}

	~storage()
		{ if (clean_on_destroy) mem().DeAlloc( buf_, alen_*sizeof(T) ); }

	void Clear()
		{ mem().DeAlloc( buf_, alen_*sizeof(T) ); };

	//@{ 末尾に要素を追加 //@}
	bool Add( const T& obj )
		{
			if( len_ >= alen_ )
				if( !ReAllocate( alen_<<1 ) )
					return false;
			buf_[ len_++ ] = obj;
			return true;
		}

	//@{
	//	配列サイズを強制変更
	//
	//	縮小/拡大のどちらも可能。コンストラクタと違い、
	//	指定した値に基づき最大indexが変化します。
	//	@param new_size 新しいサイズ。
	//@}
	bool ForceSize( size_t newSize )
		{
			if( newSize > alen_ )
				if( !ReAllocate( newSize ) )
					return false;
			len_ = newSize;
			return true;
		}

public:

	//@{ 要素数	//@}
	size_t size() const
		{ return len_; }

	//@{ 要素取得 //@}
	T& operator[]( size_t i )
		{ return buf_[i]; }

	//@{ 要素取得(const) //@}
	const T& operator[]( size_t i ) const
		{ return buf_[i]; }

	//@{ 配列先頭のポインタを返す //@}
	const T* head() const { return buf_; }

private:

	bool ReAllocate( size_t siz )
		{
			T* newbuf = static_cast<T*>(mem().Alloc( siz*sizeof(T) ));
			if( !newbuf ) return false;
			memmove( newbuf, buf_, len_*sizeof(T) );
			mem().DeAlloc( buf_, alen_*sizeof(T) );
			alen_ = siz;
			buf_ = newbuf;
			return true;
		}

protected:

	size_t alen_;
	size_t len_;
	T*    buf_;
};

//=========================================================================
template<class T, bool free_on_del=true>
struct sstorage
{
	struct ss { size_t len_; T *buf_; };
	enum { SSZ = sizeof(ss) / sizeof(T) };

	sstorage()
	: slen_ ( 0 )
	, alen_ ( SSZ )
	{ }

	~sstorage() { if( free_on_del ) Clear(); }

	void Clear()
	{
		if( alen_ > SSZ )
			ki::mem().DeAlloc( u.s.buf_, alen_*sizeof(T) );
	}

	size_t size() const { return alen_<=SSZ? slen_ : u.s.len_; };

	bool Add( T val )
	{
		size_t prevlen;
		if( alen_ <= SSZ )
		{
			if( slen_ < alen_ )
			{
				u.a[ slen_++ ] = val;
				return true;
			}
			prevlen = slen_;
		} else
			prevlen = u.s.len_;

		if( prevlen >= alen_ )
			if( !ReAllocate( alen_<<1 ) )
				return false;
		u.s.buf_[ u.s.len_++ ] = val;
		return true;
	}

	bool ForceSize( size_t newSize )
	{
		if( alen_ <= SSZ )
		{
			slen_= newSize;
			return true;
		}

		if( newSize > alen_ )
			if( !ReAllocate( newSize ) )
				return false;

		u.s.len_ = newSize;
		return true;
	}

	bool ReAllocate( size_t siz )
	{
		T* newbuf = static_cast<T*>(ki::mem().Alloc( siz*sizeof(T) ));
		if( !newbuf ) return false;
		memmove( newbuf, alen_ > SSZ? u.s.buf_ : u.a, size()*sizeof(T) );
		if ( alen_ > SSZ )
		{
			memmove( newbuf, u.s.buf_, u.s.len_*sizeof(T) );
			ki::mem().DeAlloc( u.s.buf_, alen_*sizeof(T) );
		}
		else
		{
			memmove( newbuf, u.a, slen_*sizeof(T) );
			u.s.len_ = slen_;
		}

		alen_ = siz;
		u.s.buf_ = newbuf;

		return true;
	}

	T operator [] (size_t i) const
	{
		if (alen_ <= SSZ)
			return u.a[i];
		return u.s.buf_[i];
	}

	T& operator [] (size_t i)
	{
		if (alen_ <= SSZ)
			return u.a[i];
		return u.s.buf_[i];
	}
private:
	size_t slen_ : 3;
	size_t alen_ : sizeof(size_t)*8 - 3;
	union { struct ss s; T a[SSZ]; } u;
};

//=========================================================================
//@{
//	オブジェクト型にも使える単方向リスト
//
//	ほとんど何も出来ません。出来るのは末尾への追加と、独自の
//	iteratorによるシーケンシャルなアクセスのみ。
//@}
//=========================================================================

template <class T>
class A_WUNUSED olist
{
private:

	struct Node {
		explicit Node( const T& obj )
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
		explicit iterator( Node* p=NULL ) : ptr_(p)   {}
		T& operator*()                       { return ptr_->obj_; }
		T* operator->() const                { return &ptr_->obj_; }
		bool operator==( const iterator& i ) { return i.ptr_==ptr_; }
		bool operator!=( const iterator& i ) { return i.ptr_!=ptr_; }
		iterator& operator++()    { ptr_=ptr_->next_; return *this; }
	private:
		Node* ptr_;
	};

public:

	//@{ コンストラクタ //@}
	olist()
		: top_( NULL ) {}

	//@{ デストラクタ //@}
	~olist()
		{ empty(); }

	//@{ 空にする //@}
	void empty()
		{ delete top_; top_ = NULL; }

	//@{ 先頭 //@}
	iterator begin()
		{ return iterator(top_); }

	//@{ 末尾 //@}
	iterator end()
		{ return iterator(); }

	//@{ 末尾に要素を追加 //@}
	void Add( const T& obj )
		{
			Node* pN = new Node( obj );
			if( !pN ) return;
			(top_ == NULL) ? top_=pN : top_->Add( pN );
		}

	//@{ 指定要素を削除 //@}
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

	//@{ 指定要素以降全てを削除 //@}
	void DelAfter( iterator d )
		{
			if( d != end() )
			{
				if( d == begin() )
				{
					empty();
				}
				else
				{
					Node *p=top_, *q=NULL;
					for( ; p!=NULL; q=p, p=p->next_ )
						if( &p->obj_ == &*d )
							break;
					delete p;
					if( q )
						q->next_ = NULL;
				}
			}
		}

private:

	Node* top_;
	NOCOPY(olist);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_KTL_ARRAY_H_
