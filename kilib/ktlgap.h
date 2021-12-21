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
//	基本型専用ギャップバッファ
//
//	小耳に挟んだギャップバッファというもの。
//	ものすごい勘違いをして別物ができているかもしれませんが、
//	細かいことはあまり気にしないでください。
//	配列のようにランダムアクセス可能で、
//	同一箇所への連続した挿入/削除が速いというデータ構造です。
//	( 下の図で言うと、gap_startへの挿入/削除にはデータの移動が必要ない )
//	<pre>
//@@  D  <--0
//@@  D
//@@  |  <--gap_start
//@@  |
//@@  D  <--gap_end
//@@  D
//@@     <--array_end
//	</pre>
//	メモリイメージをそのままコピーしてる実装なので、
//	プリミティブ型以外では絶対に使わないこと。
//@}
//=========================================================================

template<class T>
class gapbuf : public Object
{
public:

	//@{
	//	コンストラクタ
	//	@param alloc_size
	//		最初に確保する"メモリの"サイズ。
	//		"配列の"サイズではないことに注意。
	//@}
	explicit gapbuf( ulong alloc_size=40 )
		: alen_( Max(alloc_size, (ulong)10) )
		, gs_  ( 0 )
		, ge_  ( alen_ )
		, buf_ ( new T[alen_] )
		{}
	~gapbuf()
		{ delete [] buf_; }

	//@{ 要素挿入 //@}
	void InsertAt( ulong i, const T& x )
		{
			MakeGapAt( i );
			buf_[gs_++] = x;

			if( gs_==ge_ )
				Reallocate( alen_<<1 );
		}

	//@{ 要素挿入(複数) //@}
	void InsertAt( ulong i, const T* x, ulong len )
		{
			MakeGapAt( size() );
			MakeGapAt( i );
			if( ge_-gs_ <= len )
				Reallocate( Max(alen_+len+1, alen_<<1) );

			memmove( buf_+gs_, x, len*sizeof(T) );
			gs_ += len;
		}

	//@{ 末尾に要素を追加 //@}
	void Add( const T& x )
		{ InsertAt( size(), x ); }

	//@{ 末尾に要素を追加(複数) //@}
	void Add( const T* x, ulong len )
		{ InsertAt( size(), x, len ); }

	//@{ 要素削除 //@}
	void RemoveAt( ulong i, ulong len=1 )
		{
			if( i <= gs_ && gs_ <= i+len )
			{
				// この場合はメモリ移動の必要がない
				// まず前半を削除
				len -= (gs_-i);
				gs_ = i;
			}
			else
			{
				MakeGapAt( i );
			}

			// 後半を削除
			ge_ += len;
		}

	//@{ 要素削除(全部) //@}
	void RemoveAll()
		{ RemoveAt( 0, size() ); }

	//@{ 要素削除(指定index以降全部) //@}
	void RemoveToTail( ulong i )
		{ RemoveAt( i, size()-i ); }

	//@{ 要素コピー(指定index以降全部) //@}
	ulong CopyToTail( ulong i, T* x )
		{ return CopyAt( i, size()-i, x ); }

	//@{ 要素コピー //@}
	ulong CopyAt( ulong i, ulong len, T* x )
		{
			ulong copyed=0;
			if( i < gs_ )
			{
				// 前半
				copyed += Min( len, gs_-i );
				memmove( x, buf_+i, copyed*sizeof(T) );
				x   += copyed;
				len -= copyed;
				i   += copyed;
			}

			// 後半
			memmove( x, buf_+(i-gs_)+ge_, len*sizeof(T) );
			return copyed + len;
		}

public:

	//@{ 要素数 //@}
	ulong size() const
		{ return alen_ - (ge_-gs_); }

	//@{ 要素取得 //@}
	T& operator[]( ulong i )
		{ return buf_[ ( i<gs_ ) ? i : i+(ge_-gs_) ]; }

	//@{ 要素取得(const) //@}
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
				memmove( buf_+ge_, buf_+i, (gs_-i)*sizeof(T) );
			}
			else if( i>gs_ )
			{
				int j = i+(ge_-gs_);
				memmove( buf_+gs_, buf_+ge_, (j-ge_)*sizeof(T) );
				ge_ = j;
			}
			gs_ = i;
		}

	void Reallocate( ulong newalen )
		{
			T *tmp = new T[newalen], *old=buf_;
			const ulong tail = alen_-ge_;

			memmove( tmp, old, gs_*sizeof(T) );
			memmove( tmp+newalen-tail, old+ge_, tail*sizeof(T) );
			delete [] old;

			buf_  = tmp;
			ge_   = newalen-tail;
			alen_ = newalen;
		}

private:

	NOCOPY(gapbuf<T>);
};



//=========================================================================
//@{
//	gapbuf + smartptr のふり
//
//	要素削除時にdeleteを実行しっちゃったりするバージョン。
//	任意オブジェクトをギャップバッファで使いたいときは
//	これでてきとーに代用すべし。
//@}
//=========================================================================

template<class T>
class gapbufobj : public gapbuf<T*>
{
public:

	explicit gapbufobj( ulong alloc_size=40 )
		: gapbuf<T*>( alloc_size )
		{ }

	void RemoveAt( ulong i, ulong len=1 )
		{
			ulong& gs_ = gapbuf<T*>::gs_;
			ulong& ge_ = gapbuf<T*>::ge_;
			T**&   buf_= gapbuf<T*>::buf_;

			if( i <= gs_ && gs_ <= i+len )
			{
				// 前半を削除
				for( ulong j=i, ed=gs_; j<ed; ++j )
					delete buf_[j];

				len -= (gs_-i);
				gs_  = i;
			}
			else
			{
				gapbuf<T*>::MakeGapAt( i );
			}

			// 後半を削除
			for( ulong j=ge_, ed=ge_+len; j<ed; ++j )
				delete buf_[j];
			ge_ = ge_+len;
		}

	~gapbufobj()
		{ RemoveAt( 0, gapbuf<T*>::size() ); }

	void RemoveAll( ulong i )
		{ RemoveAt( 0, gapbuf<T*>::size() ); }

	void RemoveToTail( ulong i )
		{ RemoveAt( i, gapbuf<T*>::size()-i ); }

public:

	T& operator[]( ulong i )
		{ return *gapbuf<T*>::operator[](i); }

	const T& operator[]( ulong i ) const
		{ return *gapbuf<T*>::operator[](i); }

private:

	NOCOPY(gapbufobj<T>);
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_KTL_GAP_H_
