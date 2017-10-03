/* FatLib Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the FatLib Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the FatLib Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef MCU
#include <string.h>
#include "FatVolume.h"

cache_t* FatCache::read(uint32_t lbn_, uint8_t option) {
	if (m_lbn != lbn_) {
		if (! sync()) {
			DBG_FAIL_MACRO;
			return 0;
		}
		if (! (option & CACHE_OPTION_NO_READ)) {
			if (! m_vol->readBlock(lbn_, m_block.data)) {
				DBG_FAIL_MACRO;
				return 0;
			}
		}
		m_status = 0;
		m_lbn = lbn_;
	}
	m_status |= option & CACHE_STATUS_MASK;

	return &m_block;
}

bool FatCache::sync() {
	if (m_status & CACHE_STATUS_DIRTY) {
		const bool lock_set(FatVolume::os_lock());

		if (! m_vol->writeBlock(m_lbn, m_block.data)) {
			FatVolume::os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
		// mirror second FAT
		if (m_status & CACHE_STATUS_MIRROR_FAT) {
			uint32_t lbn_ = m_lbn + m_vol->blocksPerFat();
			if (! m_vol->writeBlock(lbn_, m_block.data)) {
				FatVolume::os_unlock(lock_set);
				DBG_FAIL_MACRO;
				return false;
			}
		}
		m_status &= ~CACHE_STATUS_DIRTY;

		FatVolume::os_unlock(lock_set);
	}
	return true;
}

bool FatVolume::allocateCluster(uint32_t current, uint32_t* next) {
	const auto lock_set(os_lock());

	uint32_t find = current ? current : m_allocSearchStart;
	uint32_t start = find;
	while (1) {
		find++;
		// If at end of FAT go to beginning of FAT.
		if (find > m_lastCluster) {
			find = 2;
		}
		uint32_t f;
		int8_t fg = fatGet(find, &f);
		if (fg < 0) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
		if (fg && f == 0) {
			break;
		}
		if (find == start) {
			// Can't find space checked all clusters.
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
	}
	// mark end of chain
	if (! fatPutEOC(find)) {
		os_unlock(lock_set);
		DBG_FAIL_MACRO;
		return false;
	}
	if (current) {
		// link clusters
		if (! fatPut(current, find)) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
	} else {
		// Remember place for search start.
		m_allocSearchStart = find;
	}
	updateFreeClusterCount(-1);
	*next = find;

	os_unlock(lock_set);
	return true;
}

// find a contiguous group of clusters
bool FatVolume::allocContiguous(uint32_t count, uint32_t* firstCluster) {
	const auto lock_set(os_lock());

	// flag to save place to start next search
	bool setStart = true;
	// start of group
	uint32_t bgnCluster;
	// end of group
	uint32_t endCluster;
	// Start at cluster after last allocated cluster.
	uint32_t startCluster = m_allocSearchStart;
	endCluster = bgnCluster = startCluster + 1;

	// search the FAT for free clusters
	while (1) {
		// If past end - start from beginning of FAT.
		if (endCluster > m_lastCluster) {
			bgnCluster = endCluster = 2;
		}
		uint32_t f;
		int8_t fg = fatGet(endCluster, &f);
		if (fg < 0) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
		if (f || fg == 0) {
			// cluster in use try next cluster as bgnCluster
			bgnCluster = endCluster + 1;

			// don't update search start if unallocated clusters before endCluster.
			if (bgnCluster != endCluster) {
				setStart = false;
			}
		} else if ((endCluster - bgnCluster + 1) == count) {
			// done - found space
			break;
		}
		// Can't find space if all clusters checked.
		if (startCluster == endCluster) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
		endCluster++;
	}
	// remember possible next free cluster
	if (setStart) {
		m_allocSearchStart = endCluster + 1;
	}

	// mark end of chain
	if (! fatPutEOC(endCluster)) {
		os_unlock(lock_set);
		DBG_FAIL_MACRO;
		return false;
	}
	// link clusters
	while (endCluster > bgnCluster) {
		if (! fatPut(endCluster - 1, endCluster)) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return false;
		}
		endCluster--;
	}
	// Maintain count of free clusters.
	updateFreeClusterCount(-count);

	// return first cluster number to caller
	*firstCluster = bgnCluster;

	os_unlock(lock_set);
	return true;
}

uint32_t FatVolume::clusterStartBlock(uint32_t cluster) const {
	return m_dataStartBlock + ((cluster - 2) << m_clusterSizeShift);
}

// Fetch a FAT entry - return -1 error, 0 EOC, else 1.
int8_t FatVolume::fatGet(uint32_t cluster, uint32_t* value) {
	uint32_t lba;
	uint32_t next;
	cache_t* pc;

	// error if reserved cluster of beyond FAT
	DBG_HALT_IF(cluster < 2 || cluster > m_lastCluster);

	const bool lock_set(os_lock());

	if (m_fatType == 32) {
		lba = m_fatStartBlock + (cluster >> 7);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_READ);
		if (! pc) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return -1;
		}
		next = pc->fat32[cluster & 0X7F] & FAT32MASK;
		goto done;
	}

	if (m_fatType == 16) {
		lba = m_fatStartBlock + ((cluster >> 8) & 0XFF);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_READ);
		if (! pc) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return -1;
		}
		next = pc->fat16[cluster & 0XFF];
		goto done;
	}
	if (FAT12_SUPPORT && m_fatType == 12) {
		uint16_t index = cluster;
		index += index >> 1;
		lba = m_fatStartBlock + (index >> 9);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_READ);
		if (! pc) {
			os_unlock(lock_set);
			DBG_FAIL_MACRO;
			return -1;
		}
		index &= 0X1FF;
		uint16_t tmp = pc->data[index];
		index++;
		if (index == 512) {
			pc = cacheFetchFat(lba + 1, FatCache::CACHE_FOR_READ);
			if (! pc) {
				os_unlock(lock_set);
				DBG_FAIL_MACRO;
				return -1;
			}
			index = 0;
		}
		tmp |= pc->data[index] << 8;
		next = cluster & 1 ? tmp >> 4 : tmp & 0XFFF;
		goto done;
	} else {
		os_unlock(lock_set);
		DBG_FAIL_MACRO;
		return -1;
	}

done:
	os_unlock(lock_set);
	if (isEOC(next)) {
		return 0;
	}
	*value = next;
	return 1;
}

// Store a FAT entry
bool FatVolume::fatPut(uint32_t cluster, uint32_t value) {
	uint32_t lba;
	cache_t* pc;

	// error if reserved cluster of beyond FAT
	DBG_HALT_IF(cluster < 2 || cluster > m_lastCluster);

	if (m_fatType == 32) {
		lba = m_fatStartBlock + (cluster >> 7);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_WRITE);
		if (!pc) {
			DBG_FAIL_MACRO;
			return false;
		}
		pc->fat32[cluster & 0X7F] = value;
		return true;
	}

	if (m_fatType == 16) {
		lba = m_fatStartBlock + ((cluster >> 8) & 0XFF);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_WRITE);
		if (!pc) {
			DBG_FAIL_MACRO;
			return false;
		}
		pc->fat16[cluster & 0XFF] = value;
		return true;
	}

	if (FAT12_SUPPORT && m_fatType == 12) {
		uint16_t index = cluster;
		index += index >> 1;
		lba = m_fatStartBlock + (index >> 9);
		pc = cacheFetchFat(lba, FatCache::CACHE_FOR_WRITE);
		if (!pc) {
			DBG_FAIL_MACRO;
			return false;
		}
		index &= 0X1FF;
		uint8_t tmp = value;
		if (cluster & 1) {
			tmp = (pc->data[index] & 0XF) | tmp << 4;
		}
		pc->data[index] = tmp;

		index++;
		if (index == 512) {
			lba++;
			index = 0;
			pc = cacheFetchFat(lba, FatCache::CACHE_FOR_WRITE);
			if (!pc) {
				DBG_FAIL_MACRO;
				return false;
			}
		}
		tmp = value >> 4;
		if (! (cluster & 1)) {
			tmp = ((pc->data[index] & 0XF0)) | tmp >> 4;
		}
		pc->data[index] = tmp;
		return true;
	} else {
		DBG_FAIL_MACRO;
		return false;
	}
}

// free a cluster chain
bool FatVolume::freeChain(uint32_t cluster) {
	uint32_t next;
	int8_t fg;
	do {
		fg = fatGet(cluster, &next);
		if (fg < 0) {
			DBG_FAIL_MACRO;
			return false;
		}
		// free cluster
		if (! fatPut(cluster, 0)) {
			DBG_FAIL_MACRO;
			return false;
		}
		// Add one to count of free clusters.
		updateFreeClusterCount(1);

		if (cluster < m_allocSearchStart) {
			m_allocSearchStart = cluster;
		}
		cluster = next;
	} while (fg);

	return true;
}

int32_t FatVolume::freeClusterCount() {
#if MAINTAIN_FREE_CLUSTER_COUNT
	if (m_freeClusterCount >= 0) {
		return m_freeClusterCount;
	}
#endif // MAINTAIN_FREE_CLUSTER_COUNT
	uint32_t free = 0;
	uint32_t lba;
	uint32_t todo = m_lastCluster + 1;
	uint16_t n;

	if (FAT12_SUPPORT && m_fatType == 12) {
		for (unsigned i = 2; i < todo; i++) {
			uint32_t c;
			int8_t fg = fatGet(i, &c);
			if (fg < 0) {
				DBG_FAIL_MACRO;
				return -1;
			}
			if (fg && c == 0) {
				free++;
			}
		}
	} else if (m_fatType == 16 || m_fatType == 32) {
		lba = m_fatStartBlock;
		while (todo) {
			cache_t* pc = cacheFetchFat(lba++, FatCache::CACHE_FOR_READ);
			if (!pc) {
				DBG_FAIL_MACRO;
				return -1;
			}
			n = m_fatType == 16 ? 256 : 128;
			if (todo < n) {
				n = todo;
			}
			if (m_fatType == 16) {
				for (uint16_t i = 0; i < n; i++) {
					if (pc->fat16[i] == 0) {
						free++;
					}
				}
			} else {
				for (uint16_t i = 0; i < n; i++) {
					if (pc->fat32[i] == 0) {
						free++;
					}
				}
			}
			todo -= n;
		}
	} else {
		// invalid FAT type
		DBG_FAIL_MACRO;
		return -1;
	}
	setFreeClusterCount(free);
	return free;
}

bool FatVolume::init(uint8_t part) {
	uint32_t clusterCount_;
	uint32_t totalBlocks;
	uint32_t volumeStartBlock = 0;
	fat32_boot_t* fbs;
	cache_t* pc;
	uint8_t tmp;
	m_fatType = 0;
	m_allocSearchStart = 1;

	m_cache.init(this);
#if USE_SEPARATE_FAT_CACHE
	m_fatCache.init(this);
#endif  // USE_SEPARATE_FAT_CACHE

	// if part == 0 assume super floppy with FAT boot sector in block zero
	// if part > 0 assume mbr volume with partition table
	if (part) {
		if (part > 4) {
			DBG_FAIL_MACRO;
			return false;
		}
		pc = cacheFetchData(0, FatCache::CACHE_FOR_READ);
		if (! pc) {
			DBG_FAIL_MACRO;
			return false;
		}
		part_t* p = &pc->mbr.part[part - 1];
		if ((p->boot & 0X7F) != 0 || p->firstSector == 0) {
			// not a valid partition
			DBG_FAIL_MACRO;
			return false;
		}
		volumeStartBlock = p->firstSector;
	}
	pc = cacheFetchData(volumeStartBlock, FatCache::CACHE_FOR_READ);
	if (! pc) {
		DBG_FAIL_MACRO;
		return false;
	}
	fbs = &(pc->fbs32);
	if (fbs->bytesPerSector != 512 || fbs->fatCount != 2 || fbs->reservedSectorCount == 0) {
		// not valid FAT volume
		DBG_FAIL_MACRO;
//		LOG_DEBUG("bytesPerSector=%u", fbs->bytesPerSector);
//		LOG_DEBUG("fatCount=%u", fbs->fatCount);
//		LOG_DEBUG("reservedSectorCount=%u", fbs->reservedSectorCount);
		return false;
	}
	m_blocksPerCluster = fbs->sectorsPerCluster;
	m_clusterBlockMask = m_blocksPerCluster - 1;

	// determine shift that is same as multiply by m_blocksPerCluster
	m_clusterSizeShift = 0;
	for (tmp = 1; m_blocksPerCluster != tmp; tmp <<= 1, m_clusterSizeShift++) {
		if (tmp == 0) {
			DBG_FAIL_MACRO;
			return false;
		}
	}

	m_blocksPerFat = fbs->sectorsPerFat16 ? fbs->sectorsPerFat16 : fbs->sectorsPerFat32;

	m_fatStartBlock = volumeStartBlock + fbs->reservedSectorCount;

	// count for FAT16 zero for FAT32
	m_rootDirEntryCount = fbs->rootDirEntryCount;

	// directory start for FAT16 dataStart for FAT32
	m_rootDirStart = m_fatStartBlock + 2 * m_blocksPerFat;
	// data start for FAT16 and FAT32
	m_dataStartBlock = m_rootDirStart + ((32 * fbs->rootDirEntryCount + 511) / 512);

	// total blocks for FAT16 or FAT32
	totalBlocks = fbs->totalSectors16 ? fbs->totalSectors16 : fbs->totalSectors32;
	// total data blocks
	clusterCount_ = totalBlocks - (m_dataStartBlock - volumeStartBlock);

	// divide by cluster size to get cluster count
	clusterCount_ >>= m_clusterSizeShift;
	m_lastCluster = clusterCount_ + 1;

	// Indicate unknown number of free clusters.
	setFreeClusterCount(-1);

	// FAT type is determined by cluster count
	if (clusterCount_ < 4085) {
		m_fatType = 12;
		if (! FAT12_SUPPORT) {
			DBG_FAIL_MACRO;
			return false;
		}
	} else if (clusterCount_ < 65525) {
		m_fatType = 16;
	} else {
		m_rootDirStart = fbs->fat32RootCluster;
		m_fatType = 32;
	}
	return true;
}

#if SDFAT_PRINT_SUPPORT && SDFAT_WIPE_SUPPORT
bool FatVolume::wipe(print_t* pr) {
	cache_t* cache;
	uint16_t count;
	uint32_t lbn_;
	if (! m_fatType) {
		DBG_FAIL_MACRO;
		goto fail;
	}
	cache = cacheClear();
	if (! cache) {
		DBG_FAIL_MACRO;
		goto fail;
	}
	memset(cache->data, 0, 512);
	// Zero root.
	if (m_fatType == 32) {
		lbn_ = clusterStartBlock(m_rootDirStart);
		count = m_blocksPerCluster;
	} else {
		lbn_ = m_rootDirStart;
		count = m_rootDirEntryCount / 16;
	}
	for (uint32_t n = 0; n < count; n++) {
		if (!writeBlock(lbn_ + n, cache->data)) {
			DBG_FAIL_MACRO;
			goto fail;
		}
	}
	// Clear FATs.
	count = 2 * m_blocksPerFat;
	lbn_ = m_fatStartBlock;
	for (uint32_t nb = 0; nb < count; nb++) {
		if (pr && (nb & 0XFF) == 0) {
			pr->write('.');
		}
		if (! writeBlock(lbn_ + nb, cache->data)) {
			DBG_FAIL_MACRO;
			goto fail;
		}
	}
	// Reserve first two clusters.
	if (m_fatType == 32) {
		cache->fat32[0] = 0x0FFFFFF8;
		cache->fat32[1] = 0x0FFFFFFF;
	} else if (m_fatType == 16) {
		cache->fat16[0] = 0XFFF8;
		cache->fat16[1] = 0XFFFF;
	} else if (FAT12_SUPPORT && m_fatType == 12) {
		cache->fat32[0] = 0XFFFFF8;
	} else {
		DBG_FAIL_MACRO;
		goto fail;
	}
	if (! writeBlock(m_fatStartBlock, cache->data) || ! writeBlock(m_fatStartBlock + m_blocksPerFat, cache->data)) {
		DBG_FAIL_MACRO;
		goto fail;
	}
	if (m_fatType == 32) {
		// Reserve root cluster.
		if (! fatPutEOC(m_rootDirStart) || ! cacheSync()) {
			DBG_FAIL_MACRO;
			goto fail;
		}
	}
	if (pr) {
		pr->write('\r');
		pr->write('\n');
	}
	m_fatType = 0;
	return true;

	fail: m_fatType = 0;
	return false;
}
#endif // SDFAT_PRINT_SUPPORT && SDFAT_WIPE_SUPPORT

#endif // MCU
