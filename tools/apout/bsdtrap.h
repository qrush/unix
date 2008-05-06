/* bsdtrap.h. Definitions for values and structures used in bsdtrap.c
 *
 * $Revision: 1.30 $
 * $Date: 1999/03/01 19:14:16 $
 */

/* In this file, we list the trap number for each system call,
 * and the structures associated with several of the systems
 * calls in 2.11BSD UNIX
 */

#define S_INDIR		0
#define S_EXIT		1
#define S_FORK		2
#define S_READ		3
#define S_WRITE		4
#define S_OPEN		5
#define S_CLOSE		6
#define S_WAIT4		7
#define S_CREAT		8
#define S_LINK		9
#define S_UNLINK	10
#define S_EXECV		11
#define S_CHDIR		12
#define S_FCHDIR	13
#define S_MKNOD		14
#define S_CHMOD		15
#define S_CHOWN		16
#define S_CHFLAGS	17
#define S_FCHFLAGS	18
#define S_LSEEK		19
#define S_GETPID	20
#define S_MOUNT		21
#define S_UMOUNT	22
#define S_SYSCTL	23
#define S_GETUID	24
#define S_GETEUID	25
#define S_PTRACE	26
#define S_GETPPID	27
#define S_STATFS	28
#define S_FSTATFS	29
#define S_GETFSSTAT	30
#define S_SIGACTION	31
#define S_SIGPROCMASK	32
#define S_ACCESS	33
#define S_SIGPENDING	34
#define S_SIGALTSTACK	35
#define S_SYNC		36
#define S_KILL		37
#define S_STAT		38
#define S_GETLOGIN	39
#define S_LSTAT		40
#define S_DUP		41
#define S_PIPE		42
#define S_SETLOGIN	43
#define S_PROFIL	44
#define S_SETUID	45
#define S_SETEUID	46
#define S_GETGID	47
#define S_GETEGID	48
#define S_SETGID	49
#define S_SETEGID	50
#define S_ACCT		51
#define S_OLDPHYS	52
#define S_OLDLOCK	53
#define S_IOCTL		54
#define S_REBOOT	55
#define S_SYMLINK	57
#define S_READLINK	58
#define S_EXECVE	59
#define S_UMASK		60
#define S_CHROOT	61
#define S_FSTAT		62
#define S_GETPAGESIZE	64
#define S_VFORK		66
#define S_SBRK		69
#define S_VHANGUP	76
#define S_GETGROUPS	79
#define S_SETGROUPS	80
#define S_GETPGRP	81
#define S_SETPGRP	82
#define S_SETITIMER	83
#define S_WAIT		84
#define S_GETITIMER	86
#define S_GETHOSTNAME	87
#define S_SETHOSTNAME	88
#define S_GETDTABLESIZE 89
#define S_DUP2		90
#define S_UNUSED1	91
#define S_FCNTL		92
#define S_SELECT	93
#define S_UNUSED2	94
#define S_FSYNC		95
#define S_SETPRIORITY	96
#define S_SOCKET	97
#define S_CONNECT	98
#define S_ACCEPT	99
#define S_GETPRIORITY	100
#define S_SEND		101
#define S_RECV		102
#define S_SIGRETURN	103
#define S_BIND		104
#define S_SETSOCKOPT	105
#define S_LISTEN	106
#define S_SIGSUSPEND	107
#define S_SIGVEC	108
#define S_SIGBLOCK	109
#define S_SIGSETMASK	110
#define S_SIGPAUSE	111
#define S_SIGSTACK	112
#define S_RECVMSG	113
#define S_SENDMSG	114
#define S_GETTIMEOFDAY	116
#define S_GETRUSAGE	117
#define S_GETSOCKOPT	118
#define S_READV		120
#define S_WRITEV	121
#define S_SETTIMEOFDAY	122
#define S_FCHOWN	123
#define S_FCHMOD	124
#define S_RECVFROM	125
#define S_SETREUID	126
#define S_SETREGID	127
#define S_RENAME	128
#define S_TRUNCATE	129
#define S_FTRUNCATE	130
#define S_FLOCK		131
#define S_SENDTO	133
#define S_SHUTDOWN	134
#define S_SOCKETPAIR	135
#define S_MKDIR		136
#define S_RMDIR		137
#define S_UTIMES	138
#define S_ADJTIME	140
#define S_GETPEERNAME	141
#define S_GETHOSTID	142
#define S_SETHOSTID	143
#define S_GETRLIMIT	144
#define S_SETRLIMIT	145
#define S_KILLPG	146
#define S_NOSYS147	147
#define S_SETQUOTA	148
#define S_QUOTA		149
#define S_GETSOCKNAME	150


/*
 * System call names.
 */
#ifdef BSDTRAP_NAME
char *bsdtrap_name[] = {
	"indir",		/*   0 = indir */
	"exit",			/*   1 = exit */
	"fork",			/*   2 = fork */
	"read",			/*   3 = read */
	"write",		/*   4 = write */
	"open",			/*   5 = open */
	"close",		/*   6 = close */
	"wait4",		/*   7 = wait4 */
	"creat",		/*   8 = creat */
	"link",			/*   9 = link */
	"unlink",		/*  10 = unlink */
	"execv",		/*  11 = execv */
	"chdir",		/*  12 = chdir */
	"fchdir",		/*  13 = fchdir */
	"mknod",		/*  14 = mknod */
	"chmod",		/*  15 = chmod */
	"chown",		/*  16 = chown; now 3 args */
	"chflags",		/*  17 = chflags */
	"fchflags",		/*  18 = fchflags */
	"lseek",		/*  19 = lseek */
	"getpid",		/*  20 = getpid */
	"mount",		/*  21 = mount */
	"umount",		/*  22 = umount */
	"__sysctl",		/*  23 = __sysctl */
	"getuid",		/*  24 = getuid */
	"geteuid",		/*  25 = geteuid */
	"ptrace",		/*  26 = ptrace */
	"getppid",		/*  27 = getppid */
	"statfs",		/*  28 = statfs */
	"fstatfs",		/*  29 = fstatfs */
	"getfsstat",		/*  30 = getfsstat */
	"sigaction",		/*  31 = sigaction */
	"sigprocmask",		/*  32 = sigprocmask */
	"access",		/*  33 = access */
	"sigpending",		/*  34 = sigpending */
	"sigaltstack",		/*  35 = sigaltstack */
	"sync",			/*  36 = sync */
	"kill",			/*  37 = kill */
	"stat",			/*  38 = stat */
	"getlogin",		/*  39 = getlogin */
	"lstat",		/*  40 = lstat */
	"dup",			/*  41 = dup */
	"pipe",			/*  42 = pipe */
	"setlogin",		/*  43 = setlogin */
	"profil",		/*  44 = profil */
	"setuid",		/*  45 = setuid */
	"seteuid",		/*  46 = seteuid */
	"getgid",		/*  47 = getgid */
	"getegid",		/*  48 = getegid */
	"setgid",		/*  49 = setgid */
	"setegid",		/*  50 = setegid */
	"acct",			/*  51 = turn acct off/on */
	"old phys",		/*  52 = old set phys addr */
	"old lock",		/*  53 = old lock in core */
	"ioctl",		/*  54 = ioctl */
	"reboot",		/*  55 = reboot */
	"old mpx - nosys",	/*  56 = old mpxchan */
	"symlink",		/*  57 = symlink */
	"readlink",		/*  58 = readlink */
	"execve",		/*  59 = execve */
	"umask",		/*  60 = umask */
	"chroot",		/*  61 = chroot */
	"fstat",		/*  62 = fstat */
	"#63",			/*  63 = used internally */
	"getpagesize",		/*  64 = getpagesize */
	"4.3 mremap - nosys",	/*  65 = mremap */
	"vfork",		/*  66 = vfork */
	"old vread - nosys",	/*  67 = old vread */
	"old vwrite - nosys",	/*  68 = old vwrite */
	"sbrk",			/*  69 = sbrk */
	"4.3 sstk - nosys",	/*  70 = sstk */
	"4.3 mmap - nosys",	/*  71 = mmap */
	"old vadvise - nosys",	/*  72 = old vadvise */
	"4.3 munmap - nosys",	/*  73 = munmap */
	"4.3 mprotect - nosys", /*  74 = mprotect */
	"4.3 madvise - nosys",	/*  75 = madvise */
	"vhangup",		/*  76 = vhangup */
	"old vlimit - nosys",	/*  77 = old vlimit */
	"4.3 mincore - nosys",	/*  78 = mincore */
	"getgroups",		/*  79 = getgroups */
	"setgroups",		/*  80 = setgroups */
	"getpgrp",		/*  81 = getpgrp */
	"setpgrp",		/*  82 = setpgrp */
	"setitimer",		/*  83 = setitimer */
	"wait",			/*  84 = wait */
	"4.3 swapon - nosys",	/*  85 = swapon */
	"getitimer",		/*  86 = getitimer */
	"gethostname",		/*  87 = gethostname */
	"sethostname",		/*  88 = sethostname */
	"getdtablesize",	/*  89 = getdtablesize */
	"dup2",			/*  90 = dup2 */
	"nosys",		/*  91 = unused */
	"fcntl",		/*  92 = fcntl */
	"select",		/*  93 = select */
	"nosys",		/*  94 = unused */
	"fsync",		/*  95 = fsync */
	"setpriority",		/*  96 = setpriority */
	"socket",		/*  97 = socket */
	"connect",		/*  98 = connect */
	"accept",		/*  99 = accept */
	"getpriority",		/* 100 = getpriority */
	"send",			/* 101 = send */
	"recv",			/* 102 = recv */
	"sigreturn",		/* 103 = sigreturn */
	"bind",			/* 104 = bind */
	"setsockopt",		/* 105 = setsockopt */
	"listen",		/* 106 = listen */
	"sigsuspend",		/* 107 = sigsuspend */
	"sigvec",		/* 108 = sigvec */
	"sigblock",		/* 109 = sigblock */
	"sigsetmask",		/* 110 = sigsetmask */
	"sigpause",		/* 111 = sigpause */
	"sigstack",		/* 112 = sigstack */
	"recvmsg",		/* 113 = recvmsg */
	"sendmsg",		/* 114 = sendmsg */
	"old vtrace - nosys",	/* 115 = old vtrace */
	"gettimeofday",		/* 116 = gettimeofday */
	"getrusage",		/* 117 = getrusage */
	"getsockopt",		/* 118 = getsockopt */
	"4.3 resuba - nosys",	/* 119 = resuba */
	"readv",		/* 120 = readv */
	"writev",		/* 121 = writev */
	"settimeofday",		/* 122 = settimeofday */
	"fchown",		/* 123 = fchown */
	"fchmod",		/* 124 = fchmod */
	"recvfrom",		/* 125 = recvfrom */
	"setreuid",		/* 126 = setreuid */
	"setregid",		/* 127 = setregid */
	"rename",		/* 128 = rename */
	"truncate",		/* 129 = truncate */
	"ftruncate",		/* 130 = ftruncate */
	"flock",		/* 131 = flock */
	"old portal - nosys",	/* 132 = old portal */
	"sendto",		/* 133 = sendto */
	"shutdown",		/* 134 = shutdown */
	"socketpair",		/* 135 = socketpair */
	"mkdir",		/* 136 = mkdir */
	"rmdir",		/* 137 = rmdir */
	"utimes",		/* 138 = utimes */
	"4.2 sigreturn - nosys",	/* 139 = old 4.2 sigreturn */
	"adjtime",		/* 140 = adjtime */
	"getpeername",		/* 141 = getpeername */
	"gethostid",		/* 142 = gethostid */
	"sethostid",		/* 143 = sethostid */
	"getrlimit",		/* 144 = getrlimit */
	"setrlimit",		/* 145 = setrlimit */
	"killpg",		/* 146 = killpg */
	"#147",			/* 147 = nosys */
	"setquota",		/* 148 = setquota */
	"quota",		/* 149 = quota */
	"getsockname",		/* 150 = getsockname */
};
#endif

/* fcntl defines used by open */
#define BSD_RDONLY        0x0000          /* open for reading only */
#define BSD_WRONLY        0x0001          /* open for writing only */
#define BSD_RDWR          0x0002          /* open for reading and writing */
#define BSD_NONBLOCK      0x0004          /* no delay */
#define BSD_APPEND        0x0008          /* set append mode */
#define BSD_SHLOCK        0x0010          /* open with shared file lock */
#define BSD_EXLOCK        0x0020          /* open with exclusive file lock */
#define BSD_ASYNC         0x0040          /* signal pgrp when data ready */
#define BSD_FSYNC         0x0080          /* synchronous writes */
#define BSD_CREAT         0x0200          /* create if nonexistant */
#define BSD_TRUNC         0x0400          /* truncate to zero length */
#define BSD_EXCL          0x0800          /* error if already exists */


/* stat struct, used by S_STAT, S_FSTAT, S_LSTAT */
struct tr_stat
{
	int16_t	   st_dev;
	u_int16_t   st_ino;
	u_int16_t   st_mode;
	int16_t	   st_nlink;
	u_int16_t   st_uid;
	u_int16_t   st_gid;
	int16_t	   st_rdev;
	int8_t	   st_size[4];		/* Alignment problems */
	int8_t	   st_atim[4];		/* Alignment problems */
	int16_t	   st_spare1;
	int8_t	   st_mtim[4];		/* Alignment problems */
	int16_t	   st_spare2;
	int8_t	   st_ctim[4];		/* Alignment problems */
	int16_t	   st_spare3;
	int8_t	   st_blksize[4];	/* Alignment problems */
	int8_t	   st_blocks[4];	/* Alignment problems */
	u_int16_t   st_flags;
	u_int16_t   st_spare4[3];
};

/* Directory entry */
#define TR_DIRBLKSIZ	512
#define TR_MAXNAMLEN	63
struct	tr_direct {
    u_int16_t d_ino;			/* inode number of entry */
    u_int16_t d_reclen;			/* length of this record */
    u_int16_t d_namlen;			/* length of string in d_name */
    char d_name[TR_MAXNAMLEN+1];	/* name must be no longer than this */
};

/* used by S_ADJTIME */
struct tr_timeval {
    u_int32_t    tv_sec;	    /* seconds */
    u_int32_t    tv_usec;	    /* and microseconds */
};
/* Used by S_GETTIMEOFDAY */
struct tr_timezone {
    int16_t	    tz_minuteswest; /* minutes west of Greenwich */
    int16_t	    tz_dsttime;	    /* type of dst correction */
};

/* used in itimer calls */
struct	tr_itimerval {
    struct	tr_timeval it_interval;	   /* timer interval */
    struct	tr_timeval it_value;	   /* current value */
};

/* Used by socket calls */
struct tr_sockaddr {
    u_int16_t sa_family;		/* address family */
    char	 sa_data[14];		/* up to 14 bytes of direct address */
};

/* used in rlimit calls */
struct tr_rlimit {
    int32_t rlim_cur;		/* current (soft) limit */
    int32_t rlim_max;		/* maximum value for rlim_cur */
};

struct  tr_rusage {
        struct tr_timeval ru_utime;        /* user time used */
        struct tr_timeval ru_stime;        /* system time used */
        u_int32_t  ru_maxrss;
        u_int32_t  ru_ixrss;               /* integral shared memory size */
        u_int32_t  ru_idrss;               /* integral unshared data size */
        u_int32_t  ru_isrss;               /* integral unshared stack size */
        u_int32_t  ru_minflt;              /* page reclaims */
        u_int32_t  ru_majflt;              /* page faults */
        u_int32_t  ru_ovly;                /* overlay changes */
        u_int32_t  ru_nswap;               /* swaps */
        u_int32_t  ru_inblock;             /* block input operations */
        u_int32_t  ru_oublock;             /* block output operations */
        u_int32_t  ru_msgsnd;              /* messages sent */
        u_int32_t  ru_msgrcv;              /* messages received */
        u_int32_t  ru_nsignals;            /* signals received */
        u_int32_t  ru_nvcsw;               /* voluntary context switches */
        u_int32_t  ru_nivcsw;              /* involuntary context switches */
};

/* for writev, readv */
struct tr_iovec {
    u_int16_t   iov_base;
    u_int16_t   iov_len;
};


/* A union which will point at the trap args, so that
 * we can get at the various args of different types
 */
typedef union {
    int16_t   sarg[6];	/* Signed 16-bit args */
    u_int16_t uarg[6];	/* Unsigned 16-bit args */
} arglist;

#define sarg1	A->sarg[0]
#define sarg2	A->sarg[1]
#define sarg3	A->sarg[2]
#define sarg4	A->sarg[3]
#define sarg5	A->sarg[4]
#define sarg6	A->sarg[5]
#define uarg1	A->uarg[0]
#define uarg2	A->uarg[1]
#define uarg3	A->uarg[2]
#define uarg4	A->uarg[3]
#define uarg5	A->uarg[4]
#define uarg6	A->uarg[5]
