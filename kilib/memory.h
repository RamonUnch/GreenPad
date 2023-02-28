#ifndef _KILIB_MEMORY_H_
#define _KILIB_MEMORY_H_
#include "types.h"
#include "thread.h"
#ifndef __ccdoc__
namespace ki {
#endif


// W版ではHeapAllocを直接呼び出すバージョンを使う
//#if !defined(_UNICODE) && defined(SUPERTINY)
//	#define USE_ORIGINAL_MEMMAN
//#endif

#ifdef WIN64
	// 小規模と見なすオブジェクトの最大サイズ, Maximum size of objects considered small
	#define SMALL_MAX 255
	// 一度に確保するヒープブロックのサイズ, Size of heap block to be allocated at one time
	#define BLOCK_SIZ 8192
#else
	#ifdef STACK_MEM_POOLS
	#define SMALL_MAX 128
	#define BLOCK_SIZ 4096
	#else
	#define SMALL_MAX 255
	#define BLOCK_SIZ 4096
	#endif
#endif
// 内部実装
struct MemBlock;



//=========================================================================
//@{ @pkg ki.Memory //@}
//@{
//	メモリ割り当て・解放機構
//
//	SUPERTINYオプションを付けてコンパイルすると、標準の
//	mallocやfreeを使えなくなるため、HeapAlloc等のAPIを
//	直接呼び出す必要が出てきます。しかし、こいつらを本当に
//	毎回直に呼んでいると、遅い。もうアホかと、バカかと、
//	って勢いで遅い。そこで、主にnewで動的に小規模メモリを
//	確保することに主眼を据えた簡単なアロケータを使うことにしました。
//
//	<a href="http://cseng.aw.com/book/0,3828,0201704315,00.html">loki</a>
//	ライブラリほぼそのまんまな実装です。
//@}
//=========================================================================

class MemoryManager : public EzLockable
{
public:

	//@{ メモリ割り当て //@}
	void* Alloc( size_t siz );

	//@{ メモリ解放 //@}
	void DeAlloc( void* ptr, size_t siz );

#ifdef USE_ORIGINAL_MEMMAN
private:
	struct FixedSizeMemBlockPool
	{
		void Construct( byte siz );
		void Destruct();
		void*  Alloc();
		void DeAlloc( void* ptr );
		bool isValid();
	private:
		MemBlock* blocks_;
		int       blockNum_;
		int       blockNumReserved_;
		byte      fixedSize_;
		byte      numPerBlock_;
		int       lastA_;
		int       lastDA_;
//		CRITICAL_SECTION lock_;
	};
	#ifdef STACK_MEM_POOLS
	FixedSizeMemBlockPool pools_[ SMALL_MAX ];
	#else
	FixedSizeMemBlockPool *pools_;
	#endif
#endif

private:

	MemoryManager();
	~MemoryManager();

private:

	static MemoryManager* pUniqueInstance_;

private:

	friend void APIENTRY Startup();
	friend inline MemoryManager& mem();
	NOCOPY(MemoryManager);
};



//-------------------------------------------------------------------------

//@{ 唯一のメモリ管理オブジェクトを返す //@}
inline MemoryManager& mem()
	{ return *MemoryManager::pUniqueInstance_; }

//@{ ゼロ埋め作業 //@}
inline void mem00( void* ptrv, int siz )
	{ BYTE* ptr = (BYTE*)ptrv;
	  for(;siz>3;siz-=4,ptr+=4) *(DWORD*)ptr = 0x00000000;
	  for(;siz;--siz,++ptr) *ptr = 0x00; }

//@{ FF埋め作業 //@}
inline void memFF( void* ptrv, int siz )
	{ BYTE* ptr = (BYTE*)ptrv;
	  for(;siz>3;siz-=4,ptr+=4) *(DWORD*)ptr = 0xffffffff;
	  for(;siz;--siz,++ptr) *ptr = 0xff; }

inline void *memCP( void* dst, const void* src, size_t siz )
{
	BYTE* d = (BYTE*)dst;
	const BYTE* s = (const BYTE*)src;
	for(;siz>3;siz-=4,d+=4, s+=4) *(DWORD*)d = *(DWORD*)s;
	for(;siz;--siz,++d,++s) *d = *s;
	return dst;
}

inline bool memEQ( const void *s1, const void *s2, size_t siz )
{
	const BYTE *a = (const BYTE *)s1;
	const BYTE *b = (const BYTE *)s2;
	for ( ; siz>3 ; siz-=4, a+=4, b+=4)
	{
		if ( *(const DWORD*)a != *(const DWORD*)b )
			return false;
	}
	for ( ; siz ; siz--, a++, b++)
	{
		if ( *a != *b )
			return false;
	}
	return true;
}



//=========================================================================
//@{
//	標準基底クラス
//
//	JavaのObject や MFCのCObject みたいに使う…わけではなく、
//	単にここから派生すると自動で operator new/delete が高速版に
//	なるので便利だよ、という使い方のための基底クラスです。
//@}
//=========================================================================

class Object
{
#ifdef USE_ORIGINAL_MEMMAN
public:

	static void* operator new( size_t siz )
		{ return mem().Alloc( siz ); }

	static void operator delete( void* ptr, size_t siz )
		{ mem().DeAlloc( ptr, siz ); }
#endif

protected:
	virtual ~Object()
		{}
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_MEMORY_H_
