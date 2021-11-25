#ifndef _KILIB_KTL_APTR_H_
#define _KILIB_KTL_APTR_H_
#include "types.h"
#ifdef _MSC_VER
#pragma warning( disable : 4284 ) // 警告：->のリターン型がうにゃうにゃ
#pragma warning( disable : 4150 ) // 警告：deleteの定義がうにょうにょ
#endif
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.KTL //@}
//@{
//	自動ポインタ
//
//	私の期待する範囲では概ね std::auto_ptr と同じ動作をすると思う…。
//	車輪の最発明ばんざーい！
//@}
//=========================================================================

template<class T>
class aptr
{
public:

	//@{ コンストラクタ //@}
	explicit aptr( T* p = NULL )
		: obj_( p ) {}

	//@{ デストラクタ //@}
	~aptr()
		{ delete obj_; }

	//@{ 所有権移動 //@}
	aptr( aptr<T>& r )
		: obj_ ( r.release() ) {}

	//@{ 所有権移動 //@}
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

	//@{ 間接演算子 //@}
	T& operator*() const
		{ return *obj_; }

	//@{ メンバ参照 //@}
	T* operator->() const
		{ return obj_; }

	//@{ ポインタ取得 //@}
	T* get() const
		{ return obj_; }

	//@{ 所有権解放 //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ 有効かどうか //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

private:

	mutable T* obj_;
};



//=========================================================================
//@{
//	自動ポインタ（配列版）
//@}
//=========================================================================

template<class T>
class aarr
{
public:

	//@{ コンストラクタ //@}
	explicit aarr( T* p = NULL )
		: obj_( p ) {}

	//@{ デストラクタ //@}
	~aarr()
		{ delete [] obj_; }

	//@{ 所有権移動 何故かbccで上手く行かない部分があるのでconst付き //@}
	aarr( const aarr<T>& r )
		: obj_ ( const_cast<aarr<T>&>(r).release() ) {}

	//@{ 所有権移動 //@}
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

	//@{ ポインタ取得 //@}
	T* get() const
		{ return obj_; }

	//@{ 所有権解放 //@}
	T* release()
		{
			T* ptr = obj_;
			obj_ = NULL;
			return ptr;
		}

	//@{ 有効かどうか //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ 配列要素アクセス //@}
	T& operator[]( int i ) const
		{ return obj_[i]; }

private:

	mutable T* obj_;
};



//=========================================================================
//@{
//	削除権専有ポインタ
//
//	「リソースの獲得はコンストラクタで・解放はデストラクタで」を
//	徹底できるならこんなの使わずに、迷わず const auto_ptr を用いる
//	べきです。べきですが、メンバ初期化リストで this を使うとVC++の
//	コンパイラに怒られるのが気持悪いので、ついこっちを使ってコンストラクタ
//	関数内で初期化してしまうのでふ…(^^;
//@}
//=========================================================================

template<class T>
class dptr
{
public:

	//@{ コンストラクタ //@}
	explicit dptr( T* p = NULL )
		: obj_( p ) {}

	//@{ デストラクタ //@}
	~dptr()
		{ delete obj_; }

	//@{ 新しいオブジェクトを所有。古いのは削除 //@}
	void operator=( T* p )
		{
			delete obj_; // 古いのは削除
			obj_ = p;
		}

	//@{ 有効かどうか //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ 間接演算子 //@}
	T& operator*() const
		{ return *obj_; }

	//@{ メンバ参照 //@}
	T* operator->() const
		{ return obj_; }

	//@{ ポインタ取得 //@}
	T* get() const
		{ return obj_; }

private:

	T* obj_;

private:

	NOCOPY(dptr<T>);
};



//=========================================================================
//@{
//	削除権専有ポインタ（配列版）
//@}
//=========================================================================

template<class T>
class darr
{
public:

	//@{ コンストラクタ //@}
	explicit darr( T* p = NULL )
		: obj_( p ) {}

	//@{ デストラクタ //@}
	~darr()
		{ delete [] obj_; }

	//@{ 新しいオブジェクトを所有。古いのは削除 //@}
	void operator=( T* p )
		{
			delete [] obj_; // 古いのは削除
			obj_ = p;
		}

	//@{ 有効かどうか //@}
	bool isValid() const
		{
			return (obj_ != NULL);
		}

public:

	//@{ 配列要素アクセス //@}
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
