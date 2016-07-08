/*
 * wlc_ltrol.h
 *
 * Latency Tolerance Reporting (offload device driver)
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_ltrol.h 467328 2014-04-03 01:23:40Z $
 *
 */

/**
 * @file
 * @brief
 * Latency Tolerance Reporting (LTR) is a mechanism for devices to report their
 * service latency requirements for Memory Reads and Writes to the PCIe Root Complex
 * such that platform resources can be power managed without impacting device
 * functionality and performance. Platforms want to consume the least amount of power.
 * Devices want to provide the best performance and functionality (which needs power).
 * A device driver can mediate between these conflicting goals by intelligently reporting
 * device latency requirements based on the state of the device.
 */


#ifndef _wlc_ltrol_h_
#define _wlc_ltrol_h_

extern wlc_dngl_ol_ltr_info_t *wlc_dngl_ol_ltr_attach(wlc_dngl_ol_info_t *wlc_dngl_ol);
extern void wlc_dngl_ol_ltr_proc_msg(wlc_dngl_ol_info_t *wlc_dngl_ol, void *buf, int len);
void wlc_dngl_ol_ltr_update(wlc_dngl_ol_info_t *wlc_dngl_ol, osl_t *osh);

#endif /* _wlc_ltrol_h_ */
