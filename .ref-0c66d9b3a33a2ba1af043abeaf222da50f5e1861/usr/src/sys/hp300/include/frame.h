/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * %sccs.include.redist.c%
 *
 * from: Utah $Hdr: frame.h 1.1 90/07/09$
 *
 *	@(#)frame.h	7.2 (Berkeley) %G%
 */

struct frame {
	int	f_regs[16];
	short	f_stackadj;
	u_short	f_sr;
	u_int	f_pc;
	u_short	f_format:4,
		f_vector:12;
	union F_u {
		struct fmt2 {
			u_int	f_iaddr;
		} F_fmt2;

		struct fmt9 {
			u_int	f_iaddr;
			u_short	f_iregs[4];
		} F_fmt9;

		struct fmtA {
			u_short	f_ir0;
			u_short	f_ssw;
			u_short	f_ipsc;
			u_short	f_ipsb;
			u_int	f_dcfa;
			u_short	f_ir1, f_ir2;
			u_int	f_dob;
			u_short	f_ir3, f_ir4;
		} F_fmtA;

		struct fmtB {
			u_short	f_ir0;
			u_short	f_ssw;
			u_short	f_ipsc;
			u_short	f_ipsb;
			u_int	f_dcfa;
			u_short	f_ir1, f_ir2;
			u_int	f_dob;
			u_short	f_ir3, f_ir4;
			u_short	f_ir5, f_ir6;
			u_int	f_sba;
			u_short	f_ir7, f_ir8;
			u_int	f_dib;
			u_short	f_iregs[22];
		} F_fmtB;
	} F_u;
};

#define	f_fmt2		F_u.F_fmt2
#define	f_fmtA		F_u.F_fmtA
#define	f_fmtB		F_u.F_fmtB

/* common frame size */
#define	CFSIZE		(sizeof(struct frame) - sizeof(union F_u))
#define	NFMTSIZE	8

#define	FMT0		0x0
#define	FMT1		0x1
#define	FMT2		0x2
#define	FMT9		0x9
#define	FMTA		0xA
#define	FMTB		0xB

/* frame specific info sizes */
#define	FMT0SIZE	0
#define	FMT1SIZE	0
#define	FMT2SIZE	sizeof(struct fmt2)
#define	FMT9SIZE	sizeof(struct fmt9)
#define	FMTASIZE	sizeof(struct fmtA)
#define	FMTBSIZE	sizeof(struct fmtB)

#define	V_BUSERR	0x008
#define	V_ADDRERR	0x00C
#define	V_TRAP1		0x084

#define	SSW_RC		0x2000
#define	SSW_RB		0x1000
#define	SSW_DF		0x0100
#define	SSW_RM		0x0080
#define	SSW_RW		0x0040
#define	SSW_FCMASK	0x0007

struct fpframe {
	union FPF_u1 {
		u_int	FPF_null;
		struct {
			u_char	FPF_version;
			u_char	FPF_fsize;
			u_short	FPF_res1;
		} FPF_nonnull;
	} FPF_u1;
	union FPF_u2 {
		struct fpidle {
			u_short	fpf_ccr;
			u_short	fpf_res2;
			u_int	fpf_iregs1[8];
			u_int	fpf_xops[3];
			u_int	fpf_opreg;
			u_int	fpf_biu;
		} FPF_idle;

		struct fpbusy {
			u_int	fpf_iregs[53];
		} FPF_busy;
	} FPF_u2;
	u_int	fpf_regs[8*3];
	u_int	fpf_fpcr;
	u_int	fpf_fpsr;
	u_int	fpf_fpiar;
};

#define fpf_null	FPF_u1.FPF_null
#define fpf_version	FPF_u1.FPF_nonnull.FPF_version
#define fpf_fsize	FPF_u1.FPF_nonnull.FPF_fsize
#define fpf_res1	FPF_u1.FPF_nonnull.FPF_res1
#define fpf_idle	FPF_u2.FPF_idle
#define fpf_busy	FPF_u2.FPF_busy
