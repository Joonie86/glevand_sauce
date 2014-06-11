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

#ifndef _LV2_SYSCALL_H_
#define _LV2_SYSCALL_H_

#include <stdint.h>

#include <psl1ght/lv2.h>


/***********************************************************************
* lv2_peek
***********************************************************************/
static inline uint64_t lv2_peek(uint64_t addr)
{
	return Lv2Syscall1(6, addr);
}

/***********************************************************************
* lv2_poke
***********************************************************************/
static inline void lv2_poke(uint64_t addr, uint64_t val)
{
	Lv2Syscall2(7, addr, val);
}

/***********************************************************************
* lv2_storage_open
***********************************************************************/
static inline int lv2_storage_open(uint64_t dev_id, uint32_t *dev_handle)
{
	return Lv2Syscall4(600, dev_id, 0, (uint64_t) dev_handle, 0);
}

/***********************************************************************
* lv2_storage_close
***********************************************************************/
static inline int lv2_storage_close(uint32_t dev_handle)
{
	return Lv2Syscall1(601, dev_handle);
}

/***********************************************************************
* lv2_storage_read
***********************************************************************/
static inline int lv2_storage_read(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	return Lv2Syscall7(602, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t ) buf, (uint64_t) unknown2, flags);
}
 
/***********************************************************************
* lv2_storage_write
***********************************************************************/
static inline int lv2_storage_write(uint32_t dev_handle, uint64_t unknown1, uint64_t start_sector, uint64_t sector_count,
	const void *buf, uint32_t *unknown2, uint64_t flags)
{
	return Lv2Syscall7(603, dev_handle, unknown1, start_sector, sector_count,
		(uint64_t ) buf, (uint64_t) unknown2, flags);
}

/***********************************************************************
* lv2_storage_create_region
***********************************************************************/
static inline int lv2_storage_create_region(uint32_t dev_handle, uint64_t start_sector,
	uint64_t sector_count, uint64_t unknown, uint64_t laid, uint64_t *region_id)
{
	return Lv2Syscall6(614, dev_handle, start_sector, sector_count, unknown, laid, (uint64_t) region_id);
}

/***********************************************************************
* lv2_storage_delete_region
***********************************************************************/
static inline int lv2_storage_delete_region(uint32_t dev_handle, uint64_t region_id)
{
	return Lv2Syscall2(615, dev_handle, region_id);
}

#endif
