#Linux Makefile

include $(SRCBASE_ROUTER)/.config

INCLUDE += -I. -I.. -I$(SRCBASE)/include ${WLAN_StdIncPathA} -I$(SRCBASE)/../components/shared -I$(SRCBASE)/common/include -I$(SRCBASE)/shared -I$(SRCBASE_ROUTER)/shared -I$(SRCBASE_ROUTER)/nas -I$(SRCBASE_ROUTER)/nas/nas_fbt

CFLAGS += $(INCLUDE)
CFLAGS += $(WLAN_ComponentIncPath)
CFLAGS += -DEAPD_WKSP_AUTO_CONFIG
#CFLAGS += -DNAS_GTK_PER_STA
CFLAGS += -Wall -DEAPDDUMP

CFLAGS  += -s


ifneq ($(LINUXDIR),)
CFLAGS += -Dlinux
endif

LDFLAGS += -L$(TOP)/nvram -L$(INSTALLDIR)/nvram/usr/lib -lnvram
LDFLAGS += -L$(TOP)/shared -L$(INSTALLDIR)/shared/usr/lib -lshared
LDFLAGS += $(EXTRA_LDFLAGS)

vpath %.c $(SRCBASE_ROUTER)/eapd $(SRCBASE)/shared

OBJS = eapd.o eapd_linux.o
OBJS += nas_eap.o wps_eap.o


#ifdef BCM_DCS
ifeq ($(CONFIG_BCMDCS),y)
OBJS += dcs_eap.o
CFLAGS += -DBCM_DCS
endif
#endif

#ifdef BCM_MEVENT
ifeq ($(CONFIG_BCM_MEVENT),y)
OBJS += mevent_eap.o
CFLAGS += -DBCM_MEVENT
endif
#endif

#ifdef BCM_BSD
ifeq ($(CONFIG_BCMBSD),y)
OBJS += bsd_eap.o
CFLAGS += -DBCM_BSD
endif
#endif

#ifdef BCM_DRSDBD
ifeq ($(CONFIG_BCMDRSDBD),y)
OBJS += drsdbd_eap.o
CFLAGS += -DBCM_DRSDBD
endif
#endif

#ifdef BCM_SSD
ifeq ($(CONFIG_BCMSSD),y)
OBJS += ssd_eap.o
CFLAGS += -DBCM_SSD
endif
#endif

#ifdef BCM_ASPMD
ifeq ($(LINUX_VERSION),2_6_36)
ifeq ($(CONFIG_BCMASPMD),y)
OBJS += aspm_eap.o
CFLAGS += -DBCM_ASPMD
endif
endif
#endif

#ifdef CONFIG_VISUALIZATION
ifeq ($(CONFIG_VISUALIZATION),y)
OBJS += visdcoll_eap.o
endif
#endif

all: eapd

eapd: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

install: all
	install -d $(INSTALLDIR)/bin
	install eapd $(INSTALLDIR)/bin
	$(STRIP) $(INSTALLDIR)/bin/eapd

clean:
	rm -f *.o eapd
