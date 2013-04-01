/**
 * @file  minthread.c
 * @author Kun Wang <ifreedom.cn@gmail.com>
 * @date 2013/03/29 07:57:03
 *
 *  Copyright  2013  Kun Wang <ifreedom.cn@gmail.com>
 */

#include "minthread.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else

#include <sys/time.h>
#include <pthread.h>

#endif

#include <stdlib.h>
#include <string.h>

#define LOCKTYPE_CRITICAL 1
#define LOCKTYPE_MUTEX 2

struct mthread_s
{
#ifdef WIN32
	HANDLE thread;
#else
	pthread_t thread;
#endif

	mthread_func func;
	void* arg;
};

struct mt_lock_s
{
	int type;
};

#ifdef WIN32
DWORD WINAPI thread_func(LPVOID arg)
#else
static void* thread_func(void* arg)
#endif
{
	mthread_t pt = (mthread_t)arg;

	pt->func(pt->arg);

	return 0;
}

MTenum mt_createThread(mthread_t* t, mthread_func func, void* arg, MTenum priority)
{
	mthread_t pt;

	pt = (mthread_t)malloc(sizeof(struct mthread_s));

	pt->func = func;
	pt->arg = arg;

#ifdef WIN32
	pt->thread = CreateThread(0, 0, thread_func, pt, 0, NULL);
	if (pt->thread == INVALID_HANDLE_VALUE) goto error;

	{
		int pr;
		switch (priority)
		{
		case MT_EPRIORITY_HIGHEST:
			pr = THREAD_PRIORITY_HIGHEST;
			break;

		case MT_EPRIORITY_AB_NORMAL:
			pr = THREAD_PRIORITY_ABOVE_NORMAL;
			break;

		case MT_EPRIORITY_NORMAL:
			pr = THREAD_PRIORITY_NORMAL;
			break;

		case MT_EPRIORITY_BE_NORMAL:
			pr = THREAD_PRIORITY_BELOW_NORMAL;
			break;

		case MT_EPRIORITY_LOWEST:
			pr = THREAD_PRIORITY_LOWEST;
			break;
		}

		SetThreadPriority(pt->thread, pr);
	}

#else
	{
		pthread_attr_t attr;

		struct sched_param param;

		pthread_attr_init(&attr);

		pthread_attr_setschedpolicy(&attr, SCHED_RR);

		switch (priority)
		{
		case MT_EPRIORITY_HIGHEST:
			param.sched_priority = 50;
			break;

		case MT_EPRIORITY_AB_NORMAL:
			param.sched_priority = 30;
			break;

		case MT_EPRIORITY_NORMAL:
			param.sched_priority = 20;
			break;

		case MT_EPRIORITY_BE_NORMAL:
			param.sched_priority = 10;
			break;

		case MT_EPRIORITY_LOWEST:
			param.sched_priority = 0;
			break;
		}

		pthread_attr_setschedparam(&attr, &param);

		if (pthread_create(&pt->thread, &attr, thread_func, pt) < 0) goto error;

		pthread_attr_destroy(&attr);
	}
#endif

	*t = pt;
	return MT_SUCCESS;

error:
	free(pt);
	return MT_ERR_CREATE_FAILED;
}

void mt_releaseThread(mthread_t* t)
{
	mthread_t pt = *t;

	if (pt == NULL) return;

#ifdef WIN32
	WaitForSingleObject(pt->thread, INFINITE);
	CloseHandle(pt->thread);
#else
	pthread_join(pt->thread, NULL);
#endif

	free(pt);
	*t = NULL;
}

void mt_sleep(int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	struct timespec req, rem;
	req.tv_sec = timeInMs/1000;
	req.tv_nsec = (timeInMs%1000) * 1000000L;

	while (1)
	{
		if(nanosleep(&req , &rem) == 0)
			break;

		req = rem;
	}
#endif
}

uint32_t mt_getTick()
{
#ifdef WIN32
	return GetTickCount();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32_t now = tv.tv_sec * 1000 + tv.tv_usec/1000;

	return now;
#endif
}

typedef struct mt_lock_critical_s
{
	int type;
#ifdef WIN32
	CRITICAL_SECTION data;
#else
	pthread_spinlock_t data;
#endif
} mt_lock_critical_t;

MTenum mt_createCriticalLock(mt_lock_t* l)
{
	mt_lock_critical_t* pl = (mt_lock_critical_t*)malloc(sizeof(mt_lock_critical_t));

	pl->type = LOCKTYPE_CRITICAL;

#ifdef WIN32
	InitializeCriticalSection(&pl->data);
#else
	pthread_spin_init(&pl->data, 0);
#endif

	*l = (mt_lock_t)pl;

	return MT_SUCCESS;
}

MTenum mt_releaseCriticalLock(mt_lock_t* l)
{
	mt_lock_critical_t* pl = (mt_lock_critical_t*)(*l);

	if (pl == NULL) return MT_SUCCESS;
	if (pl->type != LOCKTYPE_CRITICAL) return MT_ERR_WRONG_TYPE;

#ifdef WIN32
	DeleteCriticalSection(&pl->data);
#else
	pthread_spin_destroy(&pl->data);
#endif

	free(pl);
	*l = NULL;
	return MT_SUCCESS;
}

typedef struct mt_lock_mutex_s
{
	int type;
#ifdef WIN32
	HANDLE data;
	int needRelease;
#else
	pthread_mutex_t data;
#endif
} mt_lock_mutex_t;

MTenum mt_createMutexLock(mt_lock_t* l)
{
	mt_lock_mutex_t* pl = (mt_lock_mutex_t*)malloc(sizeof(mt_lock_mutex_t));

	pl->type = LOCKTYPE_MUTEX;

#ifdef WIN32
	pl->data = CreateMutex(NULL, FALSE, NULL);
	if (pl->data == INVALID_HANDLE_VALUE) goto error;
	pl->needRelease = FALSE;
#else
	pthread_mutex_init(&pl->data);
#endif

	*l = (mt_lock_t)pl;
	return MT_SUCCESS;

error:
	free(pl);
	return MT_ERR_CREATE_FAILED;
}

MTenum mt_releaseMutexLock(mt_lock_t* l)
{
	mt_lock_mutex_t* pl = (mt_lock_mutex_t*)(*l);

	if (pl == NULL) return MT_SUCCESS;
	if (pl->type != LOCKTYPE_MUTEX) return MT_ERR_WRONG_TYPE;

#ifdef WIN32
	CloseHandle(pl->data);
#else
	pthread_mutex_destroy(&pl->data);
#endif

	free(pl);
	*l = NULL;
	return MT_SUCCESS;
}

void mt_lock(mt_lock_t lock)
{
	if (lock == NULL) return;

	switch (lock->type)
	{
	case LOCKTYPE_CRITICAL:
	{
		mt_lock_critical_t* pl = (mt_lock_critical_t*)lock;
#ifdef WIN32
		EnterCriticalSection(&pl->data);
#else
		pthread_spin_lock(&pl->data);
#endif
	}
	break;

	case LOCKTYPE_MUTEX:
	{
		DWORD _ret;
		mt_lock_mutex_t* pl = (mt_lock_mutex_t*)lock;
#ifdef WIN32
		_ret = WaitForSingleObject(pl->data, INFINITE);
		if (_ret == WAIT_OBJECT_0)
			pl->needRelease = TRUE;
#else
		pthread_mutex_lock(&pl->data);
#endif
	}
	break;
	}
}

void mt_unlock(mt_lock_t lock)
{
	switch (lock->type)
	{
	case LOCKTYPE_CRITICAL:
	{
		mt_lock_critical_t* pl = (mt_lock_critical_t*)lock;
#ifdef WIN32
		LeaveCriticalSection(&pl->data);
#else
		pthread_spin_unlock(&pl->data);
#endif
	}
		break;

	case LOCKTYPE_MUTEX:
	{
		mt_lock_mutex_t* pl = (mt_lock_mutex_t*)lock;
#ifdef WIN32
		if (pl->needRelease)
			ReleaseMutex(pl->data);
#else
		pthread_mutex_unlock(&pl->data);
#endif
	}
	break;
	}
}
