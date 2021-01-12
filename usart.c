#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>


int usart_open(int fd,char *port)
{
	//printf("port is %s\n", port);
	fd = open(port,O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0)
	{
		printf("Can't Open Serial Port\r\n");
		return -1;
	}
	if (fcntl(fd,F_SETFL,0) < 0)
	{
		printf("fcntl failed\r\n");
		return -1;
	}else{
		//printf("fcntl = %d\r\n", fcntl(fd,F_SETFL,0));
	}
	return fd;
}

void usart_close(int fd)
{
	close(fd);
}

int usart_set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)
{
	int i;
	int status;
	int speed_arr[] = {B115200,B19200,B9600,B4800,B2400,B1200,B300};
	int name_arr[] = {115200,19200,9600,4800,2400,1200,300};

	struct termios newtio;

	if (tcgetattr(fd,&newtio) != 0)
	{
		printf("SetupSerial 1\r\n");
		return -1;
	}

	for (i = 0; i < sizeof(speed_arr)/sizeof(int); ++i)
	{
		if (speed == name_arr[i])
		{
			cfsetispeed(&newtio,speed_arr[i]);
			cfsetospeed(&newtio,speed_arr[i]);
		}
	}
	newtio.c_cflag |= CLOCAL;
	newtio.c_cflag |= CREAD;

	switch(flow_ctrl)
	{
		case 0:newtio.c_cflag &= ~CRTSCTS;break;
		case 1:newtio.c_cflag |= CRTSCTS;break;
		case 2:newtio.c_cflag |= IXON | IXOFF | IXANY;break;
		default:printf("unsuported flow_ctrl\r\n");return -1;
	}

	newtio.c_cflag &= ~CSIZE;
	switch(databits)
	{
		case 5:newtio.c_cflag |= CS5;break;
		case 6:newtio.c_cflag |= CS6;break;
		case 7:newtio.c_cflag |= CS7;break;
		case 8:newtio.c_cflag |= CS8;break;
		default:printf("unsuported data size\r\n");return -1;
	}

	switch(parity)
	{
		case 'n':
		case 'N':newtio.c_cflag &= ~PARENB;
			newtio.c_iflag |= ~INPCK;
			break;
		case 'o':
		case 'O':newtio.c_cflag |= (PARODD | PARENB);
			newtio.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			newtio.c_iflag |= INPCK;
			break;
		case 's':
		case 'S':newtio.c_cflag &= ~PARENB;
			newtio.c_cflag &= ~CSTOPB;
			break;
		default:printf("unsuported parity\r\n");
			return -1;
	}

	switch(stopbits)
	{
		case 1:newtio.c_cflag &= ~CSTOPB;break;
		case 2:newtio.c_cflag |= CSTOPB;break;
		default:printf("unsuported stop bits\r\n");
			return -1;
	}
	newtio.c_oflag &= ~OPOST;
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 1;

	tcflush(fd,TCIFLUSH);

	if (tcsetattr(fd,TCSANOW,&newtio) != 0)
	{
		printf("com set error\r\n");
		return -1;
	}
	return 1;
}

int usart_send(int fd,char *send_buf,int data_len)
{
	int len = 0;
	len = write(fd,send_buf,data_len);
	if (len == data_len)
	{
		//printf("send data is %s\r\n", send_buf);
		printf("send data success\n");
		return len;
	}else{
		tcflush(fd,TCOFLUSH);
		return -1;
	}
}

int usart_recv(int fd,char *recv_buf,int data_len)
{
	int len = 0;
	len = read(fd,recv_buf,data_len);
	if (len > 0)
	{
		printf("recv data is %s\r\n", recv_buf);
		return len;
	}else{
		printf("cannot recvive data\r\n");
		return -1;
	}
}

void usart(char *sendbuf,int length)
{
	int fd;
	char *port = "/dev/ttySAC4";
	fd = usart_open(fd,port);
	usart_set(fd,115200,0,8,1,'N');
	usart_send(fd,sendbuf,length);
	usart_close(fd);
}

/*int main(int argc,char *argv[])
{
	int fd,res,i = 8;
	char *opt[8] = {"#L01,O!\r\n","#L01,C!\r\n","#L02,O!\r\n","#L02,C!\r\n","#L03,O!\r\n","#L03,C!\r\n","#L04,O!\r\n","#L04,C!\r\n"};
	char *port = "/dev/ttys5";
	fd = uart_open(fd,port);
	uart_set(fd,115200,0,8,1,'N');
	while(i--)
	{
		uart_send(fd,opt[i],12);
	}
	uart_close(fd);
}*/