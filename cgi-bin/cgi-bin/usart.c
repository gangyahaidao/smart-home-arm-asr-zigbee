#include "usart.h"

int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	    			B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {115200, 38400,  19200,  9600,  4800,  2400,  1200,  300,
	    			38400,  19200,  9600, 4800, 2400, 1200,  300, };

/**
	fd is the open tty ;speed is the rate
*/
void set_speed(int fd, int speed)
{
  int   i;
  int   status;
  struct termios   Opt;
  tcgetattr(fd, &Opt);
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
  {
   	if  (speed == name_arr[i])
   	{
   	    tcflush(fd, TCIOFLUSH);
    	cfsetispeed(&Opt, speed_arr[i]);
    	cfsetospeed(&Opt, speed_arr[i]);
    	status = tcsetattr(fd, TCSANOW, &Opt);
    	if  (status != 0)
            perror("tcsetattr fd1");
     	return;
    }
    tcflush(fd,TCIOFLUSH);
  }
}	    

/**
	set data bit , stop bit and checksum bit 
*/
int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options, save_termios;;
 	if( tcgetattr( fd,&options)  !=  0)
  	{
 	 	perror("SetupSerial 1");
 	 	return(FALSE);
  	}
	save_termios = options;
 	options.c_cflag &= ~(CSIZE | PARENB);
	options.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);
	options.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	options.c_oflag &= ~(OPOST);
  	switch (databits)
  	{
  		case 7:
  			options.c_cflag |= CS7;
  			break;
  		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n");
			return (FALSE);
	}
	
   	switch (parity)
  	{
 	 	case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   	//Clear parity enable 
			options.c_iflag &= ~INPCK;     	// Enable parity checking
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB); //set as odd check 
			options.c_iflag |= INPCK;             		//Disnable parity check
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;    		//Enable parity 
			options.c_cflag &= ~PARODD; 
			options.c_iflag |= INPCK;      		//Disnable parity checking
			break;		
		case 'S':
		case 's':  //as no parity
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported parity\n");
			return (FALSE);
		}

 	 switch (stopbits)
  	{
  		case 1:
  			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr,"Unsupported stop bits\n");
			return (FALSE);
	}
  	//Set input parity option
  	if (parity != 'n')
  		options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 15; // seconds/10,终端会每隔1.5秒查询一下终端是否有数据，这样CPU就有时间去做其他事情
    options.c_cc[VMIN] = 0;//读取的字节数目

  	tcflush(fd,TCIFLUSH); 		// Update the options and do it NOW
  	if (tcsetattr(fd,TCSANOW,&options) != 0)
  	{
  		perror("SetupSerial 3");
		return (FALSE);
	}
  	return (TRUE);
}			
/**
	send n bytes data
*/
int writen(int fd, uint8_t *buf, size_t size)
{
	int total = 0, n;
	while(total < size)
	{
		if((n = write(fd, buf + total, size - total)) < 0)
		{
			
			fprintf(stderr, "[%s:%d]read failured:%s\n", __FILE__, __LINE__, strerror(errno));
			exit(-1);
		}
		total += n;
	}
	//printf("write total n = %d\n", total);
	return total;
}

/**
	read n data
*/
int readn(int fd, unsigned char *buf, size_t size)
{
	int total = 0, n;
	while(total < size)
	{
		if((n = read(fd, buf + total, size - total)) < 0)
		{
			
			fprintf(stderr, "[%s:%d]read failured:%s\n", __FILE__, __LINE__, strerror(errno));
			exit(-1);
		}
		total += n;
		//printf("read n = %d\n", n);
	}
	return total;
}