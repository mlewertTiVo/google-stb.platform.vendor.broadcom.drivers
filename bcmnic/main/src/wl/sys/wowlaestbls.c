/* Init required for broadcast key rotation for Wake-on-wireless LAN
 *
 * THIS IS GENERATED FILE and should remain CONSTANT!!!
 *
 * Copyright (C) 2016, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wowlaestbls.c 467328 2014-04-03 01:23:40Z $
 */

#include <typedefs.h>
CONST uint16 aes_invsbox[128] = {
	0x0952,
	0xd56a,
	0x3630,
	0x38a5,
	0x40bf,
	0x9ea3,
	0xf381,
	0xfbd7,
	0xe37c,
	0x8239,
	0x2f9b,
	0x87ff,
	0x8e34,
	0x4443,
	0xdec4,
	0xcbe9,
	0x7b54,
	0x3294,
	0xc2a6,
	0x3d23,
	0x4cee,
	0x0b95,
	0xfa42,
	0x4ec3,
	0x2e08,
	0x66a1,
	0xd928,
	0xb224,
	0x5b76,
	0x49a2,
	0x8b6d,
	0x25d1,
	0xf872,
	0x64f6,
	0x6886,
	0x1698,
	0xa4d4,
	0xcc5c,
	0x655d,
	0x92b6,
	0x706c,
	0x5048,
	0xedfd,
	0xdab9,
	0x155e,
	0x5746,
	0x8da7,
	0x849d,
	0xd890,
	0x00ab,
	0xbc8c,
	0x0ad3,
	0xe4f7,
	0x0558,
	0xb3b8,
	0x0645,
	0x2cd0,
	0x8f1e,
	0x3fca,
	0x020f,
	0xafc1,
	0x03bd,
	0x1301,
	0x6b8a,
	0x913a,
	0x4111,
	0x674f,
	0xeadc,
	0xf297,
	0xcecf,
	0xb4f0,
	0x73e6,
	0xac96,
	0x2274,
	0xade7,
	0x8535,
	0xf9e2,
	0xe837,
	0x751c,
	0x6edf,
	0xf147,
	0x711a,
	0x291d,
	0x89c5,
	0xb76f,
	0x0e62,
	0x18aa,
	0x1bbe,
	0x56fc,
	0x4b3e,
	0xd2c6,
	0x2079,
	0xdb9a,
	0xfec0,
	0xcd78,
	0xf45a,
	0xdd1f,
	0x33a8,
	0x0788,
	0x31c7,
	0x12b1,
	0x5910,
	0x8027,
	0x5fec,
	0x5160,
	0xa97f,
	0xb519,
	0x0d4a,
	0xe52d,
	0x9f7a,
	0xc993,
	0xef9c,
	0xe0a0,
	0x4d3b,
	0x2aae,
	0xb0f5,
	0xebc8,
	0x3cbb,
	0x5383,
	0x6199,
	0x2b17,
	0x7e04,
	0x77ba,
	0x26d6,
	0x69e1,
	0x6314,
	0x2155,
	0x7d0c,
	};

CONST uint16 aes_xtime9dbe[512] = {
	0x0000,
	0x0000,
	0x0d09,
	0x0e0b,
	0x1a12,
	0x1c16,
	0x171b,
	0x121d,
	0x3424,
	0x382c,
	0x392d,
	0x3627,
	0x2e36,
	0x243a,
	0x233f,
	0x2a31,
	0x6848,
	0x7058,
	0x6541,
	0x7e53,
	0x725a,
	0x6c4e,
	0x7f53,
	0x6245,
	0x5c6c,
	0x4874,
	0x5165,
	0x467f,
	0x467e,
	0x5462,
	0x4b77,
	0x5a69,
	0xd090,
	0xe0b0,
	0xdd99,
	0xeebb,
	0xca82,
	0xfca6,
	0xc78b,
	0xf2ad,
	0xe4b4,
	0xd89c,
	0xe9bd,
	0xd697,
	0xfea6,
	0xc48a,
	0xf3af,
	0xca81,
	0xb8d8,
	0x90e8,
	0xb5d1,
	0x9ee3,
	0xa2ca,
	0x8cfe,
	0xafc3,
	0x82f5,
	0x8cfc,
	0xa8c4,
	0x81f5,
	0xa6cf,
	0x96ee,
	0xb4d2,
	0x9be7,
	0xbad9,
	0xbb3b,
	0xdb7b,
	0xb632,
	0xd570,
	0xa129,
	0xc76d,
	0xac20,
	0xc966,
	0x8f1f,
	0xe357,
	0x8216,
	0xed5c,
	0x950d,
	0xff41,
	0x9804,
	0xf14a,
	0xd373,
	0xab23,
	0xde7a,
	0xa528,
	0xc961,
	0xb735,
	0xc468,
	0xb93e,
	0xe757,
	0x930f,
	0xea5e,
	0x9d04,
	0xfd45,
	0x8f19,
	0xf04c,
	0x8112,
	0x6bab,
	0x3bcb,
	0x66a2,
	0x35c0,
	0x71b9,
	0x27dd,
	0x7cb0,
	0x29d6,
	0x5f8f,
	0x03e7,
	0x5286,
	0x0dec,
	0x459d,
	0x1ff1,
	0x4894,
	0x11fa,
	0x03e3,
	0x4b93,
	0x0eea,
	0x4598,
	0x19f1,
	0x5785,
	0x14f8,
	0x598e,
	0x37c7,
	0x73bf,
	0x3ace,
	0x7db4,
	0x2dd5,
	0x6fa9,
	0x20dc,
	0x61a2,
	0x6d76,
	0xadf6,
	0x607f,
	0xa3fd,
	0x7764,
	0xb1e0,
	0x7a6d,
	0xbfeb,
	0x5952,
	0x95da,
	0x545b,
	0x9bd1,
	0x4340,
	0x89cc,
	0x4e49,
	0x87c7,
	0x053e,
	0xddae,
	0x0837,
	0xd3a5,
	0x1f2c,
	0xc1b8,
	0x1225,
	0xcfb3,
	0x311a,
	0xe582,
	0x3c13,
	0xeb89,
	0x2b08,
	0xf994,
	0x2601,
	0xf79f,
	0xbde6,
	0x4d46,
	0xb0ef,
	0x434d,
	0xa7f4,
	0x5150,
	0xaafd,
	0x5f5b,
	0x89c2,
	0x756a,
	0x84cb,
	0x7b61,
	0x93d0,
	0x697c,
	0x9ed9,
	0x6777,
	0xd5ae,
	0x3d1e,
	0xd8a7,
	0x3315,
	0xcfbc,
	0x2108,
	0xc2b5,
	0x2f03,
	0xe18a,
	0x0532,
	0xec83,
	0x0b39,
	0xfb98,
	0x1924,
	0xf691,
	0x172f,
	0xd64d,
	0x768d,
	0xdb44,
	0x7886,
	0xcc5f,
	0x6a9b,
	0xc156,
	0x6490,
	0xe269,
	0x4ea1,
	0xef60,
	0x40aa,
	0xf87b,
	0x52b7,
	0xf572,
	0x5cbc,
	0xbe05,
	0x06d5,
	0xb30c,
	0x08de,
	0xa417,
	0x1ac3,
	0xa91e,
	0x14c8,
	0x8a21,
	0x3ef9,
	0x8728,
	0x30f2,
	0x9033,
	0x22ef,
	0x9d3a,
	0x2ce4,
	0x06dd,
	0x963d,
	0x0bd4,
	0x9836,
	0x1ccf,
	0x8a2b,
	0x11c6,
	0x8420,
	0x32f9,
	0xae11,
	0x3ff0,
	0xa01a,
	0x28eb,
	0xb207,
	0x25e2,
	0xbc0c,
	0x6e95,
	0xe665,
	0x639c,
	0xe86e,
	0x7487,
	0xfa73,
	0x798e,
	0xf478,
	0x5ab1,
	0xde49,
	0x57b8,
	0xd042,
	0x40a3,
	0xc25f,
	0x4daa,
	0xcc54,
	0xdaec,
	0x41f7,
	0xd7e5,
	0x4ffc,
	0xc0fe,
	0x5de1,
	0xcdf7,
	0x53ea,
	0xeec8,
	0x79db,
	0xe3c1,
	0x77d0,
	0xf4da,
	0x65cd,
	0xf9d3,
	0x6bc6,
	0xb2a4,
	0x31af,
	0xbfad,
	0x3fa4,
	0xa8b6,
	0x2db9,
	0xa5bf,
	0x23b2,
	0x8680,
	0x0983,
	0x8b89,
	0x0788,
	0x9c92,
	0x1595,
	0x919b,
	0x1b9e,
	0x0a7c,
	0xa147,
	0x0775,
	0xaf4c,
	0x106e,
	0xbd51,
	0x1d67,
	0xb35a,
	0x3e58,
	0x996b,
	0x3351,
	0x9760,
	0x244a,
	0x857d,
	0x2943,
	0x8b76,
	0x6234,
	0xd11f,
	0x6f3d,
	0xdf14,
	0x7826,
	0xcd09,
	0x752f,
	0xc302,
	0x5610,
	0xe933,
	0x5b19,
	0xe738,
	0x4c02,
	0xf525,
	0x410b,
	0xfb2e,
	0x61d7,
	0x9a8c,
	0x6cde,
	0x9487,
	0x7bc5,
	0x869a,
	0x76cc,
	0x8891,
	0x55f3,
	0xa2a0,
	0x58fa,
	0xacab,
	0x4fe1,
	0xbeb6,
	0x42e8,
	0xb0bd,
	0x099f,
	0xead4,
	0x0496,
	0xe4df,
	0x138d,
	0xf6c2,
	0x1e84,
	0xf8c9,
	0x3dbb,
	0xd2f8,
	0x30b2,
	0xdcf3,
	0x27a9,
	0xceee,
	0x2aa0,
	0xc0e5,
	0xb147,
	0x7a3c,
	0xbc4e,
	0x7437,
	0xab55,
	0x662a,
	0xa65c,
	0x6821,
	0x8563,
	0x4210,
	0x886a,
	0x4c1b,
	0x9f71,
	0x5e06,
	0x9278,
	0x500d,
	0xd90f,
	0x0a64,
	0xd406,
	0x046f,
	0xc31d,
	0x1672,
	0xce14,
	0x1879,
	0xed2b,
	0x3248,
	0xe022,
	0x3c43,
	0xf739,
	0x2e5e,
	0xfa30,
	0x2055,
	0xb79a,
	0xec01,
	0xba93,
	0xe20a,
	0xad88,
	0xf017,
	0xa081,
	0xfe1c,
	0x83be,
	0xd42d,
	0x8eb7,
	0xda26,
	0x99ac,
	0xc83b,
	0x94a5,
	0xc630,
	0xdfd2,
	0x9c59,
	0xd2db,
	0x9252,
	0xc5c0,
	0x804f,
	0xc8c9,
	0x8e44,
	0xebf6,
	0xa475,
	0xe6ff,
	0xaa7e,
	0xf1e4,
	0xb863,
	0xfced,
	0xb668,
	0x670a,
	0x0cb1,
	0x6a03,
	0x02ba,
	0x7d18,
	0x10a7,
	0x7011,
	0x1eac,
	0x532e,
	0x349d,
	0x5e27,
	0x3a96,
	0x493c,
	0x288b,
	0x4435,
	0x2680,
	0x0f42,
	0x7ce9,
	0x024b,
	0x72e2,
	0x1550,
	0x60ff,
	0x1859,
	0x6ef4,
	0x3b66,
	0x44c5,
	0x366f,
	0x4ace,
	0x2174,
	0x58d3,
	0x2c7d,
	0x56d8,
	0x0ca1,
	0x377a,
	0x01a8,
	0x3971,
	0x16b3,
	0x2b6c,
	0x1bba,
	0x2567,
	0x3885,
	0x0f56,
	0x358c,
	0x015d,
	0x2297,
	0x1340,
	0x2f9e,
	0x1d4b,
	0x64e9,
	0x4722,
	0x69e0,
	0x4929,
	0x7efb,
	0x5b34,
	0x73f2,
	0x553f,
	0x50cd,
	0x7f0e,
	0x5dc4,
	0x7105,
	0x4adf,
	0x6318,
	0x47d6,
	0x6d13,
	0xdc31,
	0xd7ca,
	0xd138,
	0xd9c1,
	0xc623,
	0xcbdc,
	0xcb2a,
	0xc5d7,
	0xe815,
	0xefe6,
	0xe51c,
	0xe1ed,
	0xf207,
	0xf3f0,
	0xff0e,
	0xfdfb,
	0xb479,
	0xa792,
	0xb970,
	0xa999,
	0xae6b,
	0xbb84,
	0xa362,
	0xb58f,
	0x805d,
	0x9fbe,
	0x8d54,
	0x91b5,
	0x9a4f,
	0x83a8,
	0x9746,
	0x8da3,
	};
