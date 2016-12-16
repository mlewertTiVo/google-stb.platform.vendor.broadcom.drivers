/*
 * Chip-specific hardware definitions for
 * Broadcom 802.11abg Networking Device Driver
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: $
 */

#ifndef	_D11REGS_H
#define	_D11REGS_H

#include <typedefs.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <sbconfig.h>

typedef volatile struct {
	uint32	intstatus;
	uint32	intmask;
} intctrlregs_t;

/**
 * read: 32-bit register that can be read as 32-bit or as 2 16-bit
 * write: only low 16b-it half can be written
 */
typedef volatile union {
	uint32 pmqhostdata;		/**< read only! */
	struct {
		uint16 pmqctrlstatus;	/**< read/write */
		uint16 PAD;
	} w;
} pmqreg_t;

/** pio register set 2/4 bytes union for d11 fifo */
typedef volatile union {
	pio2regp_t	b2;		/**< < corerev 8 */
	pio4regp_t	b4;		/**< >= corerev 8 */
} u_pioreg_t;

/** dma/pio corerev >= 11 */
typedef volatile struct {
	dma64regs_t	dmaxmt;		/* dma tx */
	pio4regs_t	piotx;		/* pio tx */
	dma64regs_t	dmarcv;		/* dma rx */
	pio4regs_t	piorx;		/* pio rx */
} fifo64_t;

/** indirect dma corerev >= 64 */
typedef volatile struct {
	dma64regs_t	dma;		/**< dma tx */
	uint32		indintstatus;
	uint32		indintmask;
} ind_dma_t;

#define GET_MACINTSTATUS(osh, regs)		R_REG((osh), &((regs)->macintstatus))
#define SET_MACINTSTATUS(osh, regs, val)	W_REG((osh), &((regs)->macintstatus), (val))
#define GET_MACINTMASK(osh, regs)		R_REG((osh), &((regs)->macintmask))
#define SET_MACINTMASK(osh, regs, val)		W_REG((osh), &((regs)->macintmask), (val))

#define GET_MACINTSTATUS_X(osh, regs)		R_REG((osh), &((regs)->macintstatus_x))
#define SET_MACINTSTATUS_X(osh, regs, val)	W_REG((osh), &((regs)->macintstatus_x), (val))
#define GET_MACINTMASK_X(osh, regs)		R_REG((osh), &((regs)->macintmask_x))
#define SET_MACINTMASK_X(osh, regs, val)	W_REG((osh), &((regs)->macintmask_x), (val))

/**
 * Host Interface Registers
 * - primed from hnd_cores/dot11mac/systemC/registers/ihr.h
 * - but definitely not complete
 */
typedef volatile struct _d11regs {
	/* Device Control ("semi-standard host registers") */
	uint32	PAD[3];			/* 0x0 - 0x8 */
	uint32	biststatus;		/* 0xC */
	uint32	biststatus2;		/* 0x10 */
	uint32	PAD;			/* 0x14 */
	uint32	gptimer;		/* 0x18 */	/* for corerev >= 3 */
	uint32	usectimer;		/* 0x1c */	/* for corerev >= 26 */

	/** Interrupt Control */		/* 0x20 */
	intctrlregs_t	intctrlregs[8];

	/** New altintmask on corerev 40 */
	uint32	altintmask[6];	/* 0x60 - 0x74 */
	uint32	PAD[2];			/* 0x78 - 0x7c */

	/** Indirect DMA channel registers corerev >= 64 */
	ind_dma_t	inddma;		/* 0x80 - 0x9c */

	/** Indirect AQM DMA channel registers corerev >= 64 */
	ind_dma_t	indaqm;		/* 0xa0 - 0xbc */

	/** More TX DMA channel registers corerev >= 64 */
	uint32	indqsel;		/* 0xc0 */
	uint32	suspreq;		/* 0xc4 */
	uint32	flushreq;		/* 0xc8 */
	uint32	chnflushstatus;		/* 0xcc */
	uint32  chnsuspstatus;		/* 0xd0 */
	uint32	PAD[11];		/* 0xd4 - 0xfc */

	/* tx fifos 6-7 and rx fifos 1-3 removed in corerev 5 */
	uint32	intrcvlazy[4];		/* 0x100 - 0x10C */

	uint32	PAD[4];			/* 0x110 - 0x11c */

	uint32	maccontrol;		/* 0x120 */
	uint32	maccommand;		/* 0x124 */
	uint32	macintstatus;		/* 0x128 */
	uint32	macintmask;		/* 0x12C */

	/* Transmit Template Access */
	uint32	tplatewrptr;		/* 0x130 */
	uint32	tplatewrdata;		/* 0x134 */
	uint32	PAD[2];			/* 0x138 - 0x13C */

	/* PMQ registers */
	pmqreg_t pmqreg;		/* 0x140 */
	uint32	pmqpatl;		/* 0x144 */
	uint32	pmqpath;		/* 0x148 */
	uint32	PAD;			/* 0x14C */

	uint32	chnstatus;		/* 0x150 */
	uint32	psmdebug;		/* 0x154 */	/* for corerev >= 3 */
	uint32	phydebug;		/* 0x158 */	/* for corerev >= 3 */
	uint32	machwcap;		/* 0x15C */	/* Corerev >= 13 */

	/** Extended Internal Objects */
	uint32	objaddr;		/* 0x160 */
	uint32	objdata;		/* 0x164 */

	/** New altmacintstatus/mask on corerev >=40 */
	uint32	altmacintstatus;	/* 0x168 */
	uint32  altmacintmask;	/* 0x16c */

	/* New txstatus registers on corerev >= 5 */
	uint32	frmtxstatus;		/* 0x170 */
	uint32	frmtxstatus2;		/* 0x174 */
	uint32	frmtxstatus3;		/* 0x178 */	/* for corerev >= 40 */
	uint32	frmtxstatus4;		/* 0x17C */	/* for corerev >= 40 */

	/* New TSF host access on corerev >= 3 */
	uint32	tsf_timerlow;		/* 0x180 */
	uint32	tsf_timerhigh;		/* 0x184 */
	uint32	tsf_cfprep;		/* 0x188 */
	uint32	tsf_cfpstart;		/* 0x18c */
	uint32	tsf_cfpmaxdur32;	/* 0x190 */

	/** AVB timer on corerev >= 28 */
	uint32	avbrx_timestamp;	/* 0x194 */
	uint32	avbtx_timestamp;	/* 0x198 */
	uint32	PAD[1];			/* 0x19c */
	uint32	maccontrol1;		/* 0x1a0 */
	uint32	machwcap1;		/* 0x1a4 */
	uint32	PAD[1];			/* 0x1a8 */
	uint32	psm_patchcopy_ctrl;	/* 0x1ac */
	/* PSMx host access registers corerev >= 64  and corerev < 80 */
	uint32	gptimer_x;		/* 0x1b0 */
	uint32	maccontrol_x;		/* 0x1b4 */
	uint32	maccontrol1_x;		/* 0x1b8 */
	uint32	maccommand_x;		/* 0x1bc */
	uint32	macintstatus_x;		/* 0x1c0 */
	uint32	macintmask_x;		/* 0x1c4 */
	uint32	altmacintstatus_x;	/* 0x1c8 */
	uint32	altmacintmask_x;	/* 0x1cc */
	uint32	psmdebug_x;		/* 0x1d0 */
	uint32	phydebug_x;		/* 0x1d4 */
	uint32	statctr2;		/* 0x1d8 */
	uint32	statctr3;		/* 0x1dc */

	/* Clock control and hardware workarounds (corerev >= 13) */
	uint32	clk_ctl_st;		/* 0x1e0 */
	uint32	hw_war;
	uint32	powerctl;               /* 0x1e8 */
	uint32	PAD[5];			/* 0x1ec - 0x1fc */

	/** 0x200-0x37F dma/pio registers */
	fifo64_t	f64regs[6];	/* on corerev >= 11 */

	/** FIFO diagnostic port access */
	dma32diag_t dmafifo;		/* 0x380 - 0x38C */

	uint32	aggfifocnt;		/* 0x390 */
	uint32	aggfifodata;		/* 0x394 */
	uint32	dbgstrmask;		/* 0x398 - DebugStoreMask */
	uint32	dbgstrtrigmask;		/* 0x39c - DebugStoreTriggerMask */
	uint32	dbgstrtrig;		/* 0x3a0 - DebugStoreTrigger */
	uint32	PAD[9];			/* 0x3a4 - 0x3c4 */
	uint16	altradioregaddr;	/* 0x3c8 */
	uint16	altradioregdata;	/* 0x3ca */
	uint32	PAD[3];			/* 0x3cc - 0x3d4 */
	uint16  radioregaddr;		/* 0x3d8 */
	uint16  radioregdata;		/* 0x3da */

	/** time delay between the change on rf disable input and radio shutdown corerev 10 */
	uint32	rfdisabledly;		/* 0x3DC */

	/* PHY register access */
	uint16	phyversion;		/* 0x3e0 - 0x0 */
	uint16	phybbconfig;		/* 0x3e2 - 0x1 */
	uint16	phyadcbias;		/* 0x3e4 - 0x2	Bphy only */
	uint16	phyanacore;		/* 0x3e6 - 0x3	pwwrdwn on aphy */
	uint16	phyrxstatus0;		/* 0x3e8 - 0x4 */
	uint16	phyrxstatus1;		/* 0x3ea - 0x5 */
	uint16	phycrsth;		/* 0x3ec - 0x6 */
	uint16	phytxerror;		/* 0x3ee - 0x7 */
	uint16	phychannel;		/* 0x3f0 - 0x8 */
	uint16	PAD[1];			/* 0x3f2 - 0x9 */
	uint16	phytest;		/* 0x3f4 - 0xa */
	uint16	phy4waddr;		/* 0x3f6 - 0xb */
	uint16	phy4wdatahi;		/* 0x3f8 - 0xc */
	uint16	phy4wdatalo;		/* 0x3fa - 0xd */
	uint16	phyregaddr;		/* 0x3fc - 0xe */
	uint16	phyregdata;		/* 0x3fe - 0xf */

	/* IHR */			/* 0x400 - 0x7FE */

	/* RXE Block */
	uint16	PAD[3];			/* 0x400 - 0x404 */
	uint16	rcv_fifo_ctl;		/* 0x406 */
	uint16	rcv_ctl;		/* 0x408 */
	uint16	rcv_frm_cnt;		/* 0x40a */
	uint16	rcv_status_len;		/* 0x40c */
	uint16	PAD[3];			/* 0x40e - 0x412 */
	uint16	rssi;			/* 0x414 */
	uint16	PAD[1];			/* 0x416 */
	uint16  rxe_rxcnt;		/* 0x418 */
	uint16  rxe_status1;		/* 0x41a */
	uint16  rxe_status2;		/* 0x41c */
	uint16  rxe_plcp_len;		/* 0x41e */

	union {
		struct {
			uint16	rcm_ctl;		/* 0x420 */
			uint16	rcm_mat_data;		/* 0x422 */
			uint16	rcm_mat_mask;		/* 0x424 */
			uint16	rcm_mat_dly;		/* 0x426 */
			uint16	rcm_cond_mask_l;	/* 0x428 */
			uint16	rcm_cond_mask_h;	/* 0x42A */
			uint16	rcm_cond_dly;		/* 0x42C */
			uint16	PAD[1];			/* 0x42E */
			uint16	ext_ihr_addr;		/* 0x430 */
			uint16	ext_ihr_data;		/* 0x432 */
			uint16	rxe_phyrs_2;		/* 0x434 */
			uint16	rxe_phyrs_3;		/* 0x436 */
			uint16	phy_mode;		/* 0x438 */
			uint16	rcmta_ctl;		/* 0x43a */
			uint16	rcmta_size;		/* 0x43c */
			uint16	rcmta_addr0;		/* 0x43e */
			uint16	rcmta_addr1;		/* 0x440 */
			uint16	rcmta_addr2;		/* 0x442 */
			uint16	PAD[2];			/* 0x444 - 0x446 */
			uint16	dagg_ctl;		/* 0x448 For corerev < 40 */
			uint16  dagg_count;		/* 0x44A For corerev < 40 */
			uint16  dagg_length;	/* 0x44C For corerev < 40 */
			uint16	PAD[8];		/* 0x44e - 0x45c */
			uint16	ext_ihr_status;		/* 0x45e */
			uint16	radio_ihr_addr;		/* 0x460 */
			uint16	radio_ihr_data;		/* 0x462 */
			uint16	PAD[2];			/* 0x464 - 0x466 */
			uint16	radio_ihr_status;	/* 0x468 */
			uint16	PAD[11];		/* 0x46a - 0x47e */
		} d11regs;

		struct {
			uint16	rcv_frm_cnt_q0;		/* 0x420 */
			uint16	rcv_frm_cnt_q1;		/* 0x422 */
			uint16	rcv_wrd_cnt_q0;		/* 0x424 */
			uint16	rcv_wrd_cnt_q1;		/* 0x426 */
			uint16	PAD[2];			/* 0x428 - 0x42A */
			uint16	rcv_bm_sp_q0;		/* 0x42C */
			uint16	rcv_bm_ep_q0;		/* 0x42E */
			uint16	PAD[5];			/* 0x430 - 0x438 */
			uint16	rcv_bm_sp_q1;		/* 0x43a */
			uint16	rcv_bm_ep_q1;		/* 0x43c */
			uint16  rcv_copcnt_q1;		/* 0x43e */
			uint16	PAD[4];			/* 0x440 - 0x446 */
			/* below is corerev >= 64 */
			uint16  rxe_errval;		/* 0x448 */
			uint16	PAD;			/* 0x44a */
			uint16  rxe_status3;		/* 0x44c */
			uint16	PAD[25];		/* 0x44a - 0x47e */
		} d11acregs;
	} u_rcv;

	/* PSM Block */			/* 0x480 - 0x500 */
	uint16 PAD;			/* 0x480 */
	uint16 psm_maccontrol_h;	/* 0x482 */
	uint16 psm_macintstatus_l;	/* 0x484 */
	uint16 psm_macintstatus_h;	/* 0x486 */
	uint16 psm_macintmask_l;	/* 0x488 */
	uint16 psm_macintmask_h;	/* 0x48A */
	uint16 PAD;			/* 0x48C */
	uint16 psm_maccommand;		/* 0x48E */
	uint16 psm_brc;			/* 0x490 */
	uint16 psm_phy_hdr_param;	/* 0x492 */
	uint16 psm_int_sel_0;		/* 0x494 */
	uint16 psm_int_sel_1;		/* 0x496 */
	uint16 psm_int_sel_2;		/* 0x498 */
	uint16 psm_gpio_in;		/* 0x49A */
	uint16 psm_gpio_out;		/* 0x49C */
	uint16 psm_gpio_oe;		/* 0x49E */
	uint16 psm_bred_0;		/* 0x4A0 */
	uint16 psm_bred_1;		/* 0x4A2 */
	uint16 psm_bred_2;		/* 0x4A4 */
	uint16 psm_bred_3;		/* 0x4A6 */
	uint16 psm_brcl_0;		/* 0x4A8 */
	uint16 psm_brcl_1;		/* 0x4AA */
	uint16 psm_brcl_2;		/* 0x4AC */
	uint16 psm_brcl_3;		/* 0x4AE */
	uint16 psm_brpo_0;		/* 0x4B0 */
	uint16 psm_brpo_1;		/* 0x4B2 */
	uint16 psm_brpo_2;		/* 0x4B4 */
	uint16 psm_brpo_3;		/* 0x4B6 */
	uint16 psm_brwk_0;		/* 0x4B8 */
	uint16 psm_brwk_1;		/* 0x4BA */
	uint16 psm_brwk_2;		/* 0x4BC */
	uint16 psm_brwk_3;		/* 0x4BE */
	/* Corerev < 64 */
	uint16 psm_base_lt64[4];	/* 0x4C0 - 0x4C6 */
	uint16 psm_power_req_sts;	/* 0x4C8 */
	uint16 PAD[2];			/* 0x4CA - 0x4CC */
	uint16 psm_ihr_err;		/* 0x4CE */
	uint16 psm_pc_reg_0;		/* 0x4D0 */
	uint16 psm_pc_reg_1;		/* 0x4D2 */
	uint16 psm_pc_reg_2;		/* 0x4D4 */
	uint16 psm_pc_reg_3;		/* 0x4D6 */
	uint16 psm_brc_1;		/* 0x4D8 */
	uint16 PAD[0x8];                /* 0x4DA - 0x4E8 */
	uint16 psm_sbreg_addr;          /* 0x4EA */
	uint16 psm_sbreg_dataL;         /* 0x4EC */
	uint16 psm_sbreg_dataH;         /* 0x4EE */
	uint16 psm_corectlsts;          /* 0x4f0 */	/* Corerev >= 13 */
	uint16 PAD;                     /* 0x4f2 - 0x4fE */
	uint16 psm_sbbar0;              /* 0x4f4 */
	uint16 psm_sbbar1;              /* 0x4f6 */
	uint16 psm_sbbar2;              /* 0x4f8 */
	uint16 psm_sbbar3;              /* 0x4fa */
	uint16 PAD[0x2];                /* 0x4fc - 0x4fE */

	/* TXE0 Block */		/* 0x500 - 0x580 */
	uint16	txe_ctl;		/* 0x500 */
	uint16	txe_aux;		/* 0x502 */
	uint16	txe_ts_loc;		/* 0x504 */
	uint16	txe_time_out;		/* 0x506 */
	uint16	txe_wm_0;		/* 0x508 */
	uint16	txe_wm_1;		/* 0x50A */
	uint16	txe_phyctl;		/* 0x50C */
	uint16	txe_status;		/* 0x50E */
	uint16	txe_mmplcp0;		/* 0x510 */
	uint16	txe_mmplcp1;		/* 0x512 */
	uint16	txe_phyctl1;		/* 0x514 */
	uint16	txe_phyctl2;		/* 0x516 */
	uint16	PAD[4];			/* 0x518 - 0x51E */

	union {
		struct {
			/* Transmit control */
			uint16	xmtfifodef;		/* 0x520 */
			uint16  xmtfifo_frame_cnt;      /* 0x522 */     /* Corerev >= 16 */
			uint16  xmtfifo_byte_cnt;       /* 0x524 */     /* Corerev >= 16 */
			uint16  xmtfifo_head;           /* 0x526 */     /* Corerev >= 16 */
			uint16  xmtfifo_rd_ptr;         /* 0x528 */     /* Corerev >= 16 */
			uint16  xmtfifo_wr_ptr;         /* 0x52A */     /* Corerev >= 16 */
			uint16  xmtfifodef1;            /* 0x52C */     /* Corerev >= 16 */

			/* AggFifo */
			uint16  aggfifo_cmd;            /* 0x52e */
			uint16  aggfifo_stat;           /* 0x530 */
			uint16  aggfifo_cfgctl;         /* 0x532 */
			uint16  aggfifo_cfgdata;        /* 0x534 */
			uint16  aggfifo_mpdunum;        /* 0x536 */
			uint16  aggfifo_len;            /* 0x538 */
			uint16  aggfifo_bmp;            /* 0x53A */
			uint16  aggfifo_ackedcnt;       /* 0x53C */
			uint16  aggfifo_sel;            /* 0x53E */

			uint16	xmtfifocmd;		/* 0x540 */
			uint16	xmtfifoflush;		/* 0x542 */
			uint16	xmtfifothresh;		/* 0x544 */
			uint16	xmtfifordy;		/* 0x546 */
			uint16	xmtfifoprirdy;		/* 0x548 */
			uint16	xmtfiforqpri;		/* 0x54A */
			uint16	xmttplatetxptr;		/* 0x54C */
			uint16	PAD;			/* 0x54E */
			uint16	xmttplateptr;		/* 0x550 */
			uint16  smpl_clct_strptr;       /* 0x552 */	/* Corerev >= 22 */
			uint16  smpl_clct_stpptr;       /* 0x554 */	/* Corerev >= 22 */
			uint16  smpl_clct_curptr;       /* 0x556 */	/* Corerev >= 22 */
			uint16  aggfifo_data;           /* 0x558 */
			uint16	PAD[0x03];		/* 0x55A - 0x55E */
			uint16	xmttplatedatalo;	/* 0x560 */
			uint16	xmttplatedatahi;	/* 0x562 */

			uint16	PAD[2];			/* 0x564 - 0x566 */

			uint16	xmtsel;			/* 0x568 */
			uint16	xmttxcnt;		/* 0x56A */
			uint16	xmttxshmaddr;		/* 0x56C */

			uint16	PAD;			/* 0x56E */

			uint16	xmt_ampdu_ctl;		/* 0x570 */

			uint16	PAD[0x07];		/* 0x572 - 0x57E */

			/* TXE1 Block */
			uint16	PAD[0x40];		/* 0x580 - 0x5FE */

			/* TSF Block */
			uint16	PAD[0X02];		/* 0x600 - 0x602 */
			uint16	tsf_cfpstrt_l;		/* 0x604 */
			uint16	tsf_cfpstrt_h;		/* 0x606 */
			uint16	PAD[0X05];		/* 0x608 - 0x610 */
			uint16	tsf_cfppretbtt;		/* 0x612 */
			uint16	PAD[0XD];		/* 0x614 - 0x62C */
			uint16  tsf_clk_frac_l;         /* 0x62E */
			uint16  tsf_clk_frac_h;         /* 0x630 */
			uint16	PAD[0X0A];		/* 0x632 - 0x645 */
			uint16	tsf_gpt0_stat;	/* 0x646 */
			uint16  tsf_gpt1_stat;	/* 0x648 */
			uint16  PAD[0X08];		/* 0x64A - 0x658 */
			uint16	tsf_random;		/* 0x65A */
			uint16	PAD[0x05];		/* 0x65C - 0x664 */

			/* GPTimer 2 registers are corerev >= 3 */
			uint16	tsf_gpt2_stat;		/* 0x666 */
			uint16	tsf_gpt2_ctr_l;		/* 0x668 */
			uint16	tsf_gpt2_ctr_h;		/* 0x66A */
			uint16	tsf_gpt2_val_l;		/* 0x66C */
			uint16	tsf_gpt2_val_h;		/* 0x66E */
			uint16	tsf_gptall_stat;	/* 0x670 */
			uint16	PAD[0x07];		/* 0x672 - 0x67E */

			/* IFS Block */
			uint16	ifs_sifs_rx_tx_tx;	/* 0x680 */
			uint16	ifs_sifs_nav_tx;	/* 0x682 */
			uint16	ifs_slot;		/* 0x684 */
			uint16	PAD;			/* 0x686 */
			uint16	ifs_ctl;		/* 0x688 */
			uint16	ifs_boff;		/* 0x68a */
			uint16	PAD[0x2];		/* 0x68c - 0x68F */
			uint16	ifsstat;		/* 0x690 */
			uint16	ifsmedbusyctl;		/* 0x692 */
			uint16	iftxdur;		/* 0x694 */
			uint16	PAD[0x3];		/* 0x696 - 0x69b */
			/* EDCF support in dot11macs with corerevs >= 16 */
			uint16	ifs_aifsn;		/* 0x69c */
			uint16	ifs_ctl1;		/* 0x69e */

			/* New slow clock registers on corerev >= 5 */
			uint16	scc_ctl;		/* 0x6a0 */
			uint16	scc_timer_l;		/* 0x6a2 */
			uint16	scc_timer_h;		/* 0x6a4 */
			uint16	scc_frac;		/* 0x6a6 */
			uint16	scc_fastpwrup_dly;	/* 0x6a8 */
			uint16	scc_per;		/* 0x6aa */
			uint16	scc_per_frac;		/* 0x6ac */
			uint16	scc_cal_timer_l;	/* 0x6ae */
			uint16	scc_cal_timer_h;	/* 0x6b0 */
			uint16	PAD;			/* 0x6b2 */

			/* BTCX block on corerev >=13 */
			uint16	btcx_ctrl;		/* 0x6b4 */
			uint16	btcx_stat;		/* 0x6b6 */
			uint16	btcx_trans_ctrl;	/* 0x6b8 */
			uint16	btcx_pri_win;		/* 0x6ba */
			uint16	btcx_tx_conf_timer;	/* 0x6bc */
			uint16	btcx_ant_sw_timer;	/* 0x6be */

			uint16	btcx_prv_rfact_timer;	/* 0x6c0 */
			uint16	btcx_cur_rfact_timer;	/* 0x6c2 */
			uint16	btcx_rfact_dur_timer;	/* 0x6c4 */

			uint16	ifs_ctl_sel_pricrs;	/* 0x6c6 */
			uint16	ifs_ctl_sel_seccrs;	/* 0x6c8 */
			uint16	PAD[19];		/* 0x6ca - 0x6ee */

			/* ECI regs on corerev >=14 */
			uint16	btcx_eci_addr;		/* 0x6f0 */
			uint16	btcx_eci_data;		/* 0x6f2 */
			uint16	btcx_eci_mask_addr;	/* 0x6f4 */
			uint16	btcx_eci_mask_data;	/* 0x6f6 */
			uint16	coex_io_mask;		/* 0x6f8 */
			uint16 btcx_eci_event_addr;	/* 0x6fa */
			uint16 btcx_eci_event_data;	/* 0x6fc */

			uint16	PAD[1];

			/* NAV Block */
			uint16	nav_ctl;		/* 0x700 */
			uint16	navstat;		/* 0x702 */
			uint16	PAD[0x3e];		/* 0x702 - 0x77E */

			/* WEP/PMQ Block */		/* 0x780 - 0x7FE */
			uint16 PAD[0x20];		/* 0x780 - 0x7BE */

			uint16 wepctl;			/* 0x7C0 */
			uint16 wepivloc;		/* 0x7C2 */
			uint16 wepivkey;		/* 0x7C4 */
			uint16 wepwkey;			/* 0x7C6 */

			uint16 PAD[4];			/* 0x7C8 - 0x7CE */
			uint16 pcmctl;			/* 0X7D0 */
			uint16 pcmstat;			/* 0X7D2 */
			uint16 PAD[6];			/* 0x7D4 - 0x7DE */

			uint16 pmqctl;			/* 0x7E0 */
			uint16 pmqstatus;		/* 0x7E2 */
			uint16 pmqpat0;			/* 0x7E4 */
			uint16 pmqpat1;			/* 0x7E6 */
			uint16 pmqpat2;			/* 0x7E8 */
			uint16 pmqdat;			/* 0x7EA */
			uint16 pmqdataor_mat;		/* 0x7EC */
			uint16 pmqdataor_all;		/* 0x7EE */

			/* PMQ regs on corerev >=64 */
			uint16 pmqdataor_mat1;		/* 0x7F0 */
			uint16 pmqdataor_mat2;		/* 0x7F2 */
			uint16 pmqdataor_mat3;		/* 0x7F4 */
			uint16 pmq_auxsts;		/* 0x7F6 */
			uint16 pmq_ctl1;		/* 0x7F8 */
			uint16 pmq_status1;		/* 0x7FA */
			uint16 pmq_addthr;		/* 0x7FC */

			/* SHM */
			uint16  PAD[0x171];		/* 0x7fe - 0xade */
			uint16  ClkGateReqCtrl0;	/* 0xae0 */
			uint16  ClkGateReqCtrl1;	/* 0xae2 */
			uint16  ClkGateReqCtrl2;	/* 0xae4 */
			uint16  ClkGateUcodeReq0;	/* 0xae6 */
			uint16  ClkGateUcodeReq1;	/* 0xae8 */
			uint16  ClkGateUcodeReq2;	/* 0xaea */
			uint16	ClkGateStretch0;	/* 0xaec */
			uint16	ClkGateStretch1;	/* 0xaee */
			uint16	ClkGateMisc;		/* 0xaf0 */
			uint16	ClkGateDivCtrl;		/* 0xaf2 */
			uint16	ClkGatePhyClkCtrl;	/* 0xaf4 */
			uint16	ClkGateSts;			/* 0xaf6 */
			uint16	ClkGateExtReq0;		/* 0xaf8 */
			uint16	ClkGateExtReq1;		/* 0xafa */
			uint16	ClkGateExtReq2;		/* 0xafc */
			uint16	ClkGateUcodePhyClkCtrl;	/* 0xafe */
			uint16  PAD[0x1B8];    /* 0xb00 - 0xEFE */

		} d11regs;

		struct {
			uint16  XmtFIFOFullThreshold;	/* 0x520 */
			uint16  XmtFifoFrameCnt;	/* 0x522 */
			uint16	PAD[1];
			uint16  BMCReadReq;		/* 0x526 */
			uint16  BMCReadOffset;		/* 0x528 */
			uint16  BMCReadLength;		/* 0x52a */
			uint16  BMCReadStatus;		/* 0x52c */
			uint16  XmtShmAddr;		/* 0x52e */
			uint16  PsmMSDUAccess;		/* 0x530 */
			uint16  MSDUEntryBufCnt;	/* 0x532 */
			uint16  MSDUEntryStartIdx;	/* 0x534 */
			uint16  MSDUEntryEndIdx;	/* 0x536 */
			uint16  SampleCollectPlayPtrHigh;	/* 0x538 */
			uint16  SampleCollectCurPtrHigh;	/* 0x53a */
			uint16  BMCCmd1;		/* 0x53c */
			uint16	BMCDynAllocStatus;	/* 0x53e */
			uint16  BMCCTL;			/* 0x540 */
			uint16  BMCConfig;		/* 0x542 */
			uint16  BMCStartAddr;		/* 0x544 */
			uint16  BMCSize;		/* 0x546 */
			uint16  BMCCmd;			/* 0x548 */
			uint16  BMCMaxBuffers;		/* 0x54a */
			uint16  BMCMinBuffers;		/* 0x54c */
			uint16  BMCAllocCtl;		/* 0x54e */
			uint16  BMCDescrLen;		/* 0x550 */
			uint16  SampleCollectStartPtr;	/* 0x552 */
			uint16  SampleCollectStopPtr;	/* 0x554 */
			uint16  SampleCollectCurPtr;	/* 0x556 */
			uint16  SaveRestoreStartPtr;	/* 0x558 */
			uint16  SamplePlayStartPtr;	/* 0x55a */
			uint16  SamplePlayStopPtr;	/* 0x55c */
			uint16  XmtDMABusy;		/* 0x55e */
			uint16  XmtTemplateDataLo;	/* 0x560 */
			uint16  XmtTemplateDataHi;	/* 0x562 */
			uint16  XmtTemplatePtr;		/* 0x564 */
			uint16  XmtSuspFlush;		/* 0x566 */
			uint16  XmtFifoRqPrio;		/* 0x568 */
			uint16  BMCStatCtl;		/* 0x56a */
			uint16  BMCStatData;		/* 0x56c */
			uint16  BMCMSDUFifoStat;	/* 0x56e */
			uint16  PAD[4];			/* 0x570-576 */
			uint16  txe_status1;		/* 0x578 */
			uint16	PAD[323];		/* 0x57a - 0x800 */

			union {
				struct {
					/* AQM */
					uint16  AQMConfig;		/* 0x800 */
					uint16  AQMFifoDef;		/* 0x802 */
					uint16  AQMMaxIdx;		/* 0x804 */
					uint16  AQMRcvdBA0;		/* 0x806 */
					uint16  AQMRcvdBA1;		/* 0x808 */
					uint16  AQMRcvdBA2;		/* 0x80a */
					uint16  AQMRcvdBA3;		/* 0x80c */
					uint16  AQMBaSSN;		/* 0x80e */
					uint16  AQMRefSN;		/* 0x810 */
					uint16  AQMMaxAggLenLow;	/* 0x812 */
					uint16  AQMMaxAggLenHi;		/* 0x814 */
					uint16  AQMAggParams;		/* 0x816 */
					uint16  AQMMinMpduLen;		/* 0x818 */
					uint16  AQMMacAdjLen;		/* 0x81a */
					uint16  DebugBusCtrl;		/* 0x81c */
					uint16	PAD[1];
					uint16  AQMAggStats;		/* 0x820 */
					uint16  AQMAggLenLow;		/* 0x822 */
					uint16  AQMAggLenHi;		/* 0x824 */
					uint16  AQMIdxFifo;		/* 0x826 */
					uint16  AQMMpduLenFifo;		/* 0x828 */
					uint16  AQMTxCntFifo;		/* 0x82a */
					uint16  AQMUpdBA0;		/* 0x82c */
					uint16  AQMUpdBA1;		/* 0x82e */
					uint16  AQMUpdBA2;		/* 0x830 */
					uint16  AQMUpdBA3;		/* 0x832 */
					uint16  AQMAckCnt;		/* 0x834 */
					uint16  AQMConsCnt;		/* 0x836 */
					uint16  AQMFifoReady;		/* 0x838 */
					uint16  AQMStartLoc;		/* 0x83a */
					uint16	AQMRptr;		/* 0x83c */
					uint16	AQMTxcntRptr;		/* 0x83e */
				} lt64;
				struct {
					/* corerev >= 64. AQM block moved to 0xb60 */
					uint16  TXE_ctmode;		/* 0x800 */
					uint16  PAD[4];			/* 0x802 - 0x808 */
					uint16  TXE_SCM_HVAL_L;		/* 0x80a */
					uint16  TXE_SCM_HVAL_H;		/* 0x80c */
					uint16	TXE_SCT_HMASK_L;	/* 0x80e */
					uint16	TXE_SCT_HMASK_H;	/* 0x810 */
					uint16  TXE_SCT_HVAL_L;		/* 0x812 */
					uint16  TXE_SCT_HVAL_H;		/* 0x814 */
					uint16  TXE_SCX_HMASK_L;	/* 0x816 */
					uint16  TXE_SCX_HMASK_H;	/* 0x818 */
					uint16  PAD;			/* 0x81a */
					uint16  TXE_DebugBusCtrl;	/* 0x81c */
					uint16	BMC_ReadQID;		/* 0x81e */
					uint16	BMC_BQFrmCnt;		/* 0x820 */
					uint16	BMC_BQByteCnt_L;	/* 0x822 */
					uint16	BMC_BQByteCnt_H;	/* 0x824 */
					uint16	AQM_BQFrmCnt;		/* 0x826 */
					uint16	AQM_BQByteCnt_L;	/* 0x828 */
					uint16	AQM_BQByteCnt_H;	/* 0x82a */
					uint16	AQM_BQPrelWM;		/* 0x82c */
					uint16	AQM_BQPrelStatus;	/* 0x82e */
					uint16	AQM_BQStatus;		/* 0x830 */
					uint16	BMC_MUDebugConfig;	/* 0x832 */
					uint16	BMC_MUDebugStatus;	/* 0x834 */
					uint16	BMCBQCutThruSt0;	/* 0x836 */
					uint16	BMCBQCutThruSt1;	/* 0x838 */
					uint16	BMCBQCutThruSt2;	/* 0x83a */
					uint16	PAD[2];			/* 0x83c -x 0x83e */
				} ge64;
			} u0;
			uint16  TDCCTL;			/* 0x840 */
			uint16  TDC_Plcp0;		/* 0x842 */
			uint16  TDC_Plcp1;		/* 0x844 */
			uint16  TDC_FrmLen0;		/* 0x846 */
			uint16  TDC_FrmLen1;		/* 0x848 */
			uint16  TDC_Txtime;		/* 0x84a */
			uint16  TDC_VhtSigB0;		/* 0x84c */
			uint16  TDC_VhtSigB1;		/* 0x84e */
			uint16  TDC_LSigLen;		/* 0x850 */
			uint16  TDC_NSym0;		/* 0x852 */
			uint16  TDC_NSym1;		/* 0x854 */
			uint16  TDC_VhtPsduLen0;	/* 0x856 */
			uint16  TDC_VhtPsduLen1;	/* 0x858 */
			union {
				struct {
					/* Corerev < 64 */
					uint16  TDC_VhtMacPad;		/* 0x85a */
					uint16	PAD[1];
					uint16  AQMCurTxcnt;		/* 0x85e */
				} lt64;
				struct {
					/* Corerev >= 64 */
					uint16  TDC_VhtMacPad0;		/* 0x85a */
					uint16	TDC_VhtMacPad1;		/* 0x85c */
					uint16  TDC_MuVhtMCS;		/* 0x85e */
				} ge64;
			} u1;
			uint16  ShmDma_Ctl;		/* 0x860 */
			uint16  ShmDma_TxdcAddr;	/* 0x862 */
			uint16	ShmDma_ShmAddr;		/* 0x864 */
			uint16  ShmDma_XferCnt;		/* 0x866 */
			uint16  Txdc_Addr;		/* 0x868 */
			uint16  Txdc_Data;		/* 0x86a */
			uint16	PAD[10];		/* 0x86c - 0x880 */

			/* RXE Register */
			uint16	MHP_Status;		/* 0x880 */
			uint16	MHP_FC;			/* 0x882 */
			uint16	MHP_DUR;		/* 0x884 */
			uint16	MHP_SC;			/* 0x886 */
			uint16	MHP_QOS;		/* 0x888 */
			uint16	MHP_HTC_H;		/* 0x88a */
			uint16	MHP_HTC_L;		/* 0x88c */
			uint16	MHP_Addr1_H;		/* 0x88e */
			uint16	MHP_Addr1_M;		/* 0x890 */
			uint16	MHP_Addr1_L;		/* 0x892 */
			uint16	PAD[6];			/* 0x894 - 0x8a0 */
			uint16	MHP_Addr2_H;		/* 0x8a0 */
			uint16	MHP_Addr2_M;		/* 0x8a2 */
			uint16	MHP_Addr2_L;		/* 0x8a4 */
			uint16	MHP_Addr3_H;		/* 0x8a6 */
			uint16	MHP_Addr3_M;		/* 0x8a8 */
			uint16	MHP_Addr3_L;		/* 0x8aa */
			uint16	MHP_Addr4_H;		/* 0x8ac */
			uint16	MHP_Addr4_M;		/* 0x8ae */
			uint16	MHP_Addr4_L;		/* 0x8b0 */
			uint16	MHP_CFC;		/* 0x8b2 */
			uint16	PAD[6];			/* 0x8b4 - 0x8c0 */
			uint16	DAGG_CTL2;		/* 0x8c0 */
			uint16	DAGG_BYTESLEFT;		/* 0x8c2 */
			uint16	DAGG_SH_OFFSET;		/* 0x8c4 */
			uint16	DAGG_STAT;		/* 0x8c6 */
			uint16	DAGG_LEN;		/* 0x8c8 */
			uint16	TXBA_CTL;		/* 0x8ca */
			uint16	TXBA_DataSel;		/* 0x8cc */
			uint16	TXBA_Data;		/* 0x8ce */
			uint16	DAGG_LEN_THR;		/* 0x8d0 */
			uint16	PAD[7];			/* 0x8d2 - 0x8de */
			uint16	AMT_CTL;		/* 0x8e0 */
			uint16	AMT_Status;		/* 0x8e2 */
			uint16	AMT_Limit;		/* 0x8e4 */
			uint16	AMT_Attr;		/* 0x8e6 */
			uint16	AMT_Match1;		/* 0x8e8 */
			uint16	AMT_Match2;		/* 0x8ea */
			uint16	AMT_Table_Addr;		/* 0x8ec */
			uint16	AMT_Table_Data;		/* 0x8ee */
			uint16	AMT_Table_Val;		/* 0x8f0 */
			uint16	AMT_DBG_SEL;		/* 0x8f2 */
			uint16	PAD[6];			/* 0x8f4 - 0x8fe */
			uint16	RoeCtrl;		/* 0x900 */
			uint16	RoeStatus;		/* 0x902 */
			uint16	RoeIPChkSum;		/* 0x904 */
			uint16	RoeTCPUDPChkSum;	/* 0x906 */
			uint16	PAD[12];		/* 0x908 - 0x91e */
			uint16	PSOCtl;			/* 0x920 */
			uint16	PSORxWordsWatermark;	/* 0x922 */
			uint16	PSORxCntWatermark;	/* 0x924 */
			uint16	PAD[5];			/* 0x926 - 0x92e */
			uint16	OBFFCtl;		/* 0x930 */
			uint16	OBFFRxWordsWatermark;	/* 0x932 */
			uint16	OBFFRxCntWatermark;	/* 0x934 */
			uint16	PAD[4];			/* 0x936 - 0x93c */
			uint16	RCVHdrConvStats;	/* 0x93e */
			uint16	RCVHdrConvStats1;	/* 0x940 */
			uint16	RcvLB_DAGG_CTL;		/* 0x942 */
			uint16	RcvFifo0Len;		/* 0x944 */
			uint16	RcvFifo1Len;		/* 0x946 */
			uint16	PAD[92];		/* 0x948 - 0xa00 */

			/* TOE */
			uint16  ToECTL;			/* 0xa00 */
			uint16  ToERst;			/* 0xa02 */
			uint16  ToECSumNZ;		/* 0xa04 */
			uint16	PAD[29];		/* 0xa06 - 0xa3e */

			uint16  TxSerialCtl;		/* 0xa40 */
			uint16  TxPlcpLSig0;		/* 0xa42 */
			uint16  TxPlcpLSig1;		/* 0xa44 */
			uint16  TxPlcpHtSig0;		/* 0xa46 */
			uint16  TxPlcpHtSig1;		/* 0xa48 */
			uint16  TxPlcpHtSig2;		/* 0xa4a */
			uint16  TxPlcpVhtSigB0;		/* 0xa4c */
			uint16  TxPlcpVhtSigB1;		/* 0xa4e */
			uint16	PAD[1];

			uint16  MacHdrFromShmLen;	/* 0xa52 */
			uint16  TxPlcpLen;		/* 0xa54 */
			uint16	PAD[1];

			uint16  TxBFRptLen;		/* 0xa58 */
			uint16	PAD[3];

			uint16  TXBFCtl;		/* 0xa60 */
			uint16  BfmRptOffset;		/* 0xa62 */
			uint16  BfmRptLen;		/* 0xa64 */
			uint16  TXBFBfeRptRdCnt;	/* 0xa66 */
			uint16	PAD[20];		/* 0xa68 - 0xa8e */
			uint16	psm_reg_mux;		/* 0xa90 */
			uint16	PAD[7];			/* 0xa92 - 0xa9e */
			uint16	psm_base[14];		/* 0xaa0 - 0xaba */
			uint16	psm_base_x;		/* 0xabc */
			uint16	PAD[17];		/* 0xabe - 0xade */

			uint16  ClkGateReqCtrl0;	/* 0xae0 */
			uint16  ClkGateReqCtrl1;	/* 0xae2 */
			uint16  ClkGateReqCtrl2;	/* 0xae4 */
			uint16  ClkGateUcodeReq0;	/* 0xae6 */
			uint16  ClkGateUcodeReq1;	/* 0xae8 */
			uint16  ClkGateUcodeReq2;	/* 0xaea */
			uint16	ClkGateStretch0;	/* 0xaec */
			uint16	ClkGateStretch1;	/* 0xaee */
			uint16	ClkGateMisc;		/* 0xaf0 */
			uint16	ClkGateDivCtrl;		/* 0xaf2 */
			uint16	ClkGatePhyClkCtrl;	/* 0xaf4 */
			uint16	ClkGateSts;		/* 0xaf6 */
			uint16	ClkGateExtReq0;		/* 0xaf8 */
			uint16	ClkGateExtReq1;		/* 0xafa */
			uint16	ClkGateExtReq2;		/* 0xafc */
			uint16	ClkGateUcodePhyClkCtrl;	/* 0xafe */

			uint16  RXMapFifoSize;          /* 0xb00 */
			uint16  RXMapStatus;          /* 0xb02 */
			uint16  MsduThreshold;		 /* 0xb04 */
			uint16	PAD[4];			/* 0xb06 - 0xb0c */
			uint16  BMCCore0TXAllMaxBuffers; /* 0xb0e */
			uint16  BMCCore1TXAllMaxBuffers; /* 0xb10 */
			uint16  BMCDynAllocStatus1;		 /* 0xb12 */

			/* Corerev >= 50, empty otherwise */
			uint16	TXE_DMAMaxOutStBuffers;		/* 0xb14 */
			uint16	TXE_SCS_MASK_L;	/* 0xb16 - SampleCollectStoreMaskLo */
			uint16	TXE_SCS_MASK_H;	/* 0xb18 - SampleCollectStoreMaskHi */
			uint16	TXE_SCM_MASK_L;	/* 0xb1a - SampleCollectMatchMaskLo */
			uint16	TXE_SCM_MASK_H;	/* 0xb1c - SampleCollectMatchMaskHi */
			uint16	TXE_SCM_VAL_L;	/* 0xb1e - SampleCollectMatchValueLo */
			uint16	TXE_SCM_VAL_H;	/* 0xb20 - SampleCollectMatchValueHi */
			uint16	TXE_SCT_MASK_L;	/* 0xb22 - SampleCollectTriggerMaskLo */
			uint16	TXE_SCT_MASK_H;	/* 0xb24 - SampleCollectTriggerMaskHi */
			uint16	TXE_SCT_VAL_L;	/* 0xb26 - SampleCollectTriggerValueLo */
			uint16	TXE_SCT_VAL_H;	/* 0xb28 - SampleCollectTriggerValueHi */
			uint16	TXE_SCX_MASK_L;	/* 0xb2a - SampleCollectTransMaskLo */
			uint16	TXE_SCX_MASK_H;	/* 0xb2c - SampleCollectTransMaskHi */
			/* End CoreRev >= 50 Block */

			uint16  SampleCollectPlayCtrl; /* 0xb2e */
			uint16  Core0BMCAllocStatusTID7;	/* b30 */
			uint16  Core1BMCAllocStatusTID7;	/* b32 */
			uint16	PAD[1];			/* 0xb34 */
			uint16  BmcVpConfig;		/* 0xb36 */
			uint16  PAD[1];			/* 0xb38 */
			uint16	XmtTemplatePtrOffset;	/* b3a */
			uint16	PAD[6];			/* 0xb3c - 0xb46 */
			uint16	SysMStartAddrHi;	/* 0xb48 */
			uint16	SysMStartAddrLo;	/* 0xb4a */
			uint16	PAD[12];		/* 0xb4c - 0xb62 */

			/* AQM block for corerev >= 64, empty otherwise */
			uint16	AQM_REG_SEL;		/* 0xb64 */
			uint16	AQMQMAP;		/* 0xb66 */
			uint16	AQMCmd;			/* 0xb68 */
			uint16	AQMConsMsdu;		/* 0xb6a */
			uint16	AQMDMACTL;		/* 0xb6c */
			uint16	AQMMaxIdx;		/* 0xb6e */
			uint16	AQMRcvdBA0;		/* 0xb70 */
			uint16	AQMRcvdBA1;		/* 0xb72 */
			uint16	AQMRcvdBA2;		/* 0xb74 */
			uint16	AQMRcvdBA3;		/* 0xb76 */
			uint16	AQMBaSSN;		/* 0xb78 */
			uint16	AQMRefSN;		/* 0xb7a */
			uint16	AQMMaxAggLenLow;	/* 0xb7c */
			uint16	AQMMaxAggLenHi;		/* 0xb7e */
			uint16	AQMAggParams;		/* 0xb80 */
			uint16	AQMMinMpduLen;		/* 0xb82 */
			uint16	AQMMacAdjLen;		/* 0xb84 */
			uint16	AQMMinCons;		/* 0xb86 */
			uint16	MsduMinCons;		/* 0xb88 */
			uint16	AQMAggStats;		/* 0xb8a */
			uint16	AQMAggNum;		/* 0xb8c */
			uint16	AQMAggLenLow;		/* 0xb8e */
			uint16	AQMAggLenHi;		/* 0xb90 */
			uint16	AQMMpduLen;		/* 0xb92 */
			uint16	AQMStartLoc;		/* 0xb94 */
			uint16	AQMAggEntry;		/* 0xb96 */
			uint16	AQMAggIdx;		/* 0xb98 */
			uint16	AQMTxCnt;		/* 0xb9a */
			uint16	AQMAggRptr;		/* 0xb9c */
			uint16	AQMTxcntRptr;		/* 0xb9e */
			uint16	AQMCurTxcnt;		/* 0xba0 */
			uint16	AQMFiFoRptr;		/* 0xba2 */
			uint16	AQMFIFO_SOFDPtr;	/* 0xba4 */
			uint16	AQMFIFO_SWDCnt;		/* 0xba6 */
			uint16	AQMFIFO_TXDPtr_L;	/* 0xba8 */
			uint16	AQMFIFO_TXDPtr_ML;	/* 0xbaa */
			uint16	AQMFIFO_TXDPtr_MU;	/* 0xbac */
			uint16	AQMFIFO_TXDPtr_H;	/* 0xbae */
			uint16	AQMUpdBA0;		/* 0xbb0 */
			uint16	AQMUpdBA1;		/* 0xbb2 */
			uint16	AQMUpdBA2;		/* 0xbb4 */
			uint16	AQMUpdBA3;		/* 0xbb6 */
			uint16	AQMAckCnt;		/* 0xbb8 */
			uint16	AQMConsCnt;		/* 0xbba */
			uint16	AQMFifoRdy_L;		/* 0xbbc */
			uint16	AQMFifoRdy_H;		/* 0xbbe */
			uint16	AQMFifo_Status;		/* 0xbc0 */
			uint16	AQMFifoFull_L;		/* 0xbc2 */
			uint16	AQMFifoFull_H;		/* 0xbc4 */
			uint16	AQMTBCP_Busy_L;		/* 0xbc6 */
			uint16	AQMTBCP_Busy_H;		/* 0xbc8 */
			uint16	AQMDMA_SuspFlush;	/* 0xbca */
			uint16	AQMFIFOSusp_L;		/* 0xbcc */
			uint16	AQMFIFOSusp_H;		/* 0xbce */
			uint16	AQMFIFO_SuspPend_L;	/* 0xbd0 */
			uint16	AQMFIFO_SuspPend_H;	/* 0xbd2 */
			uint16	AQMFIFOFlush_L;		/* 0xbd4 */
			uint16	AQMFIFOFlush_H;		/* 0xbd6 */
			uint16	AQMTXD_CTL;		/* 0xbd8 */
			uint16	AQMTXD_RdOffset;	/* 0xbda */
			uint16	AQMTXD_RdLen;		/* 0xbdc */
			uint16	AQMTXD_DestAddr;	/* 0xbde */
			uint16	AQMTBCP_QSEL;		/* 0xbe0 */
			uint16	AQMTBCP_Prio;		/* 0xbe2 */
			uint16	AQMTBCP_PrioFifo;	/* 0xbe4 */
			uint16	AQMFIFO_MPDULen;	/* 0xbe6 */
			uint16	AQMTBCP_Max_ReqEntry;	/* 0xbe8 */
			uint16	AQMTBCP_FCNT;		/* 0xbea */
			uint16	PAD[6];			/* 0xbec - 0xbf6 */
			uint16	AQMSTATUS;		/* 0xbf8 */
			uint16	AQMDBG_CTL;		/* 0xbfa */
			uint16	AQMDBG_DATA;		/* 0xbfc */
			uint16	PAD[385];		/* 0xbfe - 0xEFE */
		} d11acregs;
	} u;
	/* SB configuration registers: 0xF00 */
	sbconfig_t	sbconfig;	/**< sb config regs occupy top 256 bytes */
} d11regs_t;

#endif	/* _D11REGS_H */
