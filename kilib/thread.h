#ifndef _KILIB_THREAD_H_
#define _KILIB_THREAD_H_
#include "types.h"
#ifndef __ccdoc__
namespace ki {
#endif

	

//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	�}���`�X���b�h�̊Ǘ�
//
//	�܂��������܂���
//@}
//=========================================================================


class ThreadManager
{
public:
	//@{ ���s�\�I�u�W�F�N�g���ɋN���������� //@}
	void Run( class Runnable& r );

	//@{ �����X���b�h�������Ă��邩�ǂ����H //@}
	bool isMT() const;

private:

	ThreadManager();

private:

	int threadNum_;
	static ThreadManager* pUniqueInstance_;

private:

	static DWORD WINAPI ThreadStubFunc(void*);

private:

	friend void APIENTRY Startup();
	friend inline ThreadManager& thd();
	NOCOPY(ThreadManager);
};



//-------------------------------------------------------------------------

//@{ �B��̃X���b�h�Ǘ��I�u�W�F�N�g��Ԃ� //@}
inline ThreadManager& thd()
	{ return *ThreadManager::pUniqueInstance_; }

inline bool ThreadManager::isMT() const
	{ return 0 != threadNum_; }



//=========================================================================
//@{
//	���s�\�I�u�W�F�N�g���
//@}
//=========================================================================

class Runnable
{
protected:
	Runnable();
	virtual ~Runnable();

	virtual void StartThread() = 0;
	void PleaseExit();
	bool isExitRequested() const;
	bool isRunning() const;

private:
	void FinalizeThread();
	friend class ThreadManager;
	HANDLE hEvent_;
	HANDLE hThread_;
};



//=========================================================================
//@{
//	�}���`�X���b�h�E�|���V�[�P
//
//	���̃N���X����h������ƁAAutoLock �N���X���g����悤�ɂȂ�܂��B
//	���̃N���X�� this �|�C���^��n�����ƂŔr����Ԃɓ���A�f�X�g���N�^��
//	�����o����悤�ɂȂ�܂��BNoLockable::AutoLock �͎��ۂ͉������܂���B
//@}
//=========================================================================

class NoLockable
{
protected:
	struct AutoLock
	{
		AutoLock( NoLockable* host ) {}
	};
};



//=========================================================================
//@{
//	�}���`�X���b�h�E�|���V�[�Q
//
//	���̃N���X����h������ƁAAutoLock �N���X���g����悤�ɂȂ�܂��B
//	���̃N���X�� this �|�C���^��n�����ƂŔr����Ԃɓ���A�f�X�g���N�^��
//	�����o����悤�ɂȂ�܂��BEzLockable::AutoLock �́A�V���O���X���b�h��
//	���삷��Ƃ����قƂ�ǂƂ����A�v���P�[�V���������ɁA�����������
//	�s���S�Ȕr��������s���܂��B�Q�{�ڂ̃X���b�h�����グ�̏u�Ԃ���Ȃ��B
//@}
//=========================================================================

class EzLockable
{
protected:
	EzLockable()
		{ ::InitializeCriticalSection( &csection_ ); }
	~EzLockable()
		{ ::DeleteCriticalSection( &csection_ ); }

	struct AutoLock
	{
		AutoLock( EzLockable* host )
		{
			if( NULL != (pCs_=(thd().isMT() ? &host->csection_ : NULL)) )
				::EnterCriticalSection( pCs_ );
		}
		~AutoLock()
		{
			if( pCs_ )
				::LeaveCriticalSection( pCs_ );
		}
	private:
		NOCOPY(AutoLock);
	    CRITICAL_SECTION* pCs_;
	};

private:
	CRITICAL_SECTION csection_;
	#ifdef __DMC__
		friend struct EzLockable::AutoLock;
	#else
		friend struct AutoLock;
	#endif
};



//=========================================================================
//@{
//	�}���`�X���b�h�E�|���V�[�R
//
//	���̃N���X����h������ƁAAutoLock �N���X���g����悤�ɂȂ�܂��B
//	���̃N���X�� this �|�C���^��n�����ƂŔr����Ԃɓ���A�f�X�g���N�^��
//	�����o����悤�ɂȂ�܂��BLockable::AutoLock �́A���S�Ȕr�������
//	�s���܂��B���S�������Ȃ炩�Ȃ炸���̃N���X��p���܂��傤�B
//@}
//=========================================================================

class Lockable
{
protected:
	Lockable()
		{ ::InitializeCriticalSection( &csection_ ); }
	~Lockable()
		{ ::DeleteCriticalSection( &csection_ ); }

	struct AutoLock
	{
		AutoLock( Lockable* host )
		{
			pCs_ = &host->csection_;
			::EnterCriticalSection( pCs_ );
		}
		~AutoLock()
		{
			::LeaveCriticalSection( pCs_ );
		}
	private:
		NOCOPY(AutoLock);
	    CRITICAL_SECTION* pCs_;
	};
	friend struct AutoLock;

private:
	CRITICAL_SECTION csection_;
};



//=========================================================================

}      // namespace ki
#endif // _KILIB_THREAD_H_
