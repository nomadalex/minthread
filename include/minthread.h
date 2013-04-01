/**
 * @file  minthread.h
 * @author Kun Wang <ifreedom.cn@gmail.com>
 * @date 2013/04/01 03:07:42
 *
 *  Copyright  2013  Kun Wang <ifreedom.cn@gmail.com>
 */

#ifndef _MINTHREAD_H
#define _MINTHREAD_H

#include <stdint.h>

typedef void (*mthread_func)(void* arg);

typedef struct mthread_s* mthread_t;
typedef struct mt_lock_s* mt_lock_t;

typedef int MTenum;

#define MT_SUCCESS 0

#define MT_ERR_CREATE_FAILED -1
#define MT_ERR_WRONG_TYPE -2

#define MT_EPRIORITY_HIGHEST    1
#define MT_EPRIORITY_AB_NORMAL  2
#define MT_EPRIORITY_NORMAL     3
#define MT_EPRIORITY_BE_NORMAL  4
#define MT_EPRIORITY_LOWEST     5

#ifdef __cplusplus
extern "C" {
#endif

	MTenum mt_createThread(mthread_t* t, mthread_func func, void* arg, MTenum priority);
	void mt_releaseThread(mthread_t* t);

	MTenum mt_createCriticalLock(mt_lock_t* l);
	MTenum mt_releaseCriticalLock(mt_lock_t* l);

	MTenum mt_createMutexLock(mt_lock_t* l);
	MTenum mt_releaseMutexLock(mt_lock_t* l);

	void mt_lock(mt_lock_t lock);
	void mt_unlock(mt_lock_t lock);

	void mt_sleep(int ms);
	uint32_t mt_getTick();

#ifdef __cplusplus
}
#endif

#endif /* _MINTHREAD_H */
