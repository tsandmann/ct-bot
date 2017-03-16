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
#ifndef FatApiConstants_h
#define FatApiConstants_h

#ifdef MCU
// use the gnu style oflag in open()
/** open() oflag for reading */
constexpr uint8_t const O_READ = 0x01;
/** open() oflag - same as O_IN */
constexpr uint8_t const O_RDONLY = O_READ;
/** open() oflag for write */
constexpr uint8_t const O_WRITE = 0x02;
/** open() oflag - same as O_WRITE */
constexpr uint8_t const O_WRONLY = O_WRITE;
/** open() oflag for reading and writing */
constexpr uint8_t const O_RDWR = (O_READ | O_WRITE);
/** open() oflag mask for access modes */
constexpr uint8_t const O_ACCMODE = (O_READ | O_WRITE);
/** The file offset shall be set to the end of the file prior to each write. */
constexpr uint8_t const O_APPEND = 0x04;
/** synchronous writes - call sync() after each write */
constexpr uint8_t const O_SYNC = 0x08;
/** truncate the file to zero length */
constexpr uint8_t const O_TRUNC = 0x10;
/** set the initial position at the end of the file */
constexpr uint8_t const O_AT_END = 0x20;
/** create the file if nonexistent */
constexpr uint8_t const O_CREAT = 0x40;
/** If O_CREAT and O_EXCL are set, open() shall fail if the file exists */
constexpr uint8_t const O_EXCL = 0x80;

// FatFile class static and const definitions
// flags for ls()
/** ls() flag for list all files including hidden. */
constexpr uint8_t const LS_A = 1;
/** ls() flag to print modify. date */
constexpr uint8_t const LS_DATE = 2;
/** ls() flag to print file size. */
constexpr uint8_t const LS_SIZE = 4;
/** ls() flag for recursive list of subdirectories */
constexpr uint8_t const LS_R = 8;

// flags for timestamp
/** set the file's last access date */
constexpr uint8_t const T_ACCESS = 1;
/** set the file's creation date and time */
constexpr uint8_t const T_CREATE = 2;
/** Set the file's write date and time */
constexpr uint8_t const T_WRITE = 4;

#endif // MCU
#endif // FatApiConstants_h
