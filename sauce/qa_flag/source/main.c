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

// IDPS = eine Folge von Bytes die den Konsolen Typ definiert
#define USE_AIM 																												// benutze AIM zum erhalten der IDPS

#define AIM_PACKET_ID_GET_DEV_ID												0x19003					// aim_spu_module.self Packet ID = 0x19003
#define IDPS_SIZE																				16							// IDPS größe in Byte

#define INDI_INFO_MGR_PACKET_ID_GET_DATA_SIZE_BY_INDEX	0x17001					// Indi Info Manager Packet ID = 0x17001 (Read EID Data Size By Index/Read metldr Size)
#define INDI_INFO_MGR_PACKET_ID_GET_DATA_BY_INDEX				0x17002					// Indi Info Manager Packet ID = 0x17002 (Read EID Data By Index/Read metldr)
#define EID0_INDEX																			0								// EID0_INDEX = 0

#define UPDATE_MGR_PACKET_ID_SET_TOKEN									0x600a					// Update Manager Packet ID = 0x600a (Set Token)
#define UPDATE_MGR_PACKET_ID_READ_EPROM									0x600b					// Update Manager Packet ID = 0x600b (Read EPROM)
#define EPROM_QA_FLAG_OFFSET														0x48c0a					// Position(offset) des QA_Flags im EPROM
#define TOKEN_SIZE																			80							// Token größe in Byte

// array: erk zum decrypten/encrypten mit aes256cbc
static uint8_t erk[] = {
	0x34, 0x18, 0x12, 0x37, 0x62, 0x91, 0x37, 0x1c, 0x8b, 0xc7, 0x56, 0xff, 0xfc, 0x61, 0x15, 0x25,
	0x40, 0x3f, 0x95, 0xa8, 0xef, 0x9d, 0x0c, 0x99, 0x64, 0x82, 0xee, 0xc2, 0x16, 0xb5, 0x62, 0xed
};

// array: iv(Initialisierungsvektor)
static uint8_t iv[] = {
	0xe8, 0x66, 0x3a, 0x69, 0xcd, 0x1a, 0x5c, 0x45, 0x4a, 0x76, 0x1e, 0x72, 0x8c, 0x7c, 0x25, 0x4e
};

// array: hmac zum erstellen der 20 byte Signatur am ende des token
static uint8_t hmac[] = {
	0xcc, 0x30, 0xc4, 0x22, 0x91, 0x13, 0xdb, 0x25, 0x73, 0x35, 0x53, 0xaf, 0xd0, 0x6e, 0x87, 0x62,
	0xb3, 0x72, 0x9d, 0x9e, 0xfa, 0xa6, 0xd5, 0xf3, 0x5a, 0x6f, 0x58, 0xbf, 0x38, 0xff, 0x8b, 0x5f,
	0x58, 0xa2, 0x5b, 0xd9, 0xc9, 0xb5, 0x0b, 0x01, 0xd1, 0xab, 0x40, 0x28, 0x67, 0x69, 0x68, 0xea,
	0xc7, 0xf8, 0x88, 0x33, 0xb6, 0x62, 0x93, 0x5d, 0x75, 0x06, 0xa6, 0xb5, 0xe0, 0xf9, 0xd9, 0x7a
};

// main
int main(int argc, char **argv)
{
#ifndef USE_AIM																													// wenn nicht existiert USE_AIM, dann
	uint64_t eid0_size;																										// eid0_size
	uint8_t eid0[4096];																										// eid0[4096]
#endif																																	// ende der if Struktur
	uint8_t value, idps[IDPS_SIZE], seed[TOKEN_SIZE], token[TOKEN_SIZE];
	AES_KEY aes_ctx;
	int i, result;

	netInitialize();																											//netzwerk init(zum debugen)

	udp_printf_init();																										//udp_printf init(udp protokoll für debug nachrichten)

	PRINTF("%s:%d: start\n", __func__, __LINE__);
	
#ifdef USE_AIM

	// benutze AIM zum erhalten der IDPS
	//
	result = lv2_ss_aim_if(AIM_PACKET_ID_GET_DEV_ID, (uint64_t) &idps);
	if (result) {
		PRINTF("%s:%d: lv2_ss_aim_if(GET_DEV_ID) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

#else

	// Wenn kein AIM, benutze Indi Info Manager zum erhalten der EID0
	// 868, CEX/DEX, root, aim_manager_if:
	// 0x17001, Read EID Data Size By Index/Read metldr Size,
	// 0x17002, Read EID Data By Index/Read metldr
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

	//kopiere aus der EID0 die ersten 16 Byte(IDPS_SIZE) und speichere sie als IDPS
	// (ziel(idps16byte), quelle(eid0), länge(IDPS_SIZE,16byte)
	memcpy(idps, eid0, IDPS_SIZE);

#endif
	
	//ausgeben der IDPS (zum debugen)
	PRINTF("%s: %d: IDPS: %02x %02x %02x %02x %02x %02x %02x %02x "
		"%02x %02x %02x %02x %02x %02x %02x %02x\n", __func__, __LINE__,
		idps[0], idps[1], idps[ 2], idps[ 3], idps[ 4], idps[ 5], idps[ 6], idps[ 7],
		idps[8], idps[9], idps[10], idps[11], idps[12], idps[13], idps[14], idps[15]);

	//token seed erstellen/zusammenbauen:
	memset(seed, 0, TOKEN_SIZE);																					//seed = TOKEN_SIZE(80 Byte) nur nullbytes (00h)
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	
	memcpy(seed + 4, idps, IDPS_SIZE);																		//ab Byte 4, IDPS_SIZE(16 Byte) mit IDPS überschreiben
	//00 00 00 00 00 00 00 01  00 85 00 0a 14 01 a7 70
	//6b 06 1c 0e 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	
	//verschiedene settings für neuen token
	seed[3] = 1;																													//Byte 3 auf 01h setzen
	seed[39] |= 0x1; 																											//QA_FLAG_EXAM_API_ENABLE
	seed[39] |= 0x2; 																											//QA_FLAG_QA_MODE_ENABLE
	seed[47] |= 0x2;																											//checked by lv2_kernel.self and sys_init_osd.self
	seed[47] |= 0x4; 																											//can run sys_init_osd.self from /app_home ?																	
	seed[51] |= 0x1; 																											//QA_FLAG_ALLOW_NON_QA
	seed[51] |= 0x2; 																											//QA_FLAG_FORCE_UPDATE
	//00 00 00 01 00 00 00 01  00 85 00 0a 14 01 a7 70
	//6b 06 1c 0e 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

	//20 Byte digest erstellen und ab Byte 60 in die seed schreiben (die letzten 20 Byte der seed überschreiben)
	hmac_sha1(hmac, sizeof(hmac), seed, 60, seed + 60);
	//00 00 00 01 00 00 00 01  00 85 00 0a 14 01 a7 70
	//6b 06 1c 0e 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
	//00 00 00 00 00 00 00 00  00 00 00 00 XX XX XX XX
	//XX XX XX XX XX XX XX XX  XX XX XX XX XX XX XX XX

	//verschlüsselung vorbereiten
	result = AES_set_encrypt_key(erk, 256, &aes_ctx);
	if (result) {
		PRINTF("%s:%d: AES_set_encrypt_key failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	//seed mit AES 256 CBC verschlüsseln um fertigen token zu erhalten
	AES_cbc_encrypt(iv, seed, token, TOKEN_SIZE, &aes_ctx);

	//token seed ausgeben (zum debugen)
	PRINTF("%s: %d: TOKEN SEED:\n", __func__, __LINE__);
	for (i = 0; i < TOKEN_SIZE; i += 16)
		PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			seed[i+ 0], seed[i+ 1], seed[i+ 2], seed[i+ 3], seed[i+ 4], seed[i+ 5],
			seed[i+ 6], seed[i+ 7], seed[i+ 8], seed[i+ 9], seed[i+10], seed[i+11],
			seed[i+12], seed[i+13], seed[i+14], seed[i+15]);

	//fertigen token ausgeben (zum debugen)
	PRINTF("%s: %d: TOKEN:\n", __func__, __LINE__);
	for (i = 0; i < TOKEN_SIZE; i += 16)
		PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			token[i+ 0], token[i+ 1], token[i+ 2], token[i+ 3], token[i+ 4], token[i+ 5],
			token[i+ 6], token[i+ 7], token[i+ 8], token[i+ 9], token[i+10], token[i+11],
			token[i+12], token[i+13], token[i+14], token[i+15]);

	//fertigen token in eeprom schreiben
	//863, CEX/DEX, root, lv2_ss_update_mgr_if:
	//0x600A 	Set Token,
	//0x600B 	Read EPROM
	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_SET_TOKEN, (uint64_t) token, TOKEN_SIZE, 0, 0, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv1_ss_update_mgr_if(SET_TOKEN) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	//qa flag unset (kein plan warum unset, es wird nur gelesen und dann ausgegeben)
	result = lv2_ss_update_mgr_if(UPDATE_MGR_PACKET_ID_READ_EPROM, EPROM_QA_FLAG_OFFSET, (uint64_t) &value, 0, 0, 0, 0);
	if (result) {
		PRINTF("%s:%d: lv1_ss_update_mgr_if(READ_EPROM) failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: current QA flag 0x%02x\n", __func__, __LINE__, value);
	PRINTF("%s:%d: end\n", __func__, __LINE__);

	//beep, erfolgsmeldung
	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

done:																																		//sprungmarke für abbruch

	udp_printf_deinit();																									//deinit udp

	netDeinitialize();																										//deinit net

	return 0;
}
