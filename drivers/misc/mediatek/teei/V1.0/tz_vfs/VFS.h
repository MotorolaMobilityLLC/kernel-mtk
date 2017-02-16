/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __TZ_VFS_H__
#define __TZ_VFS_H__

int vfs_thread_function(unsigned long phy_addr, unsigned long para_vaddr, unsigned long buff_vaddr);
#if 0
int tz_socket(int family, int type, int protocol, unsigned long para_address, unsigned long buffer_addr);
int tz_htons(unsigned short int h);
long tz_inet_addr(char *cp);
int tz_connect(int fd, struct sockaddr *address, int addrlen, unsigned long para_address, unsigned long buffer_addr);
int tz_send(int fd, void *buff, size_t len, unsigned int flags, unsigned long para_address, unsigned long buffer_add);
long tz_recv(int fd, void *ubuf, size_t size, unsigned flags, unsigned long para_address, unsigned long buffer_add);
long tz_close(int fd,  unsigned long para_address, unsigned long buffer_add);
#endif

#endif /* __TZ_VFS_H__ */
