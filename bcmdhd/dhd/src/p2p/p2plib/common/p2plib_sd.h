/* P2P Service Discovery header file
 *
 * Copyright (C) 1999-2017, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: p2plib_sd.h,v 1.13 2010-12-10 02:00:50 $
 */

#ifndef _p2plib_sd_h_
#define _p2plib_sd_h_

#ifndef SOFTAP_ONLY

extern BCMP2P_STATUS p2plib_sd_register_svc_data(uint32 svc_id, p2psd_svc_protype_t pro_type,
	const uint8 *req_data, uint32 req_data_len, const uint8 * resp_data,
	uint32 resp_data_len, void **hsd);

extern BCMP2P_STATUS p2plib_sd_deregister_svc_data(void *hsd);

extern BCMP2P_STATUS p2plib_sd_get_registered_service(BCMP2P_SVC_PROTYPE svcProtocol,
	uint8 *queryData, uint32 queryDataLen, uint8 *respDataBuf,
	uint32 *respDataLen, uint32 *svc_id);

extern BCMP2P_STATUS
p2plib_sd_start_req_to_peer(p2papi_instance_t* hdl, struct ether_addr *peer_mac,
	BCMP2P_CHANNEL *channel, BCMP2P_SVC_LIST *svc_req_list, bool ch_sync);

extern BCMP2P_API BCMP2P_STATUS
p2plib_sd_cancel_req_svc(p2papi_instance_t* hdl, struct ether_addr* peer_mac);

extern int p2plib_sd_on_peer_found(int old_peer_count, p2papi_instance_t* hdl);

extern void p2plib_sd_on_wl_event(p2papi_instance_t *hdl, wl_event_msg_t *event, void* data,
	uint32 data_len);

extern void
p2plib_sd_wl_event_handler(p2papi_instance_t *hdl, BCMP2P_BOOL is_primary,
                           wl_event_msg_t *event, void* data, uint32 data_len);

extern void
p2plib_sd_disable_auto_req_svc(p2papi_instance_t *hdl);

extern BCMP2P_SVC_LIST *
p2plib_sd_get_peer_svc(struct ether_addr *peer_mac);

extern void
p2plib_sd_cleanup(p2papi_instance_t *hdl);

extern bool
p2plib_sd_is_svc_discovered(struct ether_addr *peer_mac);

#endif /* not  SOFTAP_ONLY */

#endif  /* _p2plib_sd_h_ */
