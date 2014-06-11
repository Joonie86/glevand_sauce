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

/*
 * main
 */
int main(int argc, char **argv)
{
	int result;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	result = lv2_sm_shutdown(0x8202, NULL, 0);
	if (result) {
		PRINTF("%s:%d: lv2_sm_shutdown failed (0x%08x)\n", __func__, __LINE__, result);
		goto done;
	}

	PRINTF("%s:%d: end\n", __func__, __LINE__);

done:

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
