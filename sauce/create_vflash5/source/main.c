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

#include <unistd.h>

#include <psl1ght/lv2/net.h>

#include <lv2_syscall.h>
#include <udp_printf.h>

#define VFLASH_DEV_ID									0x100000000000001ull
#define VFLASH_REGION_START_SECTOR		0x73a00ull
#define VFLASH_REGION_SECTOR_COUNT		0xc000ull 										//25MB should be more than enough for petitboot :)
#define VFLASH_REGION_LAID						0x1070000002000001ull

/* main */
int main(int argc, char **argv)
{
	uint32_t dev_handle;
	uint64_t region_id;
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

	result = lv2_storage_create_region(dev_handle, VFLASH_REGION_START_SECTOR, VFLASH_REGION_SECTOR_COUNT,
		0, VFLASH_REGION_LAID, &region_id);
	if (result) {
		PRINTF("%s:%d: lv2_storage_create_region failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: region_id (%d)\n", __func__, __LINE__, region_id);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

done:

	result = lv2_storage_close(dev_handle);
	if (result)
		PRINTF("%s:%d: lv2_storage_close failed (0x%08x)\n", __func__, __LINE__, result);

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
