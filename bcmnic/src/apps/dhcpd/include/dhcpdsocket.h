/*
 * Broadcom DHCP Server
 * DHCP socket definitions. 
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: dhcpdsocket.h,v 1.5 2011-01-24 15:13:09 $
 */

int socketInit();
int socketDeinit();
int socketGet(struct Packet **p);
int socketSend(struct Packet *p);
int socketBind();
