/* magic.c - determine the environment for certain PDP-11 a.out binaries
 *
 * Some binaries in V1, V2, V5, V6, V7 and 2.11BSD are not caught with the
 * magic numbers in aout.c. If this is the case, we fall into the
 * special_magic() function, which calculates a checksum on the
 * a.out header. If it matches any of the checksums below, it returns
 * the appropriate environment value. Otherwise, it returns IS_UNKNOWN.
 *
 * $Revision: 1.13 $
 * $Date: 2000/01/10 01:31:48 $
 */
#include "defines.h"

struct spec_aout {
	u_int32_t cksum;
	int environment;
};

static struct spec_aout S[]= {
	{ 0x1042c2, IS_V6 },	/* V6 bin/dc */
	{ 0x10f02, IS_V5 },	/* V5 etc/update */
	{ 0x11002, IS_V5 },	/* V5 bin/clri */
	{ 0x1117c2, IS_V7 },	/* V7 bin/roff */
	{ 0x11702, IS_V6 },	/* V6 etc/update */
	{ 0x11a82, IS_V5 },	/* V5 bin/sum */
	{ 0x1319c2, IS_V5 },	/* V5 usr/fort/fc1 */
	{ 0x1332c2, IS_V2 },	/* /lib/c0 dated Jun 30 1973 from s2 tape */
	{ 0x13642, IS_V5 },	/* V5 bin/rew */
	{ 0x139e02, IS_V5 },	/* V5 bin/dc */
	{ 0x13c0, IS_V6 },	/* V6 usr/lib/tmgc */
	{ 0x14042, IS_V6 },	/* V6 bin/tty */
	{ 0x143c2, IS_V5 },	/* V5 bin/tty */
	{ 0x152ac2, IS_V6 },	/* V6 usr/lib/tmg */
	{ 0x15f42, IS_V5 },	/* V5 bin/kill */
	{ 0x16802, IS_V5 },	/* V5 bin/dsw */
	{ 0x16902, IS_V5 },	/* V5 bin/mkdir */
	{ 0x1720c2, IS_V6 },	/* V6 bin/cdb */
	{ 0x17742, IS_V5 },	/* V5 usr/bin/pfe */
	{ 0x17cc2, IS_V5 },	/* V5 usr/bin/mesg */
	{ 0x18702, IS_V5 },	/* V5 bin/rmdir */
	{ 0x194c2, IS_V6 },	/* V6 bin/chgrp */
	{ 0x197c2, IS_V6 },	/* V6 bin/chown */
	{ 0x19a42, IS_V5 },	/* V5 bin/chown */
	{ 0x19b342, IS_V6 },	/* V6 usr/bin/nroff */
	{ 0x19f682, IS_V6 },	/* V6 usr/fort/fc1 */
	{ 0x1b102, IS_V5 },	/* V5 bin/strip */
	{ 0x1ba02, IS_V6 },	/* V6 bin/strip */
	{ 0x1c342, IS_V5 },	/* V5 bin/cat */
	{ 0x1c8442, IS_V7 },	/* V7 usr/games/maze */
	{ 0x1cc782, IS_V6 },	/* V6 lib/fc0 */
	{ 0x1dfc2, IS_V5 },	/* V5 etc/getty */
	{ 0x1f9c2, IS_V2 },	/* /bin/nm dated Jun 30 1973 from s2 tape */
	{ 0x20202, IS_V5 },	/* V5 usr/games/bj */
	{ 0x21e42, IS_V6 },	/* V6 usr/bin/units */
	{ 0x23f82, IS_V5 },	/* V5 usr/bin/passwd */
	{ 0x260642, IS_V6 },	/* V6 lib/fc1 */
	{ 0x262a82, IS_211BSD }, /* 2.11 usr/new/m11 */
	{ 0x27e82, IS_V5 },	/* V5 usr/bin/grep */
	{ 0x290c2, IS_V7 },	/* V7 usr/games/cubic */
	{ 0x299c2, IS_V5 },	/* V5 usr/games/cubic */
	{ 0x2f482, IS_V5 },	/* V5 usr/bin/form */
	{ 0x3382, IS_V6 },	/* V6 bin/write */
	{ 0x326642, IS_V7 },	/* 2.9 awk */
	{ 0x33c42, IS_211BSD },	/* 2.11 usr/games/moo */
	{ 0x351382, IS_211BSD }, /* 2.11 usr/games/lib/zork */
	{ 0x3702, IS_V5 },	/* V5 usr/games/moo */
	{ 0x3b402, IS_V5 },	/* V5 bin/ar */
	{ 0x3cc02, IS_V2 },	/* /bin/size from from s2 tape */
	{ 0x4382, IS_V5 },	/* V5 bin/write */
	{ 0x451f42, IS_V7 },	/* 2.9 /lib/c1 */
	{ 0x47042, IS_211BSD },	/* 2.11 usr/games/ttt */
	{ 0x4fa02, IS_V5 },	/* V5 bin/ld */
	{ 0x51342, IS_211BSD },	/* 2.11 usr/games/bj */
	{ 0x53302, IS_V6 },	/* V6 usr/lib/suftab */
	{ 0x55882, IS_V7 },	/* 2.9 /bin/as */
	{ 0x54702, IS_V5 },	/* V5 usr/games/ttt */
	{ 0x55702, IS_V7 },	/* V7 bin/as */
	{ 0x5c342, IS_V2 },    /* /bin/cc dated Jun 30 1973 from s2 tape */
	{ 0x6f742, IS_V6 },	/* V6 usr/bin/sa */
	{ 0x7042, IS_V7 },	/* V7 bin/factor */
	{ 0x71702, IS_V7 },	/* V7 lib/as2 */
	{ 0x7342, IS_V5 },	/* V5 bin/du */
	{ 0x73782, IS_V7}, 	/* 2.9 /lib/as2 */
	{ 0x73e00, IS_V2 },	/* /bin/ld from s2 tape */
	{ 0x7a242, IS_V6 },	/* V6 lib/as2 */
	{ 0x7b102, IS_V6 },	/* V6 bin/as */
	{ 0x7d082, IS_V5 },	/* V5 bin/as */
	{ 0x7d6844, IS_V1 },	/* bin/cal from s2 tape */
	{ 0x7d942, IS_V5 },	/* V5 lib/as2 */
	{ 0x8002, IS_V5 },	/* V5 etc/lpd */
	{ 0x85842, IS_V5 },	/* V5 bin/ed */
	{ 0x8f00, IS_V6 },	/* V6 usr/lib/tmga */
	{ 0x915c2, IS_V6 },	/* V6 bin/bas */
	{ 0x94542, IS_V5 },	/* V5 bin/db */
	{ 0x98442, IS_V6 },	/* V6 usr/bin/ac */
	{ 0x9adc2, IS_V6 },	/* V6 bin/db */
	{ 0xa242, IS_V7 },	/* V7 bin/primes */
	{ 0xa4602, IS_V2 },	/* /bin/as from s2 tape */
	{ 0xa702, IS_V5 },	/* V5 bin/time */
	{ 0xad882, IS_V7 },	/* V7 bin/bas */
	{ 0xadc42, IS_V2 },	/* /usr/lib/c1 from s2 tape */
	{ 0xb5a82, IS_V6 },	/* V6 usr/bin/prof */
	{ 0xc1e42, IS_V5 },	/* V5 usr/bin/fed */
	{ 0xc3102, IS_V6 },	/* V6 bin/tp */
	{ 0xc8bc2, IS_V5 },	/* V5 bin/tp */
	{ 0xe1642, IS_V6 },	/* V6 usr/bin/roff */
	{ 0xe1f42, IS_V5 },	/* V5 usr/bin/roff */
	{ 0xec582, IS_V5 },	/* V5 bin/bas */
	{ 0xfc2, IS_V6 },	/* V6 usr/bin/typo */
	{ 0xfc002, IS_V2 },	/* /bin/as dated Jun 30 1973 from s2 tape */
	{ 0x38ec0, IS_V5 },	/* V5 bin/ar, Warrens */
	{ 0, 0 }
};

/* cptr points at the start of the a.out header */
int special_magic(u_int16_t *cptr)
{
    u_int32_t cksum=0;
    int i;

    if (cptr==NULL) return(IS_UNKNOWN);
				/* Calculate the checksum */
    for (i=0;i<8; i++) { cksum ^= cptr[i]; cksum = cksum<<1; }

				/* Try and find a match */
    for (i=0; S[i].cksum!=0; i++) if (S[i].cksum==cksum) {
      TrapDebug((dbg_file, "This a.out has special magic %d\n",i));
      return(S[i].environment);
    }

   				/* None, return 0 */
    (void)printf("Apout - unknown magic in header: 0x%x\n",cksum);
    return(IS_UNKNOWN);
}
