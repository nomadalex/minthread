/**
 * @file  test.c
 * @author Kun Wang <ifreedom.cn@gmail.com>
 * @date 2013/04/01 07:46:16
 *
 *  Copyright  2013  Kun Wang <ifreedom.cn@gmail.com>
 */

#include "minthread.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

int quit;
mt_lock_t lock;
int counter;

void pub_func(void* arg)
{
	assert(arg == (void*)1);

	while (1)
	{
		mt_lock(lock);
		printf("Pub %d\n", counter);
		counter++;
		mt_unlock(lock);

		if (quit) break;

		mt_sleep(100);
	}
}

void cos_func(void* arg)
{
	assert(arg == (void*)1);

	while (1)
	{
		mt_lock(lock);
		printf("Cos %d\n", counter);
		counter++;
		mt_unlock(lock);

		if (quit) break;

		mt_sleep(100);
	}
}

int main(int argc, const char* argv[])
{
	mthread_t tid, tid2;

	quit = FALSE;
	lock = NULL;
	counter = 0;

//	mt_createCriticalLock(&lock);
	mt_createMutexLock(&lock);

	if (mt_createThread(&tid, pub_func, (void*)1, MT_EPRIORITY_AB_NORMAL) < 0)
	{
		printf("Create Thread Fail!\n");
		return -1;
	}
	if (mt_createThread(&tid2, cos_func, (void*)1, MT_EPRIORITY_AB_NORMAL) < 0)
	{
		printf("Create Thread Fail!\n");
		return -1;
	}

	mt_sleep(1000);

	quit = TRUE;
	mt_releaseThread(&tid);
	mt_releaseThread(&tid2);

//	mt_releaseCriticalLock(&lock);
	mt_releaseMutexLock(&lock);
}
