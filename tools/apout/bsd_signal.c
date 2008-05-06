/*
 * Much of this file comes from 2.11BSD's /usr/include/signal.h and is
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	Code to deal with 2.11BSD signals
 */
#include "defines.h"
#include <signal.h>
#include "bsdtrap.h"


#define NBSDSIG	32

#define	BSDSIGHUP	1	/* hangup */
#define	BSDSIGINT	2	/* interrupt */
#define	BSDSIGQUIT	3	/* quit */
#define	BSDSIGILL	4	/* illegal instruct (not reset when caught) */
#define	BSDSIGTRAP	5	/* trace trap (not reset when caught) */
#define	BSDSIGIOT	6	/* IOT instruction */
#define	BSDSIGEMT	7	/* EMT instruction */
#define	BSDSIGFPE	8	/* floating point exception */
#define	BSDSIGKILL	9	/* kill (cannot be caught or ignored) */
#define	BSDSIGBUS	10	/* bus error */
#define	BSDSIGSEGV	11	/* segmentation violation */
#define	BSDSIGSYS	12	/* bad argument to system call */
#define	BSDSIGPIPE	13	/* write on a pipe with no one to read it */
#define	BSDSIGALRM	14	/* alarm clock */
#define	BSDSIGTERM	15	/* software termination signal from kill */
#define	BSDSIGURG	16	/* urgent condition on IO channel */
#define	BSDSIGSTOP	17	/* sendable stop signal not from tty */
#define	BSDSIGTSTP	18	/* stop signal from tty */
#define	BSDSIGCONT	19	/* continue a stopped process */
#define	BSDSIGCHLD	20	/* to parent on child stop or exit */
#define	BSDSIGTTIN	21	/* to readers pgrp upon background tty read */
#define	BSDSIGTTOU	22   /* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	BSDSIGIO	23	/* input/output possible signal */
#define	BSDSIGXCPU	24	/* exceeded CPU time limit */
#define	BSDSIGXFSZ	25	/* exceeded file size limit */
#define	BSDSIGVTALRM	26	/* virtual time alarm */
#define	BSDSIGPROF	27	/* profiling time alarm */
#define BSDSIGWINCH	28	/* window size changes */
#define BSDSIGUSR1	30	/* user defined signal 1 */
#define BSDSIGUSR2	31	/* user defined signal 2 */
#define bsdsigismember(set, signo) ((*(set) & (1L << ((signo) - 1))) != 0)

#define	BSDSIG_ERR	-1
#define	BSDSIG_DFL	 0
#define	BSDSIG_IGN	 1

/*
 * Signal vector "template" used in sigaction call.
 */
struct	bsd_sigaction {
	int16_t	sig_handler;		/* signal handler */
	u_int32_t sa_mask;		/* signal mask to apply */
	int16_t	sa_flags;		/* see signal options below */
};

#define BSD_ONSTACK	0x0001	/* take signal on signal stack */
#define BSD_RESTART	0x0002	/* restart system on signal return */
#define	BSD_DISABLE	0x0004	/* disable taking signals on alternate stack */
#define BSD_NOCLDSTOP	0x0008	/* do not generate SIGCHLD on child stop */


/* Translate 2.11BSD signal value to our value */

static int bsdsig[] = {
	0, SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGIOT, SIGEMT,
	SIGFPE, SIGKILL, SIGBUS, SIGSEGV, SIGSYS, SIGPIPE, SIGALRM,
	SIGTERM, SIGURG, SIGSTOP, SIGTSTP, SIGCONT, SIGCHLD, SIGTTIN,
	SIGTTOU, SIGIO, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF, SIGWINCH,
	SIGUSR1, SIGUSR2
};

/* We keep a set of struct sigactions
 * for each 2.11BSD signal
 */
struct bsd_sigaction Sigact[NBSDSIG];


/* Set all signals to their default value */
void set_bsdsig_dfl(void)
{
  int i;

  for (i=0;i<NBSDSIG;i++) {
	if (bsdsig[i]) signal(bsdsig[i], SIG_DFL);
	Sigact[i].sig_handler= BSDSIG_DFL;
	Sigact[i].sa_mask= Sigact[i].sa_flags= 0;
  }
}

int do_sigaction(int sig, int a, int oa)
{
  int i;
  struct bsd_sigaction *act, *oact;
  struct sigaction ouraction;

  if ((sig<0) || (sig >= NBSDSIG)) return(EINVAL);

  if (oa) {
    oact= (struct bsd_sigaction *)&dspace[oa];
    memcpy(oact, &Sigact[sig], sizeof(struct bsd_sigaction));
  }

  if (a) {
    act= (struct bsd_sigaction *)&dspace[a];

		/* If required, map mask here */
		/* Currently, the assumption is a 1-1 match */
    sigemptyset(&(ouraction.sa_mask));
    for (i=1; i<NBSDSIG;i++) {
	if bsdsigismember(&(act->sa_mask), i)
		sigaddset(&(ouraction.sa_mask), i);
    }
		/* If required, map flags here */
    ouraction.sa_flags= act->sa_flags;
    ouraction.sa_handler= sigcatcher;
  }

  i= sigaction(bsdsig[sig], &ouraction, NULL);
  if (i==-1) return(i);

		/* Else save the new sigaction */
  act= (struct bsd_sigaction *)&dspace[a];
  memcpy(&Sigact[sig], act, sizeof(struct bsd_sigaction));
  return(i);
}



















/* For now, the rest commented out. One day I might
 * get around to implementing 2.11BSD signals properly
 */

#if 0
int	(*signal())();

typedef unsigned long sigset_t;


/*
 * Flags for sigprocmask:
 */
#define	BSDSIG_BLOCK	1	/* block specified signal set */
#define	BSDSIG_UNBLOCK	2	/* unblock specified signal set */
#define	BSDSIG_SETMASK	3	/* set specified signal set */

typedef	int (*sig_t)();		/* type of signal function */

/*
 * Structure used in sigaltstack call.
 */
struct	sigaltstack {
	char	*ss_base;		/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SA_DISABLE and/or SA_ONSTACK */
};
#define	MINBSDSIGSTKSZ	128			/* minimum allowable stack */
#define	BSDSIGSTKSZ	(MINBSDSIGSTKSZ + 384)	/* recommended stack size */

/*
 * 4.3 compatibility:
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	int	(*sv_handler)();	/* signal handler */
	long	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};
#define SV_ONSTACK	SA_ONSTACK	/* take signal on signal stack */
#define SV_INTERRUPT	SA_RESTART	/* same bit, opposite sense */
#define sv_onstack sv_flags		/* isn't compatibility wonderful! */

/*
 * 4.3 compatibility:
 * Structure used in sigstack call.
 */
struct	sigstack {
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 */
struct	sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	long	sc_mask;		/* signal mask to restore */
	int	sc_sp;			/* sp to restore */
	int	sc_fp;			/* fp to restore */
	int	sc_r1;			/* r1 to restore */
	int	sc_r0;			/* r0 to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_ps;			/* psl to restore */
	int	sc_ovno			/* overlay to restore */
};

/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(m)		(1L << ((m)-1))
#define sigaddset(set, signo)	(*(set) |= 1L << ((signo) - 1), 0)
#define sigdelset(set, signo)	(*(set) &= ~(1L << ((signo) - 1)), 0)
#define sigemptyset(set)	(*(set) = (sigset_t)0, (int)0)
#define sigfillset(set)         (*(set) = ~(sigset_t)0, (int)0)
#define sigismember(set, signo) ((*(set) & (1L << ((signo) - 1))) != 0)

#endif /* 0 */
