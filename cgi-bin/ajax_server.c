#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "usart.h"

int main(void)
{
    // 进行串口的初始化，zigbee串口
    int ret = 0;
    char *port = "/dev/ttyFIQ0";
    int fd = usart_open(fd,port);
    usart_set(fd,115200,0,8,1,'N');

    char temp_byte = 0;
    char buf[10] = {0};
    int index = 0;
    int hasStarted = 0; // 是否开始进行接收，0表示不接收
    while(1) {
        ret = usart_recv(fd, &temp_byte, 1); // 一次只读取一个字节
        if(ret < 0) {
            printf("Content-type: text/html\n\n");
            printf("{\"temp_value\":\"Read Error!\"}");
            break;
        } else {
            if(temp_byte == 0xDD) { // 说明后面的数据使我们需要的
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
                int temp = buf[3]; // 先获取温度的高字节
                temp = temp << 8;
                temp = temp | buf[4]; // 与低字节进行合并
                // 计算实际温度值
                float real_temp = temp*0.0625;
                
                printf("Content-type: text/html\n\n");
                float curr_temp = 23.1;
                printf("{\"temp_value\":\"%.2f\"}", real_temp);
                break; // 接收完一帧数据之后退出
            }
        }
    }
    usart_close(fd);

    return 0;
}