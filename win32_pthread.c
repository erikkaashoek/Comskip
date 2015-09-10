/*
 * Copyright (C) 2009 Andrzej K. Haczewski <ahaczewski@gmail.com>
 *
 * DISCLAIMER: The implementation is Git-specific, it is subset of original
 * Pthreads API, without lots of other features that Git doesn't use.
 * Git also makes sure that the passed arguments are valid, so there's
 * no need for double-checking.
 */

#include "win32_pthread.h"

int err_win_to_posix(DWORD winerr)
{
	int error = ENOSYS;
	switch(winerr) {
	case ERROR_ACCESS_DENIED: error = EACCES; break;
	case ERROR_ACCOUNT_DISABLED: error = EACCES; break;
	case ERROR_ACCOUNT_RESTRICTION: error = EACCES; break;
	case ERROR_ALREADY_ASSIGNED: error = EBUSY; break;
	case ERROR_ALREADY_EXISTS: error = EEXIST; break;
	case ERROR_ARITHMETIC_OVERFLOW: error = ERANGE; break;
	case ERROR_BAD_COMMAND: error = EIO; break;
	case ERROR_BAD_DEVICE: error = ENODEV; break;
	case ERROR_BAD_DRIVER_LEVEL: error = ENXIO; break;
	case ERROR_BAD_EXE_FORMAT: error = ENOEXEC; break;
	case ERROR_BAD_FORMAT: error = ENOEXEC; break;
	case ERROR_BAD_LENGTH: error = EINVAL; break;
	case ERROR_BAD_PATHNAME: error = ENOENT; break;
	case ERROR_BAD_PIPE: error = EPIPE; break;
	case ERROR_BAD_UNIT: error = ENODEV; break;
	case ERROR_BAD_USERNAME: error = EINVAL; break;
	case ERROR_BROKEN_PIPE: error = EPIPE; break;
	case ERROR_BUFFER_OVERFLOW: error = ENAMETOOLONG; break;
	case ERROR_BUSY: error = EBUSY; break;
	case ERROR_BUSY_DRIVE: error = EBUSY; break;
	case ERROR_CALL_NOT_IMPLEMENTED: error = ENOSYS; break;
	case ERROR_CANNOT_MAKE: error = EACCES; break;
	case ERROR_CANTOPEN: error = EIO; break;
	case ERROR_CANTREAD: error = EIO; break;
	case ERROR_CANTWRITE: error = EIO; break;
	case ERROR_CRC: error = EIO; break;
	case ERROR_CURRENT_DIRECTORY: error = EACCES; break;
	case ERROR_DEVICE_IN_USE: error = EBUSY; break;
	case ERROR_DEV_NOT_EXIST: error = ENODEV; break;
	case ERROR_DIRECTORY: error = EINVAL; break;
	case ERROR_DIR_NOT_EMPTY: error = ENOTEMPTY; break;
	case ERROR_DISK_CHANGE: error = EIO; break;
	case ERROR_DISK_FULL: error = ENOSPC; break;
	case ERROR_DRIVE_LOCKED: error = EBUSY; break;
	case ERROR_ENVVAR_NOT_FOUND: error = EINVAL; break;
	case ERROR_EXE_MARKED_INVALID: error = ENOEXEC; break;
	case ERROR_FILENAME_EXCED_RANGE: error = ENAMETOOLONG; break;
	case ERROR_FILE_EXISTS: error = EEXIST; break;
	case ERROR_FILE_INVALID: error = ENODEV; break;
	case ERROR_FILE_NOT_FOUND: error = ENOENT; break;
	case ERROR_GEN_FAILURE: error = EIO; break;
	case ERROR_HANDLE_DISK_FULL: error = ENOSPC; break;
	case ERROR_INSUFFICIENT_BUFFER: error = ENOMEM; break;
	case ERROR_INVALID_ACCESS: error = EACCES; break;
	case ERROR_INVALID_ADDRESS: error = EFAULT; break;
	case ERROR_INVALID_BLOCK: error = EFAULT; break;
	case ERROR_INVALID_DATA: error = EINVAL; break;
	case ERROR_INVALID_DRIVE: error = ENODEV; break;
	case ERROR_INVALID_EXE_SIGNATURE: error = ENOEXEC; break;
	case ERROR_INVALID_FLAGS: error = EINVAL; break;
	case ERROR_INVALID_FUNCTION: error = ENOSYS; break;
	case ERROR_INVALID_HANDLE: error = EBADF; break;
	case ERROR_INVALID_LOGON_HOURS: error = EACCES; break;
	case ERROR_INVALID_NAME: error = EINVAL; break;
	case ERROR_INVALID_OWNER: error = EINVAL; break;
	case ERROR_INVALID_PARAMETER: error = EINVAL; break;
	case ERROR_INVALID_PASSWORD: error = EPERM; break;
	case ERROR_INVALID_PRIMARY_GROUP: error = EINVAL; break;
	case ERROR_INVALID_SIGNAL_NUMBER: error = EINVAL; break;
	case ERROR_INVALID_TARGET_HANDLE: error = EIO; break;
	case ERROR_INVALID_WORKSTATION: error = EACCES; break;
	case ERROR_IO_DEVICE: error = EIO; break;
	case ERROR_IO_INCOMPLETE: error = EINTR; break;
	case ERROR_LOCKED: error = EBUSY; break;
	case ERROR_LOCK_VIOLATION: error = EACCES; break;
	case ERROR_LOGON_FAILURE: error = EACCES; break;
	case ERROR_MAPPED_ALIGNMENT: error = EINVAL; break;
	case ERROR_META_EXPANSION_TOO_LONG: error = E2BIG; break;
	case ERROR_MORE_DATA: error = EPIPE; break;
	case ERROR_NEGATIVE_SEEK: error = ESPIPE; break;
	case ERROR_NOACCESS: error = EFAULT; break;
	case ERROR_NONE_MAPPED: error = EINVAL; break;
	case ERROR_NOT_ENOUGH_MEMORY: error = ENOMEM; break;
	case ERROR_NOT_READY: error = EAGAIN; break;
	case ERROR_NOT_SAME_DEVICE: error = EXDEV; break;
	case ERROR_NO_DATA: error = EPIPE; break;
	case ERROR_NO_MORE_SEARCH_HANDLES: error = EIO; break;
	case ERROR_NO_PROC_SLOTS: error = EAGAIN; break;
	case ERROR_NO_SUCH_PRIVILEGE: error = EACCES; break;
	case ERROR_OPEN_FAILED: error = EIO; break;
	case ERROR_OPEN_FILES: error = EBUSY; break;
	case ERROR_OPERATION_ABORTED: error = EINTR; break;
	case ERROR_OUTOFMEMORY: error = ENOMEM; break;
	case ERROR_PASSWORD_EXPIRED: error = EACCES; break;
	case ERROR_PATH_BUSY: error = EBUSY; break;
	case ERROR_PATH_NOT_FOUND: error = ENOENT; break;
	case ERROR_PIPE_BUSY: error = EBUSY; break;
	case ERROR_PIPE_CONNECTED: error = EPIPE; break;
	case ERROR_PIPE_LISTENING: error = EPIPE; break;
	case ERROR_PIPE_NOT_CONNECTED: error = EPIPE; break;
	case ERROR_PRIVILEGE_NOT_HELD: error = EACCES; break;
	case ERROR_READ_FAULT: error = EIO; break;
	case ERROR_SEEK: error = EIO; break;
	case ERROR_SEEK_ON_DEVICE: error = ESPIPE; break;
	case ERROR_SHARING_BUFFER_EXCEEDED: error = ENFILE; break;
	case ERROR_SHARING_VIOLATION: error = EACCES; break;
	case ERROR_STACK_OVERFLOW: error = ENOMEM; break;
	case ERROR_SWAPERROR: error = ENOENT; break;
	case ERROR_TOO_MANY_MODULES: error = EMFILE; break;
	case ERROR_TOO_MANY_OPEN_FILES: error = EMFILE; break;
	case ERROR_UNRECOGNIZED_MEDIA: error = ENXIO; break;
	case ERROR_UNRECOGNIZED_VOLUME: error = ENODEV; break;
	case ERROR_WAIT_NO_CHILDREN: error = ECHILD; break;
	case ERROR_WRITE_FAULT: error = EIO; break;
	case ERROR_WRITE_PROTECT: error = EROFS; break;
	}
	return error;
}

void die(const char *err, ...)
{
	fputs(err, stderr);
	exit(128);
}

static unsigned __stdcall win32_start_routine(void *arg)
{
	pthread_t *thread = arg;
	thread->tid = GetCurrentThreadId();
	thread->arg = thread->start_routine(thread->arg);
	return 0;
}

int pthread_create(pthread_t *thread, const void *unused,
		   void *(*start_routine)(void*), void *arg)
{
	thread->arg = arg;
	thread->start_routine = start_routine;
	thread->handle = (HANDLE)
		_beginthreadex(NULL, 0, win32_start_routine, thread, 0, NULL);

	if (!thread->handle)
		return errno;
	else
		return 0;
}
extern int err_win_to_posix(DWORD winerr);
int win32_pthread_join(pthread_t *thread, void **value_ptr)
{
	DWORD result = WaitForSingleObject(thread->handle, INFINITE);
	switch (result) {
		case WAIT_OBJECT_0:
			if (value_ptr)
				*value_ptr = thread->arg;
			return 0;
		case WAIT_ABANDONED:
			return EINVAL;
		default:
			return err_win_to_posix(GetLastError());
	}
}

pthread_t pthread_self(void)
{
	pthread_t t = { NULL };
	t.tid = GetCurrentThreadId();
	return t;
}

int pthread_cond_init(pthread_cond_t *cond, const void *unused)
{
	cond->waiters = 0;
	cond->was_broadcast = 0;
	InitializeCriticalSection(&cond->waiters_lock);

	cond->sema = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	if (!cond->sema)
		die("CreateSemaphore() failed");

	cond->continue_broadcast = CreateEvent(NULL,	/* security */
				FALSE,			/* auto-reset */
				FALSE,			/* not signaled */
				NULL);			/* name */
	if (!cond->continue_broadcast)
		die("CreateEvent() failed");

	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	CloseHandle(cond->sema);
	CloseHandle(cond->continue_broadcast);
	DeleteCriticalSection(&cond->waiters_lock);
	return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, CRITICAL_SECTION *mutex)
{
	int last_waiter;

	EnterCriticalSection(&cond->waiters_lock);
	cond->waiters++;
	LeaveCriticalSection(&cond->waiters_lock);

	/*
	 * Unlock external mutex and wait for signal.
	 * NOTE: we've held mutex locked long enough to increment
	 * waiters count above, so there's no problem with
	 * leaving mutex unlocked before we wait on semaphore.
	 */
	LeaveCriticalSection(mutex);

	/* let's wait - ignore return value */
	WaitForSingleObject(cond->sema, INFINITE);

	/*
	 * Decrease waiters count. If we are the last waiter, then we must
	 * notify the broadcasting thread that it can continue.
	 * But if we continued due to cond_signal, we do not have to do that
	 * because the signaling thread knows that only one waiter continued.
	 */
	EnterCriticalSection(&cond->waiters_lock);
	cond->waiters--;
	last_waiter = cond->was_broadcast && cond->waiters == 0;
	LeaveCriticalSection(&cond->waiters_lock);

	if (last_waiter) {
		/*
		 * cond_broadcast was issued while mutex was held. This means
		 * that all other waiters have continued, but are contending
		 * for the mutex at the end of this function because the
		 * broadcasting thread did not leave cond_broadcast, yet.
		 * (This is so that it can be sure that each waiter has
		 * consumed exactly one slice of the semaphor.)
		 * The last waiter must tell the broadcasting thread that it
		 * can go on.
		 */
		SetEvent(cond->continue_broadcast);
		/*
		 * Now we go on to contend with all other waiters for
		 * the mutex. Auf in den Kampf!
		 */
	}
	/* lock external mutex again */
	EnterCriticalSection(mutex);

	return 0;
}

/*
 * IMPORTANT: This implementation requires that pthread_cond_signal
 * is called while the mutex is held that is used in the corresponding
 * pthread_cond_wait calls!
 */
int pthread_cond_signal(pthread_cond_t *cond)
{
	int have_waiters;

	EnterCriticalSection(&cond->waiters_lock);
	have_waiters = cond->waiters > 0;
	LeaveCriticalSection(&cond->waiters_lock);

	/*
	 * Signal only when there are waiters
	 */
	if (have_waiters)
		return ReleaseSemaphore(cond->sema, 1, NULL) ?
			0 : err_win_to_posix(GetLastError());
	else
		return 0;
}

/*
 * DOUBLY IMPORTANT: This implementation requires that pthread_cond_broadcast
 * is called while the mutex is held that is used in the corresponding
 * pthread_cond_wait calls!
 */
int pthread_cond_broadcast(pthread_cond_t *cond)
{
	EnterCriticalSection(&cond->waiters_lock);

	if ((cond->was_broadcast = cond->waiters > 0)) {
		/* wake up all waiters */
		ReleaseSemaphore(cond->sema, cond->waiters, NULL);
		LeaveCriticalSection(&cond->waiters_lock);
		/*
		 * At this point all waiters continue. Each one takes its
		 * slice of the semaphor. Now it's our turn to wait: Since
		 * the external mutex is held, no thread can leave cond_wait,
		 * yet. For this reason, we can be sure that no thread gets
		 * a chance to eat *more* than one slice. OTOH, it means
		 * that the last waiter must send us a wake-up.
		 */
		WaitForSingleObject(cond->continue_broadcast, INFINITE);
		/*
		 * Since the external mutex is held, no thread can enter
		 * cond_wait, and, hence, it is safe to reset this flag
		 * without cond->waiters_lock held.
		 */
		cond->was_broadcast = 0;
	} else {
		LeaveCriticalSection(&cond->waiters_lock);
	}
	return 0;
}
