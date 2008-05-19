/* v7trap.c - Deal with V7 trap instructions. V5 and V6 syscalls are also
 * done here, because the syscall interface is nearly the same as V7.
 *
 * $Revision: 1.50 $
 * $Date: 2008/05/19 13:45:58 $
 */
#include "defines.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <utime.h>
#include "v7trap.h"

#ifdef __linux__
# undef STREAM_BUFFERING		/* It seems to work */
#else
# define STREAM_BUFFERING		/* but not for Linux */
#endif

/* Forward prototypes */
#ifdef __STDC__
#define P(s) s
#else
#define P(s) ()
#endif
static int trap_exec P((int want_env));
static int open_dir P((char *name));
static int trap_gtty P((u_int16_t fd, u_int16_t ucnt));
static int trap_stty P((u_int16_t fd, u_int16_t ucnt));
static int v7signal P((int sig, int val));
static void fixv6time P((time_t *t));
#undef P


/* V7 keeps some of the arguments to syscalls in registers, and some
 * after the `sys' instruction itself. The list below gives the number
 * of words, and the number in registers.
 */
struct v7sysent {
	int nwords;
	int nregs;
};
static struct v7sysent v7arg[] = {
	{0, 0}, {1, 1}, {0, 0}, {3, 1}, {3, 1}, {2, 0}, {1, 1}, {0, 0},
	{2, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0}, {0, 0}, {3, 0}, {2, 0},
	{3, 0}, {1, 0}, {2, 0}, {4, 1}, {0, 0}, {3, 0}, {1, 0}, {1, 1},
	{0, 0}, {2, 2}, {4, 1}, {1, 1}, {2, 1}, {0, 0}, {2, 0}, {2, 1},
	{2, 1}, {2, 0}, {1, 1}, {1, 0}, {0, 0}, {2, 1}, {0, 0}, {0, 0},
	{1, 1}, {2, 2}, {0, 0}, {1, 0}, {4, 0}, {0, 0}, {1, 1}, {0, 0},
	{2, 0}, {0, 0}, {0, 0}, {1, 0}, {3, 0}, {1, 0}, {3, 0}, {0, 0},
	{4, 0}, {0, 0}, {0, 0}, {3, 0}, {1, 0}, {1, 0}, {0, 0}, {0, 0}
};

static arglist V7A;


void
v7trap()
{
    int i, pid, pfd[2];
    int whence;
    u_int16_t argbase;
    int trapnum;
    long larg;
    char *buf, *buf2;
    char *fmode;		/* used with fdopen only */
    time_t tim;

    struct stat stbuf;		/* used in STAT */
    struct tr_v7stat *t;	/* used in STAT */
    struct tr_v6stat *t6;	/* used in STAT */
    struct tr_timeb *tb;	/* used in FTIME */
    struct timezone tz;		/* used in FTIME */
    struct timeval tv;		/* used in FTIME */
    struct timeval utv[2];	/* used in UTIME */



				/* Work out the actual trap number, and */
				/* shift the PC up past any arguments */
				/* to the syscall. Calculate base of args */
    trapnum= ir & 077;
    if (trapnum==S_INDIR) {
	lli_word(regs[PC], argbase);
	ll_word(argbase, ir);
        trapnum= ir & 077; argbase+=2; regs[PC]+=2;
    } else {
	argbase=regs[PC];
	regs[PC]+= 2* (v7arg[trapnum].nwords - v7arg[trapnum].nregs);

				/* However, V6 seek() has 1 less arg */
	if ((Binary==IS_A68 || Binary==IS_V6) || (Binary==IS_V5)) {
	    if (trapnum==S_LSEEK) regs[PC]-=2;
	}
    }

			/* Move arguments into V7A so we can use them */
    for (i=0; i<v7arg[trapnum].nregs; i++) V7A.uarg[i]= regs[i];
    for (;i<v7arg[trapnum].nwords; i++,argbase+=2)
					ll_word(argbase, V7A.uarg[i]);
 
    TrapDebug((dbg_file, "pid %d %s: ", (int)getpid(),v7trap_name[trapnum]));

    switch (trapnum) {
			/* These syscalls are not implemented, and */
			/* always return EPERM to the caller */
    case S_PHYS:
    case S_PROF:
    case S_PTRACE:
    case S_ACCT:
    case S_MOUNT:
    case S_UMOUNT:
    case S_TIMES:
	i=-1; errno=EPERM; break;

			/* These syscalls are ignored, and */
			/* always return C=0 to the caller */
    case S_LOCK:
    case S_STIME:
    case S_BREAK:
	i=sarg1; break;
    case S_SYNC:
	sync(); i=0; break;

    case S_SIGNAL:
	i= v7signal(uarg1, uarg2);
	break;
    case S_EXIT:
	exit(regs[0]);
	i=-1; errno=EPERM; break;
    case S_NICE:
	i= nice(regs[0]); break;
    case S_PAUSE:
	i = pause(); break;
    case S_DUP:
	if (sarg1 > 0100) {
	    sarg1 -= 0100;
	    i = dup2(sarg1, sarg2);	/* Check that sarg2, not r1, holds */
#ifdef STREAM_BUFFERING
	    if ((i!=-1) && ValidFD(sarg2) && ValidFD(sarg1) && stream[sarg1]) {
		fmode= streammode[sarg1];
		stream[sarg2] = fdopen(sarg2, fmode);
		streammode[sarg2]=fmode;
	    }
#endif
	} else
	    i = dup(sarg1);
#ifdef STREAM_BUFFERING
	    if ((i!=-1) && ValidFD(i)&& ValidFD(sarg1) && stream[sarg1]) {
		fmode= streammode[sarg1];
		stream[i] = fdopen(i, fmode);
		streammode[i]=fmode;
	    }
#endif
	break;
    case S_TIME:
	tim= larg;
	i = time(&tim);
	
	if ((Binary==IS_A68 || Binary==IS_V6) || (Binary==IS_V5)) {
	  fixv6time(&tim);	/* Fix annoying bug in V5/V6 ctime() */
	}
	regs[1] = tim & 0xffff;
	i = tim >> 16;
	break;
    case S_ALARM:
	i = alarm(uarg1); break;
    case S_UMASK:
	i = umask(uarg1); break;
    case S_LSEEK:
				/* Work out the args before we do the lseek */
	if ((Binary==IS_A68 || Binary==IS_V6) || (Binary==IS_V5)) {
	    whence=uarg3;
	    switch (uarg3) {
		case 0: larg= uarg2; break;
		case 1: 
		case 2: larg= sarg2; break;
		case 3: whence=0; larg= 512 * uarg2; break;
		case 4: whence=1; larg= 512 * sarg2; break;
		case 5: whence=2; larg= 512 * sarg2; break;
	    }
	} else {
	    larg = (uarg2 << 16) | uarg3;
	    whence= uarg4;
	}
#ifdef STREAM_BUFFERING
	if (ValidFD(sarg1) && stream[sarg1]) {
	    i = fseek(stream[sarg1], larg, whence);
	    if (i == 0) i = ftell(stream[sarg1]);
	} else
#endif
	    i = lseek(sarg1, larg, whence);

        TrapDebug((dbg_file, " on fd %d amt %ld whence %d return %d ",
					sarg1, larg, whence, i));
	if ((Binary==IS_A68 || Binary==IS_V6) || (Binary==IS_V5)) {
	   if (i!=-1) i=0;
	   break;
	}
	regs[1] = i & 0xffff;
	i = i >> 16;
	break;
    case S_READ:
	buf = (char *)&dspace[uarg2];
#ifdef STREAM_BUFFERING
	if (ValidFD(sarg1) && stream[sarg1])
	    i = fread(buf, 1, uarg3, stream[sarg1]);
	else
#endif
	    i = read(sarg1, buf, uarg3);
        TrapDebug((dbg_file, " on fd %d return %d ",sarg1,i)); 
	break;
    case S_LINK:
	buf = xlate_filename((char *)&dspace[uarg1]);
	buf2 = xlate_filename((char *)&dspace[uarg2]);
	if (!strcmp(buf, buf2)) i=0;	/* Happens on mkdir(1) */
	else i = link(buf, buf2);
	break;
    case S_ACCESS:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = access(buf, sarg2);
	break;
    case S_WRITE:
	buf = (char *)&dspace[uarg2];
#ifdef STREAM_BUFFERING
	if (ValidFD(sarg1) && stream[sarg1])
	    i = fwrite(buf, 1, uarg3, stream[sarg1]);
	else
#endif
	    i = write(sarg1, buf, uarg3);
        TrapDebug((dbg_file, " on fd %d return %d ",sarg1,i));
	break;
    case S_CLOSE:
#ifdef STREAM_BUFFERING
	if (ValidFD(sarg1) && stream[sarg1]) {
	    i = fclose(stream[sarg1]);
	    stream[sarg1] = NULL;
	} else
#endif
	    i = close(sarg1);
        TrapDebug((dbg_file, " on fd %d return %d ",sarg1,i));
	break;
    case S_GTTY:
	i = trap_gtty(uarg1, uarg2); break;
    case S_STTY:
	i = trap_stty(uarg1, uarg2); break;
    case S_IOCTL:
	switch (uarg2) {
	case (('t' << 8) + 8):	/* GTTY */
	    i = trap_gtty(uarg1, uarg3); break;
	case (('t' << 8) + 9):	/* STTY */
	    i = trap_stty(uarg1, uarg3); break;
	default:
	    i=0;
	}
	break;
    case S_FTIME:
	buf = (char *)&dspace[uarg1];
	tb = (struct tr_timeb *) buf;
	i = gettimeofday(&tv, &tz);
	if (i == -1) break;
	copylong(tb->time, tv.tv_sec);
#if 0
	buf = (char *) &(tb->time);
	buf2 = (char *) &(tv.tv_sec);
	buf[0] = buf2[2]; buf[1] = buf2[3]; buf[2] = buf2[0]; buf[3] = buf2[1];
#endif
	tb->millitm = tv.tv_usec / 1000;
	tb->timezone = tz.tz_minuteswest;
	tb->dstflag = tz.tz_dsttime;
	break;
    case S_STAT:
	buf = xlate_filename((char *)&dspace[uarg1]);
	if (buf[0]=='\0') buf=".";		/* Not documented anywhere */
	if (uarg1==0) buf=".";
	buf2 = (char *)&dspace[uarg2];
	i = stat(buf, &stbuf);
        TrapDebug((dbg_file, " on %s return %d ",buf,i));
	goto dostat;
    case S_FSTAT:
	buf2 = (char *)&dspace[uarg2];
	i = fstat(sarg1, &stbuf);
        TrapDebug((dbg_file, " on fd %d return %d ",sarg1,i));

dostat:
	if (i == -1) break;
					/* V6 and V7 have different stats */
	if ((Binary==IS_A68 || Binary==IS_V6) || (Binary==IS_V5)) {
	    t6 = (struct tr_v6stat *) buf2;
	    t6->idev = stbuf.st_dev;
	    t6->inum = stbuf.st_ino;
	    t6->iflags = stbuf.st_mode;
	    t6->inl = stbuf.st_nlink;
	    t6->iuid = stbuf.st_uid;
	    t6->igid = stbuf.st_gid;
	    t6->isize = (u_int16_t) (stbuf.st_size & 0xffff);
	    t6->isize0 = (u_int8_t) ((stbuf.st_size>>16) & 0xff);
	  				/* Fix annoying bug in V5/V6 ctime() */
	    fixv6time(&(stbuf.st_atime));
	    fixv6time(&(stbuf.st_mtime));
	    copylong(t6->atime, stbuf.st_atime);
	    copylong(t6->mtime, stbuf.st_mtime);
#if 0
	    buf = (char *) &(t6->atime);
	    buf2 = (char *) &(stbuf.st_atime);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
	    buf = (char *) &(t6->mtime);
	    buf2 = (char *) &(stbuf.st_mtime);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
#endif
	} else {
	    t = (struct tr_v7stat *) buf2;
	    t->st_dev = stbuf.st_dev;
	    t->st_ino = stbuf.st_ino;
	    t->st_mode = stbuf.st_mode;
	    t->st_nlink = stbuf.st_nlink;
	    t->st_uid = stbuf.st_uid;
	    t->st_gid = stbuf.st_gid;
	    t->st_rdev = stbuf.st_rdev;
	    copylong(t->st_size, stbuf.st_size);
	    copylong(t->st_atim, stbuf.st_atime);
	    copylong(t->st_mtim, stbuf.st_mtime);
	    copylong(t->st_ctim, stbuf.st_ctime);
#if 0
	    buf = (char *) &(t->st_size);
	    buf2 = (char *) &(stbuf.st_size);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
	    buf = (char *) &(t->st_atim);
	    buf2 = (char *) &(stbuf.st_atime);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
	    buf = (char *) &(t->st_mtim);
	    buf2 = (char *) &(stbuf.st_mtime);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
	    buf = (char *) &(t->st_ctim);
	    buf2 = (char *) &(stbuf.st_ctime);
	    buf[0]= buf2[2]; buf[1]= buf2[3]; buf[2]= buf2[0]; buf[3]= buf2[1];
#endif
	}
	break;
    case S_UTIME:
	utv[0].tv_usec = utv[1].tv_usec = 0;
	copylong(dspace[uarg2], utv[0].tv_sec);
	copylong(dspace[uarg2+4], utv[1].tv_sec);
	buf = xlate_filename((char *)&dspace[uarg1]);
#if 0
	buf2 = &dspace[uarg2];
	buf3 = (char *) &(utv[0].tv_sec);
	buf3[0]= buf2[2]; buf3[1]= buf2[3]; buf3[2]= buf2[0]; buf3[3]= buf2[1];

	buf2 += 4;
	buf3 = (char *) &(utv[1].tv_sec);
	buf3[0]= buf2[2]; buf3[1]= buf2[3]; buf3[2]= buf2[0]; buf3[3]= buf2[1];
#endif

	i = utimes(buf, utv); break;
    case S_UNLINK:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = unlink(buf); break;
    case S_OPEN:
	buf = xlate_filename((char *)&dspace[uarg1]);

	i = stat(buf, &stbuf);	/* If file is a directory */
	if (i == 0 && (stbuf.st_mode & S_IFDIR)) {
	    i = open_dir(buf);
	    fmode = "w+";
            TrapDebug((dbg_file, "(dir) on %s return %d ",buf,i));
	} else {
	    switch (sarg2) {
	      case 0: sarg2 = O_RDONLY; fmode = "r"; break;
	      case 1: sarg2 = O_WRONLY; fmode = "w"; break;
	      default: sarg2 = O_RDWR; fmode = "w+"; break;
	    }
	    i = open(buf, sarg2);
            TrapDebug((dbg_file, " on %s return %d ",buf,i));
	}

#ifdef STREAM_BUFFERING
	if (i==-1) break;
#if 0
	/* Now get its stream pointer if possible */
	/* Can someone explain why fdopen doesn't work for O_RDWR? */
	if (ValidFD(i) && !isatty(i) && (sarg2!=O_RDWR)) {
	    stream[i] = fdopen(i, fmode); streammode[i]=fmode;
	}
#endif
	stream[i] = fdopen(i, fmode); streammode[i]=fmode;
#endif
	break;
    case S_MKNOD:
	buf = xlate_filename((char *)&dspace[uarg1]);

	if ((uarg2 & 077000) ==	040000) {
		/* It's a directory creation */
		i= mkdir(buf, uarg2 & 0777);
	} else
		i = mknod(buf, uarg2, sarg3);
	break;
    case S_CHMOD:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chmod(buf, uarg2); break;
    case S_KILL:
	i = kill(sarg1, sarg2); break;
    case S_CHOWN:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chown(buf, sarg2, sarg3); break;
    case S_PIPE:
	i = pipe(pfd);
	if (i == -1) break;
#ifdef STREAM_BUFFERING
	if (ValidFD(pfd[0])) {
		stream[pfd[0]] = fdopen(pfd[0], "r");
		streammode[pfd[0]]= "r";
	}
	if (ValidFD(pfd[1])) {
		stream[pfd[1]] = fdopen(pfd[1], "w");
		streammode[pfd[1]]= "w";
	}
#endif
	i = pfd[0]; regs[1] = pfd[1]; break;
    case S_CHROOT:
	buf = xlate_filename((char *)&dspace[uarg1]);
	if (buf == NULL) { i=-1; errno=ENOENT; break; }
	set_apout_root(buf);
	i=0; break;
    case S_CHDIR:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chdir(buf); break;
    case S_CREAT:
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = creat(buf, sarg2);
#ifdef STREAM_BUFFERING
	if (i==-1) break;
	if (ValidFD(i)) {
		stream[i] = fdopen(i, "w");
		streammode[i]= "w";
	}
#endif
	break;
    case S_EXECE:
	i= trap_exec(1); break;
	
    case S_EXEC:
	i= trap_exec(0); break;
    case S_WAIT:
	i = wait(&pid);
	if (i == -1) break;
	regs[1] = pid; break;
    case S_FORK:
	pid = getpid();
	i = fork();
	switch (i) {
	    /* Error, inform the parent */
	case -1: break;
	    /* Child gets ppid in r0 */
	case 0: i = pid; break;
	    /* Parent: Skip child `bf', pid into r0 */
	default: regs[PC] += 2; 
	}
	break;
    case S_GETUID:
	i = geteuid(); regs[1] = i;
	i = getuid(); break;
    case S_GETPID:
	i = getpid(); break;
    case S_GETGID:
	i = getegid(); regs[1] = i;
	i = getgid(); break;
    case S_SETUID:
	i = setuid(sarg1); break;
    case S_SETGID:
	i = setgid(sarg1); break;
    default:
	if (trapnum>S_CHROOT) {
	  fprintf(stderr,"Apout - unknown syscall %d at PC 0%o\n",
							trapnum,regs[PC]);
	} else {
	  fprintf(stderr,"Apout - the %s syscall is not yet implemented\n",
						v7trap_name[trapnum]);
	}
	exit(1);
    }

	/* Set r0 to either errno or i, */
	/* and clear/set C bit */

    if (i == -1) {
	SET_CC_C();
	TrapDebug((dbg_file, "errno is %s\n", strerror(errno)));
    } else {
	CLR_CC_C(); regs[0]=i;
#ifdef DEBUG
	if (trap_debug) {
	  fprintf(dbg_file, "return %d\n", i);
	  fflush(dbg_file);
	}
#endif
    }
    return;
}


/* Translate V7 signal value to our value. */
static int v7sig[] = {
      0, SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGIOT, SIGEMT,
      SIGFPE, SIGKILL, SIGBUS, SIGSEGV, SIGSYS, SIGPIPE, SIGALRM, SIGTERM
};

static int
trap_exec(int want_env)
{
    int i;
    u_int16_t cptr, cptr2;
    char *buf, *name, *origpath;

    origpath = strdup((char *)&dspace[uarg1]);
    name = xlate_filename(origpath);
    TrapDebug((dbg_file, "%s Execing %s ", progname, name));

    for (i=0;i<V7_NSIG;i++) signal(v7sig[i], SIG_DFL);

    cptr=uarg2;

    Argc=0; Envc=0;
    while (Argc < MAX_ARGS) {
	ll_word(cptr, cptr2);
	if (cptr2 == 0)
	    break;
	buf = (char *)&dspace[cptr2];
	Argv[Argc++] = strdup(buf);
	cptr += 2;
	TrapDebug((dbg_file, "%s ", buf));
    }
    Argv[Argc] = NULL;
    TrapDebug((dbg_file, "\n"));

    if (want_env) {
	cptr=uarg3;
	while (Envc < MAX_ARGS) {
	    ll_word(cptr, cptr2);
	    if (cptr2 == 0)
		break;
	    buf = (char *)&dspace[cptr2];
	    Envp[Envc++] = strdup(buf);
	    cptr += 2;
	}
    }
    Envp[Envc] = NULL;

    if (load_a_out(name, origpath, want_env) == -1) {
	for (Argc--; Argc >= 0; Argc--) free(Argv[Argc]);
	for (Envc--; Envc >= 0; Envc--) free(Envp[Envc]);
	errno= ENOENT; return(-1);
    }
    run();			/* Ok, so it's recursive, I dislike setjmp */
    return(0);
}

/* 7th Edition reads directories as if they were ordinary files.
 * The solution is to read the directory entries, and build a
 * real file, which is passed back to the open call.
 * Limitation: 32-bit inode numbers are truncated to 16-bit ones.
 */
static int
open_dir(char *name)
{
    DIR *d;
    char *tmpname;
    int i;
    struct dirent *dent;

    struct old_direct {
	int16_t d_ino;
	int8_t d_name[14];
    } odent;

    d = opendir(name);
    if (d == NULL) return (-1);
    tmpname= strdup(TMP_PLATE);
    i= mkstemp(tmpname);
    if (i == -1) {
	fprintf(stderr,"Apout - open_dir couldn't open %s\n", tmpname); exit(1);
    }
    unlink(tmpname); free(tmpname);

    while ((dent = readdir(d)) != NULL) {
	odent.d_ino = dent->d_fileno;
	strncpy((char *)odent.d_name, dent->d_name, 14);
	write(i, &odent, 16);
    }
    closedir(d);
    lseek(i, 0, SEEK_SET);
    return (i);
}

static int
trap_gtty(u_int16_t fd, u_int16_t ucnt)
{
    struct tr_sgttyb *sgtb;	/* used in GTTY/STTY */
    struct termios tios;	/* used in GTTY/STTY */
    int i;

    i = tcgetattr(fd, &tios);
    if (i == -1)
	return i;
    CLR_CC_C();
    sgtb = (struct tr_sgttyb *) & dspace[ucnt];
    sgtb->sg_ispeed = cfgetispeed(&tios); /* tios.c_ispeed; --gray */
    sgtb->sg_ospeed = cfgetospeed(&tios); /* tios.c_ospeed; --gray */
    sgtb->sg_erase = tios.c_cc[VERASE];
    sgtb->sg_kill = tios.c_cc[VKILL];
    sgtb->sg_flags = 0;
    if (tios.c_oflag & OXTABS)
	sgtb->sg_flags |= TR_XTABS;
    if (tios.c_cflag & PARENB) {
	if (tios.c_cflag & PARODD)
	    sgtb->sg_flags |= TR_ODDP;
	else
	    sgtb->sg_flags |= TR_EVENP;
    } else
	sgtb->sg_flags |= TR_ANYP;
    if (tios.c_oflag & ONLCR)
	sgtb->sg_flags |= TR_CRMOD;
    if (tios.c_lflag & ECHO)
	sgtb->sg_flags |= TR_ECHO;
    if (!(tios.c_lflag & ICANON)) {
	if (!(tios.c_lflag & ECHO))
	    sgtb->sg_flags |= TR_CBREAK;
	else
	    sgtb->sg_flags |= TR_RAW;
    }
    return 0;
}
static int
trap_stty(u_int16_t fd, u_int16_t ucnt)
{
    struct tr_sgttyb *sgtb;	/* used in GTTY/STTY */
    struct termios tios;	/* used in GTTY/STTY */
    int i;

    if (ucnt != 0) {
	sgtb = (struct tr_sgttyb *) & dspace[ucnt];
	cfsetispeed(&tios, sgtb->sg_ispeed); 
	cfsetospeed(&tios, sgtb->sg_ospeed);
	tios.c_cc[VERASE] = sgtb->sg_erase;
	tios.c_cc[VKILL] = sgtb->sg_kill;
	if (sgtb->sg_flags & TR_XTABS)
	    tios.c_oflag |= OXTABS;
	if (sgtb->sg_flags & TR_ODDP) {
	    tios.c_cflag |= PARENB;
	    tios.c_cflag &= ~PARODD;
	}
	if (sgtb->sg_flags & TR_EVENP)
	    tios.c_cflag |= PARENB | PARODD;
	if (sgtb->sg_flags & TR_ANYP)
	    tios.c_cflag &= ~PARENB;
	if (sgtb->sg_flags & TR_CRMOD)
	    tios.c_oflag |= ONLCR;
	if (sgtb->sg_flags & TR_ECHO)
	    tios.c_lflag |= ECHO;
	if (sgtb->sg_flags & TR_RAW) {
	    tios.c_lflag &= (~ICANON) & (~ECHO);
	    for (i = 0; i < NCCS; i++)
		tios.c_cc[i] = 0;
	    tios.c_cc[VMIN] = 1;
	}
	if (sgtb->sg_flags & TR_CBREAK) {
	    tios.c_lflag &= (~ICANON);
	    tios.c_lflag |= ECHO;
	    for (i = 0; i < NCCS; i++)
		tios.c_cc[i] = 0;
	    tios.c_cc[VMIN] = 1;
	}
	i = tcsetattr(fd, TCSANOW, &tios);
	return (i);
    } else
	return (-1);
}


/* Where possible, deal with signals */
static int v7signal(int sig, int val)
{
  if (sig>V7_NSIG) { errno=EINVAL; return(-1); }
  if (v7sig[sig]==0) return(0);

  switch(val) {
    case V7_SIG_IGN:
      return((int)signal(v7sig[sig], SIG_IGN));
    case V7_SIG_DFL:
      return((int)signal(v7sig[sig], SIG_DFL));
    default:
      return(0);      /* No handling of this as yet */
  }
}

/* Workaround for bug in V5/V6 ctime() */
static void fixv6time(time_t *t)
{
 struct tm *T;

 T=gmtime(t);
 if (T->tm_year>98) T->tm_year=98;
 *t=timegm(T);
}
