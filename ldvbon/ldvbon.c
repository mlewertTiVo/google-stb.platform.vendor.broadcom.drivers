/***************************************************************************
 *  Copyright (C) 2016 Broadcom.  The term "Broadcom" refers to Broadcom Limited and/or its subsidiaries.
 *
 *  This program is the proprietary software of Broadcom and/or its licensors,
 *  and may only be used, duplicated, modified or distributed pursuant to the terms and
 *  conditions of a separate, written license agreement executed between you and Broadcom
 *  (an "Authorized License").  Except as set forth in an Authorized License, Broadcom grants
 *  no license (express or implied), right to use, or waiver of any kind with respect to the
 *  Software, and Broadcom expressly reserves all rights in and to the Software and all
 *  intellectual property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU
 *  HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY
 *  NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization, constitutes the valuable trade
 *  secrets of Broadcom, and you shall use all reasonable efforts to protect the confidentiality thereof,
 *  and to use this information only in connection with your use of Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *  AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *  WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH RESPECT TO
 *  THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL IMPLIED WARRANTIES
 *  OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE,
 *  LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION
 *  OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF
 *  USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR ITS
 *  LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR
 *  EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO YOUR
 *  USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT
 *  ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *  LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF
 *  ANY LIMITED REMEDY.
 *
 * Module Description:
 *
 ************************************************************/
#include "bstd.h"
#include "blst_slist.h"
#include "nexus_pid_channel.h"
#include "nexus_parser_band.h"
#include "nexus_stc_channel.h"
#include "nexus_frontend.h"
#include "nexus_message.h"
#include "nexus_platform.h"

#include <linux/module.h>
#include <linux/export.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <dvb_demux.h>
#include <dvb_frontend.h>
#include "dvbdev.h"
#include <dmxdev.h>

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

//#define LDVBON_DEBUG
//#define LDVBON_TRACING

#define DATA_BUFFER_TIMEOUT_MS 50
#define BUFFER_WARNING (80) /* % */

#if NEXUS_MESSAGE_FILTER_SIZE < DMX_MAX_FILTER_SIZE
#define LDVBON_SF_SIZE (NEXUS_MESSAGE_FILTER_SIZE)
#else
#define LDVBON_SF_SIZE (DMX_MAX_FILTER_SIZE)
#endif

#ifdef LDVBON_DEBUG
#define LDVBON_DBG(fmt, args...) printk(fmt , ## args)
#else
#define LDVBON_DBG(fmt, args...)
#endif
#ifdef LDVBON_TRACING
#define LDVBON_TRACE() printk("%s", __func__)
#else
#define LDVBON_TRACE()
#endif
#define LDVBON_MSG(fmt, args...) printk(fmt , ## args)
#define LDVBON_WRN(fmt, args...) pr_warning(fmt , ## args)
#define LDVBON_ERR(fmt, args...) pr_err(fmt , ## args)

#define INVALID_INDEX (-1)

typedef struct {
    BLST_S_HEAD(ldvbContexts_, ldvbContextNode_s) ldvbContexts;
} ldvbon_driver_t;

typedef ldvbon_driver_t* ldvbon_driver_h;

typedef struct msgFilterNode_s {
    BLST_S_ENTRY(msgFilterNode_s) link;
    unsigned int index;
    NEXUS_MessageHandle msgFilter;
    dmx_section_cb dmx_sec_cb;
    struct dmx_section_filter *filter;
} msgFilterNode_t;

typedef msgFilterNode_t* msgFilterNode_h;

typedef struct pidChannelNode_s {
    BLST_S_ENTRY(pidChannelNode_s) link;
    unsigned int index;
    uint16_t pid;
    uint16_t refcount;
    NEXUS_PidChannelHandle pidChannel;
    dmx_ts_cb dmx_ts_cb;
    struct dmx_ts_feed *dmx_ts_feed;
    BLST_S_HEAD(msgFilters_, msgFilterNode_s) msgFilters;
} pidChannelNode_t;

typedef pidChannelNode_t* pidChannelNode_h;

typedef struct ldvbContextNode_s {
    BLST_S_ENTRY(ldvbContextNode_s) link;
    NEXUS_FrontendHandle frontend;
    NEXUS_ParserBand parserBand;
    BLST_S_HEAD(pidChannels_, pidChannelNode_s) pidChannels;

    struct mutex lock;
    pidChannelNode_h sharedDemux;

    NEXUS_FrontendOfdmSettings ofdmSettings;
    NEXUS_FrontendFastStatus fastStatus;
    NEXUS_RecpumpHandle recpump;

    struct dvb_adapter dvb_adap;
    struct dvb_frontend dvb_frontend;
    struct dvb_demux dvb_dmx;
    struct dmxdev dvb_dmxdev;
    struct task_struct *timer_task;
} ldvbContextNode_t;

typedef ldvbContextNode_t* ldvb_context_h;

static void lock_callback(void *context, int param)
{
    ldvb_context_h ldvb_context = context;
    int ret;

    LDVBON_TRACE();

    ret = NEXUS_Frontend_GetFastStatus(ldvb_context->frontend, &ldvb_context->fastStatus);
    if (ret) {
        LDVBON_ERR("%s: NEXUS_Frontend_GetFastStatus failed", __func__);
    }

#ifdef LDVBON_DEBUG
    switch (ldvb_context->fastStatus.lockStatus) {
    case NEXUS_FrontendLockStatus_eUnknown:
        LDVBON_DBG("%s: NEXUS_FrontendLockStatus_eUnknown", __func__);
        break;
    case NEXUS_FrontendLockStatus_eUnlocked:
        LDVBON_DBG("%s: NEXUS_FrontendLockStatus_eUnlocked", __func__);
        break;
    case NEXUS_FrontendLockStatus_eLocked:
        LDVBON_DBG("%s: NEXUS_FrontendLockStatus_eLocked", __func__);
        break;
    case NEXUS_FrontendLockStatus_eNoSignal:
        LDVBON_DBG("%s: NEXUS_FrontendLockStatus_eNoSignal", __func__);
        break;
    default:
        LDVBON_ERR("%s: Unknown fastStatus %u", __func__, ldvb_context->fastStatus.lockStatus);
        break;
    }
#endif
}

static void overflow_likely_callback(void *context, int param)
{
    LDVBON_WRN("%s: %s has exceeded %u capacity. Increase the size to prevent overflow.", __func__, (char *) context, BUFFER_WARNING);
}

static void overflow_callback(void *context, int param)
{
    LDVBON_ERR("%s %s", __func__, (char *) context);
}

static void error_callback(void *context, int param)
{
    LDVBON_ERR("%s %s", __func__, (char *) context);
}

static void message_callback(void *context, int param)
{
    msgFilterNode_h msgFilterNode = (msgFilterNode_h) context;
    const void *read_buffer, *wrap_buffer;
    size_t read_size, wrap_size;
    int ret;

    LDVBON_TRACE();

    ret = NEXUS_Message_GetBufferWithWrap(msgFilterNode->msgFilter, &read_buffer, &read_size, &wrap_buffer, &wrap_size);
    if (ret) {
        LDVBON_ERR("%s: NEXUS_Message_GetBuffer() failed: %d", __func__, ret);
        return;
    }

    if (read_size) {
        LDVBON_DBG("%s: Sending %u SF bytes to LDVB filter idx %u", __func__, read_size + wrap_size, msgFilterNode->index);
        msgFilterNode->dmx_sec_cb(read_buffer, read_size, wrap_buffer, wrap_size, msgFilterNode->filter);
    } else {
        LDVBON_ERR("%s: Message callback without data available", __func__);
    }

    ret = NEXUS_Message_ReadComplete(msgFilterNode->msgFilter, read_size + wrap_size);
    if (ret) {
        LDVBON_ERR("%s: NEXUS_Message_ReadComplete() failed: %d", __func__, ret);
        return;
    }
}

static int nexus_fe_get_data_buffer_timer(void *context)
{
    ldvb_context_h ldvb_context = (ldvb_context_h) context;
    const void *read_buffer, *wrap_buffer;
    size_t read_size, wrap_size;
    int ret;

    LDVBON_TRACE();

    while (!kthread_should_stop()) {

        msleep(DATA_BUFFER_TIMEOUT_MS);

        ret = NEXUS_Recpump_GetDataBufferWithWrap(ldvb_context->recpump, &read_buffer, &read_size, &wrap_buffer, &wrap_size);
        if (ret) {
            continue;
        }

        if (read_size == 0) {
            continue;
        }

        LDVBON_DBG("%s: Sending %u TS bytes to LDVB", __func__, read_size + wrap_size);

        mutex_lock(&ldvb_context->lock);
        if (ldvb_context->sharedDemux) {
            ldvb_context->sharedDemux->dmx_ts_cb(read_buffer, read_size, wrap_buffer, wrap_size, ldvb_context->sharedDemux->dmx_ts_feed);
        } else {
            LDVBON_DBG("%s: No demux callback exists, dumping data...", __func__);
        }
        mutex_unlock(&ldvb_context->lock);

        NEXUS_Recpump_DataWriteComplete(ldvb_context->recpump, read_size + wrap_size);
    }

    return 0;
}

static uint8_t nexus_fe_get_code_rate(NEXUS_FrontendOfdmSettings *ofdmSettings, enum fe_code_rate arg, bool low)
{
    NEXUS_FrontendOfdmCodeRate codeRate;

    switch (arg) {
    case FEC_AUTO:
        LDVBON_DBG("%s: %s code rate: FEC_AUTO", __func__, low ? "Low" : "High" );
        ofdmSettings->manualTpsSettings = false;
        return 0;
    case FEC_1_2:
        LDVBON_DBG("%s: %s code rate: FEC_1_2", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e1_2;
        break;
    case FEC_2_3:
        LDVBON_DBG("%s: %s code rate: FEC_2_3", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e2_3;
        break;
    case FEC_3_4:
        LDVBON_DBG("%s: %s code rate: FEC_3_4", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e3_4;
        break;
    case FEC_3_5:
        LDVBON_DBG("%s: %s code rate: FEC_3_5", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e3_5;
        break;
    case FEC_4_5:
        LDVBON_DBG("%s: %s code rate: FEC_4_5", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e4_5;
        break;
    case FEC_5_6:
        LDVBON_DBG("%s: %s code rate: FEC_5_6", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e5_6;
        break;
    case FEC_7_8:
        LDVBON_DBG("%s: %s code rate: FEC_7_8", __func__, low ? "Low" : "High" );
        codeRate = NEXUS_FrontendOfdmCodeRate_e7_8;
        break;
    default:
        LDVBON_ERR("%s: Unsupported code rate %u", __func__, arg);
        return -1;
    }

    if (low) {
        ofdmSettings->tpsSettings.lowPriorityCodeRate = codeRate;
    } else {
        ofdmSettings->tpsSettings.highPriorityCodeRate = codeRate;
    }

    return 0;
}

static int nexus_fe_set_frontend(struct dvb_frontend *fe)
{
    ldvb_context_h ldvb_context = fe->dvb->priv;
    struct dtv_frontend_properties *c = &fe->dtv_property_cache;
    int ret;

    LDVBON_TRACE();

    LDVBON_DBG("%s: frequency %u", __func__, c->frequency);
    ldvb_context->ofdmSettings.frequency = c->frequency;
    LDVBON_DBG("%s: bandwidth_hz %u", __func__, c->bandwidth_hz);
    ldvb_context->ofdmSettings.bandwidth = (c->bandwidth_hz) ? c->bandwidth_hz : 8000000;

    switch (c->delivery_system) {
    case SYS_DVBT:
        ldvb_context->ofdmSettings.mode = NEXUS_FrontendOfdmMode_eDvbt;
        LDVBON_DBG("%s: Delivery system: SYS_DVBT", __func__);
        break;
    case SYS_DVBT2:
        ldvb_context->ofdmSettings.mode = NEXUS_FrontendOfdmMode_eDvbt2;
        LDVBON_DBG("%s: Delivery system: SYS_DVBT2", __func__);
        break;
    default:
        LDVBON_ERR("%s: Unsupported delivery_system %u", __func__, c->delivery_system);
        return -1;
    }

    switch (c->inversion) {
    case INVERSION_AUTO:
        LDVBON_DBG("%s: Inversion: INVERSION_AUTO. Disabling manual OFDM settings", __func__);
        ldvb_context->ofdmSettings.manualModeSettings = false;
        break;
    case INVERSION_OFF:
        LDVBON_DBG("%s: Inversion: INVERSION_OFF", __func__);
        ldvb_context->ofdmSettings.spectralInversion = NEXUS_FrontendOfdmSpectralInversion_eNormal;
        break;
    case INVERSION_ON:
        LDVBON_DBG("%s: Inversion: INVERSION_ON", __func__);
        ldvb_context->ofdmSettings.spectralInversion = NEXUS_FrontendOfdmSpectralInversion_eInverted;
        break;
    default:
        LDVBON_ERR("%s: Unsupported inversion %u", __func__, c->guard_interval);
        return -1;
    }

    ldvb_context->ofdmSettings.manualModeSettings = true;
    switch (c->guard_interval) {
    case GUARD_INTERVAL_AUTO:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_AUTO. Disabling manual OFDM settings", __func__);
        ldvb_context->ofdmSettings.manualModeSettings = false;
        break;
    case GUARD_INTERVAL_1_4:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_1_4", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_4;
        break;
    case GUARD_INTERVAL_1_8:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_1_8", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_8;
        break;
    case GUARD_INTERVAL_1_16:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_1_16", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_16;
        break;
    case GUARD_INTERVAL_1_32:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_1_32", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_32;
        break;
    case GUARD_INTERVAL_1_128:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_1_128", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_128;
        break;
    case GUARD_INTERVAL_19_128:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_19_128", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e19_128;
        break;
    case GUARD_INTERVAL_19_256:
        LDVBON_DBG("%s: Guard interval: GUARD_INTERVAL_19_256", __func__);
        ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e19_256;
        break;
    default:
        LDVBON_ERR("%s: Unsupported guard_interval %u", __func__, c->guard_interval);
        return -1;
    }

    switch (c->transmission_mode) {
    case TRANSMISSION_MODE_AUTO:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_AUTO. Disabling manual OFDM settings", __func__);
        ldvb_context->ofdmSettings.manualModeSettings = false;
        break;
    case TRANSMISSION_MODE_1K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_1K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e1k;
        break;
    case TRANSMISSION_MODE_2K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_2K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e2k;
        break;
    case TRANSMISSION_MODE_4K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_4K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e4k;
        break;
    case TRANSMISSION_MODE_8K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_8K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e8k;
        break;
    case TRANSMISSION_MODE_16K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_16K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e16k;
        break;
    case TRANSMISSION_MODE_32K:
        LDVBON_DBG("%s: Transmission mode: TRANSMISSION_MODE_32K", __func__);
        ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e32k;
        break;
    default:
        LDVBON_ERR("%s: Unsupported transmission_mode %u", __func__, c->transmission_mode);
        return -1;
    }

    ldvb_context->ofdmSettings.manualTpsSettings = true;
    switch (c->modulation) {
    case QAM_AUTO:
        LDVBON_DBG("%s: Modulation: QAM_AUTO. Disabling manual OFDM settings", __func__);
        ldvb_context->ofdmSettings.manualTpsSettings = false;
        break;
    case QPSK:
        LDVBON_DBG("%s: Modulation: QPSK", __func__);
        ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQpsk;
        break;
    case QAM_16:
        LDVBON_DBG("%s: Modulation: QAM_16", __func__);
        ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam16;
        break;
    case QAM_64:
        LDVBON_DBG("%s: Modulation: QAM_64", __func__);
        ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam64;
        break;
    case QAM_256:
        LDVBON_DBG("%s: Modulation: QAM_256", __func__);
        ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam256;
        break;
    case DQPSK:
        LDVBON_DBG("%s: Modulation: DQPSK", __func__);
        ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eDqpsk;
        break;
    default:
        LDVBON_ERR("%s: Unsupported modulation %u", __func__, c->modulation);
        return -1;
    }

    switch (c->hierarchy) {
    case HIERARCHY_AUTO:
        LDVBON_DBG("%s: Hierarchy: HIERARCHY_AUTO. Disabling manual OFDM settings", __func__);
        ldvb_context->ofdmSettings.manualTpsSettings = false;
        break;
    case HIERARCHY_NONE:
        LDVBON_DBG("%s: Hierarchy: HIERARCHY_NONE", __func__);
        ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e0;
        break;
    case HIERARCHY_1:
        LDVBON_DBG("%s: Hierarchy: HIERARCHY_1", __func__);
        ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e1;
        break;
    case HIERARCHY_2:
        LDVBON_DBG("%s: Hierarchy: HIERARCHY_2", __func__);
        ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e2;
        break;
    case HIERARCHY_4:
        LDVBON_DBG("%s: Hierarchy: HIERARCHY_4", __func__);
        ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e4;
        break;
    default:
        LDVBON_ERR("%s: Unsupported hierarchy %u", __func__, c->hierarchy);
        return -1;
    }

    ret = nexus_fe_get_code_rate(&ldvb_context->ofdmSettings, c->code_rate_LP, true);
    if (ret < 0) {
        LDVBON_ERR("%s: nexus_fe_get_code_rate() failed: %d", __func__, ret);
        return ret;
    }

    ret = nexus_fe_get_code_rate(&ldvb_context->ofdmSettings, c->code_rate_HP, false);
    if (ret < 0) {
        LDVBON_ERR("%s: nexus_fe_get_code_rate() failed: %d", __func__, ret);
        return ret;
    }

    ldvb_context->fastStatus.lockStatus = NEXUS_FrontendLockStatus_eUnknown;
    ret = NEXUS_Frontend_TuneOfdm(ldvb_context->frontend, &ldvb_context->ofdmSettings);
    if (ret < 0) {
        LDVBON_ERR("%s: NEXUS_Frontend_TuneOfdm() failed: %d", __func__, ret);
        return ret;
    }

    return ret;
}

static int nexus_fe_get_frontend(struct dvb_frontend *fe
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
                 , struct dtv_frontend_properties *c
#endif
)
{
    LDVBON_TRACE();

    return 0;
}

static int nexus_fe_get_tune_settings(struct dvb_frontend *fe,
            struct dvb_frontend_tune_settings *settings) {

    LDVBON_TRACE();

    settings->min_delay_ms = 2000;

    return 0;
}

static int nexus_fe_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
    ldvb_context_h ldvb_context = fe->dvb->priv;

    //LDVBON_TRACE();

    switch (ldvb_context->fastStatus.lockStatus) {
    case NEXUS_FrontendLockStatus_eUnlocked:
        *status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
        break;
    case NEXUS_FrontendLockStatus_eLocked:
        *status = FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_SYNC |
              FE_HAS_LOCK | FE_HAS_VITERBI;
        break;
    default:
        *status = 0;
        break;
    }

    return 0;
}

static int nexus_fe_read_snr(struct dvb_frontend *fe, u16 *snr)
{
    LDVBON_TRACE();

    return 0;
}

static int nexus_fe_read_ber(struct dvb_frontend *fe, u32 *ber)
{
    LDVBON_TRACE();

    return 0;
}

static int nexus_fe_read_signal_strength(struct dvb_frontend *fe,
                     u16 *strength)
{
    LDVBON_TRACE();

    return 0;
}

static int nexus_fe_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
    LDVBON_TRACE();

    return 0;
}

static void nexus_fe_release(struct dvb_frontend *fe)
{
    LDVBON_TRACE();

    return;
}

static int nexus_dvb_dmx_enable_pid_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    ldvb_context_h ldvb_context = dvbdmxfeed->demux->priv;
    NEXUS_RecpumpAddPidChannelSettings recpump_pid_settings;
    int ret = 0;

    LDVBON_TRACE();

    if (pidChannelNode->index != INVALID_INDEX) {
        LDVBON_ERR("%s: Already filtering for this PID: %u", __func__, pidChannelNode->pid);
        return 1;
    }

    pidChannelNode->index = dvbdmxfeed->index;

    NEXUS_Recpump_GetDefaultAddPidChannelSettings(&recpump_pid_settings);

    switch (dvbdmxfeed->pes_type) {
    case DMX_PES_VIDEO:
        LDVBON_DBG("%s: PES type: DMX_PES_VIDEO", __func__);
        recpump_pid_settings.pidType = NEXUS_PidType_eVideo;
        break;
    case DMX_PES_AUDIO:
        LDVBON_DBG("%s: PES type: DMX_PES_AUDIO", __func__);
        recpump_pid_settings.pidType = NEXUS_PidType_eAudio;
        break;
    case DMX_PES_TELETEXT:
    case DMX_PES_PCR:
    case DMX_PES_OTHER:
        LDVBON_DBG("%s: PES type: DMX_PES_TELETEXT/DMX_PES_PCR/DMX_PES_OTHER", __func__);
        recpump_pid_settings.pidType = NEXUS_PidType_eOther;
        break;
    default:
        LDVBON_ERR("%s: Unsupported pidType %u", __func__, dvbdmxfeed->pes_type);
        return -EINVAL;
    }

    pidChannelNode->dmx_ts_cb = dvbdmxfeed->cb.ts;
    pidChannelNode->dmx_ts_feed = &dvbdmxfeed->feed.ts;

    mutex_lock(&ldvb_context->lock);
    ldvb_context->sharedDemux = pidChannelNode;
    mutex_unlock(&ldvb_context->lock);

    ret = NEXUS_Recpump_AddPidChannel(ldvb_context->recpump, pidChannelNode->pidChannel, &recpump_pid_settings);
    if (ret) {
        LDVBON_ERR("%s: NEXUS_Recpump_AddPidChannel() failed: %d", __func__, ret);
        return ret;
    }

    return ret;
}

static int nexus_dvb_dmx_disable_pid_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    ldvb_context_h ldvb_context = dvbdmxfeed->demux->priv;

    LDVBON_TRACE();

    if (pidChannelNode->index != dvbdmxfeed->index) {
        LDVBON_ERR("%s: Invalid pidfilter index: %u", __func__, dvbdmxfeed->index);
        return 1;
    }

    NEXUS_Recpump_RemovePidChannel(ldvb_context->recpump, pidChannelNode->pidChannel);

    if (ldvb_context->sharedDemux == pidChannelNode) {
        mutex_lock(&ldvb_context->lock);
        ldvb_context->sharedDemux = BLST_S_FIRST(&ldvb_context->pidChannels);
        mutex_unlock(&ldvb_context->lock);
    }

    pidChannelNode->index = INVALID_INDEX;

    return 0;
}

static int nexus_dvb_dmx_add_msg_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    NEXUS_MessageSettings settings;
    NEXUS_MessageStartSettings startSettings;
    msgFilterNode_h msgFilterNode;
    struct dmx_section_filter *filter = &dvbdmxfeed->filter->filter;
    int i, ret = 0;

    LDVBON_TRACE();

    msgFilterNode = kmalloc(sizeof(msgFilterNode_t), GFP_KERNEL);
    if (msgFilterNode == NULL) {
        LDVBON_ERR("%s: kmalloc failed", __func__);
        return ENOMEM;
    }

    msgFilterNode->index = dvbdmxfeed->index;
    msgFilterNode->dmx_sec_cb = dvbdmxfeed->cb.sec;
    msgFilterNode->filter = &dvbdmxfeed->filter->filter;

    NEXUS_Message_GetDefaultSettings(&settings);
    settings.dataReady.callback = message_callback;
    settings.dataReady.context = msgFilterNode;
    settings.overflow.callback = overflow_callback;
    settings.overflow.context = "msgFilter.overflow";
    settings.psiLengthError.callback = error_callback;
    settings.psiLengthError.context  = "msgFilter.psiLengthError";
    settings.crcError.callback      = error_callback;
    settings.crcError.context       = "msgFilter.crcError";
    settings.maxContiguousMessageSize = 0;
    msgFilterNode->msgFilter = NEXUS_Message_Open(&settings);
    if (msgFilterNode->msgFilter == NULL) {
        LDVBON_ERR("%s: NEXUS_Message_Open failed", __func__);
        kfree(msgFilterNode);
        return 1;
    }

    NEXUS_Message_GetDefaultStartSettings(msgFilterNode->msgFilter, &startSettings);
    startSettings.pidChannel = pidChannelNode->pidChannel;
    startSettings.noPaddingBytes = true;
    startSettings.includeThirdFilterByte = true;

    for (i = 0; i < LDVBON_SF_SIZE; i++) {
        startSettings.filter.coefficient[i] = filter->filter_value[i];
        startSettings.filter.mask[i] = ~filter->filter_mask[i];
        startSettings.filter.exclusion[i] = ~filter->filter_mode[i];
    }

    ret = NEXUS_Message_Start(msgFilterNode->msgFilter, &startSettings);
    if (ret < 0) {
        LDVBON_ERR("%s: NEXUS_Message_Start() failed: %d", __func__, ret);
        NEXUS_Message_Close(msgFilterNode->msgFilter);
        kfree(msgFilterNode);
        return ret;
    }

    BLST_S_INSERT_HEAD(&pidChannelNode->msgFilters, msgFilterNode, link);

    return ret;
}

static int nexus_dvb_dmx_remove_msg_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    msgFilterNode_h msgFilterNode;
    bool found;

    LDVBON_TRACE();

    for (msgFilterNode = BLST_S_FIRST(&pidChannelNode->msgFilters); msgFilterNode; msgFilterNode = BLST_S_NEXT(msgFilterNode, link)) {
        if (msgFilterNode->index == dvbdmxfeed->index) {
            found = true;
            break;
        }
    }

    if (!found) {
        LDVBON_ERR("%s: Couldn't find dvbdmxfeed->index: %u", __func__, dvbdmxfeed->index);
        return 1;
    }

    NEXUS_Message_Stop(msgFilterNode->msgFilter);
    NEXUS_Message_Close(msgFilterNode->msgFilter);

    BLST_S_REMOVE(&pidChannelNode->msgFilters, msgFilterNode, msgFilterNode_s, link);

    kfree(msgFilterNode);

    return 0;
}

static int nexus_dvb_dmx_add_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    int ret = 0;

    LDVBON_TRACE();

    switch (dvbdmxfeed->type) {
    case DMX_TYPE_TS:
        ret = nexus_dvb_dmx_enable_pid_filter(dvbdmxfeed, pidChannelNode);
        break;
    case DMX_TYPE_SEC:
        ret = nexus_dvb_dmx_add_msg_filter(dvbdmxfeed, pidChannelNode);
        break;
    default:
        LDVBON_ERR("%s: Unsupported type: %u", __func__, dvbdmxfeed->type);
        ret = 1;
    }

    if (ret) {
        return ret;
    }

    pidChannelNode->refcount++;

    return 0;
}

static int nexus_dvb_dmx_remove_filter(struct dvb_demux_feed *dvbdmxfeed, pidChannelNode_h pidChannelNode)
{
    int ret = 0;

    LDVBON_TRACE();

    switch (dvbdmxfeed->type) {
    case DMX_TYPE_TS:
        ret = nexus_dvb_dmx_disable_pid_filter(dvbdmxfeed, pidChannelNode);
        break;
    case DMX_TYPE_SEC:
        ret = nexus_dvb_dmx_remove_msg_filter(dvbdmxfeed, pidChannelNode);
        break;
    default:
        LDVBON_ERR("%s: Unsupported type: %u", __func__, dvbdmxfeed->type);
        ret = 1;
    }

    if (ret) {
        return ret;
    }

    pidChannelNode->refcount--;

    return 0;
}


static int nexus_dvb_dmx_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    ldvb_context_h ldvb_context = dvbdmxfeed->demux->priv;
    NEXUS_PidChannelSettings pidChannelSettings;
    pidChannelNode_h pidChannelNode;
    NEXUS_PidChannelHandle pidChannel;
    int ret = 0;

    LDVBON_TRACE();
    LDVBON_DBG("%s: index: %u", __func__, dvbdmxfeed->index);
    LDVBON_DBG("%s: type %u", __func__, dvbdmxfeed->type);
    LDVBON_DBG("%s: pid: %u", __func__, dvbdmxfeed->pid);

    /* Check pidchannel doesn't exist yet */
    for (pidChannelNode = BLST_S_FIRST(&ldvb_context->pidChannels); pidChannelNode; pidChannelNode = BLST_S_NEXT(pidChannelNode, link)) {
        if (pidChannelNode->pid == dvbdmxfeed->pid) {
            return nexus_dvb_dmx_add_filter(dvbdmxfeed, pidChannelNode);
        }
    }

    NEXUS_PidChannel_GetDefaultSettings(&pidChannelSettings);
    pidChannelSettings.pidChannelIndex = NEXUS_PID_CHANNEL_OPEN_MESSAGE_CAPABLE;
    pidChannel = NEXUS_PidChannel_Open(ldvb_context->parserBand, dvbdmxfeed->pid, &pidChannelSettings);
    if (pidChannel == NULL) {
        LDVBON_ERR("%s: NEXUS_PidChannel_Open failed", __func__);
        return 1;
    }

    pidChannelNode = kmalloc(sizeof(pidChannelNode_t), GFP_KERNEL);
    if (pidChannelNode == NULL) {
        LDVBON_ERR("%s: kmalloc failed", __func__);
        return ENOMEM;
    }
    pidChannelNode->index = INVALID_INDEX;
    pidChannelNode->pid = dvbdmxfeed->pid;
    pidChannelNode->pidChannel = pidChannel;
    pidChannelNode->refcount = 0;
    BLST_S_INIT(&pidChannelNode->msgFilters);

    ret = nexus_dvb_dmx_add_filter(dvbdmxfeed, pidChannelNode);
    if (ret) {
        NEXUS_PidChannel_Close(pidChannelNode->pidChannel);
        kfree(pidChannelNode);
        return ret;
    }

    BLST_S_INSERT_HEAD(&ldvb_context->pidChannels, pidChannelNode, link);

    return ret;
}

static int nexus_dvb_dmx_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    ldvb_context_h ldvb_context = dvbdmxfeed->demux->priv;
    pidChannelNode_h pidChannelNode;
    bool found = false;
    int ret = 0;

    LDVBON_TRACE();
    LDVBON_DBG("%s: index: %u", __func__, dvbdmxfeed->index);
    LDVBON_DBG("%s: type %u", __func__, dvbdmxfeed->type);
    LDVBON_DBG("%s: pid: %u", __func__, dvbdmxfeed->pid);

    for (pidChannelNode = BLST_S_FIRST(&ldvb_context->pidChannels); pidChannelNode && !found ; pidChannelNode = BLST_S_NEXT(pidChannelNode, link)) {
        if (pidChannelNode->pid == dvbdmxfeed->pid) {
            found = true;
            break;
        }
    }

    if (!found) {
        LDVBON_ERR("%s: Couldn't find dvbdmxfeed->index: %u", __func__, dvbdmxfeed->index);
        return 1;
    }

    ret = nexus_dvb_dmx_remove_filter(dvbdmxfeed, pidChannelNode);
    if (ret) {
        return ret;
    }

    if (pidChannelNode->refcount == 0) {
        NEXUS_PidChannel_Close(pidChannelNode->pidChannel);
        BLST_S_REMOVE(&ldvb_context->pidChannels, pidChannelNode, pidChannelNode_s, link);
        kfree(pidChannelNode);
    }

    return 0;
}

struct dvb_frontend_ops dvb_frontend_ops = {
            .delsys = { SYS_DVBT, SYS_DVBT2 },
            .info = {
                .name = "Broadcom Nexus LDVB",
                .frequency_min = 174000000,
                .frequency_max = 862000000,
                .frequency_stepsize = 166667,
                .caps = FE_CAN_INVERSION_AUTO
                    | FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4
                    | FE_CAN_FEC_5_6 | FE_CAN_FEC_8_9 | FE_CAN_FEC_AUTO
                    | FE_CAN_QAM_16 | FE_CAN_QAM_64 | FE_CAN_QPSK
                    | FE_CAN_QAM_AUTO
                    | FE_CAN_TRANSMISSION_MODE_AUTO
                    | FE_CAN_GUARD_INTERVAL_AUTO
                    | FE_CAN_HIERARCHY_AUTO
                    | FE_CAN_RECOVER
                    | FE_CAN_MUTE_TS
                    | FE_CAN_2G_MODULATION
            },
            .set_frontend       = nexus_fe_set_frontend,
            .get_frontend       = nexus_fe_get_frontend,
            .get_tune_settings  = nexus_fe_get_tune_settings,

            .read_status        = nexus_fe_read_status,
            .read_snr           = nexus_fe_read_snr,
            .read_ber           = nexus_fe_read_ber,
            .read_signal_strength = nexus_fe_read_signal_strength,
            .read_ucblocks      = nexus_fe_read_ucblocks,
            .release            = nexus_fe_release,
};

static int linux_dvb_adapter_init(ldvbon_driver_h ldvbon_driver, bool *done)
{
    int ret;
    ldvb_context_h ldvb_context;
    NEXUS_FrontendAcquireSettings frontendAcquireSettings;
    NEXUS_FrontendHandle frontend;
    NEXUS_ParserBandSettings parserBandSettings;
    NEXUS_RecpumpSettings recpumpSettings;
    NEXUS_RecpumpOpenSettings recpumpOpenSettings;

    LDVBON_TRACE();

    NEXUS_Frontend_GetDefaultAcquireSettings(&frontendAcquireSettings);
    frontendAcquireSettings.capabilities.ofdm = true;
    frontend = NEXUS_Frontend_Acquire(&frontendAcquireSettings);
    if (!frontend) {
        *done = true;
        return 0;
    }

    ldvb_context = kzalloc(sizeof(ldvbContextNode_t), GFP_KERNEL);
    if (ldvb_context == NULL) {
        LDVBON_ERR("%s: kzalloc failed", __func__);
        return ENOMEM;
    }

    mutex_init(&ldvb_context->lock);

    ret = dvb_register_adapter(&ldvb_context->dvb_adap,
               "Nexus Frontend", THIS_MODULE,
               NULL, adapter_nr);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_register_adapter() failed: %d", __func__, ret);
        goto error_dvb_register_adapter;
    }

    ldvb_context->dvb_adap.priv = ldvb_context;
    ldvb_context->dvb_dmx.priv = ldvb_context;

    ldvb_context->frontend = frontend;

    ldvb_context->dvb_dmx.feednum = NEXUS_NUM_PID_CHANNELS;
    ldvb_context->dvb_dmx.filternum = NEXUS_NUM_MESSAGE_FILTERS;
    ldvb_context->dvb_dmx.dmx.capabilities = DMX_TS_FILTERING | DMX_SECTION_FILTERING;
    ldvb_context->dvb_dmx.start_feed = nexus_dvb_dmx_start_feed;
    ldvb_context->dvb_dmx.stop_feed = nexus_dvb_dmx_stop_feed;
    ret = dvb_dmx_init(&ldvb_context->dvb_dmx);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_dmx_init() failed: %d", __func__, ret);
        goto error_dvb_dmx_init;
    }

    ldvb_context->dvb_dmxdev.filternum = ldvb_context->dvb_dmx.filternum;
    ldvb_context->dvb_dmxdev.demux = &ldvb_context->dvb_dmx.dmx;
    ldvb_context->dvb_dmxdev.capabilities = 0;
    ret = dvb_dmxdev_init(&ldvb_context->dvb_dmxdev, &ldvb_context->dvb_adap);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_dmxdev_init() failed: %d", __func__, ret);
        goto error_dvb_dmxdev_init;
    }

    memcpy(&ldvb_context->dvb_frontend.ops, &dvb_frontend_ops, sizeof(ldvb_context->dvb_frontend.ops));
    ret = dvb_register_frontend(&ldvb_context->dvb_adap, &ldvb_context->dvb_frontend);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_register_frontend() failed: %d", __func__, ret);
        goto error_dvb_register_frontend;
    }

    NEXUS_Recpump_GetDefaultOpenSettings(&recpumpOpenSettings);
    recpumpOpenSettings.data.dataReadyThreshold = ((recpumpOpenSettings.data.bufferSize) * BUFFER_WARNING) / 100;
    ldvb_context->recpump = NEXUS_Recpump_Open(NEXUS_ANY_ID, &recpumpOpenSettings);
    if (!ldvb_context->recpump) {
        LDVBON_ERR("%s: Unable to find open recpump", __func__);
        goto error_NEXUS_Recpump_Open;
    }

    NEXUS_Recpump_GetSettings(ldvb_context->recpump, &recpumpSettings);
    recpumpSettings.data.dataReady.callback = overflow_likely_callback;
    recpumpSettings.data.dataReady.context = "recpump.data.dataReady";
    recpumpSettings.data.overflow.callback = overflow_callback;
    recpumpSettings.data.overflow.context = "recpump.data.overflow";
    recpumpSettings.index.overflow.callback = overflow_callback;
    recpumpSettings.index.overflow.context = "recpump.index.overflow";
    NEXUS_Recpump_SetSettings(ldvb_context->recpump, &recpumpSettings);

    /* Map a parser band to the streamer input band. */
    ldvb_context->parserBand = NEXUS_ParserBand_Open(NEXUS_ANY_ID);
    if (!ldvb_context->parserBand) {
        LDVBON_ERR("%s: NEXUS_ParserBand_Open failed", __func__);
        goto error_NEXUS_ParserBand_Open;
    }
    NEXUS_ParserBand_GetSettings(ldvb_context->parserBand, &parserBandSettings);
    parserBandSettings.transportType = NEXUS_TransportType_eTs;
    parserBandSettings.sourceType = NEXUS_ParserBandSourceType_eMtsif;
    parserBandSettings.sourceTypeSettings.mtsif = NEXUS_Frontend_GetConnector(ldvb_context->frontend);
    NEXUS_ParserBand_SetSettings(ldvb_context->parserBand, &parserBandSettings);

    NEXUS_Recpump_Start(ldvb_context->recpump);

    NEXUS_Frontend_GetDefaultOfdmSettings(&ldvb_context->ofdmSettings);
    ldvb_context->ofdmSettings.acquisitionMode = NEXUS_FrontendOfdmAcquisitionMode_eAuto;
    ldvb_context->ofdmSettings.terrestrial = true;
    ldvb_context->ofdmSettings.spectrum = NEXUS_FrontendOfdmSpectrum_eAuto;
    ldvb_context->ofdmSettings.lockCallback.callback = lock_callback;
    ldvb_context->ofdmSettings.lockCallback.context = ldvb_context;
    ldvb_context->ofdmSettings.lockCallback.param = 0;
    ldvb_context->ofdmSettings.modeSettings.modeGuard = NEXUS_FrontendOfdmModeGuard_eAutoDvbt;
    ldvb_context->ofdmSettings.pullInRange = NEXUS_FrontendOfdmPullInRange_eWide;
    ldvb_context->ofdmSettings.cciMode = NEXUS_FrontendOfdmCciMode_eNone;

    ldvb_context->timer_task = kthread_run(nexus_fe_get_data_buffer_timer, (void *) ldvb_context, "LDVBON buffer reader");
    if (IS_ERR(ldvb_context->timer_task)) {
        LDVBON_ERR("%s: kthread_run failed", __func__);
        ldvb_context->timer_task = NULL;
        goto error_kthread_run;
    }

    BLST_S_INIT(&ldvb_context->pidChannels);

    BLST_S_INSERT_HEAD(&ldvbon_driver->ldvbContexts, ldvb_context, link);

    return 0;

error_kthread_run:
    NEXUS_Recpump_Stop(ldvb_context->recpump);
    NEXUS_Recpump_Close(ldvb_context->recpump);
    NEXUS_ParserBand_Close(ldvb_context->parserBand);
error_NEXUS_ParserBand_Open:
    NEXUS_Recpump_Close(ldvb_context->recpump);
error_NEXUS_Recpump_Open:
    dvb_unregister_frontend(&ldvb_context->dvb_frontend);
error_dvb_register_frontend:
    dvb_dmxdev_release(&ldvb_context->dvb_dmxdev);
error_dvb_dmxdev_init:
    dvb_dmx_release(&ldvb_context->dvb_dmx);
error_dvb_dmx_init:
    dvb_unregister_adapter(&ldvb_context->dvb_adap);
error_dvb_register_adapter:
    mutex_destroy(&ldvb_context->lock);
    kfree(ldvb_context);

    return 1;
}

static void linux_dvb_adapter_uninit(ldvb_context_h ldvb_context)
{
    pidChannelNode_h pidChannelNode;
    msgFilterNode_h msgFilterNode;

    LDVBON_TRACE();

    kthread_stop(ldvb_context->timer_task);

    NEXUS_Frontend_Untune(ldvb_context->frontend);
    NEXUS_Frontend_Release(ldvb_context->frontend);

    NEXUS_Recpump_Stop(ldvb_context->recpump);
    NEXUS_Recpump_RemoveAllPidChannels(ldvb_context->recpump);
    NEXUS_Recpump_Close(ldvb_context->recpump);

    /* Close the audio and video pid channels */
    while ((pidChannelNode = BLST_S_FIRST(&ldvb_context->pidChannels))) {
        LDVBON_ERR("%s: Cleaning up pidchannel: %u", __func__, pidChannelNode->pid);
        NEXUS_PidChannel_Close(pidChannelNode->pidChannel);
        BLST_S_REMOVE(&ldvb_context->pidChannels, pidChannelNode, pidChannelNode_s, link);

        /* Close the message filters */
        while ((msgFilterNode = BLST_S_FIRST(&pidChannelNode->msgFilters))) {
            LDVBON_ERR("%s: Cleaning up section filter: %u", __func__, pidChannelNode->pid);
            NEXUS_Message_Stop(msgFilterNode->msgFilter);
            NEXUS_Message_Close(msgFilterNode->msgFilter);
            BLST_S_REMOVE(&ldvb_context->pidChannels, pidChannelNode, pidChannelNode_s, link);
            kfree(msgFilterNode);
        }

        kfree(pidChannelNode);
    }

    NEXUS_ParserBand_Close(ldvb_context->parserBand);

    dvb_unregister_frontend(&ldvb_context->dvb_frontend);

    dvb_dmxdev_release(&ldvb_context->dvb_dmxdev);
    dvb_dmx_release(&ldvb_context->dvb_dmx);

    dvb_unregister_adapter(&ldvb_context->dvb_adap);

    mutex_destroy(&ldvb_context->lock);

    kfree(ldvb_context);
}

static void linux_dvb_adapters_cleanup(ldvbon_driver_h ldvbon_driver)
{
    ldvb_context_h ldvb_context;

    LDVBON_TRACE();

    while ((ldvb_context = BLST_S_FIRST(&ldvbon_driver->ldvbContexts))) {
        BLST_S_REMOVE(&ldvbon_driver->ldvbContexts, ldvb_context, ldvbContextNode_s, link);
        linux_dvb_adapter_uninit(ldvb_context);
    }
}

static ldvbon_driver_t ldvbon_driver;

static int __init
LDVBON_init(void)
{
    int i, ret = 0;

    LDVBON_TRACE();

    BLST_S_INIT(&ldvbon_driver.ldvbContexts);

    for (i = 0; i < NEXUS_MAX_FRONTENDS; i++) {
        bool done = false;
        ret = linux_dvb_adapter_init(&ldvbon_driver, &done);
        if (ret) {
            LDVBON_ERR("%s: Failed to init LDVB device", __func__);
            linux_dvb_adapters_cleanup(&ldvbon_driver);
            return ret;
        }
        if (done) {
            break;
        }
    }

    if (i == 0) {
        LDVBON_ERR("%s: Unable to find any OFDM-capable frontend", __func__);
        return 1;
    }

    if (NEXUS_MESSAGE_FILTER_SIZE < DMX_MAX_FILTER_SIZE) {
        LDVBON_MSG("Nexus doesn't support full LinuxDVB DMX section filter size. Restrict filters to %u bytes", NEXUS_MESSAGE_FILTER_SIZE);
    }

    LDVBON_MSG("LDVBON module loaded %u adapters", i);
    return ret;
}

static void __exit
LDVBON_exit(void)
{
    LDVBON_TRACE();

    linux_dvb_adapters_cleanup(&ldvbon_driver);

    LDVBON_MSG("LDVBON module removed");
}

module_init(LDVBON_init);
module_exit(LDVBON_exit);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Broadcom Limited");
MODULE_DESCRIPTION("Linux DVB nexus integration");