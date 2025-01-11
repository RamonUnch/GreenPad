#include "stdafx.h"
#include "memory.h"
using namespace ki;

#ifdef _DEBUG
#include "log.h"
#include "kstring.h"
#endif


//=========================================================================

#ifdef SUPERTINY

	#ifndef USE_LOCALALLOC
	static HANDLE g_heap;
	#endif

	#ifdef _DEBUG
	// デバッグ用に回数計測版(^^;
	static int allocCounter = 0;
	static int smallAllocSize = 0;
	static int smallDeAllocSize = 0;
	static ulong smallAllocs_hist[SMALL_MAX/2];
	#endif // _DEBUG

	static uchar ignorecnt=0;
	void *__cdecl malloc( size_t siz ) noexcept
	{
		#ifdef _DEBUG
		#if defined(SIMULATE_LOW_MEM)
		if(allocCounter>=512)
		{
			MessageBox(GetActiveWindow(), TEXT("OUT OF MEMORY!"), NULL, MB_OK|MB_TASKMODAL);
			return NULL;
		}
		#endif
		++allocCounter;
		#endif
		TRYLBL:
		#ifdef USE_LOCALALLOC
		void *ret = ::LocalAlloc( LMEM_FIXED, siz );
		#else
		void *ret = ::HeapAlloc( g_heap, 0, siz );
		#endif
		if (!ret && ignorecnt < 8) {
			DWORD ans = MessageBox(GetActiveWindow(), TEXT("Unable to allocate memory!"), NULL, MB_ABORTRETRYIGNORE|MB_TASKMODAL);
			switch(ans) {
			case IDABORT: ExitProcess(1); break;
			case IDRETRY: ignorecnt = 0; goto TRYLBL; break;
			case IDIGNORE: ignorecnt++; break;
			}
		}
		return ret;

	}

	void __cdecl free( void *ptr ) noexcept
	{
		if (ptr != NULL)
		{   // It is not Guarenteed that HeapFree can free NULL.
			#ifdef USE_LOCALALLOC
			::LocalFree( ptr );
			#else
			::HeapFree( g_heap, 0, ptr );
			#endif
			#ifdef _DEBUG
			--allocCounter;
			#endif
		}

	}

	void *__cdecl realloc(void *ptr, size_t siz ) noexcept
	{
		TRYLBL:
		#ifdef USE_LOCALALLOC
		void *ret = ::LocalReAlloc( ptr, LMEM_FIXED, siz );
		#else
		void *ret = ::HeapReAlloc( g_heap, 0, ptr, siz );
		#endif
		if (!ret && ignorecnt < 8) {
			DWORD ans = MessageBox(GetActiveWindow(), TEXT("Unable to reallocate memory!"), NULL, MB_ABORTRETRYIGNORE|MB_TASKMODAL);
			switch(ans) {
			case IDABORT: ExitProcess(1); break;
			case IDRETRY: ignorecnt = 0; goto TRYLBL; break;
			case IDIGNORE: ignorecnt++; break;
			}
		}
		return ret;
	}

	void* __cdecl operator new( size_t siz ) noexcept
		{ return malloc( siz ); }

	void __cdecl operator delete( void* ptr ) noexcept
		{ free( ptr ); }

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
	#if defined(_MSC_VER) && defined(_M_IX86) && !(defined(_M_AMD64) || defined(_M_X64))
	// MSVC 386 ONLY
	void* __cdecl memmove( void* dst, const void* src, size_t cnt )
	{
		__asm {
			mov  esi, [src]    ;U  esi = const void* src
			mov  edx, [cnt]    ;V  edx =       void* cnt
			mov  edi, [dst]    ;U  edi =       ulong dst
			mov  ebx, edx      ;V
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
	#if defined(__GNUC__) && defined(__i386__) && !defined(WIN64)
	// This is a convertion from the normal intel syntax
	// Converted %%edx into a generic register "q" -> %1
	// GCC seems to choose edx anyway.
	// GCC 386 ONLY
	void *__cdecl memmove(void *dst, const void *src, size_t n)
	{
		void *odst = dst;
		asm (
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
		"    and      $0x3, %1\n"     //edx = [rest bytes]%4
		"    rep movsl\n"             //N  DWORD MOVE
		"    jmp      MiniCopy\n"
		"CopyDown:    \n"
		"    std\n"
		"    lea      (-1)(%%esi,%1,1), %%esi\n"
		"    lea      (-1)(%%edi,%1,1), %%edi\n"

		"    cmp      $0x4, %1\n"     //if( cnt<=4 )
		"    jbe      MiniCopy\n"     //byte by byte copy

		"    mov      %%edi, %%ecx\n"
		"    and      $0x3, %%ecx\n"
		"    inc      %%ecx\n"        //ecx = {dst%4 0->1 1->2 2->3 3->4}
		"    sub      %%ecx, %1\n"
		"    rep movsb\n"             //N  BYTE MOVE (align dst @ dword)

		"    sub      $0x3, %%edi\n"  //U
		"    mov      %1, %%ecx\n"    //V
		"    sub      $0x3, %%esi\n"  //U
		"    shr      $0x2, %%ecx\n"  //V  ecx = [rest bytes]/4
		"    and      $0x3, %1\n"     //edx[= [rest bytes]%4
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
	#else
	// Other CPUS or no GCC/MSVC
	#pragma GCC push_options
	#pragma GCC optimize("O3")
	void *__cdecl memmove(void *dest, const void *src, size_t n)
	{
		if (dest == src)
			return dest;

		unsigned char *d = reinterpret_cast<unsigned char *>(dest);
		const unsigned char *s = reinterpret_cast<const unsigned char *>(src);

		if (d < s || s+n < d) {
			for (; n; n--) *d++ = *s++;
		} else {
			while (n) n--, d[n] = s[n];
		}
		return dest;
	}
	#pragma GCC pop_options
	#endif


	extern "C" void *__cdecl memset(void *dest, int ch, size_t n)
	{
		unsigned char *d = reinterpret_cast<unsigned char *>(dest);
		const unsigned char c = static_cast<unsigned char>(ch);
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
	void operator delete(void * ptr, size_t size) noexcept
	{
		::operator delete(ptr);
	}
	void operator delete [] (void * ptr) noexcept
	{
		::operator delete(ptr);
	}
	void operator delete [](void * ptr, size_t size) noexcept
	{
		::operator delete(ptr);
	}
	void* operator new[](size_t sz) noexcept
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
	bool  Construct( byte siz, byte num );
	void  Destruct();
	void* Alloc( byte siz );
	void  DeAlloc( void* ptr, byte siz );
	bool  isAvail();
	bool  isEmpty( byte num );
	bool  hasThisPtr( const void* ptr, size_t len );
private:
	byte* buf_;
	byte  first_, avail_;
};

bool MemBlock::Construct( byte siz, byte num )
{
	// 確保
	buf_   = (byte *)malloc( siz*num );
	if( !buf_ )
		return false;
	first_ = 0;
	avail_ = num;

	// 連結リスト初期化
	for( byte i=0,*p=buf_; i<num; p+=siz )
		*p = ++i;
	return true;
}

inline void MemBlock::Destruct()
{
	// 解放
	::free( buf_ );
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

inline bool MemBlock::hasThisPtr( const void* ptr, size_t len )
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

bool MemoryManager::FixedSizeMemBlockPool::Construct( byte siz )
{
	// メモリブロック情報域をちょこっと確保
	blocks_ = (MemBlock *)malloc( sizeof(MemBlock) * 4 );
	if( !blocks_ ) return false;

	// ブロックサイズ等計算
	int npb = BLOCK_SIZ/siz;
	numPerBlock_ = static_cast<byte>( Min( npb, 255 ) );
	fixedSize_   = siz;

	// 一個だけブロック作成
	bool ok = blocks_[0].Construct( fixedSize_, numPerBlock_ );
	if( !ok )
	{
		free( blocks_ ); // free on failure
		return false;
	}
	// All good.
	lastA_            = 0;
	lastDA_           = 0;
	blockNum_         = 1;
	blockNumReserved_ = 4;
	return true;
}

void MemoryManager::FixedSizeMemBlockPool::Destruct()
{
	// 各ブロックを解放
	for( int i=0; i<blockNum_; ++i )
		blocks_[i].Destruct();

	// ブロック情報保持領域のメモリも解放
	free( blocks_ );
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
					MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_*2 );
					if( !nb )
						return NULL;
					memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
					free( blocks_ );
					blocks_ = nb;
					blockNumReserved_ *= 2;
				}

				// 新しくブロック構築
				bool ok = blocks_[ blockNum_ ].Construct( fixedSize_, numPerBlock_ );
				if( !ok ) return NULL;
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
	const INT_PTR mx=blockNum_-1, ln=fixedSize_*numPerBlock_;
	for( INT_PTR u=lastDA_, d=lastDA_-1;; )
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
		INT_PTR end = blockNum_-1;
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

		if( blockNum_ > 4 && blockNum_ <= blockNumReserved_ >> 2 )
		{
			// Reduce size of mem pool if less than a quarter of what is needed.
			//LOGGERF( TEXT("Reducing from %d to %d"), blockNumReserved_, blockNum_ );
			MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_ );
			if( !nb )
				return;
			memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
			free( blocks_ );
			blocks_ = nb;
			blockNumReserved_ = blockNum_;
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
#if defined(SUPERTINY) && defined(_DEBUG)
	mem00(smallAllocs_hist, sizeof(smallAllocs_hist));
#endif
	// メモリプールをZEROクリア
	#ifndef STACK_MEM_POOLS
	static MemoryManager::FixedSizeMemBlockPool staticpools[ SMALL_MAX/2 ];
	pools_ = staticpools;
	#endif
	#ifdef STACK_MEM_POOLS
	mem00( pools_, /*sizeof(pools_)*/ sizeof(FixedSizeMemBlockPool) * (SMALL_MAX/2) );
	#endif

	// 唯一のインスタンスは私です
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
	// 構築済みメモリプールを全て解放, Release all built memory pools
	for( int i=0; i<SMALL_MAX/2; ++i )
		if( pools_[i].isValid() )
			pools_[i].Destruct();

//	delete [] pools_;
#if defined(SUPERTINY)
	// ::HeapDestroy( g_heap );
#if defined(_DEBUG)
	// リーク検出用
	if( allocCounter != 0 )
	{
		TCHAR buf[128];
		::wsprintf(buf, TEXT("alloc - dealloc = %d\nSmall Allocs/DeAllocs=%d - %d = %d bytes")
		              , allocCounter, smallAllocSize, smallDeAllocSize, smallAllocSize-smallDeAllocSize );
		::MessageBox( GetActiveWindow(), buf, TEXT("MemoryLeak!"), MB_OK|MB_TOPMOST );
	}
	LOGGER( "small allocs histogram:" );
	TCHAR tmp[ULONG_DIGITS+1];
	for(int i=0; i<countof(smallAllocs_hist); i++)
		LOGGERS( Ulong2lStr(tmp, smallAllocs_hist[i]) );
#endif // _DEBUG
#endif // SUPERTINY
}

void* A_HOT MemoryManager::Alloc( size_t siz )
{
	siz = siz + (siz&1);
#if defined(SUPERTINY) && defined(_DEBUG)
	++allocCounter;
	smallAllocSize += siz;
	if( siz < SMALL_MAX )
		smallAllocs_hist[(siz-1)/2]++;
//	LOGGERF( TEXT("Alloc(%lu) - %lu tot"), siz, smallAllocSize );
#endif

	// サイズが零か大きすぎるなら
	// デフォルトの new 演算子に任せる
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
		return malloc( siz );

	// マルチスレッド対応
	AutoLock al(this);

	// このサイズのメモリ確保が初めてなら
	// ここでメモリプールを作成する。
	if( !pools_[i].isValid() )
	{
		bool ok = pools_[i].Construct( static_cast<byte>(siz) );
		if( !ok ) return NULL;
	}

	// ここで割り当て
	return pools_[i].Alloc();
}

void A_HOT MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	siz = siz + (siz&1);
#if defined(SUPERTINY) && defined(_DEBUG)
	--allocCounter;
	smallDeAllocSize += siz;
//	LOGGERF( TEXT("Free(%lu) - %lu tot"), siz, smallAllocSize );
#endif

	// サイズが零か大きすぎるなら
	// デフォルトの delete 演算子に任せる
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
	{
		::free( ptr );
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
	{
		TCHAR buf[128];
		::wsprintf(buf, TEXT("alloc - dealloc = %d\n"), allocCounter);
		::MessageBox( GetActiveWindow(), buf, TEXT("MemoryLeak!"), MB_OK|MB_TOPMOST );
	}
#endif
}

void* MemoryManager::Alloc( size_t siz )
{
	return ::malloc(siz);
}
void MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	::free(ptr);
}

#endif
