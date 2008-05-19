/* bsdtrap.c - Deal with 2.11BSD trap instructions.
 *
 * $Revision: 1.66 $
 * $Date: 2008/05/19 13:26:42 $
 */
#ifdef EMU211

/* NOTE NOTE NOTE NOTE
 * Grep for the word DONE in this file to see the implemented syscalls.
 */

#define BSDTRAP_NAME
#include "defines.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <utime.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "bsdtrap.h"
#ifdef __linux__
#include <grp.h>		/* setgroups() */
#endif

#define MAX_BLKSIZE	1024	/* Maximum block size from stat/fstat */

#undef STREAM_BUFFERING		/* This works, but doesn't seem to give */
				/* any speed improvement */

arglist *A;			/* Pointer to various arguments on stack */


/* Forward prototypes */
#ifdef __STDC__
#define P(s) s
#else
#define P(s) ()
#endif
static int trap_execve P((int));
static int bsdopen_dir P((char *name));
#ifdef NEED_MAP_FCNTL
static int16_t map_fcntl P((int16_t f));
#endif
#undef P

void
bsdtrap()
{
    int i, j, len, pid, pfd[2];
    char *buf, *buf2;
    int16_t *shortptr;
    long larg1;
    char *fmode;				/* used with fdopen only */
    struct stat stbuf;				/* used in STAT */
    struct tr_stat *tr_stbuf;			/* used in STAT */
    struct tr_timeval *tr_del, *tr_oldel;	/* used in ADJTIME */
    struct timeval del, oldel;			/* used in ADJTIME */
    struct timeval utv[2];			/* used in UTIMES */
    struct tr_timezone *tr_zone;		/* used in GETTIMEOFDAY */
    struct timezone zone;			/* used in GETTIMEOFDAY */
    struct tr_itimerval *tr_tval, *tr_oltval;	/* used in itimer calls */
    struct itimerval tval, oltval;		/* used in itimer calls */
    struct tr_sockaddr *tr_sock;		/* used in socket calls */
    struct sockaddr sock;			/* used in socket calls */
    gid_t *gidset;				/* used in GETGROUPS */
    struct tr_rlimit *tr_rlp;			/* used in rlimit calls */
    struct rlimit rlp;				/* used in rlimit calls */
    struct tr_rusage *tr_use;			/* used in getrusage */
    struct rusage use;				/* used in getrusage */
    struct iovec *ivec;				/* used in writev, readv */
    struct tr_iovec *trivec;			/* used in writev, readv */

    TrapDebug((dbg_file, "pid %d %s: ", (int)getpid(),bsdtrap_name[ir & 0xff]));

    A= (arglist *)&dspace[(regs[SP]+2)];

    i=errno=0;
    switch (ir & 0xff) {
    case S_INDIR:
	(void)printf("Does 2.11BSD use INDIR? I don't think so\n");
	exit(EXIT_FAILURE);

    case S_QUOTA: 				/* DONE - for now */
    case S_SETQUOTA:				/* DONE - for now */
	i=-1; errno=EINVAL; break;

			/* These syscalls are not implemented, and */
			/* always return EPERM to the caller */
    case S_PTRACE:				/* DONE - bad syscall */
    case S_MOUNT:				/* DONE - bad syscall */
    case S_UMOUNT:				/* DONE - bad syscall */
    case S_PROFIL:				/* DONE - bad syscall */
    case S_NOSYS147:				/* DONE - bad syscall */
    case S_SYSCTL:
	i=-1; errno=EPERM; break;
			/* These syscalls are ignored, and */
			/* always return C=0 to the caller */
    case S_OLDLOCK:				/* DONE - ok syscall */
    case S_OLDPHYS:				/* DONE - ok syscall */
    case S_FSTATFS:				/* DONE - ok syscall */
    case S_SIGPROCMASK:				/* DONE - ok syscall */
    case S_SIGRETURN:				/* DONE - ok syscall */
    case S_SIGALTSTACK:				/* DONE - ok syscall */
    case S_VHANGUP:				/* DONE - ok syscall */
	i=0; break;
    case S_SIGACTION:				/* DONE */
#define NO_SIGNALS_YET
#ifdef NO_SIGNALS_YET
	i=0;
#else
	i= do_sigaction(uarg1, uarg2, uarg3);
#endif
	break;
    case S_IOCTL:				/* DONE a bit */
	i=trap_ioctl(); break;
    case S_SBRK:				/* DONE */
	if (uarg1<regs[SP]) {
	    i=0;
	    TrapDebug((dbg_file, "set break to %d ", uarg1));
	} else {
	    i=-1; errno=ENOMEM;
	    TrapDebug((dbg_file, "break %d > SP %d", uarg1, regs[SP]));
	}
	break;
    case S_SYNC:				/* DONE */
	sync(); i=0; break;
    case S_FSYNC:				/* DONE */
	i= fsync(sarg1); break;
    case S_GETDTABLESIZE:			/* DONE */
	i= getdtablesize(); break;
    case S_EXIT:				/* DONE */
#ifdef DEBUG
        TrapDebug((dbg_file, "val %d\n",sarg1)); fflush(dbg_file);
#endif
	exit(sarg1);
    case S_DUP:					/* DONE */
        TrapDebug((dbg_file, "on %d ",sarg1));
	i = dup(sarg1);
#ifdef STREAM_BUFFERING
            if ((i!=-1) && ValidFD(sarg1) && stream[sarg1]) {
                fmode= streammode[sarg1];
                stream[i] = fdopen(i, fmode);
                streammode[i]=fmode;
            }
#endif
	break;
    case S_DUP2:				/* DONE */
        TrapDebug((dbg_file, "on %d %d ",sarg1,sarg2));
	i = dup2(sarg1,sarg2);
#ifdef STREAM_BUFFERING
            if ((i!=-1) && ValidFD(sarg2) && ValidFD(sarg1) && stream[sarg1]) {
                fmode= streammode[sarg1];
                stream[sarg2] = fdopen(sarg2, fmode);
                streammode[sarg2]=fmode;
            }
#endif
	break;
    case S_REBOOT:				/* DONE */
	(void)Reboot(sarg1); break;
    case S_UMASK:				/* DONE */
	i = umask((mode_t)sarg1); break;
    case S_GETPAGESIZE:				/* DONE */
	i = getpagesize(); break;
    case S_GETHOSTNAME:				/* DONE */
	buf = (char *)&dspace[uarg1];
	i= gethostname(buf,sarg2); break;
    case S_SETHOSTNAME:				/* DONE */
	buf = (char *)&dspace[uarg1];
	sethostname(buf,sarg2); break;
    case S_LSEEK:				/* DONE */
	larg1 = (sarg2 << 16) | uarg3;
#ifdef STREAM_BUFFERING
        if (ValidFD(sarg1) && stream[sarg1]) {
            i = fseek(stream[sarg1], larg1, sarg4);
            if (i == 0) i = ftell(stream[sarg1]);
        } else
#endif
	i = lseek(sarg1, larg1, sarg4);
	if (i == -1) break;
	else {
	    regs[1] = i & 0xffff;
	    i= (i >> 16) & 0xffff;
	    break;
	}
    case S_READ:				/* DONE */
        TrapDebug((dbg_file, "%d bytes on %d ",uarg3,sarg1));
	buf = (char *)&dspace[uarg2];
#ifdef STREAM_BUFFERING
        if (ValidFD(sarg1) && stream[sarg1])
            i = fread(buf, 1, uarg3, stream[sarg1]);
        else
#endif
	i = read(sarg1, buf, uarg3); break;
    case S_LINK:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	buf2 = xlate_filename((char *)&dspace[uarg2]);
	i = link(buf, buf2); break;
    case S_SYMLINK:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	buf2 = xlate_filename((char *)&dspace[uarg2]);
	i = symlink(buf, buf2); break;
    case S_RENAME:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	buf2 = xlate_filename((char *)&dspace[uarg2]);
	i = rename(buf, buf2); break;
    case S_READLINK:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = readlink(buf, (char *)&dspace[uarg2], sarg3); break;
    case S_ACCESS:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = access(buf, sarg2); break;
    case S_MKDIR:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = mkdir(buf, sarg2); break;
    case S_RMDIR:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = rmdir(buf); break;
    case S_ACCT:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = acct(buf); break;
    case S_WRITEV:				/* DONE */
    case S_READV:				/* DONE */
	ivec= (struct iovec *)malloc(uarg3 * sizeof(struct iovec));
	if (ivec==NULL) { i=-1; errno=EINVAL; break; }
	trivec= (struct tr_iovec *)&dspace[uarg2];

	for (j=0; j<uarg3; j++) {
	    ivec[j].iov_len= trivec[j].iov_len;
	    ivec[j].iov_base= (char *)&dspace[trivec[j].iov_base];
	}
	if ((ir & 0xff)==S_READV) i= readv(sarg1, ivec, uarg3);
	else i= writev(sarg1, ivec, uarg3);
	free(ivec);
	break;
    case S_WRITE:				/* DONE */
	buf = (char *)&dspace[uarg2];
        TrapDebug((dbg_file, "%d bytes on %d ",uarg3,sarg1));
#ifdef STREAM_BUFFERING
        if (ValidFD(sarg1) && stream[sarg1])
            i = fwrite(buf, 1, uarg3, stream[sarg1]);
        else
#endif
	i = write(sarg1, buf, uarg3); break;
    case S_CLOSE:				/* DONE */
#ifdef DEBUG
    TrapDebug((dbg_file, "on %d ",sarg1));
    if ((dbg_file!=NULL) && (sarg1==fileno(dbg_file))) {
	i=0; break;			/* Don't close our debug file! */
    }
#endif
#ifdef STREAM_BUFFERING
        if (ValidFD(sarg1) && stream[sarg1]) {
            i = fclose(stream[sarg1]);
            stream[sarg1] = NULL;
        } else
#endif
	i = close(sarg1); break;
    case S_FCNTL:
        TrapDebug((dbg_file, "on %d %d %d ",sarg1,sarg2, sarg3));
	i = fcntl(sarg1,sarg2,sarg3); break;
    case S_FLOCK:
	i = flock(sarg1,sarg2); break;
    case S_LSTAT:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	tr_stbuf = (struct tr_stat *) &dspace[uarg2];
	i = lstat(buf, &stbuf);
        TrapDebug((dbg_file, "on %s ",buf));
	goto dostat;
    case S_STAT:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	tr_stbuf = (struct tr_stat *) &dspace[uarg2];
	i = stat(buf, &stbuf);
        TrapDebug((dbg_file, "on %s ",buf));
	goto dostat;
    case S_FSTAT:				/* DONE */
	tr_stbuf = (struct tr_stat *) &dspace[uarg2];
	i = fstat(uarg1, &stbuf);
        TrapDebug((dbg_file, "on fd %d ",uarg1));

dostat:
	if (i == -1) break;
	else {
			/* The following stops blksize equalling 64K,
			 * which becomes 0 in a 16-bit int. This then
			 * causes 2.11BSD flsbuf() to malloc(0), which
			 * then causes malloc to go crazy - wkt.
			 */
	    if (stbuf.st_blksize>MAX_BLKSIZE) stbuf.st_blksize=MAX_BLKSIZE;

	    tr_stbuf->st_dev = stbuf.st_dev;
	    tr_stbuf->st_ino = stbuf.st_ino;
	    tr_stbuf->st_mode = stbuf.st_mode;
	    tr_stbuf->st_nlink = stbuf.st_nlink;
	    tr_stbuf->st_uid = stbuf.st_uid;
	    tr_stbuf->st_gid = stbuf.st_gid;
	    tr_stbuf->st_rdev = stbuf.st_rdev;
#ifndef NO_STFLAGS
	    tr_stbuf->st_flags = stbuf.st_flags;
#endif
	    copylong(tr_stbuf->st_size, stbuf.st_size);
	    copylong(tr_stbuf->st_atim, stbuf.st_atime);
	    copylong(tr_stbuf->st_mtim, stbuf.st_mtime);
	    copylong(tr_stbuf->st_ctim, stbuf.st_ctime);
	    copylong(tr_stbuf->st_blksize, stbuf.st_blksize);
	    larg1= stbuf.st_blocks; copylong(tr_stbuf->st_blocks, larg1);
	}
	break;
    case S_UTIMES:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	tr_del = (struct tr_timeval *) &dspace[uarg2];
	tr_oldel = (struct tr_timeval *) &dspace[uarg4];
	i= utimes(buf, utv);
	if (i==-1) break;
	copylong(tr_del->tv_sec,    utv[0].tv_sec);
	copylong(tr_del->tv_usec,   utv[0].tv_usec);
	copylong(tr_oldel->tv_sec,  utv[1].tv_sec);
	copylong(tr_oldel->tv_usec, utv[1].tv_usec);
	break;
    case S_ADJTIME:				/* DONE */
	tr_del = (struct tr_timeval *) &dspace[uarg1];
						/* Convert tr_del to del */
	copylong(del.tv_sec,  tr_del->tv_sec);
	copylong(del.tv_usec, tr_del->tv_usec);
	i= adjtime(&del, &oldel);

	if (uarg2) {
	    tr_oldel = (struct tr_timeval *) &dspace[uarg2];
	    copylong(tr_oldel->tv_sec,	oldel.tv_sec);
	    copylong(tr_oldel->tv_usec, oldel.tv_usec);
	}
	break;
    case S_GETTIMEOFDAY:			/* DONE */
	tr_del = (struct tr_timeval *)	 &dspace[uarg1];
	tr_zone = (struct tr_timezone *) &dspace[uarg2];
	i= gettimeofday(&del, &zone);
	copylong(tr_del->tv_sec,  del.tv_sec);
	copylong(tr_del->tv_usec, del.tv_usec);
	tr_zone->tz_minuteswest= zone.tz_minuteswest;
	tr_zone->tz_dsttime= zone.tz_dsttime;
	break;
    case S_SETTIMEOFDAY:			/* DONE */
	tr_del = (struct tr_timeval *)	 &dspace[uarg1];
	tr_zone = (struct tr_timezone *) &dspace[uarg2];
	copylong(del.tv_sec,  tr_del->tv_sec);
	copylong(del.tv_usec, tr_del->tv_usec);
	zone.tz_minuteswest= tr_zone->tz_minuteswest;
	zone.tz_dsttime= tr_zone->tz_dsttime;
	i= settimeofday(&del, &zone);
	break;
    case S_GETITIMER:				/* DONE */
	tr_tval = (struct tr_itimerval *) &dspace[uarg2];
	i= getitimer(sarg1, &tval);
	copylong(tr_tval->it_interval.tv_sec,  tval.it_interval.tv_sec);
	copylong(tr_tval->it_interval.tv_usec, tval.it_interval.tv_usec);
	copylong(tr_tval->it_value.tv_sec,  tval.it_value.tv_sec);
	copylong(tr_tval->it_value.tv_usec, tval.it_value.tv_usec);
	break;
    case S_SETITIMER:				/* DONE */
	tr_tval = (struct tr_itimerval *) &dspace[uarg2];
	tr_oltval = (struct tr_itimerval *) &dspace[uarg2];
	copylong(tval.it_interval.tv_sec,  tr_tval->it_interval.tv_sec);
	copylong(tval.it_interval.tv_usec, tr_tval->it_interval.tv_usec);
	copylong(tval.it_value.tv_sec,	   tr_tval->it_value.tv_sec);
	copylong(tval.it_value.tv_usec,	   tr_tval->it_value.tv_usec);
	i= setitimer(sarg1, &tval, &oltval);
	if (i == -1) break;
	copylong(tr_oltval->it_interval.tv_sec,	 oltval.it_interval.tv_sec);
	copylong(tr_oltval->it_interval.tv_usec, oltval.it_interval.tv_usec);
	copylong(tr_oltval->it_value.tv_sec,	 oltval.it_value.tv_sec);
	copylong(tr_oltval->it_value.tv_usec,	 oltval.it_value.tv_usec);
	break;
    case S_UNLINK:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = unlink(buf); break;
    case S_OPEN:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);

	i = stat(buf, &stbuf);	/* If file is a directory */
	if (i == 0 && (stbuf.st_mode & S_IFDIR)) {
	    i = bsdopen_dir(buf);
	    fmode = "w+";
            TrapDebug((dbg_file, "(dir) on %s ",buf));
	} else {
#ifdef NEED_MAP_FCNTL
	    sarg2= map_fcntl(sarg2);
#endif
 	    switch (sarg2 & O_ACCMODE) {
              case O_RDONLY: fmode = "r"; break;
              case O_WRONLY: fmode = "w"; break;
              default: fmode = "w+"; break;
            }
	    i = open(buf, sarg2, sarg3);
            TrapDebug((dbg_file, "on %s ",buf));
            TrapDebug((dbg_file, "sarg2 is %d, sarg3 is 0x%x ",sarg2,sarg3));
	}
#ifdef STREAM_BUFFERING
	if (i==-1) break;
        /* Now get its stream pointer if possible */
        /* Can someone explain why fdopen doesn't work for O_RDWR? */
# if 0
        if (ValidFD(i) && !isatty(i) && (sarg2!=O_RDWR)) {
            stream[i] = fdopen(i, fmode); streammode[i]=fmode;
        }
# endif
	stream[i] = fdopen(i, fmode); streammode[i]=fmode;
#endif
	break;
    case S_MKNOD:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = mknod(buf, sarg2, sarg3); break;
    case S_CHMOD:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chmod(buf, sarg2); break;
    case S_FCHMOD:				/* DONE */
	i = fchmod(sarg1, sarg2); break;
    case S_TRUNCATE:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	larg1 = (sarg2 << 16) | uarg3;
	i = truncate(buf, larg1); break;
    case S_FTRUNCATE:				/* DONE */
	larg1 = (sarg2 << 16) | uarg3;
	i = ftruncate(sarg1, larg1); break;
    case S_KILL:				/* DONE */
	i = kill(sarg1, sarg2); break;
    case S_KILLPG:				/* DONE */
	i = killpg(sarg1, sarg2); break;
    case S_CHOWN:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chown(buf, sarg2, sarg3); break;
    case S_PIPE:				/* DONE */
	i = pipe(pfd);
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
	if (i == -1) break;
	i = pfd[0]; regs[1] = pfd[1]; break;
    case S_CHROOT:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	if (buf == NULL) {
	    errno=ENOENT; i=-1; break;
	}
	set_apout_root(buf);
	i=0; break;
    case S_CHDIR:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chdir(buf); break;
    case S_FCHDIR:				/* DONE */
	i = fchdir(sarg1); break;

#ifndef NO_CHFLAGS
    case S_CHFLAGS:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = chflags(buf,uarg2); break;
    case S_FCHFLAGS:				/* DONE */
	i = fchflags(sarg1,uarg2); break;
#endif

    case S_CREAT:				/* DONE */
	buf = xlate_filename((char *)&dspace[uarg1]);
	i = creat(buf, sarg2);
#ifdef STREAM_BUFFERING
        if (ValidFD(i)) {
                stream[i] = fdopen(i, "w");
                streammode[i]= "w";
        }
#endif
	break;
    case S_EXECVE:				/* DONE, I think */
	i= trap_execve(1); break;
    case S_EXECV:				/* Not sure here */
	i= trap_execve(0); break;
    case S_WAIT:				/* Not sure here */
	i = wait(&pid);
	if (i == -1) break;
	regs[1] = pid;
	break;
    case S_WAIT4:				/* Definitely incomplete */
	TrapDebug((dbg_file, "on pid %d options %d ",sarg1,uarg3));
	if (uarg4) TrapDebug((dbg_file, " rusage on!!! "));
	shortptr = (int16_t *)&dspace[uarg2];
	i= wait4(sarg1, &j, uarg3, 0);
	*shortptr= j;
	break;
    case S_FORK:				/* DONE */
    case S_VFORK:				/* DONE */
	i = fork();
	if (i!=0) { regs[PC]+=2; }		/* Took ages to find this! */
	else ov_changes=0;
	break;
    case S_GETHOSTID:				/* DONE */
	i = gethostid();
	regs[1] = i & 0xffff;
	i= (i >> 16) & 0xffff;
	break;
    case S_SETHOSTID:				/* DONE */
	larg1 = (sarg2 << 16) | uarg3;
	sethostid(larg1); i=0; break;
    case S_GETUID:				/* DONE */
	i = getuid(); break;
    case S_SETUID:				/* DONE */
	i = setuid(uarg1); break;
    case S_GETEUID:				/* DONE */
	i = geteuid(); break;
    case S_GETPID:				/* DONE */
	i = getpid(); break;
    case S_GETPPID:				/* DONE */
	i = getppid(); break;
    case S_GETGID:				/* DONE */
	i = getgid(); break;
    case S_GETEGID:				/* DONE */
	i = getegid(); break;
#ifndef NO_GETPGID
    case S_GETPGRP:				/* DONE */
	i = getpgid(sarg1); break;
#endif
    case S_SETPGRP:				/* DONE */
	i = setpgid(sarg1,sarg2); break;
    case S_SETREGID:				/* DONE */
	i = setregid(sarg1,sarg2); break;
    case S_SETREUID:				/* DONE */
	i = setreuid(sarg1,sarg2); break;
    case S_GETPRIORITY:				/* DONE */
	i = getpriority(sarg1,sarg2); break;
    case S_SETPRIORITY:				/* DONE */
	i = setpriority(sarg1,sarg2,sarg3); break;
    case S_LISTEN:				/* DONE */
	i = listen(sarg1,sarg2); break;
    case S_SHUTDOWN:				/* DONE */
	i = shutdown(sarg1,sarg2); break;
    case S_SOCKET:				/* DONE */
	i = socket(sarg1,sarg2,sarg3); break;
    case S_SOCKETPAIR:				/* DONE */
	i = socketpair(sarg1,sarg2,sarg3, pfd); break;
	shortptr= (int16_t *)&dspace[uarg4];
	*shortptr= pfd[0]; shortptr+=2;
	*shortptr= pfd[1];
	break;
    case S_RECV:				/* DONE */
	buf = (char *)&dspace[uarg2];
	i = recv(sarg1, buf, sarg3, sarg4); break;
    case S_SEND:				/* DONE */
	buf = (char *)&dspace[uarg2];
	i = send(sarg1, buf, sarg3, sarg4); break;
    case S_ACCEPT:				/* DONE */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg2];
	sock.sa_family= tr_sock->sa_family;
	ll_word(uarg3, len);
#ifndef __linux__
	sock.sa_len=len;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	i= accept(sarg1, &sock, (socklen_t *)&len);
	if (i != -1) {
	    sl_word(uarg3,len);
	    memcpy(tr_sock->sa_data, sock.sa_data, len);
	}
	break;
    case S_GETPEERNAME:				/* DONE */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg2];
	sock.sa_family= tr_sock->sa_family;
	ll_word(uarg3, len);
#ifndef __linux__
	sock.sa_len=len;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	i= getpeername(sarg1, &sock, (socklen_t *)&len);
	if (i != -1) {
	    sl_word(uarg3,len);
	    memcpy(tr_sock->sa_data, sock.sa_data, len);
	}
	break;
    case S_GETSOCKNAME:				/* DONE */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg2];
	sock.sa_family= tr_sock->sa_family;
	ll_word(uarg3, len);
#ifndef __linux__
	sock.sa_len=len;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	i= getsockname(sarg1, &sock, (socklen_t *)&len);
	if (i != -1) {
	    sl_word(uarg3,len);
	    memcpy(tr_sock->sa_data, sock.sa_data, len);
	}
	break;
    case S_BIND:				/* DONE */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg2];
	sock.sa_family= tr_sock->sa_family;
	len= sarg3;
#ifndef __linux__
	sock.sa_len= sarg3;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	i= bind(sarg1, &sock, len);
	break;
    case S_CONNECT:				/* DONE */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg2];
	sock.sa_family= tr_sock->sa_family;
	len= sarg3;
#ifndef __linux__
	sock.sa_len= sarg3;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	i= connect(sarg1, &sock, len);
	break;
    case S_RECVFROM:				/* DONE I think */
	tr_sock= (struct tr_sockaddr *)&dspace[uarg5];
	sock.sa_family= tr_sock->sa_family;
	ll_word(uarg6, len);
#ifndef __linux__
	sock.sa_len=len;
#endif
	memcpy(sock.sa_data, tr_sock->sa_data, len);
	buf = (char *)&dspace[uarg2];
	i= recvfrom(sarg1, buf, sarg3, sarg4, &sock, (socklen_t *)&len);
	if (i != -1) {
	    sl_word(uarg6,len);
	    memcpy(tr_sock->sa_data, sock.sa_data, len);
	}
	break;
    case S_GETGROUPS:
	len= sarg1;
	gidset= (gid_t *)malloc(len * sizeof(gid_t));
	if (gidset==NULL) { i=-1; errno=EINVAL; break; }
	i= getgroups(len, gidset);
	shortptr= (int16_t *)&dspace[uarg2];
	for (j=0; j<i; j++) shortptr[j]= gidset[j];
	free(gidset);
	break;
    case S_SETGROUPS:
	len= sarg1;
	if (len>16) { i=-1; errno=EFAULT; break; }
	gidset= (gid_t *)malloc(len * sizeof(gid_t));
	if (gidset==NULL) { i=-1; errno=EINVAL; break; }
	shortptr= (int16_t *)&dspace[uarg2];
	for (j=0; j<len; j++) gidset[j]= shortptr[j];
	i= setgroups(len, gidset);
	free(gidset);
	break;
    case S_GETRLIMIT:				/* DONE */
	tr_rlp= (struct tr_rlimit *)&dspace[uarg2];
	i= getrlimit(sarg1, &rlp);
	if (i== -1) break;
	copylong(tr_rlp->rlim_cur, rlp.rlim_cur);
	copylong(tr_rlp->rlim_max, rlp.rlim_max);
	break;
    case S_SETRLIMIT:				/* DONE */
	tr_rlp= (struct tr_rlimit *)&dspace[uarg2];
	copylong(rlp.rlim_cur, tr_rlp->rlim_cur);
	copylong(rlp.rlim_max, tr_rlp->rlim_max);
	i= setrlimit(sarg1, &rlp);
	break;
    case S_GETRUSAGE:
	TrapDebug((dbg_file, "arg1 %d pointer 0%o ",sarg1,uarg2));
	tr_use = (struct tr_rusage *) &dspace[uarg2];
	i= getrusage(sarg1, &use);
	if (i==-1) break;

        /* Should do tr_use->ru_utime;        user time used */
        /* Should do tr_use->ru_stime;        system time used */
        copylong(tr_use->ru_maxrss, use.ru_maxrss);
        copylong(tr_use->ru_ixrss, use.ru_ixrss);
        copylong(tr_use->ru_idrss, use.ru_idrss);
        copylong(tr_use->ru_isrss, use.ru_isrss);
        copylong(tr_use->ru_minflt, use.ru_minflt);
        copylong(tr_use->ru_majflt, use.ru_majflt);
        copylong(tr_use->ru_ovly, ov_changes);
        copylong(tr_use->ru_nswap, use.ru_nswap);
        copylong(tr_use->ru_inblock, use.ru_inblock);
        copylong(tr_use->ru_oublock, use.ru_oublock);
        copylong(tr_use->ru_msgsnd, use.ru_msgsnd);
        copylong(tr_use->ru_msgrcv, use.ru_msgrcv);
        copylong(tr_use->ru_nsignals, use.ru_nsignals);
        copylong(tr_use->ru_nvcsw, use.ru_nvcsw);
        copylong(tr_use->ru_nivcsw, use.ru_nivcsw);
	break;
    default:
	if ((ir & 0xff)>S_GETSOCKNAME) {
	  TrapDebug((stderr,"Apout - unknown syscall %d at PC 0%o\n",
							ir & 0xff,regs[PC]));
	} else {
          (void)fprintf(stderr,
		"Apout - the 2.11BSD %s syscall is not yet implemented\n",
						bsdtrap_name[ir & 0xff]);
	}
	exit(EXIT_FAILURE);
    }

	/* Set r0 to either errno or i, */
	/* and clear/set C bit */

    if (i == -1) {
	SET_CC_C(); regs[0] = errno;
	TrapDebug((dbg_file, "errno is %s\n", strerror(errno)));
    } else {
	CLR_CC_C(); regs[0]=i;
	TrapDebug((dbg_file, "return %d\n", i));
    }
    return;
}



static int
trap_execve(int want_env)
{
    u_int16_t cptr, cptr2;
    char *buf, *name, *origpath;

    origpath = strdup((char *)&dspace[uarg1]);
    name = xlate_filename(origpath);
    TrapDebug((dbg_file, "%s Execing %s (%s) ", progname, name, origpath));

    set_bsdsig_dfl();	/* Set signals to default values */

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

    if (want_env)
      { cptr=uarg3;
        while (Envc < MAX_ARGS) {
	  ll_word(cptr, cptr2);
	  if (cptr2 == 0)
	    break;
	  buf = (char *)&dspace[cptr2];
	  Envp[Envc++] = strdup(buf);
	  cptr += 2;
        }
       Envp[Envc] = NULL;
      }

    if (load_a_out(name,origpath,want_env) == -1) {
	for (Argc--; Argc >= 0; Argc--)
	    free(Argv[Argc]);
	if (want_env) for (Envc--; Envc >= 0; Envc--)
	    free(Envp[Envc]);
	errno= ENOENT; return(-1);
    }
    run();			/* Ok, so it's recursive, I dislike setjmp */
    return(0);			/* Just to shut the compiler up */
}

/* 2.11BSD reads directories as if they were ordinary files.
 * The solution is to read the directory entries, and build a
 * real file, which is passed back to the open call.
 *
 * A directory consists of some number of blocks of DIRBLKSIZ
 * bytes, where DIRBLKSIZ is chosen such that it can be transferred
 * to disk in a single atomic operation (e.g. 512 bytes on most machines).
 *
 * Each DIRBLKSIZ byte block contains some number of directory entry
 * structures, which are of variable length.  Each directory entry has
 * a struct direct at the front of it, containing its inode number,
 * the length of the entry, and the length of the name contained in
 * the entry.  These are followed by the name padded to a 4 byte boundary
 * with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 *
 * The macro DIRSIZ(dp) gives the amount of space required to represent
 * a directory entry.  Free space in a directory is represented by
 * entries which have dp->d_reclen > DIRSIZ(dp).  All DIRBLKSIZ bytes
 * in a directory block are claimed by the directory entries.  This
 * usually results in the last entry in a directory having a large
 * dp->d_reclen.  When entries are deleted from a directory, the
 * space is returned to the previous entry in the same directory
 * block by increasing its dp->d_reclen.  If the first entry of
 * a directory block is free, then its dp->d_ino is set to 0.
 * Entries other than the first in a directory do not normally have
 * dp->d_ino set to 0.
 */

static int
bsdopen_dir(char *name)
{
    DIR *d;
    char *tmpname;
    int i, nlen, total_left;
    struct dirent *dent;
    struct tr_direct odent, empty;

    d = opendir(name);
    if (d == NULL) return (-1);
    tmpname= strdup(TMP_PLATE);
    i= mkstemp(tmpname);
    if (i == -1) {
	(void)fprintf(stderr,"Apout - open_dir couldn't open %s\n", tmpname);
	exit(EXIT_FAILURE);
    }
    unlink(tmpname); free(tmpname);

    total_left=TR_DIRBLKSIZ;
    empty.d_ino=0; empty.d_namlen=0; empty.d_name[0]='\0';
    empty.d_name[1]='\0'; empty.d_name[2]='\0'; empty.d_name[3]='\0';

    while ((dent = readdir(d)) != NULL) {
	memset(odent.d_name, 0, TR_MAXNAMLEN+1);	/* Null name */
	nlen= strlen(dent->d_name) +1;			/* Name length */
	if (nlen>TR_MAXNAMLEN+1) nlen=TR_MAXNAMLEN+1;
	strncpy(odent.d_name, dent->d_name, nlen);
	odent.d_ino = dent->d_fileno;
						/* Nasty hack: ensure inode */
						/* is never 0 */
	if (odent.d_ino==0) odent.d_ino=1;
	odent.d_namlen= nlen;
	nlen+= (nlen & 3);			/* Round up to mult of 4 */
	odent.d_reclen=	 nlen+6;		/* Name + 3 u_int16_ts */

						/* Not enough room, write */
						/* a blank entry */
	if ( (total_left - odent.d_reclen) < 10) {
	    empty.d_reclen=total_left;
	    write(i, &empty, empty.d_reclen);
	    total_left=TR_DIRBLKSIZ;
	}
	write(i, &odent, odent.d_reclen);
	total_left-= odent.d_reclen;
    }
    closedir(d);

    if (total_left) {
	    empty.d_reclen=total_left; write(i, &empty, empty.d_reclen);
    }
    lseek(i, 0, SEEK_SET);
    return (i);
}

#ifdef NEED_MAP_FCNTL
/* Map the 2.11BSD fcntl mode bits to the underlying
 * system's bits. We have to do this for Linux
 */
static int16_t map_fcntl(int16_t f)
{
  int16_t out=0;

  if (f & BSD_RDONLY)   out |= O_RDONLY;
  if (f & BSD_WRONLY)   out |= O_WRONLY;
  if (f & BSD_RDWR)     out |= O_RDWR;
  if (f & BSD_NONBLOCK) out |= O_NONBLOCK;
  if (f & BSD_APPEND)   out |= O_APPEND;
  if (f & BSD_SHLOCK)   out |= O_SHLOCK;
  if (f & BSD_EXLOCK)   out |= O_EXLOCK;
  if (f & BSD_ASYNC)    out |= O_ASYNC;
  if (f & BSD_FSYNC)    out |= O_FSYNC;
  if (f & BSD_CREAT)    out |= O_CREAT;
  if (f & BSD_TRUNC)    out |= O_TRUNC;
  if (f & BSD_EXCL)     out |= O_EXCL;

  TrapDebug((dbg_file, "map_fcntl: 0x%x -> 0x%x, ",f,out));
  return(out);
}
#endif
#endif	/* EMU211 */
