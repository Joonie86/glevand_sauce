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

#include <otheros_bootloader_loader.bin.h>

//GameOS system manager object in HV process 9
//#define GAMEOS_SYSMGR_OFFSET								0x000a5dd0ull /* 3.41 */
#define GAMEOS_SYSMGR_OFFSET									0x000a7dd0ull /* 3.55 */

//log2 of boot memory size for GameOS
#define LOG2_PAGE_SIZE_OFFSET									(GAMEOS_SYSMGR_OFFSET + 0x1b0ull)
#define LOG2_PAGE_SIZE												0x0000001bull

//sll_load_lv2
#define SLL_LOAD_LV2_OFFSET										0x00165a64ull
#define SLL_LOAD_LV2_OPCODE										(0x48000001ul | (0x8001c080ul - 0x80000a64ul))

//otheros_bootloader_loader
#define OTHEROS_BOOTLOADER_LOADER_OFFSET			0x00085080ull

//lv1_poke_8
static void lv1_poke_8(uint64_t addr, uint8_t val){
	uint64_t val_64;

	val_64 = lv2_lv1_peek(addr & ~0x7ull);

	val_64 &= ~(0xffull << ((sizeof(uint64_t) - (addr & 0x7ull) - 1) * 8));
	val_64 |= ((uint64_t) val) << ((sizeof(uint64_t) - (addr & 0x7ull) - 1) * 8);

	lv2_lv1_poke(addr & ~0x7ull, val_64);
}

//lv1_memcpy_to
static void lv1_memcpy_to(uint64_t addr, const void *data, unsigned int size){
	uint8_t *ptr;
	unsigned int nbytes;

	ptr = (uint8_t *) data;

	if (addr & 0x7ull) {
		nbytes = sizeof(uint64_t) - (addr & 0x7ull);

		size -= nbytes;

		while (nbytes--)
			lv1_poke_8(addr++, *ptr++);
	}

	while (size >= sizeof(uint64_t)) {
		lv2_lv1_poke(addr, *(uint64_t *) ptr);

		addr += sizeof(uint64_t);
		ptr += sizeof(uint64_t);
		size -= sizeof(uint64_t);
	}

	while (size--)
		lv1_poke_8(addr++, *ptr++);
}

//main
int main(int argc, char **argv){
	uint32_t val32;

	netInitialize();

	udp_printf_init();

	PRINTF("%s:%d: start\n", __func__, __LINE__);

	PRINTF("%s:%d: OtherOS bootloader loader size (0x%08x)\n",
		__func__, __LINE__, sizeof(otheros_bootloader_loader_bin));

	PRINTF("%s:%d: patching log2 page size\n", __func__, __LINE__);

	lv2_lv1_poke(LOG2_PAGE_SIZE_OFFSET, LOG2_PAGE_SIZE);

	PRINTF("%s:%d: patching sll load lv2\n", __func__, __LINE__);

	val32 = SLL_LOAD_LV2_OPCODE;
	lv1_memcpy_to(SLL_LOAD_LV2_OFFSET, &val32, sizeof(val32));

	lv1_memcpy_to(OTHEROS_BOOTLOADER_LOADER_OFFSET, otheros_bootloader_loader_bin,
		sizeof(otheros_bootloader_loader_bin));

	PRINTF("%s:%d: end\n", __func__, __LINE__);

	lv2_sm_ring_buzzer(0x1004, 0xa, 0x1b6);

	udp_printf_deinit();

	netDeinitialize();

	return 0;
}
