/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <psl1ght/lv2/net.h>

#include <lv2_syscall.h>
#include <udp_printf.h>

#include <sha1.h>
#include <aes.h>

#define USE_AIM /* use AIM to get IDPS */

#define AIM_PACKET_ID_GET_DEV_ID												0x19003
#define IDPS_SIZE																				16

#define INDI_INFO_MGR_PACKET_ID_GET_DATA_SIZE_BY_INDEX	0x17001
#define INDI_INFO_MGR_PACKET_ID_GET_DATA_BY_INDEX				0x17002
#define EID0_INDEX																			0

#define UPDATE_MGR_PACKET_ID_SET_TOKEN									0x600a
#define UPDATE_MGR_PACKET_ID_READ_EPROM									0x600b
#define UPDATE_MGR_PACKET_ID_WRITE_EPROM								0x600c
#define EPROM_QA_FLAG_OFFSET														0x48c0a
#define TOKEN_SIZE																			80

//spu_token_processor
// 1.00-3.56:
// token-hmac:    CC30C4229113DB25733553AFD06E8762B3729D9EFAA6D5F35A6F58BF38FF8B5F
//                58A25BD9C9B50B01D1AB4028676968EAC7F88833B662935D7506A6B5E0F9D97A
// token-key:     341812376291371c8bc756fffc611525403f95a8ef9d0c996482eec216b562ed
// token-iv:      E8663A69CD1A5C454A761E728C7C254E
//
// 3.60:
// token-public:  A55476C9E6DFB890FAAF5FBFFD961B649D0ABF1D0CD76000BD4B5F5AFE0AB87DCEA221E252A37404
// token-curve-type: 0x9

// token-key:
static uint8_t erk[] = {
	0x34, 0x18, 0x12, 0x37, 0x62, 0x91, 0x37, 0x1c, 0x8b, 0xc7, 0x56, 0xff, 0xfc, 0x61, 0x15, 0x25,
	0x40, 0x3f, 0x95, 0xa8, 0xef, 0x9d, 0x0c, 0x99, 0x64, 0x82, 0xee, 0xc2, 0x16, 0xb5, 0x62, 0xed
};

// token-iv:
static uint8_t iv[] = {
	0xe8, 0x66, 0x3a, 0x69, 0xcd, 0x1a, 0x5c, 0x45, 0x4a, 0x76, 0x1e, 0x72, 0x8c, 0x7c, 0x25, 0x4e
};

//token-hmac:
static uint8_t hmac[] = {
	0xcc, 0x30, 0xc4, 0x22, 0x91, 0x13, 0xdb, 0x25, 0x73, 0x35, 0x53, 0xaf, 0xd0, 0x6e, 0x87, 0x62,
	0xb3, 0x72, 0x9d, 0x9e, 0xfa, 0xa6, 0xd5, 0xf3, 0x5a, 0x6f, 0x58, 0xbf, 0x38, 0xff, 0x8b, 0x5f,
	0x58, 0xa2, 0x5b, 0xd9, 0xc9, 0xb5, 0x0b, 0x01, 0xd1, 0xab, 0x40, 0x28, 0x67, 0x69, 0x68, 0xea,
	0xc7, 0xf8, 0x88, 0x33, 0xb6, 0x62, 0x93, 0x5d, 0x75, 0x06, 0xa6, 0xb5, 0xe0, 0xf9, 0xd9, 0x7a
};

/* main */
int main(int argc, char **argv)
{
#ifndef USE_AIM
	uint64_t eid0_size;
	uint8_t eid0[4096];
#endif
	uint8_t value, idps[IDPS_SIZE], seed[TOKEN_SIZE], token[TOKEN_SIZE];
	AES_KEY aes_ctx;
	int i, result;

	netInitialize();
	udp_printf_init();
	PRINTF("%s:%d: start\n", __func__, __LINE__);

#ifdef USE_AIM //wenn AIM dann benutzen um IDPS zu bekommmen, eine möglichkeit

	//Use AIM to get IDPS
	result = lv2_ss_aim_if(AIM_PACKET_ID_GET_DEV_ID, (uint64_t) &idps);		// lv2_ss_aim_if(867, )
	if (result) {
		PRINTF("%s:%d: lv2_ss_aim_if(GET_DEV_ID) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

#else

	// Use Indi Info Manager to get EID0, ersten 16 byte von EID0 ist auch IDPS
	result = lv2_ss_indi_info_mgr_if(INDI_INFO_MGR_PACKET_ID_GET_DATA_SIZE_BY_INDEX, EID0_INDEX, (uint64_t) &eid0_size, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv2_ss_indi_info_mgr_if(GET_DATA_SIZE_BY_INDEX) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: EID0 size %ld\n", __func__, __LINE__, eid0_size);

	result = lv2_ss_indi_info_mgr_if(INDI_INFO_MGR_PACKET_ID_GET_DATA_BY_INDEX, EID0_INDEX, (uint64_t) eid0, sizeof(eid0), (uint64_t) &eid0_size);
	if (result) {
		PRINTF("%s:%d: lv2_ss_indi_info_mgr_if(GET_DATA_BY_INDEX) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: EID0 size %ld\n", __func__, __LINE__, eid0_size);

	//memcpy(ziel, quelle, größe) ersten 16 byte als IDPS aus EID0 holen
	memcpy(idps, eid0, IDPS_SIZE);

#endif

	//IDPS ausgeben(debug)
	PRINTF("%s: %d: IDPS: %02x %02x %02x %02x %02x %02x %02x %02x "
		"%02x %02x %02x %02x %02x %02x %02x %02x\n", __func__, __LINE__,
		idps[0], idps[1], idps[ 2], idps[ 3], idps[ 4], idps[ 5], idps[ 6], idps[ 7],
		idps[8], idps[9], idps[10], idps[11], idps[12], idps[13], idps[14], idps[15]);
	
	//seed erstellen:
	//memset(Zielspeicherblock, byte zum überschreiben, Anzahl zu kopierender Chars)
	memset(seed, 0, TOKEN_SIZE);				//leere token seed erstellen 80 byte groß nur 00h bytes's
	memcpy(seed + 4, idps, IDPS_SIZE);	//in der seed ab dem 4ten byte die IDPS einfügen
	seed[3] = 1;												//byte 3 der seed auf 01h setzen


	//hmac = key, sizeof(hmac) = key länge, seed = data in, 60 = data in länge, seed + 60 = fertiger hash ab zeichen 60 einfügen
	hmac_sha1(hmac, sizeof(hmac), seed, 60, seed + 60);


	//
	result = AES_set_encrypt_key(erk, 256, &aes_ctx);
	if (result) {
		PRINTF("%s:%d: AES_set_encrypt_key failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	//
	AES_cbc_encrypt(iv, seed, token, TOKEN_SIZE, &aes_ctx);

	PRINTF("%s: %d: TOKEN SEED:\n", __func__, __LINE__);
	for (i = 0; i < TOKEN_SIZE; i += 16)
		PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			seed[i +  0], seed[i +  1], seed[i +  2], seed[i +  3], seed[i +  4], seed[i +  5],
			seed[i +  6], seed[i +  7], seed[i +  8], seed[i +  9], seed[i + 10], seed[i + 11],
			seed[i + 12], seed[i + 13], seed[i + 14], seed[i + 15]);

	PRINTF("%s: %d: TOKEN:\n", __func__, __LINE__);
	for (i = 0; i < TOKEN_SIZE; i += 16)
		PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			token[i +  0], token[i +  1], token[i +  2], token[i +  3], token[i +  4], token[i +  5],
			token[i +  6], token[i +  7], token[i +  8], token[i +  9], token[i + 10], token[i + 11],
			token[i + 12], token[i + 13], token[i + 14], token[i + 15]);

	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_SET_TOKEN, (uint64_t) token, TOKEN_SIZE, 0, 0, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv1_ss_update_mgr_if(SET_TOKEN) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_READ_EPROM, EPROM_QA_FLAG_OFFSET, (uint64_t) &value, 0, 0, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv1_ss_update_mgr_if(READ_EPROM) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: current QA flag 0x%02x\n", __func__, __LINE__, value);

	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_WRITE_EPROM, EPROM_QA_FLAG_OFFSET, 0xff /* disable QA token */, 0, 0, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv1_ss_update_mgr_if(WRITE_EPROM) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

done:

	udp_printf_deinit();
	netDeinitialize();

	return 0;
}
