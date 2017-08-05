/*
 * c't-Bot
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/**
 * \file 	sdfat_fs_pc.c
 * \brief 	FAT-Dateisystem Unterstuetzung
 * \author 	Timo Sandmann
 * \date 	06.11.2016
 */

#ifdef PC
#include "sdfat_fs.h"

#ifdef SDFAT_AVAILABLE
#include "log.h"
#include <stdlib.h>
#include <stdio.h>

uint8_t sdfat_open(const char* filename, pFatFile* p_file, uint8_t mode) {
	char* file_mode;
	switch (mode & 0x3f) {
	case 0x1:
		file_mode = "rb";
		break;

	case 0x2:
		file_mode = "wb";
		break;

	case 0x3:
		file_mode = "r+b";
		break;

	case 0x4:
	case 0x5:
	case 0x7:
		file_mode = "a+b";
		break;

	case 0x13:
		file_mode = "w+b";
		break;

	default:
		LOG_ERROR("sdfat_open(): unknow mode 0x%x", mode);
		return 1;
	}

	*p_file = fopen(filename, file_mode);
	if (! *p_file) {
		LOG_ERROR("sdfat_open(\"%s\"): fopen failed:", filename);
		perror(NULL);
	}
	return *p_file ? 0 : 1;
}

uint8_t sdfat_seek(pFatFile p_file, int32_t offset, uint8_t origin) {
	if (fseek(p_file, offset, origin)) {
		LOG_ERROR("sdfat_seek(): fseek(%d, %u) failed:", offset, origin);
		perror(NULL);
		return 1;
	}
	return 0;
}

int32_t sdfat_tell(pFatFile p_file) {
	int pos = ftell(p_file);
	return pos >= 0 ? pos : 0xffff;
}

uint32_t sdfat_get_first_block(pFatFile p_file) {
	(void) p_file;
	return 0;
}

void sdfat_rewind(pFatFile p_file) {
	sdfat_seek(p_file, 0, SEEK_SET);
}

int16_t sdfat_read(pFatFile p_file, void* buffer, uint16_t length) {
	const size_t res = fread(buffer, 1, length, p_file);
	if (res != length) {
		LOG_ERROR("sdfat_read(): fread(%d) = %d failed:", length, res);
		perror(NULL);
	}
	return res;
}

int16_t sdfat_write(pFatFile p_file, const void* buffer, uint16_t length) {
	return fwrite(buffer, 1, length, p_file);
}

uint8_t sdfat_remove(pSdFat p_instance, const char* path) {
	(void) p_instance;
	return remove(path);
}

uint8_t sdfat_rename(pSdFat p_instance, const char* old_path, const char* new_path) {
	(void) p_instance;
	return rename(old_path, new_path);
}

uint8_t sdfat_flush(pFatFile p_file) {
	return ! fflush(p_file) ? 0 : 1;
}

uint8_t sdfat_close(pFatFile p_file) {
	return ! fclose(p_file) ? 0 : 1;
}

void sdfat_free(pFatFile p_file) {
	sdfat_close(p_file);
}

uint32_t sdfat_get_filesize(pFatFile p_file) {
	size_t pos = ftell(p_file);
	fseek(p_file, 0, SEEK_END);
	size_t size = ftell(p_file);
	fseek(p_file, pos, SEEK_SET);
	return size;
}

uint8_t sdfat_get_filename(pFatFile p_file, char* p_name, uint16_t size) {
	(void) p_file;
	(void) p_name;
	(void) size;
	return 1;
}

uint8_t sdfat_sync_vol(pSdFat p_instance) {
	(void) p_instance;
	return 0;
}

void sdfat_test(void) {
	pFatFile file;
	if (sdfat_open("test.txt", &file, 0x10) != 0) {
		LOG_ERROR("sdfat_open(%s) failed", "test.txt");
		return;
	}
	char tmp[] = "Hello World!\n";
	if (sdfat_write(file, tmp, sizeof(tmp) - 1) != sizeof(tmp) - 1) {
		LOG_ERROR("sdfat_write(%d) failed.", sizeof(tmp) - 1);
		return;
	}
	if (sdfat_flush(file)) {
		LOG_ERROR("sdfat_flush() failed");
		return;
	}
	if (sdfat_close(file)) {
		LOG_ERROR("sdfat_close() failed");
		return;
	}

	if (sdfat_open("test.txt", &file, 0x1) != 0) {
		LOG_ERROR("sdfat_open(%s) failed", "test.txt");
		return;
	}
	int16_t n = sdfat_read(file, tmp, sizeof(tmp) - 1);
	if (n != sizeof(tmp) - 1) {
		LOG_ERROR("sdfat_read(%d) failed.", sizeof(tmp) - 1);
		return;
	}
	printf("read %d byte:\n", n);
	printf("\"%s\"\n", tmp);
	printf("filesize=%u\n", sdfat_get_filesize(file));
	if (sdfat_close(file)) {
		LOG_ERROR("sdfat_close() failed");
		return;
	}
}
#endif // SDFAT_AVAILABLE
#endif // PC
