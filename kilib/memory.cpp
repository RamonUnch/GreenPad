#include "stdafx.h"
#include "memory.h"
using namespace ki;



//=========================================================================

#ifdef SUPERTINY

	static HANDLE g_heap;

	#ifndef _DEBUG

		void* __cdecl operator new( size_t siz )
		{
			return ::HeapAlloc( g_heap, HEAP_GENERATE_EXCEPTIONS, siz );
		}

		void __cdecl operator delete( void* ptr )
		{
			::HeapFree( g_heap, 0, ptr );
		}

	#else

		// �f�o�b�O�p�ɉ񐔌v����(^^;
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

#endif



#ifdef USE_ORIGINAL_MEMMAN
//=========================================================================
//	�����I�ȃ������Ǘ���ڎw���A���P�[�^
//=========================================================================

//
// �������u���b�N
// �usiz�o�C�g * num�v���̗̈���ꊇ�m�ۂ���̂��d��
//
// �󂫃u���b�N�ɂ́A�擪�o�C�g�� [���̋󂫃u���b�N��index] ���i�[�B
// �����p���āA�擪�ւ̏o������݂̂��\�ȒP�������X�g�Ƃ��Ĉ����B
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
	// �m��
	buf_   = ::new byte[siz*num];
	first_ = 0;
	avail_ = num;

	// �A�����X�g������
	for( byte i=0,*p=buf_; i<num; p+=siz )
		*p = ++i;
}

inline void MemBlock::Destruct()
{
	// ���
	::delete [] buf_;
}

void* MemBlock::Alloc( byte siz )
{
	// �������؂�o��
	//   ( avail==0 ���̃`�F�b�N�͏�ʑw�ɔC���� )
	byte* blk = buf_ + siz*first_;
	first_    = *blk;
	--avail_;
	return blk;
}

void MemBlock::DeAlloc( void* ptr, byte siz )
{
	// �������߂�
	//   ( �ςȃ|�C���^�n���ꂽ�炾�`�߁` )
	byte* blk = static_cast<byte*>(ptr);
	*blk      = first_;
	first_    = static_cast<byte>((blk-buf_)/siz);
	++avail_;
}

inline bool MemBlock::isAvail()
{
	// �󂫂�����H
	return (avail_ != 0);
}

inline bool MemBlock::isEmpty( byte num )
{
	// ���S�ɋ�H
	return (avail_ == num);
}

inline bool MemBlock::hasThisPtr( void* ptr, size_t len )
{
	// ���̃u���b�N�̃|�C���^�H
	return ( buf_<=ptr && ptr<buf_+len );
}



//-------------------------------------------------------------------------

//
// �Œ�T�C�Y�m�ېl
// �usiz�o�C�g�v�̗̈�𖈉�m�ۂ���̂��d��
//
// �������u���b�N�̃��X�g��ێ����A�󂢂Ă���u���b�N���g����
// �������v���ɉ����Ă����B�󂫂��Ȃ��Ȃ�����V����MemBlock��
// ����ă��X�g�ɉ�����B
//
// �Ō�Ƀ��������蓖��/������s����Block�����ꂼ��L�����Ă����A
// �ŏ��ɂ����𒲂ׂ邱�Ƃō�������}��B
//

void MemoryManager::FixedSizeMemBlockPool::Construct( byte siz )
{
	// �������}�l�[�W���������������܂ł́A
	// ���ʂ�auto_ptr���g��Ȃ���������c
	struct AutoDeleter
	{
		AutoDeleter( MemBlock* p ) : ptr_(p) {}
		~AutoDeleter() { ::delete [] ptr_; }
		void Release() { ptr_ = NULL; }
		MemBlock* ptr_;
	};

	// �������u���b�N��������傱���Ɗm��
	AutoDeleter a( blocks_ = ::new MemBlock[4] );

	// �u���b�N�T�C�Y���v�Z
	int npb = BLOCK_SIZ/siz;
	numPerBlock_ = static_cast<byte>( Min( npb, 255 ) );
	fixedSize_   = siz;

	// ������u���b�N�쐬
	blocks_[0].Construct( fixedSize_, numPerBlock_ );

	a.Release();
	lastA_            = 0;
	lastDA_           = 0;
	blockNum_         = 1;
	blockNumReserved_ = 4;
}

void MemoryManager::FixedSizeMemBlockPool::Destruct()
{
	// �e�u���b�N�����
	for( int i=0; i<blockNum_; ++i )
		blocks_[i].Destruct();

	// �u���b�N���ێ��̈�̃����������
	::delete [] blocks_;
	blockNum_ = 0;
}

void* MemoryManager::FixedSizeMemBlockPool::Alloc()
{
	// ������lastA_��Valid���ǂ����`�F�b�N���Ȃ��Ƃ܂����B
	// DeAlloc����ĂȂ��Ȃ��Ă邩������Ȃ��̂ŁB

	// �O�񃁃�����؂�o�����u���b�N��
	// �܂��󂫂����邩�ǂ����`�F�b�N
	if( !blocks_[lastA_].isAvail() )
	{
		// ���������ꍇ�A���X�g�̖������珇�ɐ��`�T��
		for( int i=blockNum_;; )
		{
			if( blocks_[--i].isAvail() )
			{
				// �󂫃u���b�N�����`�I
				lastA_ = i;
				break;
			}
			if( i == 0 )
			{
				// �S�����܂��Ă�...
				if( blockNum_ == blockNumReserved_ )
				{
					// ��������Ɨ̈�����t�Ȃ̂Ŋg��
					MemBlock* nb = ::new MemBlock[ blockNum_*2 ];
					memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
					::delete [] blocks_;
					blocks_ = nb;
					blockNumReserved_ *= 2;
				}

				// �V�����u���b�N�\�z
				blocks_[ blockNum_ ].Construct( fixedSize_, numPerBlock_ );
				lastA_ = blockNum_++;
				break;
			}
		}
	}

	// �u���b�N����؂�o�����蓖��
	return blocks_[lastA_].Alloc( fixedSize_ );
}

void MemoryManager::FixedSizeMemBlockPool::DeAlloc( void* ptr )
{
	// �Y���u���b�N��T��
	const int mx=blockNum_, ln=fixedSize_*numPerBlock_;
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

	// ��������s
	blocks_[lastDA_].DeAlloc( ptr, fixedSize_ );

	// ���̍폜�Ńu���b�N�����S�ɋ�ɂȂ����ꍇ
	if( blocks_[lastDA_].isEmpty( numPerBlock_ ) )
	{
		// ��������Ԍ��̃u���b�N�łȂ�������
		int end = blockNum_-1;
		if( lastDA_ != end )
		{
			// ��Ԍ�낪�󂾂�������
			if( blocks_[end].isEmpty( numPerBlock_ ) )
			{
				blocks_[end].Destruct();
				--blockNum_;
				if( lastA_ > --end )
					lastA_ = end;
			}

			// ���ƌ���
			MemBlock tmp( blocks_[lastDA_] );
			blocks_[lastDA_] = blocks_[end];
			blocks_[end]     = tmp;
		}
	}
}

inline bool MemoryManager::FixedSizeMemBlockPool::isValid()
{
	// ���Ɏg�p�J�n����Ă��邩�H
	return (blockNum_ != 0);
}



//-------------------------------------------------------------------------

//
// �ŏ�ʑw
// �w��T�C�Y�ɂ����� FixedSizeMemBlockPool �ɏ������܂킷
//
// loki�̎����ł͌Œ�T�C�Y�A���P�[�^���A�K�v�ɉ�����
// ���I�m�ۂ��Ă������A����͖ʓ|�Ȃ̂ł�߂܂����B(^^;
// �ŏ���64�m�ۂ�������ƌ����āA����ȂɃ����������Ȃ����c�B
//

MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
#ifdef SUPERTINY
	g_heap = ::GetProcessHeap();
#endif

	// �������v�[����ZERO�N���A
	mem00( pools_, sizeof(pools_) );

	// �B��̃C���X�^���X�͎��ł�
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
	// �\�z�ς݃������v�[����S�ĉ��
	for( int i=0; i<SMALL_MAX; ++i )
		if( pools_[i].isValid() )
			pools_[i].Destruct();

#if defined(SUPERTINY) && defined(_DEBUG)
	// ���[�N���o�p
	if( allocCounter != 0 )
		::MessageBox( NULL, TEXT("MemoryLeak!"), NULL, MB_OK );
#endif
}


void* MemoryManager::Alloc( size_t siz )
{
#if defined(SUPERTINY) && defined(_DEBUG)
	++allocCounter;
#endif

	// �T�C�Y���납�傫������Ȃ�
	// �f�t�H���g�� new ���Z�q�ɔC����
	uint i = static_cast<uint>( siz-1 );
	if( i >= SMALL_MAX )
		return ::operator new( siz );

	// �}���`�X���b�h�Ή�
	AutoLock al(this);

	// ���̃T�C�Y�̃������m�ۂ����߂ĂȂ�
	// �����Ń������v�[�����쐬����B
	if( !pools_[i].isValid() )
		pools_[i].Construct( static_cast<byte>(siz) );

	// �����Ŋ��蓖��
	return pools_[i].Alloc();
}

void MemoryManager::DeAlloc( void* ptr, size_t siz )
{
#if defined(SUPERTINY) && defined(_DEBUG)
	--allocCounter;
#endif

	// �T�C�Y���납�傫������Ȃ�
	// �f�t�H���g�� delete ���Z�q�ɔC����
	uint i = static_cast<uint>( siz-1 );
	if( i >= SMALL_MAX )
	{
		::operator delete( ptr );
		return; // VC�� return void ���o���Ȃ��Ƃ́c
	}

	// �}���`�X���b�h�Ή�
	AutoLock al(this);

	// �����ŉ��
	pools_[i].DeAlloc( ptr );
}

#else // USE_ORIGNAL_MEMMAN

MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
#ifdef SUPERTINY
	g_heap = ::GetProcessHeap();
#endif

	// �B��̃C���X�^���X�͎��ł�
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
#if defined(SUPERTINY) && defined(_DEBUG)
	// ���[�N���o�p
	if( allocCounter != 0 )
		::MessageBox( NULL, TEXT("MemoryLeak!"), NULL, MB_OK );
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
