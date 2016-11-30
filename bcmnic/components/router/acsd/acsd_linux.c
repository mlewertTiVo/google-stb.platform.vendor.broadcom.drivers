/*
 * ACS Linux related functions
 *
 * Broadcom Proprietary and Confidential. Copyright (C) 2016,
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom.
 *
 * $Id: acsd_linux.c 597371 2015-11-05 02:05:29Z $
 */
#include <proto/ethernet.h>
#include <proto/bcmeth.h>
#include <proto/bcmevent.h>
#include <proto/802.11.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <unistd.h>
#ifdef WL_MEDIA_ACSD
#include <acsd_config.h>
#endif
#include "acsd_svr.h"

#ifndef WL_MEDIA_ACSD
/* open a UDP packet to event dispatcher for receiving/sending data */
int
acsd_open_event_socket(acsd_wksp_t *d_info)
{
	int reuse = 1;
	struct sockaddr_in sockaddr;
	int fd = ACSD_DFLT_FD;

	/* open loopback socket to communicate with event dispatcher */
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sockaddr.sin_port = htons(EAPD_WKSP_DCS_UDP_SPORT);

	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		ACSD_ERROR("Unable to create loopback socket\n");
		goto exit1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
		ACSD_ERROR("Unable to setsockopt to loopback socket %d.\n", fd);
		goto exit1;
	}

	if (bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
		ACSD_ERROR("Unable to bind to loopback socket %d\n", fd);
		goto exit1;
	}

	ACSD_INFO("opened loopback socket %d\n", fd);
	d_info->event_fd = fd;

	return ACSD_OK;

	/* error handling */
exit1:
	if (fd != ACSD_DFLT_FD) {
		close(fd);
	}
	return errno;
}
#else

int
acsd_get_add_from_ifname(char *name, uint8 *mac)
{
	int err;
	struct ifreq ifr;
	int fd;

	/* Find the mac address for the ifname */
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, name, IFNAMSIZ-1);

	err = ioctl(fd, SIOCGIFHWADDR, &ifr);
	close(fd);

	ACS_ERR(err, "Cannot get hwaddress");
	memcpy(mac, (unsigned char *)ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

	return ACSD_OK;
}

/* Open a RAW socket for receiving data/event */
int
acsd_open_rawsocket(acs_chaninfo_t* c_info, int *event_fd)
{
	int fd = ACSD_DFLT_FD;
	struct sockaddr_ll sll;
	struct ifreq ifr;
	int err;

	fd = socket(PF_PACKET, SOCK_RAW, hton16(ETHER_TYPE_BRCM));
	if (fd < 0) {
		ACSD_ERROR("Cannot create socket %d\n", fd);
		goto exit1;
	}
	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, c_info->name, sizeof(ifr.ifr_name));
	ifr.ifr_name[sizeof(ifr.ifr_name) -1] = '\0';
	err = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (err < 0) {
		ACSD_ERROR("Cannot get index %d\n", err);
		goto exit1;
	}

	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = hton16(ETHER_TYPE_BRCM);
	sll.sll_ifindex = ifr.ifr_ifindex;
	err = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
	if (err < 0) {
		ACSD_ERROR("Cannot bind %d\n", err);
		goto exit1;
	}

	ACSD_INFO("opened raw socket %d\n", fd);
	*event_fd = fd;

	return ACSD_OK;
exit1:
	if (fd != ACSD_DFLT_FD) {
		close(fd);
	}
	return errno;
}

/* This function will return a random value in the range (TRUE, FALSE) */
bool
select_rand_value()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return ((now.tv_sec % 2) ? TRUE : FALSE);
}
#endif /* WL_MEDIA_ACSD */

int
acsd_open_listen_socket(unsigned int port, acsd_wksp_t *d_info)
{
	struct sockaddr_in s_sock;

	d_info->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (d_info->listen_fd < 0) {
		ACSD_ERROR("Socket open failed: %s\n", strerror(errno));
		return ACSD_FAIL;
	}

	memset(&s_sock, 0, sizeof(struct sockaddr_in));
	s_sock.sin_family = AF_INET;
	s_sock.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	s_sock.sin_port = htons(port);

	if (bind(d_info->listen_fd, (struct sockaddr *)&s_sock,
		sizeof(struct sockaddr_in)) < 0) {
		ACSD_ERROR("Socket bind failed: %s\n", strerror(errno));
		return ACSD_FAIL;
	}

	if (listen(d_info->listen_fd, 5) < 0) {
		ACSD_ERROR("Socket listen failed: %s\n", strerror(errno));
		return ACSD_FAIL;
	}

	return ACSD_OK;
}

/* Make acsd as daemon */
int
acsd_daemonize()
{
	int err;

	if ((err = daemon(1, 1)) == -1) {
		ACSD_ERROR("err from daemonize.\n");
	}

	return err;
}

int
acsd_check_for_data_in_event_socket(acsd_wksp_t *d_info, int *bytes)
{
	uint len;
	uint8 *pkt;

	pkt = d_info->packet;
	len = sizeof(d_info->packet);

	/* handle brcm event */
	if (d_info->event_fd !=  ACSD_DFLT_FD && FD_ISSET(d_info->event_fd, &d_info->fdset)) {
		ACSD_INFO("recved event from eventfd\n");

		d_info->stats.num_events++;

		if ((*bytes = recv(d_info->event_fd, pkt, len, 0)) <= 0)
			return -1;

		return TRUE;
	}
	return FALSE;
}
/* listen to sockets and call handlers to process packets */
int
acsd_wait_for_events(struct timeval *tv, acsd_wksp_t *d_info)
{
	int width, status = 0;
	uint len;
	struct sockaddr_in cliaddr;
	/* init file descriptor set */
	FD_ZERO(&d_info->fdset);
	d_info->fdmax = -1;

	/* build file descriptor set now to save time later */
	if (d_info->event_fd != ACSD_DFLT_FD) {
		FD_SET(d_info->event_fd, &d_info->fdset);
		d_info->fdmax = d_info->event_fd;
	}

	if (d_info->listen_fd != ACSD_DFLT_FD) {
		FD_SET(d_info->listen_fd, &d_info->fdset);
		if (d_info->listen_fd > d_info->fdmax)
			d_info->fdmax = d_info->listen_fd;
	}

	width = d_info->fdmax + 1;

	/* listen to data availible on all sockets */
	status = select(width, &d_info->fdset, NULL, NULL, tv);

	if ((status == -1 && errno == EINTR) || (status == 0))
		return -1;

	if (status <= 0) {
		ACSD_ERROR("err from select: %s", strerror(errno));
		return -1;
	}

	len = sizeof(d_info->packet);

	if (d_info->listen_fd != ACSD_DFLT_FD && FD_ISSET(d_info->listen_fd, &d_info->fdset)) {
		ACSD_INFO("recved command from a client\n");

		d_info->stats.num_cmds++;

		d_info->conn_fd = accept(d_info->listen_fd, (struct sockaddr*)&cliaddr, &len);
		if (d_info->conn_fd < 0) {
			if (errno == EINTR) return 0;
			else {
				ACSD_ERROR("accept failed: errno: %d - %s\n",
					errno, strerror(errno));
				return -1;
			}
		}

		return ACSD_DATA_FROM_LISTEN_FD;
	}

	return ACSD_DATA_FROM_EVENT_FD;
}
#ifdef WL_ACSD_ESCAN
int
acsd_escan_open_socket(char *name)
{
	int fd, err;
	struct sockaddr_ll sll;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, (IFNAMSIZ - 1));

	fd = socket(PF_PACKET, SOCK_RAW, hton16(ETHER_TYPE_BRCM));
	if (fd < 0) {
		ACSD_ERROR("Cannot create socket %d\n", fd);
		err = -1;
		return fd;
	}

	err = ioctl(fd, SIOCGIFINDEX, &ifr);
	if (err < 0) {
		ACSD_ERROR("Cannot get index %d\n", err);
		close(fd);
		return -1;
	}

	/* bind the socket first before starting escan so we won't miss any event */
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_protocol = hton16(ETHER_TYPE_BRCM);
	sll.sll_ifindex = ifr.ifr_ifindex;
	err = bind(fd, (struct sockaddr *)&sll, sizeof(sll));
	if (err < 0) {
		ACSD_ERROR("Cannot bind %d\n", err);
		close(fd);
		return -1;
	}

	return fd;
}
int
acsd_escan_wait_for_event(int fd)
{
	struct timeval tv;
	int retval;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	/* Set scan timeout */
	tv.tv_sec = WL_EVENT_TIMEOUT;
	tv.tv_usec = 0;

	retval = select(fd+1, &rfds, NULL, NULL, &tv);

	return retval;
}

int
acsd_escan_receive_event(int fd, char **data)
{
	int octets;
	octets = recv(fd, *data, ESCAN_EVENTS_BUFFER_SIZE, 0);

	return octets;
}
#endif /* WL_ACSD_ESCAN */
int acsd_open_socket()
{
	int s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return errno;
	}
	return s;
}

void
acsd_close_socket(int fd)
{
	close(fd);
}

int
acsd_ioctl(int s, char *name, wl_ioctl_t *ioc)
{
	struct ifreq ifr;

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	ifr.ifr_data = (caddr_t) ioc;
	return ioctl(s, SIOCDEVPRIVATE, &ifr);
}

int
acsd_write(int fd, char *buf, unsigned int size)
{
	int ret = 0;

	errno = 0;
	ret = write(fd, buf, size);
	if (ret < 0) {
		if (errno == EINTR)
			return ACSD_READ_WRITE_INT;
		else
			return ACSD_READ_WRITE_NO_INT;
	}

	return ret;
}

int
acsd_read(int fd, char *buf, unsigned int size)
{
	int ret = 0;

	errno = 0;
	ret = read(fd, buf, size);
	if (ret < 0) {
		if (errno == EINTR)
			return ACSD_READ_WRITE_INT;
		else
			return ACSD_READ_WRITE_NO_INT;
	}

	return ret;
}

void
sleep_ms(const unsigned int ms)
{
	usleep(1000*ms);
}

#ifdef WL_MEDIA_ACSD
int acsd_read_config(acsd_hashtable_t *hashtable)
{
	int err = 0;
	FILE *f = NULL;
	char line[CONFIG_FILE_LINE_LENGTH];

	char *token = NULL;
	char *key = NULL;
	char *value = NULL;
	char *comment_start_ptr = NULL;
#ifdef ACSD_CONFIG_INIT_DEBUG
	int line_num = 0;
#endif

	f = fopen("acsd_config.txt", "r");

	if (NULL != f) {
		while (fgets(line, CONFIG_FILE_LINE_LENGTH, f)) {

#ifdef ACSD_CONFIG_INIT_DEBUG
			line_num++;
#endif

			char *copy = strdup(line);

			/* Skip anything after # in the line
			 *   Format
			 *   1. Skip the complete line
			 *   #Config file
			 *   2. Skip after #
			 *   acs_ifnames eth0 #int name
			 */
			comment_start_ptr = strstr(copy, "#");
			if (comment_start_ptr != NULL) {
				*comment_start_ptr = '\0';
			}

			/*
			 * Reading one line at a time from config file
			 * And entries in the config file should adher the following protocol
			 * i.e.
			 * wl0_acs_scan_entry_expire 3600
			 * wl0_acs_scan_entry_expire is the key/entry/parameter
			 * 3600 is the value of the key/entry/parameter
			*/
			token = strtok(copy, " \r\n");
			key = token;
			value = strtok(NULL, " \r\n");

			if ((key == NULL) || (value == NULL))
			{
#ifdef ACSD_CONFIG_INIT_DEBUG
				ACSD_CONF_INIT_PRINT("config entry skipped: %d:%s",
					line_num, line);
#endif
				acsd_free(copy);
			} else {
				ACSD_INFO("Key[%s] Value[%s] \n", key, value);
				acsd_ht_set(hashtable, key, value);
				key = NULL;
				value = NULL;
				acsd_free(copy);
			}
		}
		fclose(f);
		err = 0;
	} else {
		err = -1;
	}
	return err;
}
#endif /* WL_MEDIA_ACSD */
