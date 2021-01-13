#ifndef USART_H
#define USART_H

int usart_open(int fd,char *port);
void usart_close(int fd);
int usart_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
int usart_send(int fd,char *send_buf,int data_len);
int usart_recv(int fd,char *recv_buf,int data_len);
void usart(char *sendbuf,int length);

#endif