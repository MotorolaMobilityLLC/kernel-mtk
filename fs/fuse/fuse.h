#ifndef ___FUSE_H___
#define ___FUSE_H___
void fuse_request_send_background_ex(struct fuse_conn *fc,
	struct fuse_req *req, __u32 size);
void fuse_request_send_ex(struct fuse_conn *fc, struct fuse_req *req,
	__u32 size);

#if defined(CONFIG_FUSE_IO_LOG)  /* IO log is only enabled in eng load */
#define FUSEIO_TRACE
#endif


#ifdef FUSEIO_TRACE
#include <linux/sched.h>
#include <linux/kthread.h>

void fuse_time_diff(struct timespec *start,
	struct timespec *end,
	struct timespec *diff);

void fuse_iolog_add(__u32 io_bytes, int type,
	struct timespec *start,
	struct timespec *end);

__u32 fuse_iolog_timeus_diff(struct timespec *start,
	struct timespec *end);

void fuse_iolog_exit(void);
void fuse_iolog_init(void);

struct fuse_rw_info {
	__u32  count;
	__u32  bytes;
	__u32  us;
};

struct fuse_proc_info {
	pid_t pid;
	__u32 valid;
	int misc_type;
	struct fuse_rw_info read;
	struct fuse_rw_info write;
	struct fuse_rw_info misc;
};

struct fuse_iolog_t {
	struct timespec _tstart, _tend;
	__u32 size;
	unsigned int opcode;
};

#define FUSE_IOLOG_MAX     12
#define FUSE_IOLOG_BUFLEN  512
#define FUSE_IOLOG_LATENCY 1

#define FUSE_IOLOG_INIT(iobytes, type)   struct fuse_iolog_t _iolog = { .size = iobytes, .opcode = type }
#define FUSE_IOLOG_START()  get_monotonic_boottime(&_iolog._tstart)
#define FUSE_IOLOG_END()    get_monotonic_boottime(&_iolog._tend)
#define FUSE_IOLOG_US()     fuse_iolog_timeus_diff(&_iolog._tstart, &_iolog._tend)
#define FUSE_IOLOG_PRINT()  fuse_iolog_add(_iolog.size, _iolog.opcode, &_iolog._tstart, &_iolog._tend)

#else

#define FUSE_IOLOG_INIT(...)
#define FUSE_IOLOG_START(...)
#define FUSE_IOLOG_END(...)
#define FUSE_IOLOG_PRINT(...)
#define fuse_iolog_init(...)
#define fuse_iolog_exit(...)

#endif

#endif
