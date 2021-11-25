#include "stdafx.h"
#include "thread.h"
using namespace ki;



//=========================================================================

ThreadManager* ThreadManager::pUniqueInstance_;

ThreadManager::ThreadManager()
	: threadNum_(0)
{
	// 唯一のインスタンスは私です。
	pUniqueInstance_ = this;
}

//=========================================================================

DWORD WINAPI ThreadManager::ThreadStubFunc(void* param)
{
	Runnable* r = static_cast<Runnable*>(param);
	r->StartThread();
	r->FinalizeThread();
	return 0;
}

void ThreadManager::Run( Runnable& r )
{
	r.PleaseExit();

	DWORD id;
	r.hThread_ = ::CreateThread(
		NULL, 0, &ThreadStubFunc, &r, CREATE_SUSPENDED, &id );
	r.hEvent_  = ::CreateEvent( NULL, TRUE, FALSE, NULL );
	::ResumeThread( r.hThread_ );
}

//=========================================================================

Runnable::Runnable()
	: hThread_( NULL )
	, hEvent_ ( NULL )
{
}

Runnable::~Runnable()
{
	PleaseExit(); // あくまで最終手段
}

bool Runnable::isRunning() const
{
	return hThread_ != NULL;
}

bool Runnable::isExitRequested() const
{
	return WAIT_OBJECT_0 == ::WaitForSingleObject( hEvent_, 0 );
}

void Runnable::FinalizeThread()
{
	::CloseHandle( hThread_ );
	hThread_ = NULL;
	::CloseHandle( hEvent_ );
	hEvent_  = NULL;
}

void Runnable::PleaseExit()
{
	if( HANDLE ht = hThread_ )
	{
		::SetEvent( hEvent_ );
		::WaitForSingleObject( ht, INFINITE );
	}
}
