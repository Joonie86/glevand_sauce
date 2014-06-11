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

#include <lv1_map.h>
#include <udp_printf.h>

/* GameOS system manager object in HV process 9 */
#define GAMEOS_SYSMGR_OFFSET				0x0075bdd0ull 	/* 0x000a5dd0ull */

/* log2 of boot memory size for GameOS (3.41) */
#define LOG2_PAGE_SIZE_OFFSET				(GAMEOS_SYSMGR_OFFSET + 0x1b0ull)
#define LOG2_PAGE_SIZE							0x0000001bull

/***********************************************************************
* main
***********************************************************************/
int main(int argc, char **argv)
{
	int result;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	result = lv1_map();
	if (result) {
		PRINTF("%s:%d: lv1_map failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: patching log2 page size\n", __func__, __LINE__);

	lv1_poke(LOG2_PAGE_SIZE_OFFSET, LOG2_PAGE_SIZE);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

done:

	result = lv1_unmap();
	if (result)
		PRINTF("%s:%d: lv1_unmap failed (0x%08x)\n", __func__, __LINE__, result);

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
