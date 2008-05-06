/* v7trap.h - Deal with V7 trap instructions. Also do V5 and V6 syscalls.
 *
 * $Revision: 2.17 $
 * $Date: 1999/12/26 08:16:33 $
 */

/* In this file, we list the trap number for each system call,
 * and the structures associated with several of the systems
 * calls in 7th Edition UNIX
 */

#define S_INDIR		0
#define S_EXIT		1
#define S_FORK		2
#define S_READ		3
#define S_WRITE		4
#define S_OPEN		5
#define S_CLOSE		6
#define S_WAIT		7
#define S_CREAT		8
#define S_LINK		9
#define S_UNLINK	10
#define S_EXEC		11
#define S_CHDIR		12
#define S_TIME		13
#define S_MKNOD		14
#define S_CHMOD		15
#define S_CHOWN		16
#define S_BREAK		17
#define S_STAT		18
#define S_LSEEK		19
#define S_GETPID	20
#define S_MOUNT		21
#define S_UMOUNT	22
#define S_SETUID	23
#define S_GETUID	24
#define S_STIME		25
#define S_PTRACE	26
#define S_ALARM		27
#define S_FSTAT		28
#define S_PAUSE		29
#define S_UTIME		30
#define S_STTY		31
#define S_GTTY		32
#define S_ACCESS	33
#define S_NICE		34
#define S_FTIME		35
#define S_SYNC		36
#define S_KILL		37
#define S_DUP		41
#define S_PIPE		42
#define S_TIMES		43
#define S_PROF		44
#define S_SETGID	46
#define S_GETGID	47
#define S_SIGNAL	48
#define S_ACCT		51
#define S_PHYS		52
#define S_LOCK		53
#define S_IOCTL		54
#define S_EXECE		59
#define S_UMASK		60
#define S_CHROOT	61


char *v7trap_name[]= {
	"indir",
	"exit",
	"fork",
	"read",
	"write",
	"open",
	"close",
	"wait",
	"creat",
	"link",
	"unlink",
	"exec",
	"chdir",
	"time",
	"mknod",
	"chmod",
	"chown",
	"break",
	"stat",
	"lseek",
	"getpid",
	"mount",
	"umount",
	"setuid",
	"getuid",
	"stime",
	"ptrace",
	"alarm",
	"fstat",
	"pause",
	"utime",
	"stty",
	"gtty",
	"access",
	"nice",
	"ftime",
	"sync",
	"kill",
	"unknown",
	"unknown",
	"unknown",
	"dup",
	"pipe",
	"times",
	"prof",
	"unknown",
	"setgid",
	"getgid",
	"signal",
	"unknown",
	"unknown",
	"acct",
	"phys",
	"lock",
	"ioctl",
	"unknown",
	"unknown",
	"unknown",
	"unknown",
	"exece",
	"umask",
	"chroot"
};


struct tr_v7stat {
    int16_t st_dev;
    u_int16_t st_ino;
    u_int16_t st_mode;
    int16_t st_nlink;
    int16_t st_uid;
    int16_t st_gid;
    int16_t st_rdev;
    int8_t st_size[4];		/* Alignment problems */
    int8_t st_atim[4];		/* Alignment problems */
    int8_t st_mtim[4];		/* Alignment problems */
    int8_t st_ctim[4];		/* Alignment problems */
};

struct tr_v6stat {
	int16_t	idev;		/* Device */
	int16_t	inum;
	int16_t	iflags;		/* Mode */
	int8_t	inl;		/* Links */
	int8_t	iuid;
	int8_t	igid;
	u_int8_t isize0;	/* Most significant 8 bits */
	u_int16_t isize;
	int16_t	iaddr[8];	/* Not used, I hope! */
	u_int32_t atime;	/* Alignment problems */
	u_int32_t mtime;	/* Alignment problems */
};

struct tr_timeb {
	u_int32_t time;
	u_int16_t millitm;
	int16_t	 timezone;
	int16_t	 dstflag;
};

struct tr_sgttyb {
	int8_t	  sg_ispeed;		  /* input speed */
	int8_t	  sg_ospeed;		  /* output speed */
	int8_t	  sg_erase;		  /* erase character */
	int8_t	  sg_kill;		  /* kill character */
	int16_t	  sg_flags;		  /* mode flags */
};

/*
 * Values for sg_flags
 */
#define TR_TANDEM  01
#define TR_CBREAK  02
#define TR_LCASE   04
#define TR_ECHO	   010
#define TR_CRMOD   020
#define TR_RAW	   040
#define TR_ODDP	   0100
#define TR_EVENP   0200
#define TR_ANYP	   0300
#define TR_XTABS   06000

/*
 * Values for signal
 */
#define V7_SIG_DFL 0
#define V7_SIG_IGN 1

#define V7_NSIG 15

#define V7_SIGHUP  1	/* hangup */
#define V7_SIGINT  2	/* interrupt */
#define V7_SIGQUIT 3	/* quit */
#define V7_SIGILL  4	/* illegal instruction (not reset when caught) */
#define V7_SIGTRAP 5	/* trace trap (not reset when caught) */
#define V7_SIGIOT  6	/* IOT instruction */
#define V7_SIGEMT  7	/* EMT instruction */
#define V7_SIGFPE  8	/* floating point exception */
#define V7_SIGKILL 9	/* kill (cannot be caught or ignored) */
#define V7_SIGBUS  10	/* bus error */
#define V7_SIGSEGV 11	/* segmentation violation */
#define V7_SIGSYS  12	/* bad argument to system call */
#define V7_SIGPIPE 13	/* write on a pipe with no one to read it */
#define V7_SIGALRM 14	/* alarm clock */
#define V7_SIGTERM 15	/* software termination signal from kill */


/* A union which will point at the trap args, so that
 * we can get at the various args of different types
 */
typedef union {
    int16_t   sarg[4];		/* Signed 16-bit args */
    u_int16_t uarg[4];		/* Unsigned 16-bit args */
} arglist;

#define sarg1	V7A.sarg[0]
#define sarg2	V7A.sarg[1]
#define sarg3	V7A.sarg[2]
#define sarg4	V7A.sarg[3]
#define uarg1	V7A.uarg[0]
#define uarg2	V7A.uarg[1]
#define uarg3	V7A.uarg[2]
#define uarg4	V7A.uarg[3]
