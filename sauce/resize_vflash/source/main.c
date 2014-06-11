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

#define HDD_DEV_ID																0x101000000000007ull
#define HDD_SECTOR_SIZE														0x200ull
#define HDD_SECTOR_COUNT													0x2ull
#define HDD_PARTITION_TABLE_2ND_REGION_OFFSET			0xc0ull

#define VFLASH_SECTOR_COUNT_OFFSET								0x38ull
#define VFLASH_NEW_SECTOR_COUNT										(0x80000ull /* 256MB */ + 0x5000000ull /* 40GB */)

#define DUMP_FILENAME															"hdd_partition_table.bin"

static const char *dump_path[] = {
	"/dev_usb000/" DUMP_FILENAME,
	"/dev_usb001/" DUMP_FILENAME
};

//open_dump
static FILE *open_dump(void){
#define N(a)	(sizeof(a) / sizeof(a[0]))

	FILE *fp;
	int i;

	fp = NULL;

	for (i = 0; i < N(dump_path); i++) {
		PRINTF("%s:%d: trying path '%s'\n", __func__, __LINE__, dump_path[i]);

		fp = fopen(dump_path[i], "w");
		if (fp)
			break;
	}

	if (fp)
		PRINTF("%s:%d: path '%s'\n", __func__, __LINE__, dump_path[i]);
	else
		PRINTF("%s:%d: file could not be opened\n", __func__, __LINE__);

	return fp;

#undef N
}

//main
int main(int argc, char **argv){
	uint32_t dev_handle;
	FILE *fp;
	int start_sector, sector_count;
	uint32_t unknown2;
	uint8_t buf[HDD_SECTOR_SIZE * HDD_SECTOR_COUNT];
	uint64_t *ptr;
	int result;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	dev_handle = 0;
	fp = NULL;

	result = lv2_storage_open(HDD_DEV_ID, &dev_handle);
	if (result) {
		PRINTF("%s:%d: lv2_storage_open failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	fp = open_dump();
	if (!fp)
		goto done;

	start_sector = 0;
	sector_count = HDD_SECTOR_COUNT;

	PRINTF("%s:%d: reading data start_sector (0x%08x) sector_count (0x%08x)\n", __func__, __LINE__, start_sector, sector_count);

	result = lv2_storage_read(dev_handle, 0, start_sector, sector_count, buf, &unknown2, 0x2ull);
	if (result) {
		PRINTF("%s:%d: lv2_storage_read failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	usleep(10000);

	PRINTF("%s:%d: dumping data\n", __func__, __LINE__, start_sector, sector_count);

	result = fwrite(buf, 1, HDD_SECTOR_COUNT * HDD_SECTOR_SIZE, fp);
	if (result < 0) {
		PRINTF("%s:%d: fwrite failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	fclose(fp);

	/* remove all HDD regions except VFLASH and increase the size of VFLASH !!! */

	memset(buf + HDD_PARTITION_TABLE_2ND_REGION_OFFSET, 0, HDD_SECTOR_COUNT * HDD_SECTOR_SIZE - HDD_PARTITION_TABLE_2ND_REGION_OFFSET);
	ptr = (uint64_t *) (buf + VFLASH_SECTOR_COUNT_OFFSET);

	PRINTF("%s:%d: VFLASH old sector_count (0x%016llx) new sector_count (0x%016llx)\n", __func__, __LINE__, *ptr, VFLASH_NEW_SECTOR_COUNT);

	*ptr = VFLASH_NEW_SECTOR_COUNT;

	PRINTF("%s:%d: writing data start_sector (0x%08x) sector_count (0x%08x)\n", __func__, __LINE__, start_sector, sector_count);

	result = lv2_storage_write(dev_handle, 0, start_sector, sector_count, buf, &unknown2, 0x2ull);
	if (result) {
		PRINTF("%s:%d: lv2_storage_write failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	usleep(10000);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

done:

	if (fp)
		fclose(fp);

	result = lv2_storage_close(dev_handle);
	if (result)
		PRINTF("%s:%d: lv2_storage_close failed (0x%08x)\n", __func__, __LINE__, result);

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
