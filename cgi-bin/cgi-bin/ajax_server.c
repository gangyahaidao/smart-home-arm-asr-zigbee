#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "usart.h"

#define SERIAL_DEVICE_NAME "/dev/ttyFIQ0"

int main(void)
{
    // 进行串口的初始化，zigbee串口
    int fd;
    if((fd = open(SERIAL_DEVICE_NAME, O_RDWR | O_NOCTTY)) < 0){
		perror("open tty failured\n");
		exit(EXIT_FAILURE);
	}else{
		printf("--open serial device ok\n");
	}
	set_speed(fd, 115200);
  	if (set_Parity(fd,8,1,'N')== FALSE)
  	{
    		printf("Set Parity Error\n");
    		exit(EXIT_FAILURE);
  	}

    char buf[10] = {0};
    unsigned char temp_byte;
    int index = 0, ret;
    int hasStarted = 0; // 是否开始进行接收，0表示不接收
    while(1) {
        ret = read(fd, &temp_byte, 1); // 一次只读取一个字节
        printf("0x%x\n", temp_byte);
        if(ret < 0) {
            printf("Content-type: text/html\n\n");
            printf("{\"temp_value\":\"Read Error!\"}");
            break;
        } else {
            if(temp_byte == 0xDD && hasStarted == 0) { // 说明后面的数据使我们需要的
                index = 0;
                hasStarted = 1; // 开始接收剩余的5个字节
                buf[index++] = temp_byte;
            }
            if(hasStarted == 1) {
                buf[index++] = temp_byte;
            }
            if(index >= 6) { // 现在一帧数据接收完毕，开始进行解析
                // 进行数据校验，先忽略

                // 拼接温度的两个字节
                int temp = buf[4]; // 先获取温度的高字节
                temp = temp << 8;
                temp = temp | buf[3]; // 与低字节进行合并
                // 计算实际温度值
                float real_temp = temp*0.0625;
                
                //printf("Content-type: text/html\n\n");
                printf("{\"temp_value\":\"%.2f\"}", real_temp);

                index = 0; // 数据复位，准备接收下一次数据
                hasStarted = 0;

                char writeBuf[10] = {0};
                sprintf(writeBuf, "%.2f", real_temp);
                FILE *fp = NULL;
                fp = fopen("./test.txt", "w+");
                fputs(writeBuf, fp);
                fclose(fp);
            }
        }
    }
    close(fd);

    return 0;
}