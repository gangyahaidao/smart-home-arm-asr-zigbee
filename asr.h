#ifndef __ASR_H__
#define __ASR_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * @brief 语音转文字
 * 
 * @param pcBuffer 输入的PCM音频数据，格式是16K 16bit single LT
 * @param nLen 输入的音频数据字节数
 * @return 有效文字信息
 * 
 */
char *AIUI_Audio2Text(const unsigned char* pcBuffer, int nLen);

/**
 * @brief 获取语音识别的结果
 * 
 * @param pcSrcBuffer 语音识别返回的数据
 * @param pcText 提取的有效文字信息
 * @param nLen /p pcText的最大长度
 * @return 返回0表示函数执行成功，否则失败
 * @note 简单实现，TODO:应该使用json解析
 * 
 */
int AIUI_GetResult(const char* pcSrcBuffer, unsigned char* pcText, int nLen);

#endif
