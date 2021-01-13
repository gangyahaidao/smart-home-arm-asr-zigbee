#ifndef SERIAL_UTILS_H
#define SERIAL_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <termios.h> 
#include <errno.h>
#include<linux/agpgart.h>
#include<sys/times.h>
#include<sys/select.h>
#include<stdint.h>

void set_speed(int fd, int speed);

int set_Parity(int fd,int databits,int stopbits,int parity);

int writen(int fd, uint8_t *buf, size_t size);

int readn(int fd, unsigned char *buf, size_t size);

#endif
