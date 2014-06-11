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

#include <lv1_hvcall.h>
#include <udp_printf.h>

/* main */
int main(int argc, char **argv){
	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	udp_printf_deinit();

	netDeinitialize();

	usleep(10000);

	lv1_panic(1);

	return 0;
}
