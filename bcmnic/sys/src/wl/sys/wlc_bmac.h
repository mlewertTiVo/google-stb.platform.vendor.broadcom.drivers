/*
 * Common (OS-independent) portion of
 * Broadcom 802.11bang Networking Device Driver
 *
 * BMAC driver external interface
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id: wlc_bmac.h 613545 2016-01-19 08:03:39Z $
 */


/* XXXXX this interface is under wlc.c by design
 * http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/WlBmacDesign
 *
 *        high driver files(e.g. wlc_ampdu.c wlc_ccx.c etc)
 *             wlc.h/wlc.c
 *         wlc_bmac.h/wlc_bmac.c
 *
 *  So don't include this in files other than wlc.c, wlc_bmac* wl_rte.c(dongle port) and wl_phy.c
 *  create wrappers in wlc.c if needed
 */

#ifndef _WLC_BMAC_H_
#define _WLC_BMAC_H_

#define WOWL_GPIO_INVALID_VALUE	0xFF

/* For 4349 core revision 50 */
#define D11CORE_TEMPLATE_REG_OFFSET OFFSETOF(d11regs_t, tplatewrptr)

/* BCMECICOEX support */

#define BCMECICOEX_ENAB(wlc)            (0)
#define BCMECICOEX_ENAB_BMAC(wlc_hw)	(0)
#define BCMSECICOEX_ENAB_BMAC(wlc_hw)	(0)
#define BCMGCICOEX_ENAB_BMAC(wlc_hw)	(0)
#define BCMCOEX_ENAB_BMAC(wlc_hw) 	(0)
#define NOTIFY_BT_CHL(sih, val) {}
#define NOTIFY_BT_BW_20(sih) {}
#define NOTIFY_BT_BW_40(sih) {}
#define NOTIFY_BT_NUM_ANTENNA(sih, val) {}
#define NOTIFY_BT_TXRX(sih, val) {}

/* Used to indicate the type of ucode */
typedef enum {
	D11_IF_SHM_STD = 0,
	D11_IF_SHM_WOWL = 1,
	D11_IF_SHM_ULP = 2
} d11_if_shm_type_t;
extern void wlc_bmac_autod11_shm_upd(wlc_hw_info_t *wlc_hw, uint8 ucodeType);
extern void wlc_bmac_update_bt_chanspec(wlc_hw_info_t *wlc_hw,
	chanspec_t chanspec, bool scan_in_progress, bool roam_in_progress);

/** Revision and other info required from BMAC driver for functioning of high ONLY driver */
typedef struct wlc_bmac_revinfo {
	uint		vendorid;	/**< PCI vendor id */
	uint		deviceid;	/**< device id of chip */

	uint		boardrev;	/**< version # of particular board */
	uint		corerev;	/**< core revision */
	uint		sromrev;	/**< srom revision */
	uint		chiprev;	/**< chip revision */
	uint		chip;		/**< chip number */
	uint		chippkg;	/**< chip package */
	uint		boardtype;	/**< board type */
	uint		boardvendor;	/**< board vendor */
	uint		bustype;	/**< SB_BUS, PCI_BUS  */
	uint		buscoretype;	/**< PCI_CORE_ID, PCIE_CORE_ID, PCMCIA_CORE_ID */
	uint		buscorerev; 	/**< buscore rev */
	uint32		issim;		/**< chip is in simulation or emulation */

	uint		nbands;
	uint		boardflags; /* boardflags */
	uint	        boardflags2; /* boardflags2 */
	uint		boardflags4; /* boardflags4 */

	struct band_info {
		uint bandunit; /**< To match on both sides */
		uint bandtype; /**< To match on both sides */
		uint radiorev;
		uint phytype;
		uint phyrev;
		uint anarev;
		uint radioid;
		uint phy_minor_rev;
	} band[MAXBANDS];

	/* put flags that advertize BMAC capabilities to high mac here  */
	uint32 _wlsrvsdb;		/**< 'true' if bmac is capable of srvsdb operation */
	uint		ampdu_ba_rx_wsize;
	uint		host_rpc_agg_size;
	uint		ampdu_mpdu;
	bool        is_ss; /**< bus support for super speed */

    uint32      wowl_gpio;
    uint32      wowl_gpiopol;
} wlc_bmac_revinfo_t;

/** dup state between BMAC(wlc_hw_info_t) and HIGH(wlc_info_t) driver */
typedef struct wlc_bmac_state {
	uint32		machwcap;	/**< mac hw capibility */
	uint32		preamble_ovr;	/**< preamble override */
} wlc_bmac_state_t;

typedef enum {
	WLCHW_STATE_ATTACH,
	WLCHW_STATE_CLK,
	WLCHW_STATE_UP,
	WLCHW_STATE_ASSOC,
	WLCHW_STATE_LAST
} wlc_bmac_state_id_t;

/** CCA and OBSS stats */
typedef struct {
	uint32 txdur;	/**< num usecs tx'ing */
	uint32 ibss;	/**< num usecs rx'ing cur bss */
	uint32 obss;	/**< num usecs rx'ing other data */
	uint32 noctg;	/**< 802.11 of unknown type */
	uint32 nopkt;	/**< non 802.11 */
	uint32 usecs;	/**< usecs in this interval */
	uint32 PM;	/**< usecs MAC spent in doze mode for PM */

	uint32 crsglitch;	/**< num rxcrsglitchs */
	uint32 badplcp;		/**< num bad plcp */
	uint32 bphy_crsglitch;  /**< num bphy rxcrsglitchs */
	uint32 bphy_badplcp;    /**< num bphy bad plcp */

	uint32 txopp;		/**< Slot counter */
	uint32 gdtxdur;		/**< Good Tx usecs */
	uint32 bdtxdur;		/**< Bad Tx usecs */
	uint32 slot_time_txop;	/**< Slot Time For txopp conversion */

	uint32 rxdrop20s;		/**< rx sec dropped */
	uint32 rx20s;			/**< rx sec pkts */
	uint32 sec_rssi_hist_hi;	/**< rx sec rssi high histogram */
	uint32 sec_rssi_hist_med;	/**< rx sec rssi medium histogram */
	uint32 sec_rssi_hist_low;	/**< rx sec rssi low histogram */
	uint32 rxcrs_pri;		/**< rx crs primary */
	uint32 rxcrs_sec20;		/**< rx crs secondary 20 */
	uint32 rxcrs_sec40;		/**< rx crs secondary 40 */

	uint32 suspend;			/**< usecs ucode suspended */
	uint32 suspend_cnt;		/**< num ucode suspended */
	uint32 txfrm;			/**< num txing */
	uint32 rxstrt;			/**< num rxing */
	uint32 rxglitch;		/**< num rx glitching */
} wlc_bmac_obss_counts_t;

typedef struct wlc_stf_rs_shm_offset {
	uint8 rate[WLC_NUMRATES];
	uint16 val[WLC_NUMRATES];
} wlc_stf_rs_shm_offset_t;

extern int wlc_bmac_attach(wlc_info_t *wlc, uint16 vendor, uint16 device, uint unit,
	bool piomode, osl_t *osh, volatile void *regsva, uint bustype, void *btparam, uint macunit);
extern int wlc_bmac_detach(wlc_info_t *wlc);

extern si_t * wlc_bmac_si_attach(uint device, osl_t *osh, volatile void *regs, uint bustype,
	void *btparam, char **vars, uint *varsz);
extern void wlc_bmac_si_detach(osl_t *osh, si_t *sih);

extern void wlc_bmac_watchdog(void *arg);
extern void wlc_bmac_rpc_agg_watchdog(void *arg);
extern void wlc_bmac_info_init(wlc_hw_info_t *wlc_hw);

/* up/down, reset, clk */
extern void wlc_bmac_xtal(wlc_hw_info_t* wlc_hw, bool want);

extern void wlc_bmac_copyto_objmem(wlc_hw_info_t *wlc_hw,
                                   uint offset, const void* buf, int len, uint32 sel);
extern void wlc_bmac_copyfrom_objmem(wlc_hw_info_t *wlc_hw,
                                     uint offset, void* buf, int len, uint32 sel);
#define wlc_bmac_copyfrom_shm(wlc_hw, offset, buf, len)                 \
	wlc_bmac_copyfrom_objmem(wlc_hw, offset, buf, len, OBJADDR_SHM_SEL)
#define wlc_bmac_copyto_shm(wlc_hw, offset, buf, len)                   \
	wlc_bmac_copyto_objmem(wlc_hw, offset, buf, len, OBJADDR_SHM_SEL)

#ifdef WL_HWKTAB
extern void wlc_bmac_copyfrom_objmem32(wlc_hw_info_t *wlc_hw, uint offset, uint8 *buf,
	int len, uint32 sel);
extern void wlc_bmac_copyto_objmem32(wlc_hw_info_t *wlc_hw, uint offset, const uint8 *buf,
	int len, uint32 sel);
extern void wlc_bmac_set_objmem32(wlc_hw_info_t *wlc_hw, uint offset, uint32 val,
	int len, uint32 sel);
#else
#define wlc_bmac_copyfrom_objmem32(wlc_hw, offset, buf, len, sel) do {} while (0)
#define wlc_bmac_copyto_objmem32(wlc_hw, offset, buf, len, sel) do {} while (0)
#define wlc_bmac_set_objmem32(wlc_hw, offset, val, len, sel) do {} while (0)
#endif /* WL_HWKTAB */

extern void wlc_bmac_core_phy_clk(wlc_hw_info_t *wlc_hw, bool clk);
extern void wlc_bmac_core_phypll_reset(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_core_phypll_ctl(wlc_hw_info_t* wlc_hw, bool on);
extern void wlc_bmac_phyclk_fgc(wlc_hw_info_t *wlc_hw, bool clk);
extern void wlc_bmac_macphyclk_set(wlc_hw_info_t *wlc_hw, bool clk);
extern void wlc_bmac_phy_reset(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_bw_reset(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_corereset(wlc_hw_info_t *wlc_hw, uint32 flags);
extern void wlc_bmac_reset(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_init(wlc_hw_info_t *wlc_hw, chanspec_t chanspec, bool mute,
	uint32 defmacintmask);
extern int wlc_bmac_up_prep(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_up_finish(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_set_ctrl_ePA(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_set_ctrl_SROM(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_set_ctrl_bt_shd0(wlc_hw_info_t *wlc_hw, bool enable);
extern int wlc_bmac_set_btswitch(wlc_hw_info_t *wlc_hw, int8 state);

#ifdef BCMNODOWN
#define wlc_bmac_down_prep(wlc_hw) (BCME_OK)
#define wlc_bmac_down_finish(wlc_hw) (BCME_OK)
#else
extern int wlc_bmac_down_prep(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_down_finish(wlc_hw_info_t *wlc_hw);
#endif

extern void wlc_bmac_corereset(wlc_hw_info_t *wlc_hw, uint32 flags);
extern void wlc_bmac_switch_macfreq(wlc_hw_info_t *wlc_hw, uint8 spurmode);
extern int wlc_bmac_4331_epa_init(wlc_hw_info_t *wlc_hw);
extern void wlc_mctrl_reset(wlc_hw_info_t *wlc_hw);

/* chanspec, ucode interface */
extern int wlc_bmac_bandtype(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_set_chanspec(wlc_hw_info_t *wlc_hw, chanspec_t chanspec, bool mute,
	ppr_t *txpwr);

extern void wlc_bmac_txfifo(wlc_hw_info_t *wlc_hw, uint fifo, void *p, bool commit, uint16 frameid,
	uint8 txpktpend);
extern int wlc_bmac_xmtfifo_sz_get(wlc_hw_info_t *wlc_hw, uint fifo, uint *blocks);


#ifdef PHYCAL_CACHING
extern void wlc_bmac_set_phycal_cache_flag(wlc_hw_info_t *wlc_hw, bool state);
extern bool wlc_bmac_get_phycal_cache_flag(wlc_hw_info_t *wlc_hw);
#endif

extern void wlc_bmac_phy_txpwr_cache_invalidate(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_mhf(wlc_hw_info_t *wlc_hw, uint8 idx, uint16 mask, uint16 val, int bands);
extern void wlc_bmac_mctrl(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 val);
extern uint16 wlc_bmac_mhf_get(wlc_hw_info_t *wlc_hw, uint8 idx, int bands);
extern int  wlc_bmac_xmtfifo_sz_set(wlc_hw_info_t *wlc_hw, uint fifo, uint16 blocks);
extern void wlc_bmac_txant_set(wlc_hw_info_t *wlc_hw, uint16 phytxant);
extern uint16 wlc_bmac_get_txant(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_antsel_type_set(wlc_hw_info_t *wlc_hw, uint8 antsel_type);
extern int  wlc_bmac_revinfo_get(wlc_hw_info_t *wlc_hw, wlc_bmac_revinfo_t *revinfo);
extern int  wlc_bmac_state_get(wlc_hw_info_t *wlc_hw, wlc_bmac_state_t *state);
extern void wlc_bmac_write_shm(wlc_hw_info_t *wlc_hw, uint offset, uint16 v);
extern void wlc_bmac_update_shm(wlc_hw_info_t *wlc_hw, uint offset, uint16 v, uint16 mask);
extern uint16 wlc_bmac_read_shm(wlc_hw_info_t *wlc_hw, uint offset);
extern void wlc_bmac_set_shm(wlc_hw_info_t *wlc_hw, uint offset, uint16 v, int len);
extern void wlc_bmac_write_template_ram(wlc_hw_info_t *wlc_hw, int offset, int len, void *buf);
extern void wlc_bmac_write_template_const_ram(wlc_hw_info_t *wlc_hw, int offset, int len,
		const void *buf);
extern void wlc_bmac_templateptr_wreg(wlc_hw_info_t *wlc_hw, int offset);
extern uint32 wlc_bmac_templateptr_rreg(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_templatedata_wreg(wlc_hw_info_t *wlc_hw, uint32 word);
extern uint32 wlc_bmac_templatedata_rreg(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_copyfrom_vars(wlc_hw_info_t *wlc_hw, char ** buf, uint *len);
extern void wlc_bmac_enable_mac(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_update_rxpost_rxbnd(wlc_hw_info_t *wlc_hw, uint8 nrxpost, uint8 rxbnd);
extern void wlc_bmac_suspend_mac_and_wait(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_ampdu_set(wlc_hw_info_t *wlc_hw, uint8 mode);

extern void wlc_bmac_process_ps_switch(wlc_hw_info_t *wlc, struct ether_addr *ea, int8 ps_on);
extern void wlc_bmac_hw_etheraddr(wlc_hw_info_t *wlc_hw, struct ether_addr *ea);
extern void wlc_bmac_set_hw_etheraddr(wlc_hw_info_t *wlc_hw, struct ether_addr *ea);
extern bool wlc_bmac_validate_chip_access(wlc_hw_info_t *wlc_hw);

extern bool wlc_bmac_radio_read_hwdisabled(wlc_hw_info_t* wlc_hw);
extern void wlc_bmac_set_shortslot(wlc_hw_info_t *wlc_hw, bool shortslot);
extern void wlc_bmac_mute(wlc_hw_info_t *wlc_hw, bool want, mbool flags);
extern void wlc_bmac_set_deaf(wlc_hw_info_t *wlc_hw, bool user_flag);


#ifdef WL11N
extern void wlc_bmac_band_stf_ss_set(wlc_hw_info_t *wlc_hw, uint8 stf_mode);
extern void wlc_bmac_txbw_update(wlc_hw_info_t *wlc_hw);
#endif

extern void wlc_bmac_filter_war_upd(wlc_hw_info_t *wlc_hw, bool set);

extern int wlc_bmac_btc_mode_set(wlc_hw_info_t *wlc_hw, int btc_mode);
extern int wlc_bmac_btc_mode_get(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_btc_wire_set(wlc_hw_info_t *wlc_hw, int btc_wire);
extern int wlc_bmac_btc_wire_get(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_btc_flags_idx_set(wlc_hw_info_t *wlc_hw, int int_val, int int_val2);
extern int wlc_bmac_btc_flags_idx_get(wlc_hw_info_t *wlc_hw, int val);
extern int wlc_bmac_btc_flags_get(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_btc_params_set(wlc_hw_info_t *wlc_hw, int int_val, int int_val2);
extern int wlc_bmac_btc_params_get(wlc_hw_info_t *wlc_hw, int int_val);
extern int wlc_bmac_btc_period_get(wlc_hw_info_t *wlc_hw, uint16 *btperiod, bool *btactive,
	uint16 *agg_off_bm, uint16 *acl_last_ts, uint16 *a2dp_last_ts);
extern void wlc_bmac_btc_rssi_threshold_get(wlc_hw_info_t *wlc_hw, uint8 *, uint8 *, uint8 *);
extern void wlc_bmac_btc_stuck_war50943(wlc_hw_info_t *wlc_hw, bool enable);
extern void wlc_bmac_wait_for_wake(wlc_hw_info_t *wlc_hw);
extern bool wlc_bmac_tx_fifo_suspended(wlc_hw_info_t *wlc_hw, uint tx_fifo);
extern void wlc_bmac_tx_fifo_suspend(wlc_hw_info_t *wlc_hw, uint tx_fifo);
extern void wlc_bmac_tx_fifo_resume(wlc_hw_info_t *wlc_hw, uint tx_fifo);
extern int  wlc_bmac_activate_srvsdb(wlc_hw_info_t *wlc_hw, chanspec_t chan0, chanspec_t chan1);
extern void wlc_bmac_deactivate_srvsdb(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_tsf_adjust(wlc_hw_info_t *wlc_hw, int delta);
extern int  wlc_bmac_srvsdb_force_set(wlc_hw_info_t *wlc_hw, uint8 force);

extern void wlc_bmac_tx_fifo_sync(wlc_hw_info_t *wlc_hw, uint fifo_bitmap, uint8 flag);

#ifdef STA
#if defined(WLRM)
extern void wlc_bmac_rm_cca_measure(wlc_hw_info_t *wlc_hw, uint32 us);
extern void wlc_bmac_rm_cca_int(wlc_hw_info_t *wlc_hw);
#endif
#endif /* STA */

extern void wlc_ucode_wake_override_set(wlc_hw_info_t *wlc_hw, uint32 override_bit);
extern void wlc_ucode_wake_override_clear(wlc_hw_info_t *wlc_hw, uint32 override_bit);
extern void wlc_upd_suspended_fifos_set(wlc_hw_info_t *wlc_hw, uint txfifo);
extern void wlc_upd_suspended_fifos_clear(wlc_hw_info_t *wlc_hw, uint txfifo);

extern void wlc_bmac_set_rcmta(wlc_hw_info_t *wlc_hw, int idx, const struct ether_addr *addr);
extern uint16 wlc_bmac_write_amt(wlc_hw_info_t *wlc_hw, int idx,
	const struct ether_addr *addr, uint16 attr);

extern void wlc_bmac_read_amt(wlc_hw_info_t *wlc_hw, int idx,
	struct ether_addr *addr, uint16 *attr);
void
wlc_bmac_get_rcmta(wlc_hw_info_t *wlc_hw, int idx, struct ether_addr *addr);

extern void wlc_bmac_set_rxe_addrmatch(wlc_hw_info_t *wlc_hw,
	int match_reg_offset, const struct ether_addr *addr);
extern void wlc_bmac_write_hw_bcntemplates(wlc_hw_info_t *wlc_hw, void *bcn, int len, bool both);

extern void wlc_bmac_read_tsf(wlc_hw_info_t* wlc_hw, uint32* tsf_l_ptr, uint32* tsf_h_ptr);
extern uint32 wlc_bmac_read_usec_timer(wlc_hw_info_t* wlc_hw);
extern void wlc_bmac_set_cwmin(wlc_hw_info_t *wlc_hw, uint16 newmin);
extern void wlc_bmac_set_cwmax(wlc_hw_info_t *wlc_hw, uint16 newmax);
extern void wlc_bmac_set_noreset(wlc_hw_info_t *wlc, bool noreset_flag);
extern bool wlc_bmac_get_noreset(wlc_hw_info_t *wlc);

#ifdef WL_PROXDETECT
extern void wlc_enable_avb_timer(wlc_hw_info_t *wlc_hw, bool enable);
extern void wlc_get_avb_timer_reg(wlc_hw_info_t *wlc_hw, uint32 *clkst, uint32 *maccontrol1);
extern void wlc_get_avb_timestamp(wlc_hw_info_t *wlc_hw, uint32* ptx, uint32* prx);
#endif

/* is the h/w capable of p2p? */
extern bool wlc_bmac_p2p_cap(wlc_hw_info_t *wlc_hw);
/* load p2p ucode */
extern int wlc_bmac_p2p_set(wlc_hw_info_t *wlc_hw, bool enable);
/* p2p ucode provides multiple connection support. */
#define wlc_bmac_mcnx_cap(hw) wlc_bmac_p2p_cap(hw)
#define wlc_bmac_mcnx_enab(hw, en) wlc_bmac_p2p_set(hw, en)
extern void wlc_bmac_retrylimit_upd(wlc_hw_info_t *wlc_hw, uint16 SRL, uint16 LRL);
extern void wlc_bmac_fifoerrors(wlc_hw_info_t *wlc_hw);
/* API for BMAC driver (e.g. wlc_phy.c etc) */

extern void wlc_bmac_bw_set(wlc_hw_info_t *wlc_hw, uint16 bw);
extern int wlc_bmac_bw_check(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_pllreq(wlc_hw_info_t *wlc_hw, bool set, mbool req_bit);
extern void wlc_bmac_set_clk(wlc_hw_info_t *wlc_hw, bool on);
extern bool wlc_bmac_taclear(wlc_hw_info_t *wlc_hw, bool ta_ok);
extern void wlc_bmac_hw_up(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_hw_down(wlc_hw_info_t *wlc_hw);
extern bool wlc_bmac_txfifo_reset(wlc_hw_info_t *wlc_hw, uint tx_fifo);

#ifdef WLLED
extern void wlc_bmac_led_hw_deinit(wlc_hw_info_t *wlc_hw, uint32 gpiomask_cache);
extern void wlc_bmac_led_hw_mask_init(wlc_hw_info_t *wlc_hw, uint32 mask);
extern bmac_led_info_t *wlc_bmac_led_attach(wlc_hw_info_t *wlc_hw);
extern int  wlc_bmac_led_detach(wlc_hw_info_t *wlc_hw);
extern int  wlc_bmac_led_blink_event(wlc_hw_info_t *wlc_hw, bool blink);
extern void wlc_bmac_led_set(wlc_hw_info_t *wlc_hw, int indx, uint8 activehi);
extern void wlc_bmac_led_blink(wlc_hw_info_t *wlc_hw, int indx, uint16 msec_on, uint16 msec_off);
extern void wlc_bmac_led(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 val, bool activehi);
extern void wlc_bmac_blink_sync(wlc_hw_info_t *wlc_hw, uint32 led_pins);
#endif

int wlc_bmac_dispatch_iov(wlc_hw_info_t *wlc_hw, uint16 tid, uint32 aid, uint16 type,
	void *p, uint p_len, void *a, int a_len, int vsize);
int wlc_bmac_dispatch_ioc(wlc_hw_info_t *wlc_hw, uint16 tid, uint16 cid, uint16 type,
	void *a, uint alen, bool *ta);

int wlc_bmac_dump(wlc_hw_info_t *wlc_hw, const char *name, struct bcmstrbuf *b);
int wlc_bmac_dump_clr(wlc_hw_info_t *wlc_hw, const char *name);

#if defined(WLPKTENG)
extern int wlc_bmac_pkteng(wlc_hw_info_t *wlc_hw, wl_pkteng_t *pkteng, void* p);
#endif 

extern void wlc_gpio_fast_deinit(wlc_hw_info_t *wlc_hw);

#ifdef SAMPLE_COLLECT
extern void wlc_ucode_sample_init(wlc_hw_info_t *wlc_hw);
#endif

extern bool wlc_bmac_radio_hw(wlc_hw_info_t *wlc_hw, bool enable, bool skip_anacore);
extern uint16 wlc_bmac_rate_shm_offset(wlc_hw_info_t *wlc_hw, uint8 rate);
extern void wlc_bmac_stf_set_rateset_shm_offset(wlc_hw_info_t *wlc_hw, uint count, uint16 pos,
  uint16 mask, wlc_stf_rs_shm_offset_t *stf_rs);

#ifdef WLEXTLOG
extern void wlc_bmac_extlog_cfg_set(wlc_hw_info_t *wlc_hw, wlc_extlog_cfg_t *cfg);
#endif

extern void wlc_bmac_assert_type_set(wlc_hw_info_t *wlc_hw, uint32 type);
extern void wlc_bmac_set_txpwr_percent(wlc_hw_info_t *wlc_hw, uint8 val);
extern void wlc_bmac_ifsctl_edcrs_set(wlc_hw_info_t *wlc_hw, bool isht);


struct cca_ucode_counts {
	uint32 txdur;	/**< num usecs tx'ing */
	uint32 ibss;	/**< num usecs rx'ing cur bss */
	uint32 obss;	/**< num usecs rx'ing other data */
	uint32 noctg;	/**< 802.11 of unknown type */
	uint32 nopkt;	/**< non 802.11 */
	uint32 usecs;	/**< usecs in this interval */
	uint32 PM;	/**< usecs MAC spent in doze mode for PM */
#ifdef ISID_STATS
	uint32 crsglitch;	/**< num rxcrsglitchs */
	uint32 badplcp;		/**< num bad plcp */
	uint32 bphy_crsglitch;  /**< num bphy rxcrsglitchs */
	uint32 bphy_badplcp;    /**< num bphy bad plcp */
#endif /* ISID_STATS */
};

extern int wlc_bmac_cca_stats_read(wlc_hw_info_t *wlc_hw, cca_ucode_counts_t *cca_counts);
extern int wlc_bmac_obss_stats_read(wlc_hw_info_t *wlc_hw, wlc_bmac_obss_counts_t *obss_counts);
extern uint32 wlc_bmac_cca_read_counter(wlc_hw_info_t* wlc_hw, int lo_off, int hi_off);
extern uint32 wlc_bmac_read_counter(wlc_hw_info_t* wlc_hw, uint baseaddr, int lo_off, int hi_off);
extern void wlc_bmac_antsel_set(wlc_hw_info_t *wlc_hw, uint32 antsel_avail);

extern void wlc_bmac_write_ihr(wlc_hw_info_t *wlc_hw, uint offset, uint16 v);

#ifdef STA
extern void wlc_bmac_pcie_power_save_enable(wlc_hw_info_t *wlc_hw, bool enable);
extern void wlc_bmac_pcie_war_ovr_update(wlc_hw_info_t *wlc_hw, uint8 aspm);
#endif

extern void wlc_bmac_minimal_radio_hw(wlc_hw_info_t *wlc_hw, bool enable);
extern bool wlc_bmac_si_iscoreup(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_wowlucode_start(wlc_hw_info_t *wlc_hw);

#ifdef WOWL
extern int wlc_bmac_wowlucode_init(wlc_hw_info_t *wlc_hw);
extern int wlc_bmac_write_inits(wlc_hw_info_t *wlc_hw,  void *inits, int len);
extern int wlc_bmac_wakeucode_dnlddone(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_dngldown(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_wowl_config_4331_5GePA(wlc_hw_info_t *wlc_hw, bool is_5G, bool is_4331_12x9);
#endif /* WOWL */

extern int wlc_bmac_process_ucode_sr(uint16 device, osl_t *osh, volatile void *regsva,
	uint bustype, void *btparam);

extern bool wlc_bmac_recv(wlc_hw_info_t *wlc_hw, uint fifo, bool bound, wlc_dpc_info_t *dpc);

#ifdef WLP2P_UCODE
extern void wlc_p2p_bmac_int_proc(wlc_hw_info_t *wlc_hw);
#else
#define wlc_p2p_bmac_int_proc(wlc_hw) do {} while (FALSE)
#endif

#ifdef STA
extern void wlc_bmac_btc_update_predictor(wlc_hw_info_t *wlc_hw);
#endif

extern bool wlc_bmac_processpmq(wlc_hw_info_t *wlc, bool bounded);
extern bool wlc_bmac_txstatus(wlc_hw_info_t *wlc_hw, bool bound, bool *fatal);
extern bool wlc_bmac_txstatus_shm(wlc_hw_info_t *wlc_hw, bool bound, bool *fatal);

#ifdef DMATXRC
extern void wlc_dmatx_reclaim(wlc_hw_info_t *wlc_hw);
#endif

#ifdef WLRXOV
extern void wlc_rxov_int(wlc_info_t *wlc);
#endif

#define TBTT_AP_MASK 		1
#define TBTT_P2P_MASK 		2
#define TBTT_MCHAN_MASK 	4
#define TBTT_WD_MASK 		8
#define TBTT_TBTT_MASK 		0x10
extern void wlc_bmac_enable_tbtt(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 val);
extern void wlc_bmac_set_defmacintmask(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 val);

#ifdef BPRESET
extern void wlc_full_reset(wlc_hw_info_t *wlc_hw, uint32 val);
extern void wlc_dma_hang_trigger(wlc_hw_info_t *wlc_hw);
#endif

extern void wlc_bmac_set_extlna_pwrsave_shmem(wlc_hw_info_t *wlc_hw);
extern uint wlc_rateset_get_legacy_rateidx(ratespec_t rspec);
extern void wlc_bmac_amt_dump(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_sync_macstate(wlc_hw_info_t *wlc_hw);
extern void wlc_bmac_update_synthpu(wlc_hw_info_t *wlc_hw);

#ifdef FWID
extern uint32 gFWID;
#endif
extern void wlc_bmac_coex_flush_a2dp_buffers(wlc_hw_info_t *wlc_hw);

extern void wlc_bmac_enable_rx_hostmem_access(wlc_hw_info_t *wlc_hw, bool hostmem_access);
#ifdef BCMPCIEDEV
extern void wlc_bmac_enable_tx_hostmem_access(wlc_hw_info_t *wlc_hw, bool hostmem_access);
#endif /* BCMPCIEDEV */
extern void wlc_bmac_clear_band_pwr_offset(ppr_t *txpwr_offsets, wlc_hw_info_t *wlc_hw);

/* 4349a0 phymode war for common registers that should have been path registers */
extern void wlc_bmac_exclusive_reg_access_core0(wlc_hw_info_t *wlc_hw, bool set);
extern void wlc_bmac_exclusive_reg_access_core1(wlc_hw_info_t *wlc_hw, bool set);

#ifndef WL_DUALMAC_RSDB
extern bool wlc_bmac_rsdb_cap(wlc_hw_info_t *wlc_hw);
#else
#define wlc_bmac_rsdb_cap(wlc_hw) (FALSE)
#endif

extern void wlc_bmac_ifsctl_vht_set(wlc_hw_info_t *wlc_hw, int ed_sel);

extern void wlc_bmac_rcvlazy_update(wlc_hw_info_t *wlc_hw, uint32 intrcvlazy);
extern void wlc_bmac_ifsctl1_regshm(wlc_hw_info_t *wlc_hw, uint32 mask, uint32 val);

#ifdef BCMDBG_TXSTUCK
extern void wlc_bmac_print_muted(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif /* BCMDBG_TXSTUCK */

extern bool wlc_bmac_pio_enab_check(wlc_hw_info_t *wlc_hw);
extern uint32 wlc_bmac_current_pmu_time(wlc_info_t *wlc);

/* get slice specific OTP/SROM parameters */
int getintvar_slicespecific(wlc_hw_info_t *wlc_hw, char *vars, const char *name);
int wlc_bmac_reset_txrx(wlc_hw_info_t *wlc_hw);
int wlc_bmac_bmc_dyn_reinit(wlc_hw_info_t *wlc_hw, uint8 bufsize_in_256_blocks);
void wlc_bmac_rsdb_mode_param_to_shmem(wlc_hw_info_t *wlc_hw);
void wlc_bmac_4349_core1_hwreqoff(wlc_hw_info_t *wlc_hw, bool mode);
extern uint16 wlc_bmac_read_eci_data_reg(wlc_hw_info_t *wlc_hw, uint8 reg_num);
unsigned int wlc_bmac_shmphymode_dump(wlc_hw_info_t *wlc_hw);

extern void wlc_bmac_rpc_watchdog(wlc_info_t *wlc);

extern int  wlc_sleep(struct wlc_info *wlc);
extern int  wlc_resume(struct wlc_info *wlc);
extern int wlc_update_splitrx_mode(wlc_hw_info_t *wlc_hw, bool mode, bool init);
#ifdef MBSS
extern bool wlc_bmac_ucodembss_hwcap(wlc_hw_info_t *wlc_hw);
#else
#define wlc_bmac_ucodembss_hwcap(a) 0
#endif

extern void wlc_coredisable(wlc_hw_info_t* wlc_hw);
extern void wlc_rxfifo_setpio(wlc_hw_info_t *wlc_hw);

int wlc_bmac_dump_list(wlc_hw_info_t *wlc_hw, struct bcmstrbuf *b);

extern uint wlc_bmac_coreunit(wlc_info_t *wlc);

#endif /* _WLC_BMAC_H_ */
