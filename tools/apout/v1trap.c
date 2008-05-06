/* v1trap.c - Deal with 1st Edition trap instructions.
 *
 * $Revision: 1.15 $
 * $Date: 2002/06/10 11:43:24 $
 */
#ifdef EMUV1
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
#include "v1trap.h"

#ifdef __linux__
# undef STREAM_BUFFERING			/* It seems to work */
#else
# define STREAM_BUFFERING			/* but not for Linux */
#endif


/* Forward prototypes */
#ifdef __STDC__
#define P(s) s
#else
#define P(s) ()
#endif
static int v1trap_exec P((void));
static int v1open_dir P((char *name));
static u_int32_t sectosixty P((time_t tim));

#undef P


/* V1 keeps some of the arguments to syscalls in registers, and some
 * after the `sys' instruction itself. The list below gives the number
 * of words, and the number in registers.
 */
struct v1sysent {
  int nwords;
  int nregs;
};
static struct v1sysent v1arg[] = {
  {0, 0}, {0, 0}, {0, 0}, {3, 1}, {3, 1}, {2, 0}, {1, 1}, {1, 1},
  {2, 0}, {2, 0}, {1, 0}, {2, 0}, {1, 0}, {0, 0}, {2, 0}, {2, 0},
  {2, 0}, {1, 0}, {2, 0}, {3, 1}, {3, 1}, {2, 0}, {1, 0}, {1, 1},
  {1, 1}, {0, 0}, {1, 0}, {1, 0}, {2, 1}, {1, 0}, {1, 0}, {2, 1},
  {2, 1}, {1, 0}
};

/* Seeks on files in /dev are done in 512-byte blocks, not bytes.
 * If a fd's entry in the following table is 1, then it's a device
 * and not a file.
 */
int8_t isdev[NFILE]= {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static arglist V1A;

void v1trap()
{
  extern int32_t AC, MQ;	/* in ke11a.c */
  int i, mode, pid;
  int status, exitval, errval;	/* used in V2 wait */
  int whence;
  u_int16_t argbase;
  int trapnum;
  long larg;
  char *buf, *buf2;
  char *fmode;			/* used with fdopen only */

  struct stat stbuf;		/* used in STAT */
  struct tr_v1stat *t1;		/* used in STAT */
  struct timeval tval[2];	/* used in SMTIME */



  /* Work out the actual trap number, and */
  /* shift the PC up past any arguments */
  /* to the syscall. Calculate base of args */
  trapnum = ir & 077;
  argbase = regs[PC];
  regs[PC] += 2 * (v1arg[trapnum].nwords - v1arg[trapnum].nregs);

  /* Move arguments into V1A so we can use them */
  for (i = 0; i < v1arg[trapnum].nregs; i++) V1A.uarg[i] = regs[i];
  for (; i < v1arg[trapnum].nwords; i++, argbase += 2)
    ll_word(argbase, V1A.uarg[i]);

  TrapDebug((dbg_file, "pid %d %s: ", (int) getpid(), v1trap_name[trapnum]));

  switch (trapnum) {

      /* XXX STILL TO DO: V1_GTTY, V1_STTY, V1_TELL */

      			/* These syscalls are ignored, and return */
      			/* with no effect on the caller */
    case V1_BREAK:
    case V1_CEMT:
    case V1_ILGINS:
    case V1_INTR:
    case V1_QUIT:
    case V1_RELE: return;

      			/* These syscalls are not implemented, and */
      			/* always return no error to the caller */
    case V1_GTTY:
    case V1_STTY: i=0; break;
      			/* These syscalls are not implemented, and */
      			/* always return an error to the caller */
    case V1_MOUNT:
    case V1_UMOUNT: i = -1; break;

    case V1_EXIT:
      if (Binary==IS_V1) exit(0);
      if (Binary==IS_V2) {
	exitval=regs[0] & 0xff;
	if (regs[PC]==16790) exitval=0;	/* s2-tape /bin/as doesn't set r0 */
	TrapDebug((dbg_file, " with exitval %d\n", exitval));
	exit(exitval);
      }
      i = -1;
      break;

#define EPOCH71	31536000		/* # seconds from 1970 to 1971 */
#define EPOCH72	63072000		/* # seconds from 1970 to 1972 */
    case V1_SMDATE:
      buf = xlate_filename(&dspace[uarg1]);
      if (buf[0] == '\0') buf = ".";	/* Not documented anywhere */
      if (uarg1 == 0) buf = ".";	/* Who knows? for V1 */
      i = stat(buf, &stbuf);
      TrapDebug((dbg_file, " on %s (stat %d) ", buf, i));
      if (i == -1) break;

					/* Copy access time to preserve it */
      tval[0].tv_sec= stbuf.st_atime; tval[0].tv_usec=0;
      larg= (AC << 16) | (MQ & 0xffff); /* Get mod time in 60ths of a second */
      TrapDebug((dbg_file, " %ld -> ", larg));
      larg= larg/60 + EPOCH72;		/* Convert to seconds since 1970 */
      TrapDebug((dbg_file, " 0x%lx ", larg));
      tval[1].tv_sec= larg; tval[1].tv_usec=0;
      i=utimes(buf, tval);
      TrapDebug((dbg_file, " and %d for utimes ", i));
      break;

    case V1_TIME:
      			/* Kludge: treat start of this year as the epoch. */
      			/* Find #seconds from yearstart to now, multiply */
      			/* by 60 so as to be in V1 units */
      larg = sectosixty(time(NULL));
      MQ = (int) larg & 0xffff;
      AC = ((int) larg >> 16) & 0xffff;
      i = 0; break;
    case V1_SEEK:
      /* Work out the args before we do the lseek */
      whence = uarg3;
      switch (uarg3) {
	case 0:
	  larg = uarg2; break;
	case 1:
	case 2:
	  larg = sarg2; break;
      }

      if (ValidFD(sarg1) && isdev[sarg1]) larg*= 512;

#ifdef STREAM_BUFFERING
      if (ValidFD(sarg1) && stream[sarg1]) {
	i = fseek(stream[sarg1], larg, whence);
	if (i == 0)
	  i = ftell(stream[sarg1]);
      } else
#endif
	i = lseek(sarg1, larg, whence);

      TrapDebug((dbg_file, " on fd %d amt %ld whence %d return %d ",
		 				sarg1, larg, whence, i));
      if (i != -1) i = 0;
      regs[0] = i;
      break;
    case V1_READ:
      buf = &dspace[uarg2];
#ifdef STREAM_BUFFERING
      if (ValidFD(sarg1) && stream[sarg1])
	i = fread(buf, 1, sarg3, stream[sarg1]);
      else
#endif
	i = read(sarg1, buf, sarg3);
      TrapDebug((dbg_file, " on fd %d return %d ", sarg1, i));
      regs[0] = i; break;
    case V1_LINK:
      buf = xlate_filename(&dspace[uarg1]);
      buf2 = xlate_filename(&dspace[uarg2]);
      i = link(buf, buf2);
      regs[0] = i; break;
    case V1_WRITE:
      buf = &dspace[uarg2];
#ifdef STREAM_BUFFERING
      if (ValidFD(sarg1) && stream[sarg1])
	i = fwrite(buf, 1, sarg3, stream[sarg1]);
      else
#endif
	i = write(sarg1, buf, sarg3);
      TrapDebug((dbg_file, " on fd %d return %d ", sarg1, i));
      regs[0] = i; break;
    case V1_CLOSE:
#ifdef STREAM_BUFFERING
      if (ValidFD(sarg1) && stream[sarg1]) {
	i = fclose(stream[sarg1]);
	stream[sarg1] = NULL;
      } else
#endif
	i = close(sarg1);
      if ((i==0) && ValidFD(sarg1)) isdev[sarg1]=0;
      TrapDebug((dbg_file, " on fd %d return %d ", sarg1, i));
      break;
    case V1_STAT:
      buf = xlate_filename(&dspace[uarg1]);
      if (buf[0] == '\0') buf = ".";	/* Not documented anywhere */
      if (uarg1 == 0) buf = ".";	/* Who knows? for V1 */
      buf2 = &dspace[uarg2];
      i = stat(buf, &stbuf);
      TrapDebug((dbg_file, " on %s return %d ", buf, i));
      goto dostat;
    case V1_FSTAT:
      buf2 = &dspace[uarg2];
      i = fstat(sarg1, &stbuf);
      TrapDebug((dbg_file, " on fd %d return %d ", sarg1, i));

  dostat:
      if (i == -1) break;
      t1 = (struct tr_v1stat *) buf2;
				/* Inode numbers <41 are reserved for */
				/* device files. Ensure we don't use them */
      t1->inum = stbuf.st_ino & 0x7fff; if (t1->inum<41) t1->inum+=100;
      t1->inl = stbuf.st_nlink;
      t1->iuid = stbuf.st_uid;
      t1->isize = (u_int16_t) (stbuf.st_size & 0xffff);
      t1->iflags = (u_int16_t) (V1_ST_USED | V1_ST_MODIFIED);
      if (stbuf.st_size > 4095)    t1->iflags |= V1_ST_LARGE;
      if (stbuf.st_mode & S_IFDIR) t1->iflags |= V1_ST_ISDIR;
      if (stbuf.st_mode & S_ISUID) t1->iflags |= V1_ST_SETUID;
      if (stbuf.st_mode & S_IXUSR) t1->iflags |= V1_ST_EXEC;
      if (stbuf.st_mode & S_IRUSR) t1->iflags |= V1_ST_OWNREAD;
      if (stbuf.st_mode & S_IWUSR) t1->iflags |= V1_ST_OWNWRITE;
      if (stbuf.st_mode & S_IROTH) t1->iflags |= V1_ST_WRLDREAD;
      if (stbuf.st_mode & S_IWOTH) t1->iflags |= V1_ST_WRLDWRITE;

      larg = sectosixty(stbuf.st_ctime); copylong(t1->ctime, larg);
      larg = sectosixty(stbuf.st_mtime); copylong(t1->mtime, larg);
      break;
    case V1_UNLINK:
      buf = xlate_filename(&dspace[uarg1]);
      i = unlink(buf);
      break;
    case V1_OPEN:
      buf = xlate_filename(&dspace[uarg1]);

      i = stat(buf, &stbuf);	/* If file is a directory */
      if (i == 0 && (stbuf.st_mode & S_IFDIR)) {
	i = v1open_dir(buf);
	fmode = "w+";
	TrapDebug((dbg_file, "(dir) on %s return %d ", buf, i));
      } else {
	switch (sarg2) {
	  case 0:
	    sarg2 = O_RDONLY; fmode = "r"; break;
	  case 1:
	    sarg2 = O_WRONLY; fmode = "w"; break;
	  default:
	    sarg2 = O_RDWR; fmode = "w+"; break;
	}
	i = open(buf, sarg2);
	TrapDebug((dbg_file, " on %s return %d ", buf, i));
      }
      regs[0] = i;

      if (ValidFD(i) && !strncmp(&dspace[uarg1],"/dev/",5)) {
	TrapDebug((dbg_file, " (device file) "));
	isdev[i]=1;
      }

#ifdef STREAM_BUFFERING
      if (i==-1) break;
#if 0
      /* Now get its stream pointer if possible */
      /* Can someone explain why fdopen doesn't work for O_RDWR? */
      if (ValidFD(i) && !isatty(i) && (sarg2 != O_RDWR)) {
	stream[i] = fdopen(i, fmode); streammode[i] = fmode;
      }
#endif
	stream[i] = fdopen(i, fmode); streammode[i] = fmode;
#endif
      break;
    case V1_CHMOD:
      buf = xlate_filename(&dspace[uarg1]);
      mode = 0;
      if (uarg2 & V1_ST_SETUID)    mode |= S_ISUID;
      if (uarg2 & V1_ST_EXEC)      mode |= S_IXUSR | S_IXGRP | S_IXOTH;
      if (uarg2 & V1_ST_OWNREAD)   mode |= S_IRUSR;
      if (uarg2 & V1_ST_OWNWRITE)  mode |= S_IWUSR;
      if (uarg2 & V1_ST_WRLDREAD)  mode |= S_IRGRP | S_IROTH;
      if (uarg2 & V1_ST_WRLDWRITE) mode |= S_IWGRP | S_IWOTH;
      i = chmod(buf, mode);
      break;
    case V1_MKDIR:
      buf = xlate_filename(&dspace[uarg1]);
      mode = 0;
      if (uarg2 & V1_ST_SETUID)    mode |= S_ISUID;
      if (uarg2 & V1_ST_EXEC)      mode |= S_IXUSR | S_IXGRP | S_IXOTH;
      if (uarg2 & V1_ST_OWNREAD)   mode |= S_IRUSR;
      if (uarg2 & V1_ST_OWNWRITE)  mode |= S_IWUSR;
      if (uarg2 & V1_ST_WRLDREAD)  mode |= S_IRGRP | S_IROTH;
      if (uarg2 & V1_ST_WRLDWRITE) mode |= S_IWGRP | S_IWOTH;
      i = mkdir(buf, mode);
      break;
    case V1_CHOWN:
      buf = xlate_filename(&dspace[uarg1]);
      uarg2&= 0x3fff;			/* Why are uids > 16384? */
      i = chown(buf, uarg2, 0);
      TrapDebug((dbg_file, " %d on %s return %d",uarg2,buf,i));
      break;
    case V1_CHDIR:
      buf = xlate_filename(&dspace[uarg1]);
      i = chdir(buf);
      break;
    case V1_CREAT:
      buf = xlate_filename(&dspace[uarg1]);
      mode = 0;
      if (uarg2 & V1_ST_SETUID)    mode |= S_ISUID;
      if (uarg2 & V1_ST_EXEC)      mode |= S_IXUSR | S_IXGRP | S_IXOTH;
      if (uarg2 & V1_ST_OWNREAD)   mode |= S_IRUSR;
      if (uarg2 & V1_ST_OWNWRITE)  mode |= S_IWUSR;
      if (uarg2 & V1_ST_WRLDREAD)  mode |= S_IRGRP | S_IROTH;
      if (uarg2 & V1_ST_WRLDWRITE) mode |= S_IWGRP | S_IWOTH;
      i = creat(buf, mode);
      TrapDebug((dbg_file, " on %s return %d ", buf, i));
#ifdef STREAM_BUFFERING
      if (ValidFD(i)) {
	stream[i] = fdopen(i, "w");
	streammode[i] = "w";
      }
#endif
      regs[0] = i; break;
    case V1_EXEC:
      i = v1trap_exec();
      break;
    case V1_WAIT:
      i = wait(&status);
      if (Binary==IS_V1) break;
					/* 2nd Edition wait is different */
      regs[0] = i;			/* Save pid found in r0 */
      if (i==-1) { MQ=0; break; }
      exitval=WEXITSTATUS(status);
      TrapDebug((dbg_file, "exitval %d ",exitval));
      errval=0;
      if (WIFSIGNALED(status)) {
	switch(WTERMSIG(status)) {
	  case SIGBUS: errval=1; break;
	  case SIGTRAP: errval=2; break;
	  case SIGILL: errval=3; break;
	  case SIGIOT: errval=4; break;
	  case SIGEMT: errval=6; break;
	  case SIGQUIT: errval=8; break;
	  case SIGINT: errval=9; break;
	  case SIGKILL: errval=10; break;
	}
	if (WCOREDUMP(status)) errval+=16;
      }
        TrapDebug((dbg_file, "errval %d ",errval));
      MQ= (exitval & 0xff) | (errval<<16);
      TrapDebug((dbg_file, "v2 return pid is %d, MQ is 0x%x ",i,MQ));
      break;
    case V1_FORK:
      pid = getpid();
      i = fork();
      switch (i) {
	  				/* Error, inform the parent */
	case -1: break;
	  				/* Child gets ppid in r0 */
	case 0:
	  i = pid; break;
	  			/* Parent: Skip child `bf', pid into r0 */
	default:
	  regs[PC] += 2;
	  if (Binary==IS_V2) regs[0]=i;
      }
      break;
    case V1_GETUID:
      i = getuid();
      break;
      regs[0] = i;
    case V1_SETUID:
      i = setuid(sarg1);
      break;
    default:
      if (trapnum > V1_ILGINS) {
	fprintf(stderr, "Apout - unknown syscall %d at PC 0%o\n",
							trapnum, regs[PC]);
      } else {
	fprintf(stderr, "Apout - the %s syscall is not yet implemented\n",
							v1trap_name[trapnum]);
      }
      exit(1);
  }

  /* Clear C bit if no error, or */
  /* set C bit as there was an error */

  if (i == -1) {
    SET_CC_C(); TrapDebug((dbg_file, "errno is %s\n", strerror(errno)));
  } else {
    CLR_CC_C(); TrapDebug((dbg_file, "return %d\n", i));
  }
#ifdef DEBUG
  fflush(dbg_file);
#endif
  return;
}


static int v1trap_exec(void)
{
  u_int16_t cptr, cptr2;
  char *buf, *name, *origpath;

  origpath = strdup(&dspace[uarg1]);
  name = xlate_filename(origpath);
  TrapDebug((dbg_file, "%s Execing %s ", progname, name));

  cptr = uarg2;

  Argc = 0;
  while (Argc < MAX_ARGS) {
    ll_word(cptr, cptr2);
    if (cptr2 == 0)
      break;
    buf = &dspace[cptr2];
    Argv[Argc++] = strdup(buf);
    cptr += 2;
    TrapDebug((dbg_file, "%s ", buf));
  }
  Argv[Argc] = NULL;
  TrapDebug((dbg_file, "\n"));

  if (load_a_out(name, origpath, 0) == -1) {
    for (Argc--; Argc >= 0; Argc--)
      free(Argv[Argc]);
    return (-1);
  }
  run();			/* Ok, so it's recursive, I dislike setjmp */
  return (0);
}

/* 1st Edition reads directories as if they were ordinary files.
 * The solution is to read the directory entries, and build a
 * real file, which is passed back to the open call.
 * Limitation: 32-bit inode numbers are truncated to 16-bit ones.
 */
static int v1open_dir(char *name)
{
  DIR *d;
  char *tmpname;
  int i;
  struct dirent *dent;

  struct v1_direct {
    int16_t d_ino;
    int8_t d_name[8];
  } v1dent;

  d = opendir(name);
  if (d == NULL) return (-1);
  tmpname = strdup(TMP_PLATE);
  i = mkstemp(tmpname);
  if (i == -1) {
    fprintf(stderr, "Apout - open_dir couldn't open %s\n", tmpname);
    exit(1);
  }
  unlink(tmpname);
  free(tmpname);

  while ((dent = readdir(d)) != NULL) {
    v1dent.d_ino = dent->d_fileno & 0x7fff;
    if (v1dent.d_ino<41) v1dent.d_ino+=100;
    strncpy(v1dent.d_name, dent->d_name, 8);
    write(i, &v1dent, 10);
  }
  closedir(d);
  lseek(i, 0, SEEK_SET);
  return (i);
}

/* Given a time, work out the number of 1/60ths of seconds since
 * the start of that time's year
 */
u_int32_t sectosixty(time_t tim)
{
  time_t epoch;
  u_int32_t diff;
  struct tm *T;

  T = gmtime(&tim);
  T->tm_sec = T->tm_min = T->tm_hour = T->tm_mon = 0;
  T->tm_mday = 1;

  epoch = timegm(T);		/* Find time at start of year */
  diff = 60 * (tim - epoch);
  if (diff > 0x71172C00) {
    fprintf(stderr, "Apout - V1 sectosixty too big by %d\n",diff-0x71172C00);
  }
  return (diff);
}
#endif				/* EMUV1 */
