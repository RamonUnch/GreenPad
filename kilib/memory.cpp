#include "stdafx.h"
#include "memory.h"
using namespace ki;



//=========================================================================

#ifdef SUPERTINY

	#ifndef USE_LOCALALLOC
	static HANDLE g_heap;
	#endif

	#ifndef _DEBUG

		void* __cdecl operator new( size_t siz )
		{
			TRYLBL:
			#ifdef USE_LOCALALLOC
			void *ret = ::LocalAlloc( LMEM_FIXED, siz );
			#else
			void *ret = ::HeapAlloc( g_heap, 0, siz );
			#endif
			if (!ret) {
				DWORD ans = MessageBox(GetActiveWindow(), TEXT("Unable to allocate memory!"), NULL, MB_ABORTRETRYIGNORE|MB_TASKMODAL);
				switch(ans) {
				case IDABORT: ExitProcess(1); break;
				case IDRETRY: goto TRYLBL; break;
				}
			}
			return ret;
		}

		void __cdecl operator delete( void* ptr )
		{
			if (ptr != NULL)
			{   // It is not Guarenteed that HeapFree can free NULL.
				#ifdef USE_LOCALALLOC
				void *ret = ::LocalFree( ptr );
				#else
				::HeapFree( g_heap, 0, ptr );
				#endif
			}
		}

	#else

		// デバッグ用に回数計測版(^^;
		static int allocCounter = 0;
		void* __cdecl operator new( size_t siz )
		{
			void *ret;
			++allocCounter;
			#ifdef USE_LOCALALLOC
			ret = ::LocalAlloc( LMEM_FIXED, siz );
			#else
			ret = ::HeapAlloc( g_heap, 0, siz );
			#endif
			if( !ret )
			{
				MessageBox(GetActiveWindow(), TEXT("Unable to allocate memory!\nEXITTING!"), NULL, MB_OK);
				ExitProcess(1);
			}
			return ret;
		}
		void __cdecl operator delete( void* ptr )
		{
			if( ptr != NULL )
			{
				#ifdef USE_LOCALALLOC
				::LocalFree( ptr );
				#else
				::HeapFree( g_heap, 0, ptr );
				#endif
				--allocCounter;
			}
		}

	#endif

	#if _MSC_VER >= 1300 && _MSC_VER != 1500
	extern "C"
	void* __cdecl memset( void* buf, int ch, size_t n )
	{
		BYTE v = BYTE(ch&0xff);
		BYTE* ptr = (BYTE*)buf;
		DWORD vvvv = (v<<24) | (v<<16) | (v<<8) | v;
		for(;n>3;n-=4,ptr+=4) *(DWORD*)ptr = vvvv;
		for(;n;--n,++ptr) *ptr = v;
		return buf;
	}
	#endif
	#if !defined(__GNUC__)
	void* __cdecl memmove( void* dst, const void* src, size_t cnt )
	{
		__asm {
			mov  esi, [src]    ;U  esi = const void* src
			mov  edx, [cnt]    ;V  edx =       void* cnt
			mov  edi, [dst]    ;U  edi =       ulong dst
			mov  ebx, edx      ;V
//			mov  eax, 03h      ;U  eax = const ulong  3 (for masking)
			add  ebx, esi      ;V  ebx = const void* src+cnt

			cmp  edi, esi      ;
			jbe  CopyUp        ;
			cmp  edi, ebx      ;   if( src < dst < src+cnt )
			jb   CopyDown      ;     downward copy

		CopyUp:
			cmp  edx, 03h      ;   if( cnt<=3 )
			jbe  MiniCopy      ;     byte by byte copy

			mov  ebx, edi      ;U
			mov  ecx, 04h      ;V
			and  ebx, 03h      ;U  ebx = (dst&3)
			sub  ecx, ebx      ;   ecx = (4-(dst&3))
			and  ecx, 03h      ;   ecx = {dst%4 0->0 1->3 2->2 3->1}
			sub  edx, ecx      ;
			rep  movsb         ;N  BYTE MOVE (align dst)

			mov  ecx, edx      ;
			shr  ecx, 2        ;   ecx = [rest bytes]/4
			and  edx, 03h      ;   edx = [rest bytes]%4
			rep  movsd         ;N  DWORD MOVE
			jmp  MiniCopy      ;

		CopyDown:
			std                  ;
			lea  esi,[esi+edx-1] ;
			lea  edi,[edi+edx-1] ;

			cmp  edx, 4        ;   if( cnt<=4 )
			jbe  MiniCopy      ;     byte by byte copy

			mov  ecx, edi      ;
			and  ecx, 03h      ;
			inc  ecx           ;   ecx = {dst%4 0->1 1->2 2->3 3->4}
			sub  edx, ecx      ;
			rep  movsb         ;N  BYTE MOVE (align dst @ dword)

			sub  edi, 03h      ;U
			mov  ecx, edx      ;V
			sub  esi, 03h      ;U
			shr  ecx, 2        ;V  ecx = [rest bytes]/4
			and  edx, 03h      ;   edx = [rest bytes]%4
			rep  movsd         ;N  DWORD MOVE
			add  edi, 03h      ;U
			add  esi, 03h      ;V

		MiniCopy:
			mov  ecx, edx      ;
			rep  movsb         ;N  BYTE MOVE

			cld                ;U
//			mov  eax, [dst]    ;V  return dst
		}
		return dst;
	}
	#else
	#if defined(__i386__)
	// This is a convertion from the normal intel syntax
	// Converted %%edx into a generic register "q" -> %1
	// GCC seems to choose edx anyway.
	void *__cdecl memmove(void *dst, const void *src, size_t n)
	{
		void *odst = dst;
		asm (
//		"    mov      %0, %%esi\n"    //U  esi = const void* src
//		"    mov      %1, %%edx\n"    //V  edx =       size_t  n
//		"    mov      %2, %%edi\n"    //U  edi =       void* dst
		"    mov      %1, %%ebx\n"    //V  ebx = n
		"    add      %%esi, %%ebx\n" //V  ebx = const void* src+cnt

		"    cmp      %%esi, %%edi\n"
		"    jbe      CopyUp\n"
		"    cmp      %%ebx, %%edi\n" //if( src < dst < src+cnt )
		"    jb       CopyDown\n"     //downward copy

		"CopyUp:    \n"
		"    cmp      $0x3, %1\n"     //if( cnt<=3 )
		"    jbe      MiniCopy\n"     //byte by byte copy

		"    mov      %%edi, %%ebx\n" //U
		"    mov      $0x4, %%ecx\n"  //ecx = 4
		"    and      $0x3, %%ebx\n"  //U  ebx = (dst&3)
		"    sub      %%ebx, %%ecx\n" //ecx = (4-(dst&3))
		"    and      $0x3, %%ecx\n"  //ecx = {dst%4 0->0 1->3 2->2 3->1}
		"    sub      %%ecx, %1\n"
		"    rep movsb\n"             //N  BYTE MOVE (align dst)

		"    mov      %1, %%ecx\n"
		"    shr      $0x2, %%ecx\n"  //ecx = [rest bytes]/4
		"    and      $0x3, %1\n"  //edx = [rest bytes]%4
		"    rep movsl\n"             //N  DWORD MOVE
		"    jmp      MiniCopy\n"
		"CopyDown:    \n"
		"    std\n"
		"    lea      (-1)(%%esi,%1,1), %%esi\n"
		"    lea      (-1)(%%edi,%1,1), %%edi\n"

		"    cmp      $0x4, %1\n"  //if( cnt<=4 )
		"    jbe      MiniCopy\n"     //byte by byte copy

		"    mov      %%edi, %%ecx\n"
		"    and      $0x3, %%ecx\n"
		"    inc      %%ecx\n"        //ecx = {dst%4 0->1 1->2 2->3 3->4}
		"    sub      %%ecx, %1\n"
		"    rep movsb\n"             //N  BYTE MOVE (align dst @ dword)

		"    sub      $0x3, %%edi\n"  //U
		"    mov      %1, %%ecx\n" //V
		"    sub      $0x3, %%esi\n"  //U
		"    shr      $0x2, %%ecx\n"  //V  ecx = [rest bytes]/4
		"    and      $0x3, %1\n"  //edx[= [rest bytes]%4
		"    rep movsl\n"             //N  DWORD MOVE
		"    add      $0x3, %%edi\n"  //U
		"    add      $0x3, %%esi\n"  //V

		"MiniCopy:    \n"
		"    mov      %1, %%ecx\n"
		"    rep movsb\n"             //N  BYTE MOVE
		"    cld\n"                   //U
		:
		:"S"(src), "q"(n), "D"(dst)
		:"memory", "ebx", "ecx"
		);
		return odst;
	}
	#else // Other CPUS
	typedef __attribute__((__may_alias__)) uintptr_t WT;
	#define WS (sizeof(WT))
	void *__cdecl memmove(void *dest, const void *src, size_t n)
	{
		if (dest == src)
			return dest;

		unsigned char *d = (unsigned char *)dest;
		const unsigned char *s = (const unsigned char *)src;

		if (d < s) {
			if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
				while ((uintptr_t)d % WS) {
					if (!n--) return dest;
					*d++ = *s++;
				}
				for (; n>=WS; n-=WS, d+=WS, s+=WS) *(WT *)d = *(WT *)s;
			}
			for (; n; n--) *d++ = *s++;
		} else {
			if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
				while ((uintptr_t)(d+n) % WS) {
					if (!n--) return dest;
					d[n] = s[n];
				}
				while (n>=WS) n-=WS, *(WT *)(d+n) = *(WT *)(s+n);
			}
			while (n) n--, d[n] = s[n];
		}
		return dest;
	}
	#endif


	extern "C" void *__cdecl memset(void *dest, int ch, size_t n)
	{
		unsigned char *d = (unsigned char *)dest;
		const unsigned char c = (unsigned char)ch;
		while( n-- )
			*d++ = c;

		return dest;
	}
//	void *__cdecl memcpy(void *dest, const void * restrict src, size_t n)
//	{
//		return memmove(dest, src, n);
//	}

	#endif

	#ifdef __GNUC__
	void operator delete(void * ptr, size_t size)
	{
		::operator delete(ptr);;
	}
	void operator delete [] (void * ptr)
	{
		::operator delete(ptr);;
	}
	void operator delete [](void * ptr, size_t size)
	{
		::operator delete(ptr);;
	}
	void* operator new[](size_t sz)
	{
		return ::operator new(sz);
	}
	#endif //__GNUC__

#endif // SUPERTINY

#ifdef USE_ORIGINAL_MEMMAN
//=========================================================================
//	効率的なメモリ管理を目指すアロケータ
//=========================================================================

//
// メモリブロック
// 「sizバイト * num個」分の領域を一括確保するのが仕事
//
// 空きブロックには、先頭バイトに [次の空きブロックのindex] を格納。
// これを用いて、先頭への出し入れのみが可能な単方向リストとして扱う。
//

struct ki::MemBlock
{
public:
	void  Construct( byte siz, byte num );
	void  Destruct();
	void* Alloc( byte siz );
	void  DeAlloc( void* ptr, byte siz );
	bool  isAvail();
	bool  isEmpty( byte num );
	bool  hasThisPtr( void* ptr, size_t len );
private:
	byte* buf_;
	byte  first_, avail_;
};

void MemBlock::Construct( byte siz, byte num )
{
	// 確保
	buf_   = ::new byte[siz*num];
	first_ = 0;
	avail_ = num;

	// 連結リスト初期化
	for( byte i=0,*p=buf_; i<num; p+=siz )
		*p = ++i;
}

inline void MemBlock::Destruct()
{
	// 解放
	::delete [] buf_;
}

void* MemBlock::Alloc( byte siz )
{
	// メモリ切り出し
	//   ( avail==0 等のチェックは上位層に任せる )
	byte* blk = buf_ + siz*first_;
	first_    = *blk;
	--avail_;
	return blk;
}

void MemBlock::DeAlloc( void* ptr, byte siz )
{
	// メモリ戻す
	//   ( 変なポインタ渡されたらだ～め～ )
	byte* blk = static_cast<byte*>(ptr);
	*blk      = first_;
	first_    = static_cast<byte>((blk-buf_)/siz);
	++avail_;
}

inline bool MemBlock::isAvail()
{
	// 空きがある？
	return (avail_ != 0);
}

inline bool MemBlock::isEmpty( byte num )
{
	// 完全に空？
	return (avail_ == num);
}

inline bool MemBlock::hasThisPtr( void* ptr, size_t len )
{
	// このブロックのポインタ？
	return ( buf_<=ptr && ptr<buf_+len );
}



//-------------------------------------------------------------------------

//
// 固定サイズ確保人
// 「sizバイト」の領域を毎回確保するのが仕事
//
// メモリブロックのリストを保持し、空いているブロックを使って
// メモリ要求に応えていく。空きがなくなったら新しくMemBlockを
// 作ってリストに加える。
//
// 最後にメモリ割り当て/解放を行ったBlockをそれぞれ記憶しておき、
// 最初にそこを調べることで高速化を図る。
//

void MemoryManager::FixedSizeMemBlockPool::Construct( byte siz )
{
	// メモリマネージャが初期化されるまでは、
	// 普通のauto_ptrも使わない方が無難…
	struct AutoDeleter
	{
		AutoDeleter( MemBlock* p ) : ptr_(p) {}
		~AutoDeleter() { ::delete [] ptr_; }
		void Release() { ptr_ = NULL; }
		MemBlock* ptr_;
	};

	// メモリブロック情報域をちょこっと確保
	AutoDeleter a( blocks_ = ::new MemBlock[4] );

	// ブロックサイズ等計算
	int npb = BLOCK_SIZ/siz;
	numPerBlock_ = static_cast<byte>( Min( npb, 255 ) );
	fixedSize_   = siz;

	// 一個だけブロック作成
	blocks_[0].Construct( fixedSize_, numPerBlock_ );

	a.Release();
	lastA_            = 0;
	lastDA_           = 0;
	blockNum_         = 1;
	blockNumReserved_ = 4;
}

void MemoryManager::FixedSizeMemBlockPool::Destruct()
{
	// 各ブロックを解放
	for( int i=0; i<blockNum_; ++i )
		blocks_[i].Destruct();

	// ブロック情報保持領域のメモリも解放
	::delete [] blocks_;
	blockNum_ = 0;
}

void* MemoryManager::FixedSizeMemBlockPool::Alloc()
{
	// ここでlastA_がValidかどうかチェックしないとまずい。
	// DeAllocされてなくなってるかもしらないので。

	// 前回メモリを切り出したブロックに
	// まだ空きがあるかどうかチェック
	if( !blocks_[lastA_].isAvail() )
	{
		// 無かった場合、リストの末尾から順に線形探索
		for( int i=blockNum_;; )
		{
			if( blocks_[--i].isAvail() )
			{
				// 空きブロック発見～！
				lastA_ = i;
				break;
			}
			if( i == 0 )
			{
				// 全部埋まってた...
				if( blockNum_ == blockNumReserved_ )
				{
					// しかも作業領域も満杯なので拡張
					MemBlock* nb = ::new MemBlock[ blockNum_*2 ];
					memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
					::delete [] blocks_;
					blocks_ = nb;
					blockNumReserved_ *= 2;
				}

				// 新しくブロック構築
				blocks_[ blockNum_ ].Construct( fixedSize_, numPerBlock_ );
				lastA_ = blockNum_++;
				break;
			}
		}
	}
	void *ret = blocks_[lastA_].Alloc( fixedSize_ );
	// ブロックから切り出し割り当て
	return ret;
}

void MemoryManager::FixedSizeMemBlockPool::DeAlloc( void* ptr )
{
	// 該当ブロックを探索
	const int mx=blockNum_-1, ln=fixedSize_*numPerBlock_;
	for( int u=lastDA_, d=lastDA_-1;; )
	{
		if( u>=0 )
			if( blocks_[u].hasThisPtr(ptr,ln) )
			{
				lastDA_ = u;
				break;
			}
			else if( u==mx )
			{
				u = -1;
			}
			else
			{
				++u;
			}
		if( d>=0 )
			if( blocks_[d].hasThisPtr(ptr,ln) )
			{
				lastDA_ = d;
				break;
			}
			else
			{
				--d;
			}
	}

	// 解放を実行
	blocks_[lastDA_].DeAlloc( ptr, fixedSize_ );

	// この削除でブロックが完全に空になった場合
	if( blocks_[lastDA_].isEmpty( numPerBlock_ ) )
	{
		// しかも一番後ろのブロックでなかったら
		int end = blockNum_-1;
		if( lastDA_ != end )
		{
			// 一番後ろが空だったら解放
			if( blocks_[end].isEmpty( numPerBlock_ ) )
			{
				blocks_[end].Destruct();
				--blockNum_;
				if( lastA_ > --end )
					lastA_ = end;
			}

			// 後ろと交換
			MemBlock tmp( blocks_[lastDA_] );
			blocks_[lastDA_] = blocks_[end];
			blocks_[end]     = tmp;
		}
	}
}

inline bool MemoryManager::FixedSizeMemBlockPool::isValid()
{
	// 既に使用開始されているか？
	return (blockNum_ != 0);
}



//-------------------------------------------------------------------------

//
// 最上位層
// 指定サイズにあった FixedSizeMemBlockPool に処理をまわす
//
// lokiの実装では固定サイズアロケータも、必要に応じて
// 動的確保していたが、それは面倒なのでやめました。(^^;
// 最初に64個確保したからと言って、そんなにメモリも喰わないし…。
//

MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
#if defined(SUPERTINY) && !defined(USE_LOCALALLOC)
	g_heap = ::GetProcessHeap();
	// Create our own non-serialized heap
	// Because we only use single thread (may change later)
	// g_heap = ::HeapCreate( HEAP_NO_SERIALIZE, 1, 0 );
#endif

	#ifndef STACK_MEM_POOLS
	static FixedSizeMemBlockPool staticpools[SMALL_MAX];
	pools_ = staticpools;
	#endif
	// メモリプールをZEROクリア
//	pools_ = new FixedSizeMemBlockPool[ SMALL_MAX ];
	#ifdef STACK_MEM_POOLS
	mem00( pools_, /*sizeof(pools_)*/ sizeof(FixedSizeMemBlockPool) * SMALL_MAX );
	#endif

	// 唯一のインスタンスは私です
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
	// 構築済みメモリプールを全て解放, Release all built memory pools
	for( int i=0; i<SMALL_MAX; ++i )
		if( pools_[i].isValid() )
			pools_[i].Destruct();

//	delete [] pools_;
#if defined(SUPERTINY)
	// ::HeapDestroy( g_heap );
#if defined(_DEBUG)
	// リーク検出用
	if( allocCounter != 0 )
		::MessageBox( GetActiveWindow(), TEXT("MemoryLeak!"), NULL, MB_OK );
#endif // _DEBUG
#endif // SUPERTINY
}


void* A_HOT MemoryManager::Alloc( size_t siz )
{
#if defined(SUPERTINY) && defined(_DEBUG)
	++allocCounter;
#endif

	// サイズが零か大きすぎるなら
	// デフォルトの new 演算子に任せる
	uint i = static_cast<uint>( siz-1 );
	if( i >= SMALL_MAX )
		return ::operator new( siz );

	// マルチスレッド対応
	AutoLock al(this);

	// このサイズのメモリ確保が初めてなら
	// ここでメモリプールを作成する。
	if( !pools_[i].isValid() )
		pools_[i].Construct( static_cast<byte>(siz) );

	// ここで割り当て
	return pools_[i].Alloc();
}

void A_HOT MemoryManager::DeAlloc( void* ptr, size_t siz )
{
#if defined(SUPERTINY) && defined(_DEBUG)
	--allocCounter;
#endif

	// サイズが零か大きすぎるなら
	// デフォルトの delete 演算子に任せる
	uint i = static_cast<uint>( siz-1 );
	if( i >= SMALL_MAX )
	{
		::operator delete( ptr );
		return; // VCで return void が出来ないとは…
	}

	// マルチスレッド対応
	AutoLock al(this);

	// ここで解放
	pools_[i].DeAlloc( ptr );
}

#else // USE_ORIGINAL_MEMMAN



MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
#if defined(SUPERTINY) && !defined(USE_LOCALALLOC)
	g_heap = ::GetProcessHeap();
#endif
	// 唯一のインスタンスは私です
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
#if defined(SUPERTINY) && defined(_DEBUG)
	// リーク検出用
	if( allocCounter != 0 )
		::MessageBox( GetActiveWindow(), TEXT("MemoryLeak!"), NULL, MB_OK|MB_TOPMOST );
#endif
}

void* MemoryManager::Alloc( size_t siz )
{
	return ::operator new(siz);
}
void MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	::operator delete(ptr);
}

#endif
