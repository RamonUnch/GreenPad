#ifndef _KILIB_THREAD_H_
#define _KILIB_THREAD_H_
#include "types.h"
#ifndef __ccdoc__
namespace ki {
#endif

	

//=========================================================================
//@{ @pkg ki.Core //@}
//@{
//	マルチスレッドの管理
//
//	まだ何もしません
//@}
//=========================================================================


class ThreadManager
{
public:
	//@{ 実行可能オブジェクトをに起動をかける //@}
	void Run( class Runnable& r );

	//@{ 複数スレッドが走っているかどうか？ //@}
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

//@{ 唯一のスレッド管理オブジェクトを返す //@}
inline ThreadManager& thd()
	{ return *ThreadManager::pUniqueInstance_; }

inline bool ThreadManager::isMT() const
	{ return 0 != threadNum_; }



//=========================================================================
//@{
//	実行可能オブジェクト基底
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
//	マルチスレッド・ポリシー１
//
//	このクラスから派生すると、AutoLock クラスを使えるようになります。
//	このクラスに this ポインタを渡すことで排他状態に入り、デストラクタで
//	抜け出せるようになります。NoLockable::AutoLock は実際は何もしません。
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
//	マルチスレッド・ポリシー２
//
//	このクラスから派生すると、AutoLock クラスを使えるようになります。
//	このクラスに this ポインタを渡すことで排他状態に入り、デストラクタで
//	抜け出せるようになります。EzLockable::AutoLock は、シングルスレッドで
//	動作するときがほとんどというアプリケーション向けに、高速だけれど
//	不完全な排他制御を行います。２本目のスレッド立ち上げの瞬間が危ない。
//@}
//=========================================================================
class EzLockable
{
protected:
	EzLockable()
		{
		#ifdef USE_THREADS
			::InitializeCriticalSection( &csection_ ); 
		#endif
		}
	~EzLockable()
		{
		#ifdef USE_THREADS
			::DeleteCriticalSection( &csection_ ); 
		#endif
		}

	struct AutoLock
	{
		AutoLock( EzLockable* host )
		{
		#ifdef USE_THREADS
			if( NULL != (pCs_=(thd().isMT() ? &host->csection_ : NULL)) )
				::EnterCriticalSection( pCs_ );
		#endif
		}
		~AutoLock()
		{
		#ifdef USE_THREADS
			if( pCs_ )
				::LeaveCriticalSection( pCs_ );
		#endif
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
//	マルチスレッド・ポリシー３
//
//	このクラスから派生すると、AutoLock クラスを使えるようになります。
//	このクラスに this ポインタを渡すことで排他状態に入り、デストラクタで
//	抜け出せるようになります。Lockable::AutoLock は、完全な排他制御を
//	行います。万全を期すならかならずこのクラスを用いましょう。
//@}
//=========================================================================
#if 0 // Not used.
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
#endif // Not used


//=========================================================================

}      // namespace ki
#endif // _KILIB_THREAD_H_
