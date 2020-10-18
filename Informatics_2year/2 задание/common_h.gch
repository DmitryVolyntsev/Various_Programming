#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <time.h>
#include <locale.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

/*
 * Logging
 */

#define log(fmt, ...) do { int _errno = errno; fprintf (stderr, fmt "\n", ## __VA_ARGS__); errno = _errno; } while (0)
#define die(fmt, ...) do { log(fmt, ## __VA_ARGS__); exit(EXIT_FAILURE); } while (0)
#define die_ret(fmt, ...) do { log(fmt, ## __VA_ARGS__); return 1; } while (0)
#define chk(expr, fmt, ...) do { if (!(expr)) die(fmt, ## __VA_ARGS__); } while (0)

/*
 * RAII-style "destructors" for C
 */

#define _cleanup_(x) \
        __attribute__((cleanup(x)))

#define DEFINE_TRIVIAL_CLEANUP_FUNC(type, func)                 \
        static inline void func##p(type *p) {                   \
                if (*p)                                         \
                        func(*p);                               \
        }

#define DEFINE_TRIVIAL_CLEANUP_FUNC_UNSAFE(type, func)          \
        static inline void func##p(void *p) {                   \
                if (*(type **)p)                                \
                        func(*(type **)p);                      \
        }

DEFINE_TRIVIAL_CLEANUP_FUNC_UNSAFE(void*, free)
#define _cleanup_free_ \
	_cleanup_(freep)

static inline void safe_fclose(FILE* file)
{
        if (file != NULL)
                fclose(file);
}
DEFINE_TRIVIAL_CLEANUP_FUNC(FILE*, safe_fclose)
#define _cleanup_fclose_ \
	_cleanup_(safe_fclosep)

static inline void safe_close(int fd)
{
        if (fd >= 0)
                close(fd);

}
DEFINE_TRIVIAL_CLEANUP_FUNC(int, safe_close)
#define _cleanup_close_ \
	_cleanup_(safe_closep)

/*
 * Various operations
 */

#define elementsof(array) (sizeof(array) / sizeof(array[0]))
#define memzero(array) memset(&array, 0, sizeof(array))

static inline bool streq(const char *_1, const char *_2)
{
	assert(_1);
	assert(_2);

	return !strcmp(_1, _2);
}

static inline bool strempty(const char *arg)
{
	assert(arg);

	return arg[0] == '\0';
}

static inline char *snprintf_static(const char *fmt, ...)
{
	static char buf[1024];
	va_list ap;

	assert(fmt);

	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);

	return buf;
}

static inline int unlink_and_mkfifo(const char *path, int mode)
{
	int r;

	assert(path);

	r = unlink(path);
	if (r < 0 && errno != ENOENT) {
		log("Failed to unlink() \"%s\": %m", path);
		goto out;
	}

	r = mkfifo(path, mode);
	if (r < 0) {
		die("Failed to mkfifo() FIFO \"%s\": %m", path);
		goto out;
	}

out:
	return r;
}

static inline void astrftime(char **result, const char *format, time_t time)
{
	struct tm tm;
	_cleanup_free_ char *buf = NULL;
	size_t buf_size, r;

	assert(result);
	assert(format);

	buf_size = 32;
	buf = malloc(buf_size);
	assert(buf);

	localtime_r(&time, &tm);
	for (;;) {
		r = strftime(buf, buf_size, format, &tm);
		if (r != 0) {
			break;
		}
		buf_size *= 2;
		buf = realloc(buf, buf_size);
		assert(buf);
	}

	*result = buf;
	buf = NULL;
}

/*
 * Operations with signals
 */

static inline int sigaction_many(struct sigaction *act, ...)
{
	int r, sig;
	int has_errors = 0;

	assert(act);

	va_list ap;

	va_start(ap, act);
	while ((sig = va_arg(ap, int)) > 0) {
		sigaddset(&act->sa_mask, sig);
	}
	va_end(ap);

	va_start(ap, act);
	while ((sig = va_arg(ap, int)) > 0) {
		r = sigaction(sig, act, NULL);
		if (r < 0) {
			log("Failed to call sigaction() to install handler for signal %d: %m", sig);
			has_errors = -1;
		}
	}
	va_end(ap);

	return has_errors;
}

/*
 * Constructs a sigset_t with given signals included in it.
 * Signal 0 marks the end of the argument list.
 */
static inline sigset_t sigset_many(int signal, ...)
{
	sigset_t result;
	va_list ap;

	assert(signal != 0);

	sigemptyset(&result);
	sigaddset(&result, signal);

	va_start(ap, signal);
	while ((signal = va_arg(ap, int)) > 0) {
		sigaddset(&result, signal);
	}
	va_end(ap);

	return result;
}

/*
 * Constructs a sigset_t with all except given signals included in it.
 * Signal 0 marks the end of the argument list.
 */
static inline sigset_t sigset_inverse_many(int signal, ...)
{
	sigset_t result;
	va_list ap;

	assert(signal != 0);

	sigemptyset(&result);
	sigfillset(&result);
	sigdelset(&result, signal);

	va_start(ap, signal);
	while ((signal = va_arg(ap, int)) > 0) {
		sigdelset(&result, signal);
	}
	va_end(ap);

	return result;
}

/*
 * Message send/receive over FIFO
 */

struct MessageHeader
{
	size_t length; /* including the header */
} __attribute__((packed));

static inline int read_request(int fd, void **pbuffer)
{
	int r;
	struct MessageHeader *header;
	ssize_t payload_length;

	assert(pbuffer);
	header = *pbuffer;

	if (header == NULL) {
		header = malloc(sizeof(*header));
		assert(header);
	}

	r = read(fd, header, sizeof(*header));
	if (r == 0) {
		goto out;
	}
	if (r != sizeof(*header)) {
		r = -1;
		log("Failed to read() message header (%zu bytes) from fd %d: %m", sizeof(*header), fd);
		goto out;
	}

	payload_length = header->length - sizeof(*header);
	if (payload_length > 0) {
		header = realloc(header, header->length);
		assert(header);

		r = read(fd, header + 1, payload_length);
		if (r != payload_length) {
			r = -1;
			log("Failed to read() message payload (%zu bytes) from fd %d: %m", payload_length, fd);
			goto out;
		}
	}

out:
	*pbuffer = header;
	return r;
}

static inline int make_request(void **pbuffer, size_t size)
{
	struct MessageHeader *header;

	assert(!*pbuffer);

	header = malloc(size);
	assert(header);

	header->length = size;

	*pbuffer = header;
	return 0;
}

static inline int send_request(int fd, struct MessageHeader *buffer)
{
	int r;

	assert(buffer);

	if (buffer->length > PIPE_BUF) {
		log("Will not send a request of %zu bytes (which is more than PIPE_BUF) because it is not atomic.", buffer->length);
		errno = ENOSPC;
		return -1;
	}

	r = write(fd, buffer, buffer->length);
	if (r < 0 || (unsigned)r != buffer->length) {
		log("Failed to write() message header (%zu bytes) to fd %d: %m", buffer->length, fd);
		return -1;
	}

	return 0;
}

/*
 * Various fd operations.
 */

static inline int open_wronly_nonblock(const char *path, int flags)
{
	_cleanup_close_ int rd_fd = -1;
	int wr_fd;

	assert(path);

	rd_fd = open(path, O_RDONLY|O_NONBLOCK);
	if (rd_fd < 0) {
		log("Failed to open() reading end of \"%s\" in non-blocking mode: %m", path);
		return -1;
	}

	wr_fd = open(path, O_WRONLY|O_NONBLOCK|flags);
	if (wr_fd < 0) {
		log("Failed to open() writing end of \"%s\" in non-blocking mode with reading end opened: %m", path);
		return -1;
	}

	return wr_fd;
}

static inline int fd_make_blocking(int fd)
{
	int r;

	r = fcntl(fd, F_GETFL);
	if (r < 0) {
		log("Failed to fcntl(F_GETFL) fd %d: %m", fd);
		return -1;
	}

	if (r & O_NONBLOCK) {
		r = fcntl(fd, F_SETFL, r & ~O_NONBLOCK);
		if (r < 0) {
			log("Failed to fcntl(F_SETFL) fd %d to make blocking: %m", fd);
			return -1;
		}
	}

	return 0;
}

enum fd_cat_result
{
	RESULT_OK,
	RESULT_READ_NIL,  /* EOF */
	RESULT_READ_ERR,
	RESULT_WRITE_ERR, /* or a short write */
	RESULT_MISC_ERR
};

static inline int fd_cat_iter(int fd_in, int fd_out, char *buf, size_t buffer_size)
{
	ssize_t read_bytes, written_bytes;

	assert(buf);

	read_bytes = read(fd_in, buf, buffer_size);

	if (read_bytes < 0) {
		log("Failed to read() %zd bytes: %m", buffer_size);
		return RESULT_READ_ERR;
	} else if (read_bytes == 0) {
		return RESULT_READ_NIL;
	}

	written_bytes = write(fd_out, buf, read_bytes);

	if (written_bytes < 0) {
		log("Failed to write() %zd bytes: %m", read_bytes);
		return RESULT_WRITE_ERR;
	} else if (written_bytes != read_bytes) {
		log("Short write(): %zd bytes out of %zd: %m", written_bytes, read_bytes);
		return RESULT_WRITE_ERR;
	}

	return RESULT_OK;
}

static inline int fd_cat(int fd_in, int fd_out, size_t buffer_size)
{
	int r;
	char *buf = malloc(buffer_size);

	if (!buf) {
		log("Failed to malloc() buffer of %zu bytes: %m", buffer_size);
		return RESULT_MISC_ERR;
	}


	/*
	 * First iteration is separated to detect early EOFs.
	 */

	r = fd_cat_iter(fd_in, fd_out, buf, buffer_size);
	if (r != RESULT_OK) {
		/*
		 * Either an error or an EOF-before-all.
		 */
		goto cleanup;
	}

	for (;;) {
		r = fd_cat_iter(fd_in, fd_out, buf, buffer_size);
		switch (r) {
		/*
		 * EOF after we've read something is OK.
		 */
		case RESULT_READ_NIL:
			r = RESULT_OK;
			goto cleanup;

		/*
		 * OK means continue to read.
		 */
		case RESULT_OK:
			break;

		/*
		 * Otherwise, it's an error.
		 */
		default:
			goto cleanup;
		}
	}

cleanup:
	free(buf);
	return r;
}

/*
 * XSI (System V) shared memory and semaphore API operations.
 */

void shm_unget(int shm)
{
	if (shm > 0) {
		int r = shmctl(shm, IPC_RMID, NULL);
		if (r < 0 && errno != EINVAL) {
			log("Failed to shmctl(IPC_RMID) the shared memory segment %d: %m", shm);
		}
	}
}

DEFINE_TRIVIAL_CLEANUP_FUNC(int, shm_unget)
#define _cleanup_shm_ \
		_cleanup_(shm_ungetp)

DEFINE_TRIVIAL_CLEANUP_FUNC_UNSAFE(void *, shmdt)
#define _cleanup_detach_ \
		_cleanup_(shmdtp)

static inline int shm_get_and_attach_slave(int ipc_key, size_t size, void **result)
{
	int shm = shmget(ipc_key, size, 0);
	void *memory;

	if (shm > 0) {
		/* OK */
	} else if (errno != ENOENT) {
		/* failed, and failure is not "inexistent" */
		log("Failed to initially shmget() the shared memory segment: %m");
	} else {
		/* failed and failure is "inexistent", try to create it */
		log("Shared memory segment not created -- sleeping for one second.");
		sleep(1);
		return shm_get_and_attach_slave(ipc_key, size, result);
	}

	if (shm > 0) {
		memory = shmat(shm, NULL, 0);
		if (memory == (void *)-1) {
			log("Failed to shmat() the shared memory segment %d: %m", shm);
			return -1;
		}
		*result = memory;
	}

	/* all done (or not) */
	return shm;
}

static inline int shm_get_and_attach_master(int ipc_key, size_t size, int mode, void **result)
{
	_cleanup_shm_ int shm = shmget(ipc_key, size, IPC_CREAT | mode);
	void *memory;
	int r;

	if (shm > 0) {
		/* OK */
	} else {
		/* failed */
		log("Failed to shmget(IPC_CREAT) a shared memory segment of %zu bytes: %m", size);
	}

	if (shm > 0) {
		memory = shmat(shm, NULL, 0);
		if (memory == (void *)-1) {
			log("Failed to shmat() the shared memory segment %d: %m", shm);
			return -1;
		}
		*result = memory;
	}

	/* all done (or not), avoid cleanup handler */
	r = shm; shm = 0;
	return r;
}

static inline int shm_get_and_attach(int ipc_key, size_t size, int mode, bool *shm_created, void **result)
{
	/* first, get the existing shared memory segment */
	_cleanup_shm_ int shm = shmget(ipc_key, size, 0);
	void *memory;
	int r;

	if (shm > 0) {
		/* OK */
		*shm_created = false;
	} else if (errno != ENOENT) {
		/* failed, and failure is not "inexistent" */
		log("Failed to initially shmget() the shared memory segment: %m");
	} else {
		/* failed and failure is "inexistent", try to create it */
		shm = shmget(ipc_key, size, IPC_CREAT | IPC_EXCL | mode);
		if (shm > 0) {
			*shm_created = true;
		} else if (errno != EEXIST) {
			/* failed and not due to the race */
			log("Failed to shmget(IPC_CREAT) a shared memory segment of %zu bytes: %m", size);
		} else {
			/* lost the race against another shm_get_and_attach(), repeat everything */
			return shm_get_and_attach(ipc_key, size, mode, shm_created, result);
		}
	}

	if (shm > 0) {
		memory = shmat(shm, NULL, 0);
		if (memory == (void *)-1) {
			log("Failed to shmat() the shared memory segment %d: %m", shm);
			return -1;
		}
		*result = memory;
	}

	/* all done (or not), avoid cleanup handler */
	r = shm; shm = 0;
	return r;
}

void sem_unget(int sem)
{
	if (sem > 0) {
		int r = semctl(sem, 0, IPC_RMID);
		if (r < 0 && errno != EINVAL) {
			log("Failed to semctl(IPC_RMID) the semaphore set %d: %m", sem);
		}
	}
}

DEFINE_TRIVIAL_CLEANUP_FUNC(int, sem_unget)
#define _cleanup_sem_ \
		_cleanup_(sem_ungetp)

union semun {
	int val;
	struct semid_ds *ds;
	unsigned short *array;
};

static inline union semun sem_arg_array(unsigned short *sem_array)
{
	union semun sem_arg = {
		.array = sem_array
	};
	return sem_arg;
}

static inline union semun sem_arg_ds(struct semid_ds *sem_ds)
{
	union semun sem_arg = {
		.ds = sem_ds
	};
	return sem_arg;
}

static inline union semun sem_arg_val(int sem_val)
{
	union semun sem_arg = {
		.val = sem_val
	};
	return sem_arg;
}

static inline int sem_wait_for_otime(int sem)
{
	int r;

	for (;;) {
		struct semid_ds sem_ds;

		r = semctl(sem, 0, IPC_STAT, sem_arg_ds(&sem_ds));
		if (r < 0) {
			log("Failed to semctl(IPC_STAT) the semaphore set %d: %m", sem);
			return r;
		}
		if (sem_ds.sem_otime != 0) {
			break;
		}
		log("Semaphore set created and not initialized -- sleeping for one second.");
		sleep(1);
	}

	return r;
}

/*
 * Creates a struct sembuf.
 */

static inline struct sembuf semop_entry(unsigned short sem_num, short sem_op, short sem_flg)
{
	struct sembuf sembuf = {
		.sem_num = sem_num,
		.sem_op = sem_op,
		.sem_flg = sem_flg
	};

	return sembuf;
}

/*
 * A shorthand for issuing a semop() for many operations.
 */

static inline int semop_many(int sem, size_t ops_nr, ...)
{
	struct sembuf *ops = alloca(sizeof(struct sembuf) * ops_nr);
	va_list ap;

	va_start(ap, ops_nr);
	for (size_t i = 0; i < ops_nr; ++i) {
		ops[i] = va_arg(ap, struct sembuf);
	}
	va_end(ap);

	return semop(sem, ops, ops_nr);
}

/*
 * A shorthand for issuing a semop() for one operation and arbitrary adjustment of semadj.
 */

static inline int semop_one_and_adj(int sem, unsigned short sem_num, short sem_op, short sem_flg, short adj)
{
	struct sembuf op = semop_entry(sem_num, sem_op, sem_flg);

	if (adj == 0) {
		return semop_many(sem, 1, op);
	} else {
		struct sembuf adjust = semop_entry(sem_num, adj, 0),
		              revert = semop_entry(sem_num, -adj, SEM_UNDO);

		if (adj > 0) {
			return semop_many(sem, 3, op, adjust, revert);
		} else {
			return semop_many(sem, 3, op, revert, adjust);
		}
	}
}

/*
 * Adjusts the semadj values of a semaphore set.
 *
 * For positive adjustments, this is done by performing the specified number of V operations
 * and then the same number of P operations with SEM_UNDO specified, all in the same semop() call
 * (hence atomically).
 *
 * Returns 1 if any adjustments were actually made, 0 if all values were zero, -1 (with errno set) on errors.
 */
static inline int sem_adj_many(int sem, int count, const short *adj_values)
{
	struct sembuf *sem_adj_ops;
	size_t sem_adj_ops_nr = 0;
	int r;

	assert(count > 0);

	/* we need `count * 2` operations at most */
	sem_adj_ops = alloca(sizeof(struct sembuf) * count * 2);

	for (unsigned short i = 0; i < count; ++i) {
		struct sembuf adjust = semop_entry(i, adj_values[i], 0),
		              revert = semop_entry(i, -adj_values[i], SEM_UNDO);

		if (adj_values[i] > 0) {
			sem_adj_ops[sem_adj_ops_nr++] = adjust; /* V */
			sem_adj_ops[sem_adj_ops_nr++] = revert; /* P */
		} else if (adj_values[i] < 0) {
			sem_adj_ops[sem_adj_ops_nr++] = revert; /* V */
			sem_adj_ops[sem_adj_ops_nr++] = adjust; /* P */
		}
	}

	if (sem_adj_ops_nr) {
		r = semop(sem, sem_adj_ops, sem_adj_ops_nr);
		if (r < 0) {
			return r;
		}
		return 1;
	} else {
		return 0;
	}
}

static inline int sem_get_and_init_slave(int ipc_key, int count, const short *adj_values)
{
	int sem = semget(ipc_key, count, 0);

	if (sem > 0) {
		/* OK, wait for it to be initialized */
		int r = sem_wait_for_otime(sem);
		if (r < 0) {
			log("Failed to wait for initialization of the semaphore set %d: %m", sem);
			return r;
		}

		/* ...and set our own semadj values */
		r = sem_adj_many(sem, count, adj_values);
		if (r < 0) {
			log("Failed to set initial semadj values of the semaphore set %d: %m", sem);
			return r;
		}
	} else if (errno != ENOENT) {
		/* failed, and failure is not "inexistent" */
		log("Failed to semget() the semaphore set: %m");
	} else {
		/* failed and failure is "inexistent", wait */
		log("Semaphore set not created -- sleeping for one second.");
		sleep(1);
		return sem_get_and_init_slave(ipc_key, count, adj_values);
	}

	return sem;
}

static inline int sem_get_and_init_master(int ipc_key, int count, int mode, const unsigned short *values, const short *adj_values)
{
	_cleanup_sem_ int sem = semget(ipc_key, count, IPC_CREAT | mode);
	int r;

	if (sem > 0) {
		/* OK, the initialization dance.
		 *
		 * - semctl(SETALL) does not set otime.
		 * - our method of setting semadj values does set otime, but only if there was something to set.
		 * - semop(..., { .sem_num = ..., .sem_op = 0, .sem_flg = IPC_NOWAIT }, 1) _hopefully_ sets otime
		 *   and does nothing else (if I read the man correctly).
		 *
		 * So let's do everything outlined here. First -- SETALL. */
		r = semctl(sem, 0, SETALL, sem_arg_array((unsigned short *)values));
		if (r < 0) {
			log("Failed to call semctl(SETALL) to initialize the semaphore set %d: %m", sem);
			return r;
		}

		/* then the initial semadj values (and set otime coincidentally if there are any nonzero initial values) */
		r = sem_adj_many(sem, count, adj_values);
		if (r < 0) {
			log("Failed to set initial semadj values of the semaphore set %d: %m", sem);
			return r;
		} else if (r == 0) {
			/* there were no semadj initial values to set, let's perform a dummy operation to set otime. */
			r = semop_many(sem, 1, semop_entry(0, 0, IPC_NOWAIT));
			if (r < 0) {
				log("Failed to do a dummy semop() on the semaphore set %d: %m", sem);
				return r;
			}
		}
	} else {
		/* failed */
		log("Failed to semget(IPC_CREAT) the semaphore set: %m");
	}

	/* all done (or not), avoid cleanup handler */
	r = sem; sem = 0;
	return r;
}

static inline int sem_get_and_init(int ipc_key, int count, int mode, const unsigned short *values, const short *adj_values)
{
	/* first, get the existing semaphore */
	_cleanup_sem_ int sem = semget(ipc_key, count, 0);
	int r;

	if (sem > 0) {
		/* OK, wait for it to be initialized */
		int r = sem_wait_for_otime(sem);
		if (r < 0) {
			log("Failed to wait for initialization of the semaphore set %d: %m", sem);
			return r;
		}

		/* ...and set our own semadj values */
		r = sem_adj_many(sem, count, adj_values);
		if (r < 0) {
			log("Failed to set initial semadj values of the semaphore set %d: %m", sem);
			return r;
		}
	} else if (errno != ENOENT) {
		/* failed, and failure is not "inexistent" */
		log("Failed to initially semget() the semaphore set: %m");
	} else {
		/* failed and failure is "inexistent", try to create it */
		sem = semget(ipc_key, count, IPC_CREAT | IPC_EXCL | mode);
		if (sem > 0) {
			/* OK, the initialization dance.
			 *
			 * - semctl(SETALL) does not set otime.
			 * - our method of setting semadj values does set otime, but only if there was something to set.
			 * - semop(..., { .sem_num = ..., .sem_op = 0, .sem_flg = IPC_NOWAIT }, 1) _hopefully_ sets otime
			 *   and does nothing else (if I read the man correctly).
			 *
			 * So let's do everything outlined here. First -- SETALL. */
			r = semctl(sem, 0, SETALL, sem_arg_array((unsigned short *)values));
			if (r < 0) {
				log("Failed to call semctl(SETALL) to initialize the semaphore set %d: %m", sem);
				return r;
			}

			/* then the initial semadj values (and set otime coincidentally if there are any nonzero initial values) */
			r = sem_adj_many(sem, count, adj_values);
			if (r < 0) {
				log("Failed to set initial semadj values of the semaphore set %d: %m", sem);
				return r;
			} else if (r == 0) {
				/* there were no semadj initial values to set, let's perform a dummy operation to set otime. */
				r = semop_many(sem, 1, semop_entry(0, 0, IPC_NOWAIT));
				if (r < 0) {
					log("Failed to do a dummy semop() on the semaphore set %d: %m", sem);
					return r;
				}
			}
		} else if (errno != EEXIST) {
			/* failed and not due to the race */
			log("Failed to semget(IPC_CREAT) the semaphore set: %m");
		} else {
			/* lost the race against another sem_get_and_init(), repeat everything */
			return sem_get_and_init(ipc_key, count, mode, values, adj_values);
		}
	}

	/* all done (or not), avoid cleanup handler */
	r = sem; sem = 0;
	return r;
}
