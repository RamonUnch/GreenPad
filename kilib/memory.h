#ifndef _KILIB_MEMORY_H_
#define _KILIB_MEMORY_H_
#include "types.h"
#include "thread.h"
#ifndef __ccdoc__
namespace ki {
#endif


// W�łł�HeapAlloc�𒼐ڌĂяo���o�[�W�������g��
//#if !defined(_UNICODE) && defined(SUPERTINY)
//	#define USE_ORIGINAL_MEMMAN
//#endif

#ifdef WIN64
	// ���K�͂ƌ��Ȃ��I�u�W�F�N�g�̍ő�T�C�Y, Maximum size of objects considered small
	#define SMALL_MAX 254
	// ��x�Ɋm�ۂ���q�[�v�u���b�N�̃T�C�Y, Size of heap block to be allocated at one time
	#define BLOCK_SIZ 8192
#else
	#ifdef STACK_MEM_POOLS
	#define SMALL_MAX 128
	#define BLOCK_SIZ 4096
	#else
	#define SMALL_MAX 254
	#define BLOCK_SIZ 4096
	#endif
#endif
// ��������
struct MemBlock;



//=========================================================================
//@{ @pkg ki.Memory //@}
//@{
//	���������蓖�āE����@�\
//
//	SUPERTINY�I�v�V������t���ăR���p�C������ƁA�W����
//	malloc��free���g���Ȃ��Ȃ邽�߁AHeapAlloc����API��
//	���ڌĂяo���K�v���o�Ă��܂��B�������A�������{����
//	���񒼂ɌĂ�ł���ƁA�x���B�����A�z���ƁA�o�J���ƁA
//	���Đ����Œx���B�����ŁA���new�œ��I�ɏ��K�̓�������
//	�m�ۂ��邱�ƂɎ��𐘂����ȒP�ȃA���P�[�^���g�����Ƃɂ��܂����B
//
//	<a href="http://cseng.aw.com/book/0,3828,0201704315,00.html">loki</a>
//	���C�u�����قڂ��̂܂�܂Ȏ����ł��B
//@}
//=========================================================================

class MemoryManager : private NoLockable // (Ez)Lockable
{
public:

	//@{ ���������蓖�� //@}
	void* Alloc( size_t siz );

	//@{ ��������� //@}
	void DeAlloc( void* ptr, size_t siz );

#ifdef USE_ORIGINAL_MEMMAN
private:
	struct FixedSizeMemBlockPool
	{
		bool Construct( byte siz );
		void Destruct();
		void*  Alloc();
		void DeAlloc( void* ptr );
		bool isValid();
	private:
		MemBlock* blocks_;
		INT_PTR   blockNum_;
		INT_PTR   blockNumReserved_;
		byte      fixedSize_;
		byte      numPerBlock_;
		INT_PTR   lastA_;
		INT_PTR   lastDA_;
	};
	#ifdef STACK_MEM_POOLS
	FixedSizeMemBlockPool pools_[ SMALL_MAX/2 ];
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

//@{ �B��̃������Ǘ��I�u�W�F�N�g��Ԃ� //@}
inline MemoryManager& mem()
	{ return *MemoryManager::pUniqueInstance_; }

//@{ �[�����ߍ�� //@}
inline void mem00( void* ptrv, int siz )
	{ BYTE* ptr = (BYTE*)ptrv;
	  for(;siz>3;siz-=4,ptr+=4) *(DWORD*)ptr = 0x00000000;
	  for(;siz;--siz,++ptr) *ptr = 0x00; }

//@{ FF���ߍ�� //@}
inline void memFF( void* ptrv, int siz )
	{ BYTE* ptr = (BYTE*)ptrv;
	  for(;siz>3;siz-=4,ptr+=4) *(DWORD*)ptr = 0xffffffff;
	  for(;siz;--siz,++ptr) *ptr = 0xff; }

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
//	�W�����N���X
//
//	Java��Object �� MFC��CObject �݂����Ɏg���c�킯�ł͂Ȃ��A
//	�P�ɂ�������h������Ǝ����� operator new/delete �������ł�
//	�Ȃ�̂ŕ֗�����A�Ƃ����g�����̂��߂̊��N���X�ł��B
//
// Standard Base Class
//
// Not to be used like Java's Object or MFC's CObject,
// It is simply a base class for the usage that operator new/delete becomes
// a fast version automatically when derived from this class.
//@}
//=========================================================================

class Object
{
#ifdef USE_ORIGINAL_MEMMAN
public:

	static void* operator new( size_t siz ) noexcept
		{ return mem().Alloc( siz ); }

	static void operator delete( void* ptr, size_t siz ) noexcept
		{ mem().DeAlloc( ptr, siz ); }
#endif
};



//=========================================================================
// Stupid arena allocator
struct Arena
{
	explicit Arena(BYTE *buf, size_t capacity)
		: sta ( buf )
		, end ( buf + capacity )
	{ }

	void *alloc(size_t sz)
	{
		size_t pad = (UINT_PTR)end & (sizeof(void*)-1);
		if( sz+pad > size_t(end - sta) )
			return NULL; // OOM

		return reinterpret_cast<void*>(end -= sz + pad);
	}

	BYTE *sta;
	BYTE *end;
};

#pragma warning(disable: 4146)
struct DArena
{
	// Dynamic mode
//	DArena( size_t reserved )
//		: sta ( (BYTE*)::VirtualAlloc(NULL, reserved, MEM_RESERVE, PAGE_READWRITE) )
//		//, end ( sta )
//		, dim ( pagesize() )
//		, res ( reserved )
//	{
//		end = sta = (BYTE*)::VirtualAlloc(sta, dim, MEM_COMMIT, PAGE_READWRITE);
//		if( !sta ) // Alloc failed
//			dim = 0;
//	}

	//DArena() {};

	void Init( size_t reserved ) noexcept
	{
		dim  = pagesize();
		res = reserved;

		sta = (BYTE*)::VirtualAlloc(NULL, reserved, MEM_RESERVE, PAGE_READWRITE);
		if( !sta ) // Unable to reserve
			res = dim;
		end = sta = (BYTE*)::VirtualAlloc(sta, dim, MEM_COMMIT, PAGE_READWRITE);
		if( !sta ) // Alloc failed
			dim = 0;
	}

	void *alloc(size_t sz) noexcept
	{
		size_t pad = -(UINT_PTR)end & (sizeof(void*)-1);
		if( sz+pad > dim - (end - sta) )
		{
			if( !sta ) return NULL;
			size_t ps = pagesize();
			ps = ps * (1 + (sz / ps));
			if( !::VirtualAlloc(sta, dim + ps, MEM_COMMIT, PAGE_READWRITE) )
				return NULL; // Unable to map!!!!
			dim += ps;
		}
		BYTE *ret = end;
		end += sz + pad;
		return reinterpret_cast<void*>(ret);
	}

	inline void freelast(void *ptr, size_t sz) noexcept
	{
		end -= sz;
	}

	void FreeAll() noexcept
	{
		::VirtualFree(sta, dim, MEM_DECOMMIT);
		::VirtualFree(sta, 0, MEM_RELEASE);
	}

	size_t pagesize() const noexcept
	{
		SYSTEM_INFO si; si.dwPageSize = 4096;
		GetSystemInfo(&si);
		return si.dwPageSize;
	}

//	void trim() noexcept
//	{
//		res = dim;
//		::VirtualFree(sta+dim, 0, MEM_RELEASE);
//	}

	void reset() noexcept
	{
		#ifdef _DEBUG
		mem00(sta, dim);
		#endif
		size_t ps = pagesize();
		if (dim <= ps)
			return;
		::VirtualFree(sta+ps, Max(dim, res)-ps, MEM_DECOMMIT);
		dim = ps;
		end = sta;
	}

	BYTE *sta;
	BYTE *end;
	size_t dim;
	size_t res;
};

extern DArena TS;
struct TmpObject
{
	static void* operator new( size_t sz ) noexcept
	{
		void *ptr = TS.alloc( sz );;
//		TCHAR buf[128];
//		::wsprintf(buf, TEXT("%x, %u"), ptr, sz);
//		MessageBox(NULL, buf, TEXT("ALLOC"), 0);

		return ptr;
	}

	static void operator delete( void* ptr, size_t sz ) noexcept
	{
//		TCHAR buf[128];
//		::wsprintf(buf, TEXT("%x, %u"), ptr, sz);
//		MessageBox(NULL, buf, TEXT("FREE"), 0);
//
		TS.freelast( ptr, sz );
	}
};

}      // namespace ki
#endif // _KILIB_MEMORY_H_
