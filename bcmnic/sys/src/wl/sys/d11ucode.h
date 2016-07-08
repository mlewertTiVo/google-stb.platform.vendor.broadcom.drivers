/*
 * Microcode declarations for Broadcom 802.11abg
 * Networking Adapter Device Driver.
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
 * $Id: d11ucode.h 612522 2016-01-14 06:40:18Z $
 */

/* ucode and inits structure */
typedef struct d11init {
	uint16	addr;
	uint16	size;
	uint32	value;
} d11init_t;

typedef struct d11axiinit {
	uint32	addr;
	uint32	size;
	uint32	value;
} d11axiinit_t;


typedef struct _d11regs_bmp_list {
	uint8 type;
	uint16 addr;
	uint32 bitmap;
	uint8 step;
	uint16 cnt; /* can be used together with bitmap or by itself */
} d11regs_list_t;

extern CONST uint32 d11ucode16_lp[];
extern CONST uint d11ucode16_lpsz;
extern CONST uint8 d11ucode16_sslpn[];
extern CONST uint d11ucode16_sslpnsz;
extern CONST uint8 d11ucode16_sslpn_nobt[];
extern CONST uint d11ucode16_sslpn_nobtsz;
extern CONST uint32 d11ucode16_mimo[];
extern CONST uint d11ucode16_mimosz;
extern CONST uint32 d11ucode22_mimo[];
extern CONST uint d11ucode22_mimosz;
extern CONST uint8 d11ucode22_sslpn[];
extern CONST uint d11ucode22_sslpnsz;
extern CONST uint32 d11ucode24_mimo[];
extern CONST uint d11ucode24_mimosz;
extern CONST uint8 d11ucode24_lcn[];
extern CONST uint d11ucode24_lcnsz;
extern CONST uint32 d11ucode25_mimo[];
extern CONST uint d11ucode25_mimosz;
extern CONST uint8 d11ucode25_lcn[];
extern CONST uint d11ucode25_lcnsz;
extern CONST uint32 d11ucode26_mimo[];
extern CONST uint d11ucode26_mimosz;
extern CONST uint32 d11ucode29_mimo[];
extern CONST uint d11ucode29_mimosz;
extern CONST uint32 d11ucode30_mimo[];
extern CONST uint d11ucode30_mimosz;
extern CONST uint32 d11ucode31_mimo[];
extern CONST uint d11ucode31_mimosz;
extern CONST uint32 d11ucode32_mimo[];
extern CONST uint d11ucode32_mimosz;
extern CONST uint8 d11ucode33_lcn40[];
extern CONST uint d11ucode33_lcn40sz;
extern CONST uint32 d11ucode34_mimo[];
extern CONST uint d11ucode34_mimosz;
extern CONST uint32 d11ucode36_mimo[];
extern CONST uint d11ucode36_mimosz;
extern CONST uint8 d11ucode37_lcn40[];
extern CONST uint d11ucode37_lcn40sz;
extern CONST uint8 d11ucode38_lcn40[];
extern CONST uint d11ucode38_lcn40sz;
extern CONST uint8 d11ucode39_lcn20[];
extern CONST uint d11ucode39_lcn20sz;
extern CONST uint32 d11ucode40[];
extern CONST uint d11ucode40sz;
extern CONST uint32 d11ucode41[];
extern CONST uint d11ucode41sz;
extern CONST uint32 d11ucode42[];
extern CONST uint d11ucode42sz;
extern CONST uint32 d11ucode43[];
extern CONST uint d11ucode43sz;
extern CONST uint32 d11ucode46[];
extern CONST uint d11ucode46sz;
extern CONST uint32 d11ucode47[];
extern CONST uint d11ucode47sz;
extern CONST uint32 d11ucode48[];
extern CONST uint d11ucode48sz;
extern CONST uint32 d11ucode49[];
extern CONST uint d11ucode49sz;
extern CONST uint32 d11ucode50[];
extern CONST uint d11ucode50sz;
extern CONST uint32 d11ucode54[];
extern CONST uint d11ucode54sz;
extern CONST uint32 d11ucode55[];
extern CONST uint d11ucode55sz;
extern CONST uint32 d11ucode56[];
extern CONST uint d11ucode56sz;
extern CONST uint32 d11ucode58_D11a[];
extern CONST uint d11ucode58_D11asz;
extern CONST uint32 d11ucode58_D11b[];
extern CONST uint d11ucode58_D11bsz;

extern CONST uint32 d11ucode59[];
extern CONST uint d11ucode59sz;
extern CONST uint32 d11ucode60[];
extern CONST uint d11ucode60sz;
extern CONST uint32 d11ucode61[];
extern CONST uint d11ucode61sz;
extern CONST uint32 d11ucode64[];
extern CONST uint d11ucode64sz;


extern CONST d11init_t d11n0initvals16[];
extern CONST d11init_t d11n0bsinitvals16[];
extern CONST d11init_t d11sslpn0initvals16[];
extern CONST d11init_t d11sslpn0bsinitvals16[];
extern CONST d11init_t d11lp0initvals16[];
extern CONST d11init_t d11lp0bsinitvals16[];
extern CONST d11init_t d11n0initvals22[];
extern CONST d11init_t d11n0bsinitvals22[];
extern CONST d11init_t d11sslpn4initvals22[];
extern CONST d11init_t d11sslpn4bsinitvals22[];

extern CONST d11init_t d11sslpn1initvals27[];
extern CONST d11init_t d11sslpn1bsinitvals27[];
extern CONST uint8 d11ucode27_sslpn[];
extern CONST uint d11ucode27_sslpnsz;
extern CONST d11init_t d11n0initvals24[];
extern CONST d11init_t d11n0bsinitvals24[];
extern CONST d11init_t d11lcn0initvals24[];
extern CONST d11init_t d11lcn0bsinitvals24[];
extern CONST d11init_t d11n0initvals25[];
extern CONST d11init_t d11n0bsinitvals25[];
extern CONST d11init_t d11n16initvals30[];
extern CONST d11init_t d11n16bsinitvals30[];
extern CONST d11init_t d11lcn0initvals25[];
extern CONST d11init_t d11lcn0bsinitvals25[];
extern CONST d11init_t d11ht0initvals26[];
extern CONST d11init_t d11ht0bsinitvals26[];
extern CONST d11init_t d11ht0initvals29[];
extern CONST d11init_t d11ht0bsinitvals29[];
extern CONST d11init_t d11n0initvals31[];
extern CONST d11init_t d11n0bsinitvals31[];
extern CONST d11init_t d11n22initvals31[];
extern CONST d11init_t d11n22bsinitvals31[];
extern CONST d11init_t d11n18initvals32[];
extern CONST d11init_t d11n18bsinitvals32[];
extern CONST d11init_t d11lcn400initvals33[];
extern CONST d11init_t d11lcn400bsinitvals33[];
extern CONST d11init_t d11n19initvals34[];
extern CONST d11init_t d11n19bsinitvals34[];
extern CONST d11init_t d11n20initvals36[];
extern CONST d11init_t d11n20bsinitvals36[];
extern CONST d11init_t d11lcn406initvals37[];
extern CONST d11init_t d11lcn406bsinitvals37[];
extern CONST d11init_t d11lcn407initvals38[];
extern CONST d11init_t d11lcn407bsinitvals38[];

extern CONST d11init_t d11lcn200initvals39[];
extern CONST d11init_t d11lcn200bsinitvals39[];
extern CONST d11init_t d11ac0initvals40[];
extern CONST d11init_t d11ac0bsinitvals40[];
extern CONST d11init_t d11ac2initvals41[];
extern CONST d11init_t d11ac2bsinitvals41[];
extern CONST d11init_t d11ac1initvals42[];
extern CONST d11init_t d11ac1bsinitvals42[];
extern CONST d11init_t d11wakeac1initvals42[];
extern CONST d11init_t d11wakeac1bsinitvals42[];
extern CONST d11init_t d11ac3initvals43[];
extern CONST d11init_t d11ac3bsinitvals43[];
extern CONST d11init_t d11wakeac6initvals46[];
extern CONST d11init_t d11wakeac6bsinitvals46[];
extern CONST d11init_t d11ac6initvals46[];
extern CONST d11init_t d11ac6bsinitvals46[];
extern CONST d11init_t d11ac7initvals47[];
extern CONST d11init_t d11ac7bsinitvals47[];
extern CONST d11init_t d11ac8initvals48[];
extern CONST d11init_t d11ac8bsinitvals48[];
extern CONST d11init_t d11ac9initvals49[];
extern CONST d11init_t d11ac9bsinitvals49[];
extern CONST d11init_t d11ac12initvals50[];
extern CONST d11init_t d11ac12bsinitvals50[];
extern CONST d11init_t d11ac12initvals50core1[];
extern CONST d11init_t d11ac12bsinitvals50core1[];
extern CONST d11init_t d11ac20initvals54[];
extern CONST d11init_t d11ac20bsinitvals54[];
extern CONST d11init_t d11ac24initvals55[];
extern CONST d11init_t d11ac24bsinitvals55[];
extern CONST d11init_t d11ac24initvals55core1[];
extern CONST d11init_t d11ac24bsinitvals55core1[];
extern CONST d11init_t d11ac24initvals56[];
extern CONST d11init_t d11ac24bsinitvals56[];
extern CONST d11init_t d11ac24initvals56core1[];
extern CONST d11init_t d11ac24bsinitvals56core1[];
extern CONST d11init_t d11ac26initvals58[];
extern CONST d11init_t d11ac26bsinitvals58[];
extern CONST d11init_t d11ac27initvals58[];
extern CONST d11init_t d11ac27bsinitvals58[];
extern CONST d11init_t d11ac24initvals59[];
extern CONST d11init_t d11ac24bsinitvals59[];
extern CONST d11init_t d11ac24initvals59core1[];
extern CONST d11init_t d11ac24bsinitvals59core1[];
extern CONST d11init_t d11ulpac36initvals60[];
extern CONST d11init_t d11ulpac36bsinitvals60[];
extern CONST d11init_t d11ac40initvals61[];
extern CONST d11init_t d11ac40bsinitvals61[];
extern CONST d11init_t d11ac32initvals64[];
extern CONST d11init_t d11ac32bsinitvals64[];

extern CONST d11init_t d11waken0initvals16[];
extern CONST d11init_t d11waken0bsinitvals16[];
extern CONST d11init_t d11wakelcn0initvals24[];
extern CONST d11init_t d11wakelcn0bsinitvals24[];
extern CONST d11init_t d11waken0initvals24[];
extern CONST d11init_t d11waken0bsinitvals24[];
extern CONST d11init_t d11waken0initvals26[];
extern CONST d11init_t d11waken0bsinitvals26[];
extern CONST d11init_t d11waken0initvals30[];
extern CONST d11init_t d11waken0bsinitvals30[];
extern CONST d11init_t d11wakelcn403initvals33[];
extern CONST d11init_t d11wakelcn403bsinitvals33[];

extern CONST uint32 d11aeswakeucode16_lp[];
extern CONST uint32 d11aeswakeucode16_sslpn[];
extern CONST uint32 d11aeswakeucode16_mimo[];
extern CONST uint32 d11aeswakeucode24_lcn[];
extern CONST uint32 d11aeswakeucode24_mimo[];
extern CONST uint32 d11aeswakeucode26_mimo[];
extern CONST uint32 d11aeswakeucode30_mimo[];
extern CONST uint32 d11aeswakeucode42[];
extern CONST uint32 d11aeswakeucode46[];

extern CONST uint32 d11ucode_wowl16_lp[];
extern CONST uint32 d11ucode_wowl16_sslpn[];
extern CONST uint32 d11ucode_wowl16_mimo[];
extern CONST uint32 d11ucode_wowl24_lcn[];
extern CONST uint32 d11ucode_wowl24_mimo[];
extern CONST uint32 d11ucode_wowl26_mimo[];
extern CONST uint32 d11ucode_wowl30_mimo[];
extern CONST uint32 d11ucode_wowl42[];
extern CONST uint32 d11ucode_wowl46[];
extern CONST uint32 d11ucode_ulp46[];
extern CONST uint32 d11ucode_ulp60[];
extern CONST uint32 d11aeswakeucode33_lcn40[];
extern CONST uint32 d11ucode_wowl33_lcn40[];

extern CONST uint d11ucode_wowl16_lpsz;
extern CONST uint d11ucode_wowl16_sslpnsz;
extern CONST uint d11ucode_wowl16_mimosz;
extern CONST uint d11ucode_wowl24_lcnsz;
extern CONST uint d11ucode_wowl24_mimosz;
extern CONST uint d11ucode_wowl26_mimosz;
extern CONST uint d11ucode_wowl30_mimosz;
extern CONST uint d11ucode_wowl42sz;
extern CONST uint d11ucode_wowl46sz;
extern CONST uint d11ucode_ulp46sz;
extern CONST uint d11ucode_ulp60sz;
extern CONST uint d11aeswakeucode33_lcn40sz;
extern CONST uint d11ucode_wowl33_lcn40sz;

extern CONST uint d11aeswakeucode16_lpsz;
extern CONST uint d11aeswakeucode16_sslpnsz;
extern CONST uint d11aeswakeucode16_mimosz;
extern CONST uint d11aeswakeucode24_lcnsz;
extern CONST uint d11aeswakeucode24_mimosz;
extern CONST uint d11aeswakeucode26_mimosz;
extern CONST uint d11aeswakeucode30_mimosz;
extern CONST uint d11aeswakeucode42sz;
extern CONST uint d11aeswakeucode46sz;

#ifdef SAMPLE_COLLECT
extern CONST uint32 d11sampleucode16[];
extern CONST uint d11sampleucode16sz;
#endif

/* BOM info for each ucode file */
extern CONST uint32 d11ucode_ge24_bommajor;
extern CONST uint32 d11ucode_ge24_bomminor;
extern CONST uint32 d11ucode_gt15_bommajor;
extern CONST uint32 d11ucode_gt15_bomminor;
extern CONST uint32 d11ucode_wowl_bommajor;
extern CONST uint32 d11ucode_wowl_bomminor;

#ifdef WLP2P_UCODE
extern CONST uint32 d11ucode_p2p_bommajor;
extern CONST uint32 d11ucode_p2p_bomminor;
extern CONST uint32 d11ucode_p2p16_lp[];
extern CONST uint d11ucode_p2p16_lpsz;
extern CONST uint8 d11ucode_p2p16_sslpn[];
extern CONST uint d11ucode_p2p16_sslpnsz;
extern CONST uint8 d11ucode_p2p16_sslpn_nobt[];
extern CONST uint d11ucode_p2p16_sslpn_nobtsz;
extern CONST uint32 d11ucode_p2p16_mimo[];
extern CONST uint d11ucode_p2p16_mimosz;

extern CONST uint8 d11ucode_p2p22_sslpn[];
extern CONST uint d11ucode_p2p22_sslpnsz;
extern CONST uint32 d11ucode_p2p22_mimo[];
extern CONST uint d11ucode_p2p22_mimosz;
extern CONST uint32 d11ucode_p2p24_mimo[];
extern CONST uint d11ucode_p2p24_mimosz;
extern CONST uint8 d11ucode_p2p24_lcn[];
extern CONST uint d11ucode_p2p24_lcnsz;
extern CONST uint32 d11ucode_p2p25_mimo[];
extern CONST uint d11ucode_p2p25_mimosz;
extern CONST uint8 d11ucode_p2p25_lcn[];
extern CONST uint d11ucode_p2p25_lcnsz;
extern CONST uint32 d11ucode_p2p26_mimo[];
extern CONST uint d11ucode_p2p26_mimosz;
extern CONST uint8 d11ucode_p2p26_lcn[];
extern CONST uint d11ucode_p2p26_lcnsz;
extern CONST uint32 d11ucode_p2p29_mimo[];
extern CONST uint d11ucode_p2p29_mimosz;
extern CONST uint32 d11ucode_p2p30_mimo[];
extern CONST uint d11ucode_p2p30_mimosz;
extern CONST uint32 d11ucode_p2p31_mimo[];
extern CONST uint d11ucode_p2p31_mimosz;
extern CONST uint32 d11ucode_p2p32_mimo[];
extern CONST uint d11ucode_p2p32_mimosz;
extern CONST uint8 d11ucode_p2p33_lcn40[];
extern CONST uint d11ucode_p2p33_lcn40sz;
extern CONST uint32 d11ucode_p2p34_mimo[];
extern CONST uint d11ucode_p2p34_mimosz;
extern CONST uint32 d11ucode_p2p36_mimo[];
extern CONST uint d11ucode_p2p36_mimosz;
extern CONST uint8 d11ucode_p2p37_lcn40[];
extern CONST uint d11ucode_p2p37_lcn40sz;
extern CONST uint8 d11ucode_p2p38_lcn40[];
extern CONST uint d11ucode_p2p38_lcn40sz;
extern CONST uint8 d11ucode_p2p39_lcn20[];
extern CONST uint d11ucode_p2p39_lcn20sz;
extern CONST uint32 d11ucode_p2p40[];
extern CONST uint d11ucode_p2p40sz;
extern CONST uint32 d11ucode_p2p41[];
extern CONST uint d11ucode_p2p41sz;
extern CONST uint32 d11ucode_p2p42[];
extern CONST uint d11ucode_p2p42sz;
extern CONST uint32 d11ucode_p2p43[];
extern CONST uint d11ucode_p2p43sz;
extern CONST uint32 d11ucode_p2p46[];
extern CONST uint d11ucode_p2p46sz;
extern CONST uint32 d11ucode_p2p47[];
extern CONST uint d11ucode_p2p47sz;
extern CONST uint32 d11ucode_p2p48[];
extern CONST uint d11ucode_p2p48sz;
extern CONST uint32 d11ucode_p2p49[];
extern CONST uint d11ucode_p2p49sz;
extern CONST uint32 d11ucode_p2p50[];
extern CONST uint d11ucode_p2p50sz;
extern CONST uint32 d11ucode_p2p54[];
extern CONST uint d11ucode_p2p54sz;
extern CONST uint32 d11ucode_p2p59[];
extern CONST uint d11ucode_p2p59sz;
extern CONST uint32 d11ucode_p2p55[];
extern CONST uint d11ucode_p2p55sz;
extern CONST uint32 d11ucode_p2p56[];
extern CONST uint d11ucode_p2p56sz;
extern CONST uint32 d11ucode_p2p60[];
extern CONST uint d11ucode_p2p60sz;
extern CONST uint32 d11ucode_p2p61[];
extern CONST uint d11ucode_p2p61sz;
extern CONST uint32 d11ucode_p2p64[];
extern CONST uint d11ucode_p2p64sz;
extern CONST uint32 d11ucode_p2p65[];
extern CONST uint d11ucode_p2p65sz;
extern CONST uint32 d11ucode_p2p58_D11a[];
extern CONST uint d11ucode_p2p58_D11asz;
extern CONST uint32 d11ucode_p2p58_D11b[];
extern CONST uint d11ucode_p2p58_D11bsz;
#endif /* WLP2P_UCODE */
extern CONST d11init_t d11ac36initvals60[];
extern CONST d11init_t d11ac36bsinitvals60[];
extern CONST uint32 d11ucode_ulp60[];
extern CONST uint d11ucode_ulp60sz;
extern CONST d11init_t d11ulpac36initvals60[];
extern CONST d11init_t d11ulpac36bsinitvals60[];
extern CONST d11axiinit_t d11ac36initvals60_axislave_order[];
extern CONST d11axiinit_t d11ac36initvals60_axislave[];
extern CONST d11axiinit_t d11ac36bsinitvals60_axislave_order[];
extern CONST d11axiinit_t d11ac36bsinitvals60_axislave[];
extern CONST d11axiinit_t d11ulpac36initvals60_axislave_order[];
extern CONST d11axiinit_t d11ulpac36initvals60_axislave[];
extern CONST d11axiinit_t d11ulpac36bsinitvals60_axislave_order[];
extern CONST d11axiinit_t d11ulpac36bsinitvals60_axislave[];
