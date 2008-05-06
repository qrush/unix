/*
 * bsd_ioctl.c - Deal with 2.11BSD ioctl system calls.
 *
 * $Revision: 1.13 $ $Date: 1999/12/27 04:38:29 $
 */
#ifdef EMU211
#include "defines.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "bsdtrap.h"
#ifdef __linux__
#include <linux/sockios.h>	/* FIOSETOWN */
#endif

/* Structures and defines required by this file */

/* First the list of ioctls handled so far */
#define TR_FIOCLEX	0x20006601	/* 0x2 is void */
#define TR_TIOCGETP	0x40067408	/* 0x4 is a read */
#define TR_TIOCSETP	0x40067409	/* 0x8 is a write */
#define	TR_TIOCSETN	0x8006740a
#define	TR_TIOCSETC	0x80067411
#define TR_TIOCGETD	0x40027400
#define TR_TIOCSETD	0x80027401
#define TR_TIOCGETC	0x40067412
#define TR_TIOCGLTC	0x40067474
#define TR_TIOCSLTC	0x80067475
#define TR_TIOCGWINSZ	0x40087468
#define TR_TIOCSWINSZ	0x40027467
#define TR_FIOSETOWN	0x8002667b
#define TR_TIOCMGET	0x4002746a
#define TR_TIOCGPGRP	0x40027477
#define TR_TIOCSPGRP	0x80027476

/* sgtty structure */
struct tr_sgttyb {
  int8_t sg_ispeed;		/* input speed */
  int8_t sg_ospeed;		/* output speed */
  int8_t sg_erase;		/* erase character */
  int8_t sg_kill;		/* kill character */
  int16_t sg_flags;		/* mode flags */
};

struct tr_tchars {
        int8_t    t_intrc;        /* interrupt */
        int8_t    t_quitc;        /* quit */
        int8_t    t_startc;       /* start output */
        int8_t    t_stopc;        /* stop output */
        int8_t    t_eofc;         /* end-of-file */
        int8_t    t_brkc;         /* input delimiter (like nl) */
};

struct tr_ltchars {
        int8_t    t_suspc;        /* stop process signal */
        int8_t    t_dsuspc;       /* delayed stop process signal */
        int8_t    t_rprntc;       /* reprint line */
        int8_t    t_flushc;       /* flush output (toggles) */
        int8_t    t_werasc;       /* word erase */
        int8_t    t_lnextc;       /* literal next character */
};

/*
 * Values for sg_flags
 */
#define      TR_TANDEM          0x00000001	/* send stopc on out q full */
#define      TR_CBREAK          0x00000002	/* half-cooked mode */
						/* 0x4 (old LCASE) */
#define      TR_ECHO            0x00000008	/* echo input */
#define      TR_CRMOD           0x00000010	/* map \r to \r\n on output */
#define      TR_RAW             0x00000020	/* no i/o processing */
#define      TR_ODDP            0x00000040	/* get/send odd parity */
#define      TR_EVENP           0x00000080	/* get/send even parity */
#define      TR_ANYP            0x000000c0	/* get any parity/send none */
						/* 0x100 (old NLDELAY) */
						/* 0x200 */
#define      TR_XTABS           0x00000400	/* expand tabs on output */

/* Values for sg_ispeed and sg_ospeed */
#define TR_B0      0
#define TR_B50     1
#define TR_B75     2
#define TR_B110    3
#define TR_B134    4
#define TR_B150    5
#define TR_B200    6
#define TR_B300    7
#define TR_B600    8
#define TR_B1200   9
#define TR_B1800   10
#define TR_B2400   11
#define TR_B4800   12
#define TR_B9600   13
#define TR_EXTA    14
#define TR_EXTB    15



/* Variables, functions and the actual code of this file */

extern arglist *A;		/* Pointer to various arguments on stack */

/* Forward prototypes */
#ifdef __STDC__
#define P(s) s
#else
#define P(s) ()
#endif

static int trap_gettermios(u_int16_t fd, u_int32_t type, u_int16_t ucnt);
static int trap_settermios(u_int16_t fd, u_int32_t type, u_int16_t ucnt);

#undef P

int trap_ioctl()
{
  int i, val;
  long larg1;
#ifdef DEBUG
  u_int8_t size, letter, value;
#endif
  int16_t *shortptr;
  struct winsize *winptr;

  larg1 = (sarg2 << 16) | uarg3;

#ifdef DEBUG
  if (trap_debug) {
    fprintf(dbg_file, "val %d ",uarg1);
    switch (larg1) {
      case TR_FIOCLEX:	  fprintf(dbg_file, "FIOCLEX "); break;
      case TR_TIOCGETP:	  fprintf(dbg_file, "TIOCGETP "); break;
      case TR_TIOCSETP:	  fprintf(dbg_file, "TIOCSETP "); break;
      case TR_TIOCSETN:	  fprintf(dbg_file, "TIOCSETN "); break;
      case TR_TIOCSETC:	  fprintf(dbg_file, "TIOCSETC "); break;
      case TR_TIOCGETD:	  fprintf(dbg_file, "TIOCGETD "); break;
      case TR_TIOCSETD:	  fprintf(dbg_file, "TIOCSETD "); break;
      case TR_TIOCGETC:	  fprintf(dbg_file, "TIOCGETC "); break;
      case TR_TIOCGLTC:	  fprintf(dbg_file, "TIOCGLTC "); break;
      case TR_TIOCSLTC:	  fprintf(dbg_file, "TIOCSLTC "); break;
      case TR_TIOCGWINSZ: fprintf(dbg_file, "TIOCGWINSZ "); break;
      case TR_TIOCSWINSZ: fprintf(dbg_file, "TIOCSWINSZ "); break;
      case TR_FIOSETOWN:  fprintf(dbg_file, "FIOSETOWN "); break;
      case TR_TIOCMGET:   fprintf(dbg_file, "TIOCMGET "); break;
      case TR_TIOCGPGRP:  fprintf(dbg_file, "TIOCGPGRP "); break;
      case TR_TIOCSPGRP:  fprintf(dbg_file, "TIOCSPGRP "); break;
      default: fprintf(dbg_file, "0x%lx ", larg1);

    	value =  larg1 & 0xff;
    	letter = (larg1 >> 8) & 0xff;
    	size =   (larg1 >> 16) & 0xff;

    	fprintf(dbg_file, "('%c' ", letter);
    	fprintf(dbg_file, "size %d ", size);
    	fprintf(dbg_file, "val %d) ", value);
    }
    if (size) fprintf(dbg_file, "ptr %d ", uarg4);
  }
#endif

  switch (larg1) {

    case TR_TIOCGETP:
    case TR_TIOCGETC:
    case TR_TIOCGLTC:
      i = trap_gettermios(uarg1, larg1, uarg4); break;

    case TR_TIOCSETP:
    case TR_TIOCSETN:
    case TR_TIOCSETC:
    case TR_TIOCSLTC:
      i = trap_settermios(uarg1, larg1, uarg4); break;

    case TR_FIOCLEX:
      i = ioctl(uarg1, FIOCLEX, NULL); break;
    case TR_TIOCGETD:
      shortptr = (int16_t *) &dspace[uarg4];
      i = ioctl(uarg1, TIOCGETD, &val);
      if (i==0) *shortptr= val;
      break;
    case TR_TIOCGPGRP:
      shortptr = (int16_t *) &dspace[uarg4];
      i = ioctl(uarg1, TIOCGPGRP, &val);
      if (i==0) *shortptr= val;
      break;
    case TR_TIOCSPGRP:
      shortptr = (int16_t *) &dspace[uarg4];
      val= *shortptr; i = ioctl(uarg1, TIOCSPGRP, &val); break;
    case TR_TIOCSETD:
      shortptr = (int16_t *) &dspace[uarg4];
      val= *shortptr; i = ioctl(uarg1, TIOCSETD, &val); break;
    case TR_FIOSETOWN:
      shortptr = (int16_t *) &dspace[uarg4];
      val= *shortptr;
				/* Something wrong here, wonder why! */
      TrapDebug((dbg_file, "fd %d val %d ",uarg1,val));
      i = ioctl(uarg1, FIOSETOWN, &val); break;
    case TR_TIOCMGET:
      shortptr = (int16_t *) &dspace[uarg4];
      i = ioctl(uarg1, TIOCMGET, &val);
      if (i==0) *shortptr= val;
      break;

    case TR_TIOCGWINSZ:
				/* 2.11BSD and POSIX winsize the same! */
      winptr = (struct winsize *) &dspace[uarg4];
      i= ioctl(uarg1, TIOCGWINSZ, winptr); break;
    case TR_TIOCSWINSZ:
      winptr = (struct winsize *) &dspace[uarg4];
      i= ioctl(uarg1, TIOCSWINSZ, winptr); break;

    default:
      i = 0;
  }
  return (i);
}


/* Convert from termios to old sgttyb structure */
static void to_sgttyb(struct tr_sgttyb * sgtb, struct termios * tios)
{
  switch (tios->c_ispeed) {
     case B0: sgtb->sg_ispeed= TR_B0; break;
     case B50: sgtb->sg_ispeed= TR_B50; break;
     case B75: sgtb->sg_ispeed= TR_B75; break;
     case B110: sgtb->sg_ispeed= TR_B110; break;
     case B134: sgtb->sg_ispeed= TR_B134; break;
     case B150: sgtb->sg_ispeed= TR_B150; break;
     case B200: sgtb->sg_ispeed= TR_B200; break;
     case B300: sgtb->sg_ispeed= TR_B300; break;
     case B600: sgtb->sg_ispeed= TR_B600; break;
     case B1200: sgtb->sg_ispeed= TR_B1200; break;
     case B1800: sgtb->sg_ispeed= TR_B1800; break;
     case B2400: sgtb->sg_ispeed= TR_B2400; break;
     case B4800: sgtb->sg_ispeed= TR_B4800; break;
     case B9600: sgtb->sg_ispeed= TR_B9600; break;
     case B19200: sgtb->sg_ispeed= TR_EXTA; break;
     case B38400: sgtb->sg_ispeed= TR_EXTB; break;
     default: sgtb->sg_ispeed= TR_B0; break;
  }
  switch (tios->c_ospeed) {
     case B0: sgtb->sg_ospeed= TR_B0; break;
     case B50: sgtb->sg_ospeed= TR_B50; break;
     case B75: sgtb->sg_ospeed= TR_B75; break;
     case B110: sgtb->sg_ospeed= TR_B110; break;
     case B134: sgtb->sg_ospeed= TR_B134; break;
     case B150: sgtb->sg_ospeed= TR_B150; break;
     case B200: sgtb->sg_ospeed= TR_B200; break;
     case B300: sgtb->sg_ospeed= TR_B300; break;
     case B600: sgtb->sg_ospeed= TR_B600; break;
     case B1200: sgtb->sg_ospeed= TR_B1200; break;
     case B1800: sgtb->sg_ospeed= TR_B1800; break;
     case B2400: sgtb->sg_ospeed= TR_B2400; break;
     case B4800: sgtb->sg_ospeed= TR_B4800; break;
     case B9600: sgtb->sg_ospeed= TR_B9600; break;
     case B19200: sgtb->sg_ospeed= TR_EXTA; break;
     case B38400: sgtb->sg_ospeed= TR_EXTB; break;
     default: sgtb->sg_ospeed= TR_B0; break;
  }
  sgtb->sg_erase = tios->c_cc[VERASE];
  sgtb->sg_kill = tios->c_cc[VKILL];
  sgtb->sg_flags = 0;
  if (tios->c_oflag & OXTABS)
    sgtb->sg_flags |= TR_XTABS;
  if (tios->c_cflag & PARENB) {
    if (tios->c_cflag & PARODD)
      sgtb->sg_flags |= TR_ODDP;
    else
      sgtb->sg_flags |= TR_EVENP;
  } else
    sgtb->sg_flags |= TR_ANYP;
  if (tios->c_oflag & ONLCR)
    sgtb->sg_flags |= TR_CRMOD;
  if (tios->c_lflag & ECHO)
    sgtb->sg_flags |= TR_ECHO;
  if (!(tios->c_lflag & ICANON)) {
    if (!(tios->c_lflag & ECHO))
      sgtb->sg_flags |= TR_CBREAK;
    else
      sgtb->sg_flags |= TR_RAW;
  }
}

/* Convert from old sgttyb to termios structure */
static void to_termios(struct tr_sgttyb * sgtb, struct termios * tios)
{
  TrapDebug((dbg_file, "\n\tto_termios: sgtty flags are 0x%x ",
							sgtb->sg_flags));

  switch (sgtb->sg_ispeed) {
     case TR_B0: tios->c_ispeed= B0; break;
     case TR_B50: tios->c_ispeed= B50; break;
     case TR_B75: tios->c_ispeed= B75; break;
     case TR_B110: tios->c_ispeed= B110; break;
     case TR_B134: tios->c_ispeed= B134; break;
     case TR_B150: tios->c_ispeed= B150; break;
     case TR_B200: tios->c_ispeed= B200; break;
     case TR_B300: tios->c_ispeed= B300; break;
     case TR_B600: tios->c_ispeed= B600; break;
     case TR_B1200: tios->c_ispeed= B1200; break;
     case TR_B1800: tios->c_ispeed= B1800; break;
     case TR_B2400: tios->c_ispeed= B2400; break;
     case TR_B4800: tios->c_ispeed= B4800; break;
     case TR_B9600: tios->c_ispeed= B9600; break;
     case TR_EXTA: tios->c_ispeed= B19200; break;
     case TR_EXTB: tios->c_ispeed= B38400; break;
     default: tios->c_ispeed= B0; break;
  }
  switch (sgtb->sg_ospeed) {
     case TR_B0: tios->c_ospeed= B0; break;
     case TR_B50: tios->c_ospeed= B50; break;
     case TR_B75: tios->c_ospeed= B75; break;
     case TR_B110: tios->c_ospeed= B110; break;
     case TR_B134: tios->c_ospeed= B134; break;
     case TR_B150: tios->c_ospeed= B150; break;
     case TR_B200: tios->c_ospeed= B200; break;
     case TR_B300: tios->c_ospeed= B300; break;
     case TR_B600: tios->c_ospeed= B600; break;
     case TR_B1200: tios->c_ospeed= B1200; break;
     case TR_B1800: tios->c_ospeed= B1800; break;
     case TR_B2400: tios->c_ospeed= B2400; break;
     case TR_B4800: tios->c_ospeed= B4800; break;
     case TR_B9600: tios->c_ospeed= B9600; break;
     case TR_EXTA: tios->c_ospeed= B19200; break;
     case TR_EXTB: tios->c_ospeed= B38400; break;
     default: tios->c_ospeed= B0; break;
  }
  tios->c_cc[VERASE] = sgtb->sg_erase;
  tios->c_cc[VKILL] = sgtb->sg_kill;

				/* Initially turn off any flags we might set */
  tios->c_oflag &= ~(OXTABS|ONLCR);
  tios->c_cflag &= ~(PARENB|PARODD);
  tios->c_lflag &= ~(ECHO);

  if (sgtb->sg_flags & TR_XTABS)
    tios->c_oflag |= OXTABS;
  if (sgtb->sg_flags & TR_ODDP) {
    tios->c_cflag |= PARENB;
    tios->c_cflag &= ~PARODD;
  }
  if (sgtb->sg_flags & TR_EVENP)
    tios->c_cflag |= PARENB | PARODD;
  if (sgtb->sg_flags & TR_ANYP)
    tios->c_cflag &= ~PARENB;
  if (sgtb->sg_flags & TR_CRMOD)
    tios->c_oflag |= ONLCR;
  if (sgtb->sg_flags & TR_ECHO)
    tios->c_lflag |= ECHO;

  if (sgtb->sg_flags & TR_RAW) {
    tios->c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG|BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    tios->c_cflag &= ~(CSIZE|PARENB);
    tios->c_cflag |= CS8;
    tios->c_oflag &= ~(OPOST);
    tios->c_cc[VMIN] = 1;
    tios->c_cc[VTIME] = 0;
  }

  if (sgtb->sg_flags & TR_CBREAK) {
  TrapDebug((dbg_file, "\n\tto_termios: setting cbreak I hope "));
    tios->c_lflag &= ~(ECHO|ICANON);
    tios->c_cc[VMIN] = 1;
    tios->c_cc[VTIME] = 0;
  }
  TrapDebug((dbg_file, "\n\tto_termios: iflag is 0x%x", (int)tios->c_iflag));
  TrapDebug((dbg_file, "\n\tto_termios: oflag is 0x%x", (int)tios->c_oflag));
  TrapDebug((dbg_file, "\n\tto_termios: lflag is 0x%x ", (int)tios->c_lflag));
}

/* Convert from termios to old [l]tchars structures */
static void to_tchars(struct tr_tchars *tc, struct tr_ltchars *ltc,
						struct termios * tios)
{
  if (tc) {
        tc->t_intrc=tios->c_cc[VINTR];
        tc->t_quitc=tios->c_cc[VQUIT];
        tc->t_startc=tios->c_cc[VSTART];
        tc->t_stopc=tios->c_cc[VSTOP];
        tc->t_eofc=tios->c_cc[VEOF];
        tc->t_brkc=tios->c_cc[VEOL];
  }
  if (ltc) {
        ltc->t_suspc=tios->c_cc[VSUSP];
        ltc->t_dsuspc=tios->c_cc[VDSUSP];
        ltc->t_rprntc=tios->c_cc[VREPRINT];
        ltc->t_flushc=tios->c_cc[VDISCARD];
        ltc->t_werasc=tios->c_cc[VERASE];
        ltc->t_lnextc=tios->c_cc[VLNEXT];
  }
}

/* Convert from old [l]tchars to termios structures */
static void tc_to_tchars(struct tr_tchars *tc, struct tr_ltchars *ltc,
						struct termios * tios)
{
  if (tc) {
	tios->c_cc[VINTR]= tc->t_intrc;
	tios->c_cc[VQUIT]= tc->t_quitc;
	tios->c_cc[VSTART]= tc->t_startc;
	tios->c_cc[VSTOP]= tc->t_stopc;
	tios->c_cc[VEOF]= tc->t_eofc;
	tios->c_cc[VEOL]= tc->t_brkc;
  }
  if (ltc) {
	tios->c_cc[VSUSP]= ltc->t_suspc;
	tios->c_cc[VDSUSP]= ltc->t_dsuspc;
	tios->c_cc[VREPRINT]= ltc->t_rprntc;
	tios->c_cc[VDISCARD]= ltc->t_flushc;
	tios->c_cc[VERASE]= ltc->t_werasc;
	tios->c_cc[VLNEXT]= ltc->t_lnextc;
  }
}
/* Handle most get ioctls that deal with termios stuff */
static int trap_gettermios(u_int16_t fd, u_int32_t type, u_int16_t ucnt)
{
  struct termios tios;
  struct tr_sgttyb *sgtb;
  struct tr_tchars *tc;
  struct tr_ltchars *ltc;
  int i;

  if (ucnt == 0) return -1;
  i = tcgetattr(fd, &tios);
  if (i == -1) return i;
  CLR_CC_C();

  switch (type) {
    case TR_TIOCGETP:
      sgtb = (struct tr_sgttyb *) &dspace[ucnt];
      to_sgttyb(sgtb, &tios); return 0;
    case TR_TIOCGETC:
      tc = (struct tr_tchars *) &dspace[ucnt];
      to_tchars(tc, NULL, &tios); return 0;
    case TR_TIOCGLTC:
      ltc = (struct tr_ltchars *) &dspace[ucnt];
      to_tchars(NULL, ltc, &tios); return 0;
  }
  /* Unknown type, should never get here */
  return -1;
}

/* Handle most set ioctls that deal with termios stuff */
static int trap_settermios(u_int16_t fd, u_int32_t type, u_int16_t ucnt)
{
  struct termios tios;
  struct tr_sgttyb *sgtb;
  struct tr_tchars *tc;
  struct tr_ltchars *ltc;
  int i;

  if (ucnt == 0) return -1;
  i = tcgetattr(fd, &tios);
  if (i == -1) return i;
  switch (type) {
    case TR_TIOCSETP:
      sgtb = (struct tr_sgttyb *) & dspace[ucnt];
      to_termios(sgtb, &tios);
      i = tcsetattr(fd, TCSANOW, &tios);
      return (i);
    case TR_TIOCSETN:
      sgtb = (struct tr_sgttyb *) & dspace[ucnt];
      to_termios(sgtb, &tios);
      i = tcsetattr(fd, TCSADRAIN, &tios);
      return (i);
    case TR_TIOCSETC:
      tc = (struct tr_tchars *) & dspace[ucnt];
      tc_to_tchars(tc, NULL, &tios);
      i = tcsetattr(fd, TCSANOW, &tios);
      return (i);
    case TR_TIOCSLTC:
      ltc = (struct tr_ltchars *) & dspace[ucnt];
      tc_to_tchars(NULL, ltc, &tios);
      i = tcsetattr(fd, TCSANOW, &tios);
      return (i);
  }
  /* Unknown type, should never get here */
  return -1;
}
#endif				/* EMU211 */
