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
#include "bkni.h"
#include "nexus_pid_channel.h"
#include "nexus_parser_band.h"
#include "nexus_stc_channel.h"
#include "nexus_frontend.h"
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

#define DATA_BUFFER_TIMEOUT_MS 50

//#define MAX_PIDCHANNELS NEXUS_NUM_PID_CHANNELS
#define MAX_PIDCHANNELS 128

/* the following define the input and its characteristics -- these will vary by input */
#define TRANSPORT_TYPE NEXUS_TransportType_eTs

#ifdef LDVBON_DEBUG
#define LDVBON_DBG(fmt, args...) printk(fmt , ## args)
#else
#define LDVBON_DBG(fmt, args...)
#endif
#define LDVBON_MSG(fmt, args...) printk(fmt , ## args)
#define LDVBON_ERR(fmt, args...) pr_err(fmt , ## args)

typedef struct ldvb_context_t {
    NEXUS_FrontendHandle frontend;
    NEXUS_ParserBand parserBand;
    NEXUS_PidChannelHandle pidChannels[MAX_PIDCHANNELS];
    NEXUS_RecpumpHandle recpump;
    NEXUS_FrontendOfdmSettings ofdmSettings;
    NEXUS_FrontendFastStatus fastStatus;

    struct dvb_adapter dvb_adap;
    struct dvb_demux dvb_dmx;
    struct dmxdev dvb_dmxdev;
    struct task_struct *timer_task;
} ldvb_context_t;

ldvb_context_t *ldvb_context;

static void lock_callback(void *context, int param)
{
    int ret;
    ret = NEXUS_Frontend_GetFastStatus(ldvb_context->frontend, &ldvb_context->fastStatus);
    if (ret) {
        LDVBON_ERR("NEXUS_Frontend_GetFastStatus failed\n");
    }

    LDVBON_DBG("%s: FastStatus %u\n", __func__, ldvb_context->fastStatus.lockStatus);
}

static void dataready_callback_index(void *context, int param)
{
    LDVBON_DBG("%s\n", __func__);
}

static void dataready_callback_data(void *context, int param)
{
    LDVBON_DBG("%s\n", __func__);
}

static void overflow_callback(void *context, int param)
{
    LDVBON_DBG("%s\n", __func__);
}

static void async_status_ready_callback(void *context, int param)
{
    LDVBON_DBG("%s\n", __func__);
}

static int nexus_fe_get_data_buffer_timer(void *context)
{
    const void *data_buffer;
    ldvb_context_t *ldvb_context = (ldvb_context_t *) context;
    size_t data_buffer_size;
    int ret;

    LDVBON_DBG("%s\n", __func__);

    while (!kthread_should_stop()) {

        msleep(DATA_BUFFER_TIMEOUT_MS);

        ret = NEXUS_Recpump_GetDataBuffer(ldvb_context->recpump, &data_buffer, &data_buffer_size);
        if (ret) {
            continue;
        }

        if (data_buffer_size == 0) {
            continue;
        }

        LDVBON_DBG("Sending %d bytes to LDVB\n", data_buffer_size);
        dvb_dmx_swfilter(&ldvb_context->dvb_dmx, data_buffer, data_buffer_size);

        NEXUS_Recpump_DataWriteComplete(ldvb_context->recpump, data_buffer_size);
    }

    return 0;
}

static uint8_t nexus_fe_get_code_rate(enum fe_code_rate arg, NEXUS_FrontendOfdmCodeRate *codeRate)
{
    switch (arg) {
    case FEC_1_2:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e1_2;
        break;
    case FEC_2_3:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e2_3;
        break;
    case FEC_3_4:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e3_4;
        break;
    case FEC_3_5:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e3_5;
        break;
    case FEC_4_5:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e4_5;
        break;
    case FEC_5_6:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e5_6;
        break;
    case FEC_7_8:
        *codeRate = NEXUS_FrontendOfdmCodeRate_e7_8;
        break;
    default:
        LDVBON_ERR("%s: Unsupported codeRate %u", __func__, *codeRate);
        return -1;
    }

    return 0;
}

static int nexus_fe_set_frontend(struct dvb_frontend *fe)
{
    struct dtv_frontend_properties *c = &fe->dtv_property_cache;
    int ret;

    LDVBON_DBG("%s\n", __func__);

    LDVBON_DBG("%s: frequency %u\n", __func__, c->frequency);
    LDVBON_DBG("%s: bandwidth_hz %u\n", __func__, c->bandwidth_hz);
    LDVBON_DBG("%s: delivery_system %u\n", __func__, c->delivery_system);
    LDVBON_DBG("%s: inversion %u\n", __func__, c->inversion);
    LDVBON_DBG("%s: guard_interval %u\n", __func__, c->guard_interval);
    LDVBON_DBG("%s: transmission_mode %u\n", __func__, c->transmission_mode);
    LDVBON_DBG("%s: modulation %u\n", __func__, c->modulation);
    LDVBON_DBG("%s: hierarchy %u\n", __func__, c->hierarchy);
    LDVBON_DBG("%s: code_rate_HP %u\n", __func__, c->code_rate_HP);
    LDVBON_DBG("%s: code_rate_LP %u\n", __func__, c->code_rate_LP);

    ldvb_context->ofdmSettings.frequency = c->frequency;
    ldvb_context->ofdmSettings.bandwidth = (c->bandwidth_hz) ? c->bandwidth_hz : 8000000;

    switch (c->delivery_system) {
    case SYS_DVBT:
        ldvb_context->ofdmSettings.mode = NEXUS_FrontendOfdmMode_eDvbt;
        break;
    case SYS_DVBT2:
        ldvb_context->ofdmSettings.mode = NEXUS_FrontendOfdmMode_eDvbt2;
        break;
    default:
        LDVBON_ERR("%s: Unsupported delivery_system %u\n", __func__, c->delivery_system);
        return -1;
    }

    switch (c->inversion) {
    case INVERSION_OFF:
    case INVERSION_AUTO:
        ldvb_context->ofdmSettings.spectralInversion = NEXUS_FrontendOfdmSpectralInversion_eNormal;
        break;
    case INVERSION_ON:
        ldvb_context->ofdmSettings.spectralInversion = NEXUS_FrontendOfdmSpectralInversion_eInverted;
        break;
    default:
        LDVBON_ERR("%s: Unsupported inversion %u\n", __func__, c->guard_interval);
        return -1;
    }

    if (c->guard_interval == GUARD_INTERVAL_AUTO || c->transmission_mode == TRANSMISSION_MODE_AUTO) {
        ldvb_context->ofdmSettings.manualModeSettings = false;
    } else {
        ldvb_context->ofdmSettings.manualModeSettings = true;

        switch (c->guard_interval) {
        case GUARD_INTERVAL_1_4:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_4;
            break;
        case GUARD_INTERVAL_1_8:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_8;
            break;
        case GUARD_INTERVAL_1_16:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_16;
            break;
        case GUARD_INTERVAL_1_32:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_32;
            break;
        case GUARD_INTERVAL_1_128:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e1_128;
            break;
        case GUARD_INTERVAL_19_128:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e19_128;
            break;
        case GUARD_INTERVAL_19_256:
            ldvb_context->ofdmSettings.modeSettings.guardInterval = NEXUS_FrontendOfdmGuardInterval_e19_256;
            break;
        default:
            LDVBON_ERR("%s: Unsupported guard_interval %u\n", __func__, c->guard_interval);
            return -1;
        }

        switch (c->transmission_mode) {
        case TRANSMISSION_MODE_1K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e1k;
            break;
        case TRANSMISSION_MODE_2K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e2k;
            break;
        case TRANSMISSION_MODE_4K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e4k;
            break;
        case TRANSMISSION_MODE_8K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e8k;
            break;
        case TRANSMISSION_MODE_16K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e16k;
            break;
        case TRANSMISSION_MODE_32K:
            ldvb_context->ofdmSettings.modeSettings.mode = NEXUS_FrontendOfdmTransmissionMode_e32k;
            break;
        default:
            LDVBON_ERR("%s: Unsupported transmission_mode %u\n", __func__, c->transmission_mode);
            return -1;
        }
    }

    if (c->modulation == QAM_AUTO || c->hierarchy == HIERARCHY_AUTO || c->code_rate_HP == FEC_AUTO || c->code_rate_LP == FEC_AUTO) {
        ldvb_context->ofdmSettings.manualTpsSettings = false;
    } else {
        ldvb_context->ofdmSettings.manualTpsSettings = true;

        switch (c->modulation) {
        case QPSK:
            ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQpsk;
            break;
        case QAM_16:
            ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam16;
            break;
        case QAM_64:
            ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam64;
            break;
        case QAM_256:
            ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eQam256;
            break;
        case DQPSK:
            ldvb_context->ofdmSettings.tpsSettings.modulation = NEXUS_FrontendOfdmModulation_eDqpsk;
            break;
        default:
            LDVBON_ERR("%s: Unsupported modulation %u\n", __func__, c->modulation);
            return -1;
        }

        switch (c->hierarchy) {
        case HIERARCHY_NONE:
            ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e0;
            break;
        case HIERARCHY_1:
            ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e1;
            break;
        case HIERARCHY_2:
            ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e2;
            break;
        case HIERARCHY_4:
            ldvb_context->ofdmSettings.tpsSettings.hierarchy = NEXUS_FrontendOfdmHierarchy_e4;
            break;
        default:
            LDVBON_ERR("%s: Unsupported hierarchy %u\n", __func__, c->hierarchy);
            return -1;
        }

        ret = nexus_fe_get_code_rate(c->code_rate_LP, &ldvb_context->ofdmSettings.tpsSettings.lowPriorityCodeRate);
        if (ret < 0) {
            LDVBON_ERR("%s: nexus_fe_get_code_rate() failed: %d\n", __func__, ret);
            return ret;
        }

        ret = nexus_fe_get_code_rate(c->code_rate_HP, &ldvb_context->ofdmSettings.tpsSettings.highPriorityCodeRate);
        if (ret < 0) {
            LDVBON_ERR("%s: nexus_fe_get_code_rate() failed: %d\n", __func__, ret);
            return ret;
        }
    }

    ldvb_context->fastStatus.lockStatus = NEXUS_FrontendLockStatus_eUnknown;
    ret = NEXUS_Frontend_TuneOfdm(ldvb_context->frontend, &ldvb_context->ofdmSettings);
    if (ret < 0) {
        LDVBON_ERR("%s: NEXUS_Frontend_TuneOfdm() failed: %d\n", __func__, ret);
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
    LDVBON_DBG("%s\n", __func__);

    return 0;
}

static int nexus_fe_get_tune_settings(struct dvb_frontend *fe,
            struct dvb_frontend_tune_settings *settings) {

    LDVBON_DBG("%s\n", __func__);

    settings->min_delay_ms = 2000;

    return 0;
}

static int nexus_fe_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
    //LDVBON_DBG("%s\n", __func__);

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
    LDVBON_DBG("%s\n", __func__);

    return 0;
}

static int nexus_fe_read_ber(struct dvb_frontend *fe, u32 *ber)
{
    LDVBON_DBG("%s\n", __func__);

    return 0;
}

static int nexus_fe_read_signal_strength(struct dvb_frontend *fe,
                     u16 *strength)
{
    LDVBON_DBG("%s\n", __func__);

    return 0;
}

static int nexus_fe_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
    LDVBON_DBG("%s\n", __func__);

    return 0;
}

static void nexus_fe_release(struct dvb_frontend *fe)
{
    LDVBON_DBG("%s\n", __func__);

    return;
}

static int nexus_dvb_dmx_start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    static NEXUS_RecpumpAddPidChannelSettings recpump_pid_settings;
    int ret = 0;

    LDVBON_DBG("%s\n", __func__);

    /* Open the audio and video pid channels */
    ldvb_context->pidChannels[dvbdmxfeed->index] = NEXUS_PidChannel_Open(ldvb_context->parserBand, dvbdmxfeed->pid, NULL);

    NEXUS_Recpump_GetDefaultAddPidChannelSettings(&recpump_pid_settings);

    LDVBON_DBG("%s: type %u\n", __func__, dvbdmxfeed->type);
    LDVBON_DBG("%s: pes_type %u\n", __func__, dvbdmxfeed->pes_type);

    if (dvbdmxfeed->type == DMX_TYPE_TS) {
        switch (dvbdmxfeed->pes_type) {
        case DMX_PES_VIDEO:
            recpump_pid_settings.pidType = NEXUS_PidType_eVideo;
            break;
        case DMX_PES_AUDIO:
            recpump_pid_settings.pidType = NEXUS_PidType_eAudio;
            break;
        case DMX_PES_TELETEXT:
        case DMX_PES_PCR:
        case DMX_PES_OTHER:
            recpump_pid_settings.pidType = NEXUS_PidType_eOther;
            break;
        default:
            LDVBON_ERR("%s: Unsupported pidType %u\n", __func__, dvbdmxfeed->pes_type);
            return -EINVAL;
        }
    } else {
        LDVBON_ERR("%s: Unsupported type %u\n", __func__, dvbdmxfeed->type);
        return -EINVAL;
    }

    ret = NEXUS_Recpump_AddPidChannel(ldvb_context->recpump, ldvb_context->pidChannels[dvbdmxfeed->index], &recpump_pid_settings);
    if (ret) {
        LDVBON_ERR("%s: NEXUS_Recpump_AddPidChannel() failed: %d\n", __func__, ret);
        return ret;
    }

    return ret;
}

static int nexus_dvb_dmx_stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
    LDVBON_DBG("%s\n", __func__);

    NEXUS_Recpump_RemovePidChannel(ldvb_context->recpump, ldvb_context->pidChannels[dvbdmxfeed->index]);
    NEXUS_PidChannel_Close(ldvb_context->pidChannels[dvbdmxfeed->index]);
    ldvb_context->pidChannels[dvbdmxfeed->index] = NULL;

    return 0;
}

struct dvb_frontend dvb_fe = {
    .ops = {
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
            .read_snr       = nexus_fe_read_snr,
            .read_ber       = nexus_fe_read_ber,
            .read_signal_strength = nexus_fe_read_signal_strength,
            .read_ucblocks      = nexus_fe_read_ucblocks,
            .release            = nexus_fe_release,
    },
};

static int __init
LDVBON_init(void)
{
    int ret;
    NEXUS_ParserBandSettings parserBandSettings;
    NEXUS_RecpumpSettings recpumpSettings;
    NEXUS_RecpumpOpenSettings recpumpOpenSettings;
    NEXUS_FrontendAcquireSettings frontendAcquireSettings;

    ldvb_context = kzalloc(sizeof(ldvb_context_t), GFP_KERNEL);

    ret = dvb_register_adapter(&ldvb_context->dvb_adap,
               "Nexus Frontend", THIS_MODULE,
               NULL, adapter_nr);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_register_adapter() failed: %d\n", __func__, ret);
        return ret;
    }

    ldvb_context->dvb_dmx.feednum = MAX_PIDCHANNELS;
    ldvb_context->dvb_dmx.filternum = MAX_PIDCHANNELS;
    ldvb_context->dvb_dmx.dmx.capabilities = DMX_TS_FILTERING;
    ldvb_context->dvb_dmx.start_feed = nexus_dvb_dmx_start_feed;
    ldvb_context->dvb_dmx.stop_feed = nexus_dvb_dmx_stop_feed;
    ret = dvb_dmx_init(&ldvb_context->dvb_dmx);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_dmx_init() failed: %d\n", __func__, ret);
        return ret;
    }

    ldvb_context->dvb_dmxdev.filternum = ldvb_context->dvb_dmx.filternum;
    ldvb_context->dvb_dmxdev.demux = &ldvb_context->dvb_dmx.dmx;
    ldvb_context->dvb_dmxdev.capabilities = 0;
    ret = dvb_dmxdev_init(&ldvb_context->dvb_dmxdev, &ldvb_context->dvb_adap);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_dmxdev_init() failed: %d\n", __func__, ret);
        return ret;
    }

    ret = dvb_register_frontend(&ldvb_context->dvb_adap, &dvb_fe);
    if (ret < 0) {
        LDVBON_ERR("%s: dvb_register_frontend() failed: %d", __func__, ret);
        return ret;
    }

    NEXUS_Recpump_GetDefaultOpenSettings(&recpumpOpenSettings);
    recpumpOpenSettings.data.dataReadyThreshold = 0;
    recpumpOpenSettings.data.atomSize = 0;
    ldvb_context->recpump = NEXUS_Recpump_Open(NEXUS_ANY_ID, &recpumpOpenSettings);
    if (!ldvb_context->recpump) {
        LDVBON_ERR("Unable to find open recpump");
        return -ENOMEM;
    }

    NEXUS_Recpump_GetSettings(ldvb_context->recpump, &recpumpSettings);
    recpumpSettings.data.dataReady.callback = dataready_callback_data;
    recpumpSettings.data.overflow.callback = overflow_callback;
    recpumpSettings.index.dataReady.callback = dataready_callback_index;
    recpumpSettings.index.overflow.callback = overflow_callback;
    NEXUS_Recpump_SetSettings(ldvb_context->recpump, &recpumpSettings);

    NEXUS_Frontend_GetDefaultAcquireSettings(&frontendAcquireSettings);
    frontendAcquireSettings.capabilities.ofdm = true;
    ldvb_context->frontend = NEXUS_Frontend_Acquire(&frontendAcquireSettings);
    if (!ldvb_context->frontend) {
        LDVBON_ERR("Unable to find OFDM-capable frontend");
        return -1;
    }

    /* Map a parser band to the streamer input band. */
    ldvb_context->parserBand = NEXUS_ParserBand_Open(NEXUS_ANY_ID);
    NEXUS_ParserBand_GetSettings(ldvb_context->parserBand, &parserBandSettings);
    parserBandSettings.transportType = TRANSPORT_TYPE;
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
    ldvb_context->ofdmSettings.asyncStatusReadyCallback.callback = async_status_ready_callback;
    ldvb_context->ofdmSettings.asyncStatusReadyCallback.context = ldvb_context;
    ldvb_context->ofdmSettings.asyncStatusReadyCallback.param = 0;
    ldvb_context->ofdmSettings.modeSettings.modeGuard = NEXUS_FrontendOfdmModeGuard_eAutoDvbt;
    ldvb_context->ofdmSettings.pullInRange = NEXUS_FrontendOfdmPullInRange_eWide;
    ldvb_context->ofdmSettings.cciMode = NEXUS_FrontendOfdmCciMode_eNone;

    ldvb_context->timer_task = kthread_run(nexus_fe_get_data_buffer_timer, (void *) ldvb_context, "LDVBON buffer reader");

    printk("LDVBON module loaded\n");

    return 0;
}

static void __exit LDVBON_cleanup(void)
{
    int i;

    kthread_stop(ldvb_context->timer_task);

    NEXUS_Frontend_Untune(ldvb_context->frontend);
    NEXUS_Frontend_Release(ldvb_context->frontend);

    NEXUS_Recpump_Stop(ldvb_context->recpump);
    NEXUS_Recpump_RemoveAllPidChannels(ldvb_context->recpump);
    NEXUS_Recpump_Close(ldvb_context->recpump);

    /* Close the audio and video pid channels */
    for (i = 0; i < MAX_PIDCHANNELS; i++) {
        if (ldvb_context->pidChannels[i]) {
            LDVBON_ERR("Cleaning up pidchannel idx: %u\n", i);
            NEXUS_PidChannel_Close(ldvb_context->pidChannels[i]);
        }
    }

    NEXUS_ParserBand_Close(ldvb_context->parserBand);

    /* unregister frontend */
    dvb_unregister_frontend(&dvb_fe);

    /* unregister demux device */
    dvb_dmxdev_release(&ldvb_context->dvb_dmxdev);
    dvb_dmx_release(&ldvb_context->dvb_dmx);

    /* unregister dvb adapter */
    dvb_unregister_adapter(&ldvb_context->dvb_adap);

    kfree(ldvb_context);

    LDVBON_MSG("LDVBON module removed\n");
}

module_init(LDVBON_init);
module_exit(LDVBON_cleanup);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Broadcom Limited");
MODULE_DESCRIPTION("Linux DVB nexus integration");