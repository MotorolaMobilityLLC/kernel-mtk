#ifndef __TZ_VFS_H__
#define __TZ_VFS_H__

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
