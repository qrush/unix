/* v1trap.h - Deal with 1st Edition trap instructions.
 *
 * $Revision: 1.1 $
 * $Date: 1999/12/26 08:16:33 $
 */

/* In this file, we list the trap number for each system call,
 * and the structures associated with several of the systems
 * calls in 1st Edition UNIX
 */

#define V1_RELE    0
#define V1_EXIT    1
#define V1_FORK    2
#define V1_READ    3
#define V1_WRITE   4
#define V1_OPEN    5
#define V1_CLOSE   6
#define V1_WAIT    7
#define V1_CREAT   8
#define V1_LINK    9
#define V1_UNLINK  10
#define V1_EXEC    11
#define V1_CHDIR   12
#define V1_TIME    13
#define V1_MKDIR   14
#define V1_CHMOD   15
#define V1_CHOWN   16
#define V1_BREAK   17
#define V1_STAT    18
#define V1_SEEK    19
#define V1_TELL    20
#define V1_MOUNT   21
#define V1_UMOUNT  22
#define V1_SETUID  23
#define V1_GETUID  24
#define V1_STIME   25
#define V1_QUIT    26
#define V1_INTR    27
#define V1_FSTAT   28
#define V1_CEMT    29
#define V1_SMDATE  30
#define V1_STTY    31
#define V1_GTTY    32
#define V1_ILGINS  33


char *v1trap_name[]= {
	"rele",
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
	"mkdir",
	"chmod",
	"chown",
	"break",
	"stat",
	"seek",
	"tell",
	"mount",
	"umount",
	"setuid",
	"getuid",
	"stime",
	"quit",
	"intr",
	"fstat",
	"cemt",
	"smdate",
	"stty",
	"gtty",
	"ilgins"
};


struct tr_v1stat {
	u_int16_t	inum;
	u_int16_t	iflags;		/* Mode */
	u_int8_t	inl;		/* Links */
	u_int8_t	iuid;
	u_int16_t 	isize;
	int16_t		iaddr[8];	/* Not used, I hope! */
	u_int32_t 	ctime;
	u_int32_t 	mtime;
	u_int16_t 	unused;
};

/* Values for v1stat iflags */
#define V1_ST_USED	0100000
#define V1_ST_ISDIR	0040000
#define V1_ST_MODIFIED	0020000
#define V1_ST_LARGE	0010000
#define V1_ST_SETUID	0000040
#define V1_ST_EXEC	0000020
#define V1_ST_OWNREAD	0000010
#define V1_ST_OWNWRITE	0000004
#define V1_ST_WRLDREAD	0000002
#define V1_ST_WRLDWRITE	0000001

/* A union which will point at the trap args, so that
 * we can get at the various args of different types
 */
typedef union {
    int16_t   sarg[4];		/* Signed 16-bit args */
    u_int16_t uarg[4];		/* Unsigned 16-bit args */
} arglist;

#define sarg1	V1A.sarg[0]
#define sarg2	V1A.sarg[1]
#define sarg3	V1A.sarg[2]
#define sarg4	V1A.sarg[3]
#define uarg1	V1A.uarg[0]
#define uarg2	V1A.uarg[1]
#define uarg3	V1A.uarg[2]
#define uarg4	V1A.uarg[3]
