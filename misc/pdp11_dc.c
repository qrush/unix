/* pdp11_dl.c: PDP-11 multiple terminal interface simulator

   Copyright (c) 1993-2006, Robert M Supnik

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior wridcen authorization from Robert M Supnik.

   dcix,dcox    DL11 terminal input/output
*/


#if defined (VM_PDP10)                                  /* PDP10 version */
#error "DC11 is not supported on the PDP-10!"

#elif defined (VM_VAX)                                  /* VAX version */
#error "DC11 is not supported on the VAX!"

#else                                                   /* PDP-11 version */
#include "pdp11_defs.h"
#endif
#include "sim_sock.h"
#include "sim_tmxr.h"

#define DCX_MASK        (DCX_LINES - 1)

#define DCIXCSR_IMP     (CSR_DONE + CSR_IE)             /* terminal input */
#define DCIXCSR_RW      (CSR_IE)
#define DCIXCSR_CD	004				/* carrier detect */
#define DCIXBUF_ERR     0100000
#define DCIXBUF_OVR     0040000
#define DCIXBUF_RBRK    0020000
#define DCOXCSR_IMP     (CSR_DONE + CSR_IE)             /* terminal output */
#define DCOXCSR_RW      (CSR_IE)

extern int32 int_req[IPL_HLVL];
extern int32 tmxr_poll;

uint16 dcix_csr[DCX_LINES] = { 0 };                     /* control/status */
uint16 dcix_buf[DCX_LINES] = { 0 };
uint32 dcix_ireq = 0;
uint16 dcox_csr[DCX_LINES] = { 0 };                     /* control/status */
uint8 dcox_buf[DCX_LINES] = { 0 };
uint32 dcox_ireq = 0;
TMLN dcx_ldsc[DCX_LINES] = { 0 };                       /* line descriptors */
TMXR dcx_desc = { DCX_LINES, 0, 0, dcx_ldsc };          /* mux descriptor */

t_stat dcx_rd (int32 *data, int32 PA, int32 access);
t_stat dcx_wr (int32 data, int32 PA, int32 access);
t_stat dcx_reset (DEVICE *dptr);
t_stat dcix_svc (UNIT *uptr);
t_stat dcox_svc (UNIT *uptr);
t_stat dcx_adcach (UNIT *uptr, char *cptr);
t_stat dcx_detach (UNIT *uptr);
t_stat dcx_summ (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat dcx_show (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat dcx_show_vec (FILE *st, UNIT *uptr, int32 val, void *desc);
t_stat dcx_set_lines (UNIT *uptr, int32 val, char *cptr, void *desc);
t_stat dcx_show_lines (FILE *st, UNIT *uptr, int32 val, void *desc);
void dcx_enbdis (int32 dis);
void dcix_clr_int (uint32 ln);
void dcix_set_int (int32 ln);
int32 dcix_iack (void);
void dcox_clr_int (int32 ln);
void dcox_set_int (int32 ln);
int32 dcox_iack (void);
void dcx_reset_ln (uint32 ln);

/* DCIX data structures

   dcix_dev      DCIX device descriptor
   dcix_unit     DCIX unit descriptor
   dcix_reg      DCIX register list
*/

DIB dcix_dib = {
    IOBA_DCIX, IOLN_DCIX, &dcx_rd, &dcx_wr,
    2, IVCL (DCIX), VEC_DCIX, { &dcix_iack, &dcox_iack }
    };

UNIT dcix_unit = { UDATA (&dcix_svc, 0, 0), KBD_POLL_WAIT };

REG dcix_reg[] = {
    { BRDATA (BUF, dcix_buf, DEV_RDX, 16, DCX_LINES) },
    { BRDATA (CSR, dcix_csr, DEV_RDX, 16, DCX_LINES) },
    { GRDATA (IREQ, dcix_ireq, DEV_RDX, DCX_LINES, 0) },
    { DRDATA (LINES, dcx_desc.lines, 6), REG_HRO },
    { GRDATA (DEVADDR, dcix_dib.ba, DEV_RDX, 32, 0), REG_HRO },
    { GRDATA (DEVVEC, dcix_dib.vec, DEV_RDX, 16, 0), REG_HRO },
    { NULL }
    };

MTAB dcix_mod[] = {
    { UNIT_ATT, UNIT_ATT, "summary", NULL, NULL, &dcx_summ },
    { MTAB_XTD | MTAB_VDV, 1, NULL, "DISCONNECT",
      &tmxr_dscln, NULL, &dcx_desc },
    { MTAB_XTD | MTAB_VDV | MTAB_NMO, 1, "CONNECTIONS", NULL,
      NULL, &dcx_show, NULL },
    { MTAB_XTD | MTAB_VDV | MTAB_NMO, 0, "STATISTICS", NULL,
      NULL, &dcx_show, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "ADDRESS", NULL,
      &set_addr, &show_addr, NULL },
    { MTAB_XTD|MTAB_VDV, 0, "VECTOR", NULL,
      &set_vec, &dcx_show_vec, NULL },
    { MTAB_XTD | MTAB_VDV, 0, "lines", "LINES",
      &dcx_set_lines, &dcx_show_lines },
    { 0 }
    };

DEVICE dcix_dev = {
    "DCIX", &dcix_unit, dcix_reg, dcix_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &dcx_reset,
    NULL, &dcx_adcach, &dcx_detach,
    &dcix_dib, DEV_UBUS | DEV_QBUS | DEV_DISABLE | DEV_DIS
    };

/* DCOX data structures

   dcox_dev      DCOX device descriptor
   dcox_unit     DCOX unit descriptor
   dcox_reg      DCOX register list
*/

UNIT dcox_unit[] = {
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT },
    { UDATA (&dcox_svc, TT_MODE_UC, 0), SERIAL_OUT_WAIT }
    };

REG dcox_reg[] = {
    { BRDATA (BUF, dcox_buf, DEV_RDX, 8, DCX_LINES) },
    { BRDATA (CSR, dcox_csr, DEV_RDX, 16, DCX_LINES) },
    { GRDATA (IREQ, dcox_ireq, DEV_RDX, DCX_LINES, 0) },
    { URDATA (TIME, dcox_unit[0].wait, 10, 31, 0,
              DCX_LINES, PV_LEFT) },
    { NULL }
    };

MTAB dcox_mod[] = {
    { TT_MODE, TT_MODE_UC, "UC", "UC", NULL },
    { TT_MODE, TT_MODE_7B, "7b", "7B", NULL },
    { TT_MODE, TT_MODE_8B, "8b", "8B", NULL },
    { TT_MODE, TT_MODE_7P, "7p", "7P", NULL },
    { MTAB_XTD|MTAB_VUN, 0, NULL, "DISCONNECT",
      &tmxr_dscln, NULL, &dcx_desc },
    { MTAB_XTD|MTAB_VUN|MTAB_NC, 0, "LOG", "LOG",
      &tmxr_set_log, &tmxr_show_log, &dcx_desc },
    { MTAB_XTD|MTAB_VUN|MTAB_NC, 0, NULL, "NOLOG",
      &tmxr_set_nolog, NULL, &dcx_desc },
    { 0 }
    };

DEVICE dcox_dev = {
    "DCOX", dcox_unit, dcox_reg, dcox_mod,
    1, 10, 31, 1, 8, 8,
    NULL, NULL, &dcx_reset,
    NULL, NULL, NULL,
    NULL, DEV_UBUS | DEV_QBUS | DEV_DISABLE | DEV_DIS
    };

/* Terminal input routines */

t_stat dcx_rd (int32 *data, int32 PA, int32 access)
{
uint32 ln = ((PA - dcix_dib.ba) >> 3) & DCX_MASK;

switch ((PA >> 1) & 03) {                               /* decode PA<2:1> */

    case 00:                                            /* dci csr */
        *data = (dcix_csr[ln] & DCIXCSR_IMP) | DCIXCSR_CD;
        return SCPE_OK;

    case 01:                                            /* dci buf */
        dcix_csr[ln] &= ~CSR_DONE;
        dcix_clr_int (ln);
        *data = dcix_buf[ln];
        return SCPE_OK;

    case 02:                                            /* dco csr */
        *data = dcox_csr[ln] & DCOXCSR_IMP;
        return SCPE_OK;

    case 03:                                            /* dco buf */
        *data = dcox_buf[ln];
        return SCPE_OK;
        }                                               /* end switch PA */

return SCPE_NXM;
}

t_stat dcx_wr (int32 data, int32 PA, int32 access)
{
uint32 ln = ((PA - dcix_dib.ba) >> 3) & DCX_MASK;

switch ((PA >> 1) & 03) {                               /* decode PA<2:1> */

    case 00:                                            /* dci csr */
        if (PA & 1) return SCPE_OK;
        if ((data & CSR_IE) == 0)
            dcix_clr_int (ln);
        else if ((dcix_csr[ln] & (CSR_DONE + CSR_IE)) == CSR_DONE)
            dcix_set_int (ln);
        dcix_csr[ln] = (uint16) ((dcix_csr[ln] & ~DCIXCSR_RW) | (data & DCIXCSR_RW));
        return SCPE_OK;

    case 01:                                            /* dci buf */
        return SCPE_OK;

    case 02:                                            /* dco csr */
        if (PA & 1) return SCPE_OK;
        if ((data & CSR_IE) == 0)
            dcox_clr_int (ln);
        else if ((dcox_csr[ln] & (CSR_DONE + CSR_IE)) == CSR_DONE)
            dcox_set_int (ln);
        dcox_csr[ln] = (uint16) ((dcox_csr[ln] & ~DCOXCSR_RW) | (data & DCOXCSR_RW));
        return SCPE_OK;

    case 03:                                            /* dco buf */
        if ((PA & 1) == 0)
            dcox_buf[ln] = data & 0377;
        dcox_csr[ln] &= ~CSR_DONE;
        dcox_clr_int (ln);
        sim_activate (&dcox_unit[ln], dcox_unit[ln].wait);
        return SCPE_OK;
        }                                               /* end switch PA */

return SCPE_NXM;
}

/* Terminal input service */

t_stat dcix_svc (UNIT *uptr)
{
int32 ln, c, temp;

if ((uptr->flags & UNIT_ATT) == 0) return SCPE_OK;      /* adcached? */
sim_activate (uptr, tmxr_poll);                         /* continue poll */
ln = tmxr_poll_conn (&dcx_desc);                        /* look for connect */
if (ln >= 0) dcx_ldsc[ln].rcve = 1;                     /* got one? rcv enb */
tmxr_poll_rx (&dcx_desc);                               /* poll for input */
for (ln = 0; ln < DCX_LINES; ln++) {                    /* loop thru lines */
    if (dcx_ldsc[ln].conn) {                            /* connected? */
        if (temp = tmxr_getc_ln (&dcx_ldsc[ln])) {      /* get char */
            if (temp & SCPE_BREAK)                      /* break? */
                c = DCIXBUF_ERR|DCIXBUF_RBRK;
            else c = sim_tt_inpcvt (temp, TT_GET_MODE (dcox_unit[ln].flags));
            if (dcix_csr[ln] & CSR_DONE)
                c |= DCIXBUF_ERR|DCIXBUF_OVR;
            else {
                dcix_csr[ln] |= CSR_DONE;
                if (dcix_csr[ln] & CSR_IE) dcix_set_int (ln);
                }
            dcix_buf[ln] = c;
            }
        }
    }
return SCPE_OK;
}

/* Terminal output service */

t_stat dcox_svc (UNIT *uptr)
{
int32 c;
uint32 ln = uptr - dcox_unit;                           /* line # */

if (dcx_ldsc[ln].conn) {                                /* connected? */
    if (dcx_ldsc[ln].xmte) {                            /* tx enabled? */
        TMLN *lp = &dcx_ldsc[ln];                       /* get line */
        c = sim_tt_outcvt (dcox_buf[ln], TT_GET_MODE (dcox_unit[ln].flags));
        if (c >= 0) tmxr_putc_ln (lp, c);               /* output char */
        tmxr_poll_tx (&dcx_desc);                       /* poll xmt */
        }
    else {
        tmxr_poll_tx (&dcx_desc);                       /* poll xmt */
        sim_activate (uptr, dcox_unit[ln].wait);        /* wait */
        return SCPE_OK;
        }
    }
dcox_csr[ln] |= CSR_DONE;                               /* set done */
if (dcox_csr[ln] & CSR_IE) dcox_set_int (ln);
return SCPE_OK;
}

/* Interrupt routines */

void dcix_clr_int (uint32 ln)
{
dcix_ireq &= ~(1 << ln);                                /* clr mux rcv int */
if (dcix_ireq == 0) CLR_INT (DCIX);                     /* all clr? */
else SET_INT (DCIX);                                    /* no, set intr */
return;
}

void dcix_set_int (int32 ln)
{
dcix_ireq |= (1 << ln);                                 /* clr mux rcv int */
SET_INT (DCIX);                                         /* set master intr */
return;
}

int32 dcix_iack (void)
{
int32 ln;

for (ln = 0; ln < DCX_LINES; ln++) {                    /* find 1st line */
    if (dcix_ireq & (1 << ln)) {
        dcix_clr_int (ln);                              /* clear intr */
        return (dcix_dib.vec + (ln * 010));             /* return vector */
        }
    }
return 0;
}

void dcox_clr_int (int32 ln)
{
dcox_ireq &= ~(1 << ln);                                /* clr mux rcv int */
if (dcox_ireq == 0) CLR_INT (DCOX);                     /* all clr? */
else SET_INT (DCOX);                                    /* no, set intr */
return;
}

void dcox_set_int (int32 ln)
{
dcox_ireq |= (1 << ln);                                 /* clr mux rcv int */
SET_INT (DCOX);                                         /* set master intr */
return;
}

int32 dcox_iack (void)
{
int32 ln;

for (ln = 0; ln < DCX_LINES; ln++) {                    /* find 1st line */
    if (dcox_ireq & (1 << ln)) {
        dcox_clr_int (ln);                              /* clear intr */
        return (dcix_dib.vec + (ln * 010) + 4);         /* return vector */
        }
    }
return 0;
}

/* Reset */

t_stat dcx_reset (DEVICE *dptr)
{
int32 ln;

dcx_enbdis (dptr->flags & DEV_DIS);                     /* sync enables */
sim_cancel (&dcix_unit);                                /* assume stop */
if (dcix_unit.flags & UNIT_ATT)                         /* if adcached, */
    sim_activate (&dcix_unit, tmxr_poll);               /* activate */
for (ln = 0; ln < DCX_LINES; ln++)                      /* for all lines */
    dcx_reset_ln (ln);
return auto_config (dcix_dev.name, dcx_desc.lines);     /* auto config */
}

/* Reset individual line */

void dcx_reset_ln (uint32 ln)
{
dcix_buf[ln] = 0;                                       /* clear buf, */
dcix_csr[ln] = CSR_DONE;
dcox_buf[ln] = 0;                                       /* clear buf */
dcox_csr[ln] = CSR_DONE;
sim_cancel (&dcox_unit[ln]);                            /* deactivate */
dcix_clr_int (ln);
dcox_clr_int (ln);
return;
}

/* Adcach master unit */

t_stat dcx_adcach (UNIT *uptr, char *cptr)
{
t_stat r;

r = tmxr_attach (&dcx_desc, uptr, cptr);                /* adcach */
if (r != SCPE_OK) return r;                             /* error */
sim_activate (uptr, tmxr_poll);                         /* start poll */
return SCPE_OK;
}

/* Detach master unit */

t_stat dcx_detach (UNIT *uptr)
{
int32 i;
t_stat r;

r = tmxr_detach (&dcx_desc, uptr);                      /* detach */
for (i = 0; i < DCX_LINES; i++)                         /* all lines, */
    dcx_ldsc[i].rcve = 0;                               /* disable rcv */
sim_cancel (uptr);                                      /* stop poll */
return r;
}

/* Show summary processor */

t_stat dcx_summ (FILE *st, UNIT *uptr, int32 val, void *desc)
{
int32 i, t;

for (i = t = 0; i < DCX_LINES; i++) t = t + (dcx_ldsc[i].conn != 0);
if (t == 1) fprintf (st, "1 connection");
else fprintf (st, "%d connections", t);
return SCPE_OK;
}

/* SHOW CONN/STAT processor */

t_stat dcx_show (FILE *st, UNIT *uptr, int32 val, void *desc)
{
int32 i, t;

for (i = t = 0; i < DCX_LINES; i++) t = t + (dcx_ldsc[i].conn != 0);
if (t) {
    for (i = 0; i < DCX_LINES; i++) {
        if (dcx_ldsc[i].conn) { 
            if (val) tmxr_fconns (st, &dcx_ldsc[i], i);
            else tmxr_fstats (st, &dcx_ldsc[i], i);
            }
        }
    }
else fprintf (st, "all disconnected\n");
return SCPE_OK;
}

/* Enable/disable device */

void dcx_enbdis (int32 dis)
{
if (dis) {
    dcix_dev.flags = dcox_dev.flags | DEV_DIS;
    dcox_dev.flags = dcox_dev.flags | DEV_DIS;
    }
else {
    dcix_dev.flags = dcix_dev.flags & ~DEV_DIS;
    dcox_dev.flags = dcox_dev.flags & ~DEV_DIS;
    }
return;
}

/* SHOW VECTOR processor */

t_stat dcx_show_vec (FILE *st, UNIT *uptr, int32 val, void *desc)
{
return show_vec (st, uptr, dcx_desc.lines * 2, desc);
}

/* Change number of lines */

t_stat dcx_set_lines (UNIT *uptr, int32 val, char *cptr, void *desc)
{
int32 newln, i, t;
t_stat r;

if (cptr == NULL) return SCPE_ARG;
newln = get_uint (cptr, 10, DCX_LINES, &r);
if ((r != SCPE_OK) || (newln == dcx_desc.lines)) return r;
if (newln == 0) return SCPE_ARG;
if (newln < dcx_desc.lines) {
    for (i = newln, t = 0; i < dcx_desc.lines; i++) t = t | dcx_ldsc[i].conn;
    if (t && !get_yn ("This will disconnect users; proceed [N]?", FALSE))
        return SCPE_OK;
    for (i = newln; i < dcx_desc.lines; i++) {
        if (dcx_ldsc[i].conn) {
            tmxr_linemsg (&dcx_ldsc[i], "\r\nOperator disconnected line\r\n");
            tmxr_reset_ln (&dcx_ldsc[i]);               /* reset line */
            }
        dcox_unit[i].flags |= UNIT_DIS;
        dcx_reset_ln (i);
        }
    }
else {
    for (i = dcx_desc.lines; i < newln; i++) {
        dcox_unit[i].flags &= ~UNIT_DIS;
        dcx_reset_ln (i);
        }
    }
dcx_desc.lines = newln;
return auto_config (dcix_dev.name, newln);             /* auto config */
}

/* Show number of lines */

t_stat dcx_show_lines (FILE *st, UNIT *uptr, int32 val, void *desc)
{
fprintf (st, "lines=%d", dcx_desc.lines);
return SCPE_OK;
}
