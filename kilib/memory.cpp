#include "stdafx.h"
#include "memory.h"
using namespace ki;



//=========================================================================

#ifdef SUPERTINY

	static HANDLE g_heap;

	#ifndef _DEBUG

		void* __cdecl operator new( size_t siz )
		{
			TRYLBL:
			void *ret = ::HeapAlloc( g_heap, 0, siz );
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
			::HeapFree( g_heap, 0, ptr );
		}

	#else

		// デバッグ用に回数計測版(^^;
		static int allocCounter = 0;
		void* __cdecl operator new( size_t siz )
		{
			++allocCounter;
			return ::HeapAlloc( g_heap, HEAP_GENERATE_EXCEPTIONS, siz );
		}
		void __cdecl operator delete( void* ptr )
		{
			::HeapFree( g_heap, 0, ptr );
			if( ptr != NULL )
				--allocCounter;
		}

	#endif

	#if _MSC_VER >= 1300
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
			mov  eax, 03h      ;U  eax = const ulong  3 (for masking)
			add  ebx, esi      ;V  ebx = const void* src+cnt

			cmp  edi, esi      ;
			jbe  CopyUp        ;
			cmp  edi, ebx      ;   if( src < dst < src+cnt )
			jb   CopyDown      ;     downward copy

		CopyUp:
			cmp  edx, eax      ;   if( cnt<=3 )
			jbe  MiniCopy      ;     byte by byte copy

			mov  ebx, edi      ;U
			mov  ecx, eax      ;V
			and  ebx, eax      ;U  ebx = (dst&3)
			inc  ecx           ;V
			sub  ecx, ebx      ;   ecx = (4-(dst&3))
			and  ecx, eax      ;   ecx = {dst%4 0->0 1->3 2->2 3->1}
			sub  edx, ecx      ;
			rep  movsb         ;N  BYTE MOVE (align dst)

			mov  ecx, edx      ;
			shr  ecx, 2        ;   ecx = [rest bytes]/4
			and  edx, eax      ;   edx = [rest bytes]%4
			rep  movsd         ;N  DWORD MOVE
			jmp  MiniCopy      ;

		CopyDown:
			std                  ;
			lea  esi,[esi+edx-1] ;
			lea  edi,[edi+edx-1] ;

			cmp  edx, 4        ;   if( cnt<=4 )
			jbe  MiniCopy      ;     byte by byte copy

			mov  ecx, edi      ;
			and  ecx, eax      ;
			inc  ecx           ;   ecx = {dst%4 0->1 1->2 2->3 3->4}
			sub  edx, ecx      ;
			rep  movsb         ;N  BYTE MOVE (align dst @ dword)

			sub  edi, eax      ;U
			mov  ecx, edx      ;V
			sub  esi, eax      ;U
			shr  ecx, 2        ;V  ecx = [rest bytes]/4
			and  edx, eax      ;   edx = [rest bytes]%4
			rep  movsd         ;N  DWORD MOVE
			add  edi, eax      ;U
			add  esi, eax      ;V

		MiniCopy:
			mov  ecx, edx      ;
			rep  movsb         ;N  BYTE MOVE

			cld                ;U
			mov  eax, [dst]    ;V  return dst
		}
		return dst;
	}
	#else
	// Stupid naive C90 memmove for GCC
	typedef __attribute__((__may_alias__)) size_t WT;
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

	#ifdef __GNUC__
	void operator delete(void * ptr, size_t size)
	{
		::operator delete(ptr);;
	}
	void operator delete [] (void * ptr)
	{
		::operator delete(ptr);;
	}
	void* operator new[](size_t sz)
	{
		return ::operator new(sz);
	}
	#endif //__GNUC__

#endif // SUPERTINY




MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
#ifdef SUPERTINY
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
void* MemoryManager::ReAlloc( void* ptr, size_t siz )
{
#ifndef _DEBUG
	return HeapReAlloc(g_heap, 0, ptr, siz);
#else
	return HeapReAlloc(g_heap, HEAP_GENERATE_EXCEPTIONS, ptr, siz);
#endif
}
