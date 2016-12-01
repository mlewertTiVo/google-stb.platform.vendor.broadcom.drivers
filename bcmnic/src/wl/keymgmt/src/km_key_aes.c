/*
 * Implementation of wlc_key algo 'aes'
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
 * $Id: km_key_aes.c 643153 2016-06-13 16:09:35Z $
 */

#include "km_key_aes_pvt.h"
#ifdef WLCXO_CTRL
#include <wlc_cxo_types.h>
#include <wlc_cxo_tsc.h>
#endif

/* internal interface */

/* BCMCCMP - s/w decr/encr for ccm */
#if !defined(BCMCCMP)
#define key_aes_rx_ccm(k, p, h, b, l, pkti) BCME_UNSUPPORTED
#define key_aes_tx_ccm(k, p, h, b, l) BCME_UNSUPPORTED
#endif /* !BCMGCMP */

/* BCMGCMP - s/w decr/encr for gcm */
#if !defined(BCMGCMP)
#define key_aes_rx_gcm(k, h, b, l, s, pkti) BCME_UNSUPPORTED
#define key_aes_tx_gcm(k, p, h, b, l, s) BCME_UNSUPPORTED
#endif /* !BCMGCMP */

#define AES_KEY_VALID(_key) AES_KEY_ALGO_VALID(_key)

static void
key_aes_seq_to_body(wlc_key_t *key, uint8 *seq, uint8 *body)
{
	if (!AES_KEY_ALGO_OCBXX(key)) {
		body[0] = seq[0];
		body[1] = seq[1];
		body[2] = 0;		/* reserved */
		body[3] = key->info.key_id << DOT11_KEY_INDEX_SHIFT | DOT11_EXT_IV_FLAG;
		body[4] = seq[2];
		body[5] = seq[3];
		body[6] = seq[4];
		body[7] = seq[5];
	} else {
		body[0] = seq[0];
		body[1] = seq[1];
		body[2] = seq[2];
		body[3] = key->info.key_id << DOT11_KEY_INDEX_SHIFT;
		body[3] |= seq[3] & 0xf;
	}
}

static void
key_aes_seq_from_body(wlc_key_t *key, uint8 *seq, uint8 *body)
{
	if (!AES_KEY_ALGO_OCBXX(key)) {
		seq[0] = body[0];
		seq[1] = body[1];
		seq[2] = body[4];
		seq[3] = body[5];
		seq[4] = body[6];
		seq[5] = body[7];
	} else {
		seq[0] = body[0];
		seq[1] = body[1];
		seq[2] = body[2];
		seq[3] = body[3] & 0xf;	/* only 28 bits for ocb  */
	}
}

static int
key_aes_destroy(wlc_key_t *key)
{
	KM_DBG_ASSERT(AES_KEY_VALID(key));

	if (key->algo_impl.ctx != NULL) {
		MFREE(KEY_OSH(key), key->algo_impl.ctx, AES_KEY_STRUCT_SIZE(key));
	}

	key->algo_impl.cb = NULL;
	key->algo_impl.ctx = NULL;
	key->info.key_len = 0;
	key->info.iv_len = 0;
	key->info.icv_len = 0;
	return BCME_OK;
}

static int
key_aes_get_data(wlc_key_t *key, uint8 *data, size_t data_size,
	size_t *data_len, key_data_type_t data_type, int ins, bool tx)
{
	uint8 *key_data;
	int err = BCME_OK;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY:
		if (data_len != NULL) {
			*data_len = key->info.key_len;
		}

		if (data_size < key->info.key_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->key; /* MFP related */
		} else {
			key_data = ((aes_key_t *)key->algo_impl.ctx)->key;
		}
		memcpy(data, key_data, key->info.key_len);
		break;
	case WLC_KEY_DATA_TYPE_SEQ:
		if (data_len != NULL) {
			*data_len = AES_KEY_SEQ_SIZE;
		}

		if (data_size < AES_KEY_SEQ_SIZE) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->seq; /* MFP related */
		} else {
			aes_key_t *aes_key;
			if (!AES_KEY_VALID_INS(key, tx, ins)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			aes_key = (aes_key_t *)key->algo_impl.ctx;
			key_data = AES_KEY_SEQ(key, aes_key, tx, ins);
		}
		memcpy(data, key_data, AES_KEY_SEQ_SIZE);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

static int
key_aes_set_data(wlc_key_t *key, const uint8 *data,
    size_t data_len, key_data_type_t data_type, int ins, bool tx)
{
	uint8 *key_data;
	int err = BCME_OK;
	int ins_begin;
	int ins_end;
	int seq_id;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	KEY_RESOLVE_SEQ(key, ins, tx, ins_begin, ins_end);

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY:
		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->key; /* MFP related */
		} else {
			key_data = ((aes_key_t *)key->algo_impl.ctx)->key;
		}

		if (data_len != key->info.key_len) {
			if (!data_len) {
				memset(key_data, 0, key->info.key_len);
				break;
			}
			if (data_len < key->info.key_len) {
				err = BCME_BADLEN;
				break;
			}
			data_len = key->info.key_len;
		}

		if (!data) {
			err = BCME_BADARG;
			break;
		}

		if (!memcmp(key_data, data, key->info.key_len)) /* no change ? */
			break;

		/* update the key. and re-init the tx/rx seq */
		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			memset(key->algo_impl.ctx, 0, sizeof(aes_igtk_t)); /* MFP related */
		} else {
			memset(key->algo_impl.ctx, 0, sizeof(aes_key_t));
		}

		/* aes legacy mode support */
		{
			scb_t *scb = wlc_keymgmt_get_scb(KEY_KM(key), key->info.key_idx);
			if (KM_SCB_LEGACY_AES(scb))
				key->info.flags |= WLC_KEY_FLAG_AES_MODE_LEGACY;
		}

		/* use ivtw (if configured), except for BIP */
#ifdef BRCMAPIVTW
		if (!WLC_KEY_ALGO_IS_BIPXX(key->info.algo))
			key->info.flags |= WLC_KEY_FLAG_USE_IVTW; /* if enabled */
#endif /* BRCMAPIVTW */

		memcpy(key_data, data, key->info.key_len);
		break;
	case WLC_KEY_DATA_TYPE_SEQ:
		for (seq_id = ins_begin; seq_id < ins_end; ++seq_id) {
			if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
				key_data = ((aes_igtk_t *)key->algo_impl.ctx)->seq; /* MFP */
			} else {
				aes_key_t *aes_key;
				if (!AES_KEY_VALID_INS(key, tx, seq_id)) {
					err = BCME_UNSUPPORTED;
					break;
				}
				aes_key = (aes_key_t *)key->algo_impl.ctx;
				key_data = AES_KEY_SEQ(key, aes_key, tx, seq_id);
			}

			if (!data_len) {
				memset(key_data, 0, AES_KEY_SEQ_SIZE);
				continue;
			}

			if (!data) {
				err = BCME_BADARG;
				break;
			}

			if (data_len < AES_KEY_SEQ_SIZE) /* ocb modes */
				memset(key_data, 0, AES_KEY_SEQ_SIZE);
			else if (data_len > AES_KEY_SEQ_SIZE) /* truncate */
				data_len = AES_KEY_SEQ_SIZE;

			memcpy(key_data, data, data_len);
		}
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

#ifdef BCMCCMP
static uint8
key_aes_ccm_nonce_flags(wlc_key_t *key, void *pkt,
	const struct dot11_header *hdr, uint8 *body, bool htc)
{
	uint16 fc;
	uint8 nonce_flags = 0;

	fc =  ltoh16(hdr->fc);

#if defined(MFP)
	if (FC_TYPE(fc) == FC_TYPE_MNG) {
		scb_t *scb = WLPKTTAGSCBGET(pkt);
		KM_ASSERT(scb != NULL);
		if (KM_SCB_MFP(scb))
			nonce_flags = AES_CCMP_NF_MANAGEMENT;
		else if (KM_SCB_CCX_MFP(scb))
			nonce_flags = 0xff;
	} else
#endif 
	if (KM_KEY_FC_TYPE_QOS_DATA(fc) &&
		!(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY)) {
		uint16 qc;
		uint8 *qcp = body;
		if (htc)
			qcp -= DOT11_HTC_LEN;
		qcp -= DOT11_QOS_LEN;
		qc = ltoh16_ua(qcp);
		nonce_flags = (uint8)(QOS_TID(qc) & AES_CCMP_NF_PRIORITY);
	}

	return nonce_flags;
}

static int
key_aes_rx_ccm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, const key_pkt_info_t *pkt_info)
{
	int err;
	size_t pkt_len;
	aes_key_t *aes_key;

	KM_DBG_ASSERT(AES_KEY_ALGO_CCMXX(key));

	pkt_len = (body + body_len) - (uint8 *)hdr;
	do {
		uint8 nonce_flags = key_aes_ccm_nonce_flags(key, pkt, hdr, body,
			(pkt_info->flags & KEY_PKT_HTC_PRESENT) != 0);
		{
			uint32 rk[4*(AES_MAXROUNDS+1)];
			int st;

			aes_key = (aes_key_t *)key->algo_impl.ctx;
			rijndaelKeySetupEnc(rk, aes_key->key, key->info.key_len << 3);
			st = aes_ccmp_decrypt(rk, key->info.key_len, pkt_len, (uint8 *)hdr,
				(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY), nonce_flags,
				key->info.icv_len);

			KM_DBG_ASSERT(st == AES_CCMP_DECRYPT_MIC_FAIL ||
				st == AES_CCMP_DECRYPT_SUCCESS);
			if (st != AES_CCMP_DECRYPT_SUCCESS)
				err = BCME_DECERR;
			else
				err = BCME_OK;
		}
	} while (0);

	return err;
}
#endif /* BCMCCMP */

#ifdef BCMGCMP
static void
key_aes_gcm_calc_params(const struct dot11_header *hdr, const uint8 *body,
	uint8 *nonce, size_t *nonce_len, uint8 *aad, size_t *aad_len, uint8 *seq, bool htc)
{
	uint8 *tmp;
	size_t i;
	uint16 fc;
	uint16 aad_fc;

	/* nonce - A2+PN. PN5 first 11adD9 */
	memcpy(nonce, hdr->a2.octet, ETHER_ADDR_LEN);
	tmp =  &nonce[ETHER_ADDR_LEN];
	for (i = AES_KEY_SEQ_SIZE; i > 0; --i)
		*(tmp++) = seq[i - 1];
	*nonce_len = tmp - nonce;

	/* process fc */
	fc =  ltoh16(hdr->fc);
	if (FC_TYPE(fc) == FC_TYPE_MNG)
		aad_fc = (fc & (FC_SUBTYPE_MASK | AES_CCMP_FC_MASK)); /* MFP and CCX */
	else
		aad_fc = (fc & AES_CCMP_FC_MASK); /* note: no legacy 11iD3 support */

	/* populate aad */
	tmp = aad;
	*tmp++ = aad_fc & 0xff;
	*tmp++ = aad_fc >> 8 & 0xff;
	memcpy(tmp, &hdr->a1, 3*ETHER_ADDR_LEN); /* copy A1, A2, A3 */
	tmp += 3*ETHER_ADDR_LEN;
	if ((fc & (FC_TODS|FC_FROMDS)) == (FC_TODS|FC_FROMDS)) { /* wds */
		memcpy(tmp, &hdr->a4, ETHER_ADDR_LEN);
		tmp += ETHER_ADDR_LEN;
	}

	if (KM_KEY_FC_TYPE_OQS_DATA(fc)) {
		uint16 qc;
		uint8 *qcp = body;
		if (htc)
			qcp -= DOT11_HTC_LEN;
		qcp -= DOT11_QOS_LEN;
		qc = ltoh16_ua(qcp);
		qc &= AES_KEY_AAD_QOS_MASK;
		*tmp++ = qc & 0xff;
		*tmp++ = qc >> 8 & 0xff;
	}

	*aad_len = tmp - aad;
}

static int
key_aes_rx_gcm(wlc_key_t *key, struct dot11_header *hdr,
	uint8 *body, int body_len, uint8 *rx_seq, const key_pkt_info_t *pkt_info)
{
	aes_key_t *aes_key;
	uint8 nonce[AES_BLOCK_SZ];
	size_t nonce_len;
	uint8 aad[AES_KEY_AAD_MAX_SIZE];
	size_t aad_len;
	int st;
	STATIC_ASSERT((AES_KEY_SEQ_SIZE + ETHER_ADDR_LEN) <= AES_BLOCK_SZ);

	KM_DBG_ASSERT(AES_KEY_ALGO_GCMXX(key));
	KM_DBG_ASSERT(key->info.icv_len == AES_BLOCK_SZ);

	key_aes_gcm_calc_params(hdr, body, nonce, &nonce_len,
		aad, &aad_len, rx_seq, (pkt_info->flags & KEY_PKT_HTC_PRESENT) != 0);
	aes_key = (aes_key_t *)key->algo_impl.ctx;
	st = aes_gcm_decrypt(aes_key->key, key->info.key_len,
		nonce, nonce_len, aad, aad_len,
		body + key->info.iv_len,
		body_len - (key->info.iv_len + key->info.icv_len),
		body + body_len - key->info.icv_len, key->info.icv_len);

	return st ? BCME_OK : BCME_DECERR;
}
#endif /* BCMGCMP */

static int
key_aes_rx_mpdu(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, const key_pkt_info_t *pkt_info)
{
	aes_key_t *aes_key;
	int err = BCME_OK;
	uint8 rx_seq[AES_KEY_SEQ_SIZE];
	key_seq_id_t ins = 0;
	uint16 fc;
	uint16 qc = 0;

	KM_DBG_ASSERT(AES_KEY_VALID(key) && pkt_info != NULL);

	aes_key = (aes_key_t *)key->algo_impl.ctx;

	fc =  ltoh16(hdr->fc);
	if ((FC_TYPE(fc) == FC_TYPE_MNG) && ETHER_ISMULTI(&hdr->a1)) {
#if defined(MFP)
		scb_t *scb = WLPKTTAGSCBGET(pkt);
#ifdef MFP
		if (KM_SCB_MFP(scb)) {
			err = km_key_aes_rx_mmpdu_mcmfp(key, pkt, hdr, body, body_len, pkt_info);
			goto done;
		}
#endif /* MFP */
		if (!KM_SCB_CCX_MFP(scb)) {
			err = BCME_UNSUPPORTED;
			goto done;
		}
#endif /* MFP || CCX */
	}


	/* ext iv bit must be set for all except ocb modes */
	if (!(body[KEY_ID_BODY_OFFSET] & DOT11_EXT_IV_FLAG) &&
		!AES_KEY_ALGO_OCBXX(key)) {
		KEY_LOG(("wl%d: %s: EXT IV is not set for pkt, key idx %d\n",
			KEY_WLUNIT(key), __FUNCTION__, key->info.key_idx));
		err = BCME_BADOPTION;
		goto done;
	}

	/* check replay with applicable counter */
	if (FC_TYPE(fc) == FC_TYPE_MNG) {
#ifdef MFP
		ins = KEY_NUM_RX_SEQ(key) - 1;
#else
		err = BCME_UNSUPPORTED;
		goto done;
#endif
	} else if (pkt_info->flags & KEY_PKT_QC_PRESENT) {
		qc = pkt_info->qc;
		ins = PRIO2IVIDX(QOS_PRIO(qc));
		KM_DBG_ASSERT(ins < KEY_NUM_RX_SEQ(key));
	}

	key_aes_seq_from_body(key, rx_seq, body);

	/* do replay check for first packet in chain (could be length of 1). if
	 * the decryption already failed, skip the replay check as we'll discard it
	 * later anyway
	 */
	if ((pkt_info->flags & KEY_PKT_PKTC_FIRST) && (pkt_info->status == BCME_OK)) {
		if (km_is_replay(KEY_KM(key), &key->info, ins,
			AES_KEY_SEQ(key, aes_key, FALSE /* rx */, ins),
			rx_seq, AES_KEY_SEQ_SIZE)) {
			err = BCME_REPLAY;
			goto done;
		}
	}

	/* nothing else for hw decrypt. caller checks hw decr status */
	if (pkt_info->flags & KEY_PKT_HWDEC) {
		err = pkt_info->status;
		goto done;
	}

	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
	case CRYPTO_ALGO_AES_CCM256:
		err = key_aes_rx_ccm(key, pkt, hdr, body, body_len, pkt_info);
		break;
	case CRYPTO_ALGO_AES_GCM:
	case CRYPTO_ALGO_AES_GCM256:
		err = key_aes_rx_gcm(key, hdr, body, body_len, rx_seq, pkt_info);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

done:
	if (err == BCME_OK) {
		/* on success, update rx iv in key for data & uc mgmt frames. for ivtw, the
		 * seq window is already updated.
		 */
		if (!WLC_KEY_USE_IVTW(&key->info)) {
			if ((FC_TYPE(fc) != FC_TYPE_MNG) || !ETHER_ISMULTI(&hdr->a1)) {
				memcpy(AES_KEY_SEQ(key, aes_key, FALSE /* rx */, ins),
					rx_seq, AES_KEY_SEQ_SIZE);
			}
		}
#ifdef BRCMAPIVTW
		else {
			bool chained;
			/* for intermediate frames in a chain, skip the iv update. for last, but
			 * not the first frame (chain of 1) do a chained update.
			 */
			chained = (pkt_info->flags & KEY_PKT_PKTC_LAST) &&
				!(pkt_info->flags & KEY_PKT_PKTC_FIRST);
			if (pkt_info->flags & (KEY_PKT_PKTC_FIRST | KEY_PKT_PKTC_LAST))
				KM_UPDATE_IVTW(KEY_KM(key), &key->info, ins, rx_seq,
					AES_KEY_SEQ_SIZE, chained);
		}
#endif
	} else { /* update counters */
		switch (err) {
		case BCME_REPLAY:
			{
				WLCNTINCR(KEY_CNT(key)->ccmpreplay);
				if (ETHER_ISMULTI(&hdr->a1))
					WLCNTINCR(KEY_CNT(key)->ccmpreplay_mcst);
			}
			break;

		case BCME_DECERR:
			{
				WLCNTINCR(KEY_CNT(key)->ccmpfmterr);
				if (ETHER_ISMULTI(&hdr->a1))
					WLCNTINCR(KEY_CNT(key)->ccmpfmterr_mcst);
			}
			/* fall through */
		default:
			WLCNTINCR(KEY_CNT(key)->ccmpundec);
			if (ETHER_ISMULTI(&hdr->a1))
				WLCNTINCR(KEY_CNT(key)->ccmpundec_mcst);
			break;
		}
	}

	return err;
}

#ifdef BCMCCMP
static int
key_aes_tx_ccm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len)
{
	int err = BCME_OK;
	size_t pkt_len;
	aes_key_t *aes_key;

	pkt_len = (body + body_len) - (uint8 *)hdr;
	do {
		{
			uint32 rk[4*(AES_MAXROUNDS+1)];
			int st;

			aes_key = (aes_key_t *)key->algo_impl.ctx;
			rijndaelKeySetupEnc(rk, aes_key->key, key->info.key_len << 3);
			st = aes_ccmp_encrypt(rk, key->info.key_len, pkt_len, (uint8 *)hdr,
				(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY),
				key_aes_ccm_nonce_flags(key, pkt, hdr, body, FALSE),
				key->info.icv_len);

			if (st != AES_CCMP_ENCRYPT_SUCCESS)
				err = BCME_ENCERR;
		}
	} while (0);

	return err;
}
#endif /* BCMCCMP */

#ifdef BCMGCMP
static int
key_aes_tx_gcm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
    uint8 *body, int body_len, uint8 *tx_seq)
{
	aes_key_t *aes_key;
	uint8 nonce[AES_BLOCK_SZ];
	size_t nonce_len;
	uint8 aad[AES_KEY_AAD_MAX_SIZE];
	size_t aad_len;
	STATIC_ASSERT((AES_KEY_SEQ_SIZE + ETHER_ADDR_LEN) <= AES_BLOCK_SZ);

	KM_DBG_ASSERT(AES_KEY_ALGO_GCMXX(key));
	KM_DBG_ASSERT(key->info.icv_len == AES_BLOCK_SZ);

	key_aes_gcm_calc_params(hdr, body, nonce, &nonce_len,
		aad, &aad_len, tx_seq, FALSE);

	aes_key = (aes_key_t *)key->algo_impl.ctx;
	aes_gcm_encrypt(aes_key->key, key->info.key_len,
		nonce, nonce_len, aad, aad_len,
		body + key->info.iv_len,
		body_len - key->info.iv_len,
		body + body_len, key->info.icv_len);

	return BCME_OK;
}
#endif /* BCMGCMP */

static int
key_aes_tx_mpdu(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, wlc_txd_t *txd)
{
	int err = BCME_OK;
	aes_key_t *aes_key;
	uint16 fc;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	fc =  ltoh16(hdr->fc);
	if ((FC_TYPE(fc) == FC_TYPE_MNG) && ETHER_ISMULTI(&hdr->a1)) {
		err = km_key_aes_tx_mmpdu_mcmfp(key, pkt, hdr, body, body_len, txd);
		goto done;
	}

	aes_key = (aes_key_t *)key->algo_impl.ctx;

	{
		if (!AES_KEY_ALGO_OCBXX(key)) {
			if (KEY_SEQ_IS_MAX(aes_key->tx_seq)) {
				err = BCME_BADKEYIDX;
				goto done;
			}
#ifdef WLCXO_CTRL
			/* Do not update txiv in offload for MPF frames. */
			if (WLCXO_ENAB(key->wlc->pub) &&
			    ((WLPKTTAG(pkt)->flags & WLF_MFP) == 0)) {
				wlc_cx_tsc_t *tsc = NULL;
				ASSERT(pkt != NULL);
#ifdef WLCXO_IPC
				tsc = wlc_cxo_ctrl_tsc_find(key->wlc, -1, &hdr->a1, NULL);
#else
				tsc = wlc_cxo_tsc_find(key->wlc->wlc_cx, -1, &hdr->a1, NULL);
#endif
				if (tsc != NULL) {
					WLPKTTAG(pkt)->flags |= WLF_CXO_TXIV_OFLD_INIT;
					WLPKTTAG(pkt)->flags |= WLF_CXO_TXIV_OFLD_INCR;
				}
			}
#endif /* WLCXO_CTRL */
			KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
		} else {
			uint32 val = ltoh32_ua(aes_key->tx_seq);
			if (val >= ((1 << 28) - 3)) {
				memset(aes_key->tx_seq, 0, AES_KEY_SEQ_SIZE);
			} else {
				KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
			}
		}
	}

	/* update iv in pkt - seq and key id  */
	key_aes_seq_to_body(key, aes_key->tx_seq, body);

	if (WLC_KEY_IN_HW(&key->info) && txd != NULL) {
		if (!KEY_USE_AC_TXD(key)) {
			d11txh_t *txh = (d11txh_t *)&txd->d11txh;
			memcpy(txh->IV, body, key->info.iv_len);
		} else {
			d11actxh_t * txh = (d11actxh_t *)&txd->txd;
			d11actxh_cache_t	*cache_info;

			cache_info = WLC_TXD_CACHE_INFO_GET(txh, KEY_PUB(key)->corerev);
			memcpy(cache_info->TSCPN, aes_key->tx_seq,
				MIN(sizeof(cache_info->TSCPN), AES_KEY_SEQ_SIZE));
		}

		goto done;
	}


	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
	case CRYPTO_ALGO_AES_CCM256:
		err = key_aes_tx_ccm(key, pkt, hdr, body, body_len);
		break;
	case CRYPTO_ALGO_AES_GCM:
	case CRYPTO_ALGO_AES_GCM256:
		err = key_aes_tx_gcm(key, pkt, hdr, body, body_len, aes_key->tx_seq);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}


done:
	return err;
}

#define KEY_AES_DUMP NULL

static const key_algo_callbacks_t
km_key_aes_callbacks = {
    key_aes_destroy,	/* destroy */
    key_aes_get_data,	/* get data */
    key_aes_set_data,	/* set data */
    key_aes_rx_mpdu,	/* rx mpdu */
    NULL,				/* rx msdu */
    key_aes_tx_mpdu,	/* tx mpdu */
    NULL,				/* tx msdu */
    KEY_AES_DUMP		/* dump */
};

/* public interface */
int
km_key_aes_init(wlc_key_t *key)
{
	int err = BCME_OK;

	KM_DBG_ASSERT(key != NULL && AES_KEY_ALGO_VALID(key));

	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
#ifdef MFP
	case CRYPTO_ALGO_BIP:
#endif /* MFP */
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MIN_ICV_SIZE;
		break;

	case CRYPTO_ALGO_AES_GCM: /* 802.11adD9 */
#ifdef MFP
	case CRYPTO_ALGO_BIP_GMAC:
#endif /* MFP */
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MAX_ICV_SIZE;
		break;

	case CRYPTO_ALGO_AES_CCM256:
#ifdef MFP
	case CRYPTO_ALGO_BIP_CMAC256:
#endif /* MFP */
	case CRYPTO_ALGO_AES_GCM256:
	case CRYPTO_ALGO_BIP_GMAC256:
		key->info.key_len = AES_KEY_MAX_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MAX_ICV_SIZE;
		break;
	case CRYPTO_ALGO_AES_OCB_MSDU:
	case CRYPTO_ALGO_AES_OCB_MPDU:
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = DOT11_IV_AES_OCB_LEN;
		key->info.icv_len = AES_KEY_MIN_ICV_SIZE;
		break;
	default:
		key->algo_impl.cb = NULL;
		key->algo_impl.ctx = NULL;
		err = BCME_UNSUPPORTED;
		break;
	}

	if (err != BCME_OK)
		goto done;

	key->algo_impl.cb = &km_key_aes_callbacks;
	key->algo_impl.ctx = MALLOCZ(KEY_OSH(key), AES_KEY_STRUCT_SIZE(key));
	err = (key->algo_impl.ctx != NULL) ? BCME_OK : BCME_NOMEM;
done:
	return err;
}
