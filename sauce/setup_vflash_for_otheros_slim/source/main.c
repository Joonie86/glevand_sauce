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

#include <string.h>
#include <unistd.h>

#include <psl1ght/lv2/net.h>

#include <lv2_syscall.h>
#include <udp_printf.h>

#define PARTITION_TABLE_MAGIC1												0x000000000face0ffull
#define PARTITION_TABLE_MAGIC2												0x00000000deadfaceull

#define VFLASH_DEV_ID																	0x100000000000001ull
#define VFLASH_SECTOR_SIZE														0x200ull
#define VFLASH_START_SECTOR														0x0ull
#define VFLASH_SECTOR_COUNT														0x2ull
#define VFLASH_FLAGS																	0x6ull

#define VFLASH_PARTITION_TABLE_6ND_REGION_OFFSET			0x270ull
#define VFLASH_6TH_REGION_NEW_SECTOR_COUNT						0xc000ull

#define VFLASH_PARTITION_TABLE_7TH_REGION_OFFSET			0x300ull
#define VFLASH_7TH_REGION_NEW_START_SECTOR						0x7fa00ull

/* setup_vflash */
static int setup_vflash(void){
	uint32_t dev_handle;
	int start_sector, sector_count;
	uint32_t unknown2;
	uint8_t buf[VFLASH_SECTOR_SIZE * VFLASH_SECTOR_COUNT];
	uint64_t *ptr;
	int result;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	dev_handle = 0;

	result = lv2_storage_open(VFLASH_DEV_ID, &dev_handle);
	if (result) {
		PRINTF("%s:%d: lv2_storage_open failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	start_sector = VFLASH_START_SECTOR;
	sector_count = VFLASH_SECTOR_COUNT;

	PRINTF("%s:%d: reading data start_sector (0x%08x) sector_count (0x%08x)\n", __func__, __LINE__, start_sector, sector_count);

	result = lv2_storage_read(dev_handle, 0, start_sector, sector_count, buf, &unknown2, VFLASH_FLAGS);
	if (result) {
		PRINTF("%s:%d: lv2_storage_read failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	usleep(10000);

	/* check partition table magic */

	if ((*((uint64_t *) buf + 2) != PARTITION_TABLE_MAGIC1) ||
		(*((uint64_t *) buf + 3) != PARTITION_TABLE_MAGIC2)) {
		PRINTF("%s:%d: invalid partition table magic\n", __func__, __LINE__);
		goto done;
	}

	/* patch sector count of VFLASH 6th region */

	ptr = (uint64_t *) (buf + VFLASH_PARTITION_TABLE_6ND_REGION_OFFSET + 0x8ull);

	PRINTF("%s:%d: VFLASH 6th region old sector_count (0x%016llx) new sector_count (0x%016llx)\n",	__func__, __LINE__, *ptr, VFLASH_6TH_REGION_NEW_SECTOR_COUNT);

	*ptr = VFLASH_6TH_REGION_NEW_SECTOR_COUNT;

	/* patch start sector of VFLASH 7th region */

	ptr = (uint64_t *) (buf + VFLASH_PARTITION_TABLE_7TH_REGION_OFFSET);

	PRINTF("%s:%d: VFLASH 7th region old start_sector (0x%016llx) new start_sector (0x%016llx)\n", __func__, __LINE__, *ptr, VFLASH_7TH_REGION_NEW_START_SECTOR);

	*ptr = VFLASH_7TH_REGION_NEW_START_SECTOR;

	PRINTF("%s:%d: writing data start_sector (0x%08x) sector_count (0x%08x)\n", __func__, __LINE__, start_sector, sector_count);

	result = lv2_storage_write(dev_handle, 0, start_sector, sector_count, buf, &unknown2, VFLASH_FLAGS);
	if (result) {
		PRINTF("%s:%d: lv2_storage_write failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	usleep(10000);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	return 0;

done:

	result = lv2_storage_close(dev_handle);
	if (result)
		PRINTF("%s:%d: lv2_storage_close failed (0x%08x)\n", __func__, __LINE__, result);

	return result;
}

/* main */
int main(int argc, char **argv){
	int result;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	result = setup_vflash();
	if (result) {
		PRINTF("%s:%d: setup_vflash failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

done:

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
