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

// ���K�͂ƌ��Ȃ��I�u�W�F�N�g�̍ő�T�C�Y
#define SMALL_MAX 64
// ��x�Ɋm�ۂ���q�[�v�u���b�N�̃T�C�Y
#define BLOCK_SIZ 4096
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

class MemoryManager : public EzLockable
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
	};
	FixedSizeMemBlockPool pools_[ SMALL_MAX ];
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



//=========================================================================
//@{
//	�W�����N���X
//
//	Java��Object �� MFC��CObject �݂����Ɏg���c�킯�ł͂Ȃ��A
//	�P�ɂ�������h������Ǝ����� operator new/delete �������ł�
//	�Ȃ�̂ŕ֗�����A�Ƃ����g�����̂��߂̊��N���X�ł��B
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
