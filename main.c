#include "asr.h"
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include "usart.h"

// 指令	参数	     说明
// -D	plughw:2,0	选择设备，2代表card2，0代表subdevice
// -r	44100	    采样率
// -f	S16_LE	    录音格式，16bit位宽
// -d	5	        录制5秒
// -t	wav	        输出音频格式为wav
// 命令行测试：arecord -D "plughw:0,0" -c 1 -f S16_LE -r 44100 -d 5 -t wav file.wav
// 播放测试命令：aplay -c 1 -r 44100 -f S16_LE test_u.pcm
#define DEFAULT_CHANNELS         (1) // 通道数(1: 单通道， 2: 立体声）
#define DEFAULT_SAMPLE_RATE      (44100)  // 采样率：表示每一秒对声音的波形模拟量取样的次数，频率越高，音质越好
#define DEFAULT_SAMPLE_LENGTH    (16) // 单位采样数据位数，数据位数越高，数据表示的模拟量越精细，音质越好，1s的音频数据大小为 2bytes * 44100 = 88200B = 88.2KB
#define DEFAULT_DURATION_TIME    (2)

extern char  aiui_appid[128];
extern char  aiui_api_key[128];

char *prepare = NULL;
static int mutex = 0;

typedef struct PCMContainer
{
    snd_pcm_t *handle;
    snd_output_t *log;
    snd_pcm_uframes_t chunk_size;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_format_t format;
    unsigned int channels; // 声道
    unsigned int chunk_bytes;
    unsigned int bits_per_sample; // 采样率
    unsigned int bits_per_frame; // 比特率

    unsigned char *data_buf;
}PCMContainer_t;

// WAVE file header format
typedef struct {
    char   chunk_id[4];        // RIFF string
    unsigned int    chunk_size;         // overall size of file in bytes (36 + data_size)
    char   sub_chunk1_id[8];   // WAVEfmt string with trailing null char
    unsigned int    sub_chunk1_size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
    unsigned short  audio_format;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned short  num_channels;       // Mono = 1, Stereo = 2
    unsigned int    sample_rate;        // 8000, 16000, 44100, etc. (blocks per second)
    unsigned int    byte_rate;          // SampleRate * NumChannels * BitsPerSample/8
    unsigned short  block_align;        // NumChannels * BitsPerSample/8
    unsigned short  bits_per_sample;    // bits per sample, 8- 8bits, 16- 16 bits etc
    char   sub_chunk2_id[4];   // Contains the letters "data"
    unsigned int    sub_chunk2_size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} wav_header_t;

/**
 * 从音频设备读取数据
*/
int readpcm(PCMContainer_t *sndpcm,unsigned int rcount)
{
    int res;
    unsigned int result = 0;
    unsigned int count = rcount;
    unsigned char *data = sndpcm->data_buf;
    if (count != sndpcm->chunk_size)
    {
        count = sndpcm->chunk_size;
    }
    while(count > 0)
    {
        res = snd_pcm_readi(sndpcm->handle,data,count);
        if (res == -EAGAIN || (res >= 0 && (unsigned int)res < count))
        {
            snd_pcm_wait(sndpcm->handle, 1000);
        }else if (res == -EPIPE)
        {
            snd_pcm_prepare(sndpcm->handle);
        }else if (res == -ESTRPIPE)
        {
            fprintf(stderr, "need suspend\r\n");
        }else if (res < 0)
        {
            fprintf(stderr, "Error snd_pcm_writei:[%s]\r\n", snd_strerror(res));
            return -1;
        }
        if (res > 0)
        {
            result += res;
            count -= res;
            data += res*sndpcm->bits_per_frame/8;
        }
    }
    return rcount;
}

void get_wav_header(int raw_sz, wav_header_t *wh)
{
	// RIFF chunk
	strcpy(wh->chunk_id, "RIFF");
	wh->chunk_size = 36 + raw_sz;

	// fmt sub-chunk (to be optimized)
	strncpy(wh->sub_chunk1_id, "WAVEfmt ", strlen("WAVEfmt "));
	wh->sub_chunk1_size = 16;
	wh->audio_format = 1;
	wh->num_channels = 1;
	wh->sample_rate = 16000;
	wh->bits_per_sample = 16;
	wh->block_align = wh->num_channels * wh->bits_per_sample / 8;
	wh->byte_rate = wh->sample_rate * wh->num_channels * wh->bits_per_sample / 8;

	// data sub-chunk
	strncpy(wh->sub_chunk2_id, "data", strlen("data"));
	wh->sub_chunk2_size = raw_sz;
}
/**
 * 将PCM格式数据转换成wav文件保存
*/
void pcm2wav(uint8_t* pcm_buf, int raw_sz, char* dest_wavfile){
	FILE *fwav;
	wav_header_t wheader;

	memset (&wheader, '\0', sizeof (wav_header_t));
	// construct wav header
	get_wav_header (raw_sz, &wheader);
	//print_dump_wav_header(&wheader);
	// write out the .wav file
	fwav = fopen(dest_wavfile, "wb");
	fwrite(&wheader, 1, sizeof(wheader), fwav);
	fwrite(pcm_buf, 1, raw_sz, fwav);
	fclose(fwav);
}

/**
 * 完成s16格式音频采样率转换功能，如将44100转换成16000
 * 
*/
uint64_t resample_s16_audio(const int16_t *input, int16_t *output, int inSampleRate, int outSampleRate, uint64_t inputSize,
                      uint32_t channels
) {
    if (input == NULL)
        return 0;
    uint64_t outputSize = (uint64_t) (inputSize * (double) outSampleRate / (double) inSampleRate);
    outputSize -= outputSize % channels;
    if (output == NULL)
        return outputSize;
    double stepDist = ((double) inSampleRate / (double) outSampleRate);
    const uint64_t fixedFraction = (1LL << 32);
    const double normFixed = (1.0 / (1LL << 32));
    uint64_t step = ((uint64_t) (stepDist * fixedFraction + 0.5));
    uint64_t curOffset = 0;
    for (uint32_t i = 0; i < outputSize; i += 1) {
        for (uint32_t c = 0; c < channels; c += 1) {
            *output++ = (int16_t) (input[c] + (input[c + channels] - input[c]) * (
                    (double) (curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)
            )
            );
        }
        curOffset += step;
        input += (curOffset >> 32) * channels;
        curOffset &= (fixedFraction - 1);
    }
    return outputSize;
}

// 全局变量，存储一次录音字节数
int record_data_size_g = 0; 
void records(PCMContainer_t *sndpcm)
{
    unsigned int c,frame_size,rest,rest_back,length = 0;
    char *voice = NULL;

    // 计算DEFAULT_DURATION_TIME秒采样需要的字节数
    rest = DEFAULT_DURATION_TIME*(sndpcm->channels*DEFAULT_SAMPLE_LENGTH/8)*DEFAULT_SAMPLE_RATE;
    rest_back = rest; // 备份rest的值
    voice = (char *)malloc(rest);
    memset(voice, 0, rest);    
    printf("malloc record data size = %d\n", rest);
    while(rest)
    {
        printf("rest data = %d\n", rest);
        c = (rest <= sndpcm->chunk_bytes) ? rest : sndpcm->chunk_bytes; // 计算一次读取音频数据字节个数
        frame_size = c * 8 / sndpcm->bits_per_frame; // 计算录音数据帧数 // sndpcm->bits_per_frame = sndpcm->bits_per_sample * sndpcm->channels; // 16bit*声道数
        // 从设备读取音频数据
        if(readpcm(sndpcm,frame_size) != frame_size) {
            printf("readpcm() error\n");
            break;
        }            
        memcpy(voice + length,sndpcm->data_buf,c);
        length += c;
        rest -= c;
    }
    printf("rest data = 0\n");

    // 将录音的44100采样率语音转换成16000，语音数据s16类型个数为rest/2
    uint64_t targetSampleCount = resample_s16_audio((const uint16_t*)voice, NULL, DEFAULT_SAMPLE_RATE, 16000, rest_back/2, DEFAULT_CHANNELS);
    record_data_size_g = targetSampleCount*2; // 重新采样之后的音频字节个数
    printf("resample data size = %d\n", record_data_size_g);
    uint16_t* trans_voice = (uint16_t*)malloc(targetSampleCount*sizeof(uint16_t));
    if(trans_voice) { // 分配内存成功
        resample_s16_audio((const uint16_t*)voice, trans_voice, DEFAULT_SAMPLE_RATE, 16000, rest_back/2, DEFAULT_CHANNELS);
    } else {
        printf("trans_voice malloc() failed\n");
        return;
    }
    prepare = (char*)trans_voice;

    // 将数据保存为wav文件
    pcm2wav(prepare, record_data_size_g, "test.wav");
    mutex = 1; // 开始进行语音识别
}

int setparams(PCMContainer_t *sndpcm)
{
    // 在往音频设备（声卡）写入音频数据之前，必须设置访问类型、采样格式、采样率、声道数等
    snd_pcm_hw_params_t *hwparams; // 此结构包含用来播放 PCM 数据流的硬件信息配置
    snd_pcm_format_t format;
    int res;
    unsigned int exact_rate;
    unsigned int buffer_time,period_time;

    snd_pcm_hw_params_alloca(&hwparams); // 给参数结构体分配空间
    res = snd_pcm_hw_params_any(sndpcm->handle, hwparams); // 使用pcm设备初始化设置参数
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_any\r\n");
        return -1;
    } else {
        printf("set hwparams default value success\n");
    }

    // 设置访问类型
    // 交错访问。在缓冲区的每个 PCM 帧都包含所有设置的声道的连续的采样数据。
    // 比如声卡要播放采样长度是 16-bit 的 PCM 立体声数据，表示每个 PCM 帧中有 16-bit 的左声道数据，然后是 16-bit 右声道数据
    res = snd_pcm_hw_params_set_access(sndpcm->handle, hwparams,SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_set_access\r\n");
        return -1;
    } else {
        printf("set access type success\n");
    }

    // 设置数据格式，主要控制输入的音频数据的类型、无符号还是有符号、是 little-endian 还是 bit-endian
    // 设置为有符号16 bit Little Endian
    format = SND_PCM_FORMAT_S16_LE;
    res = snd_pcm_hw_params_set_format(sndpcm->handle, hwparams, format);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_set_format\r\n");
        return -1;
    } else {
        printf("set data format SND_PCM_FORMAT_S16_LE success\n");
    }

    // 设置音频数据格式
    sndpcm->format = format;
    res = snd_pcm_hw_params_set_channels(sndpcm->handle, hwparams,sndpcm->channels);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_set_channels %d\r\n",sndpcm->channels);
        return -1;
    } else {
        printf("set data channels success\n");
    }

    // 设置音频数据的最接近目标的采样率
    exact_rate = DEFAULT_SAMPLE_RATE;
    res = snd_pcm_hw_params_set_rate_near(sndpcm->handle, hwparams, &exact_rate, 0);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_set_rate_near\r\n");
        return -1;
    } else {
        printf("set rate near success\n");
    }

    // 可以获取该Audio Codec可以支持的最大buffer_time
    res = snd_pcm_hw_params_get_buffer_time_max(hwparams, &buffer_time,0);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_get_buff_time_max\r\n");
        return -1;
    }
    printf("set buffer_time success, buffer_time = %d\n", buffer_time);
    if (buffer_time > 500000) {
        buffer_time = 500000;
    }        
    period_time = buffer_time/4;
    res = snd_pcm_hw_params_set_period_time_near(sndpcm->handle, hwparams, &period_time,0);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params_set_period_time_near\r\n");
        return -1;
    } else {
        printf("set period time near success\n");
    }

    // 设置初始化的参数到声卡
    res = snd_pcm_hw_params(sndpcm->handle, hwparams);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_hw_params\r\n");
        snd_pcm_dump(sndpcm->handle, sndpcm->log); // 打印snd错误日志信息
        return -1;
    } else {
        printf("set changed hwparams success\n");
    }

    // period_size指PCM DMA单次传送数据帧的大小
    // 在ALSA中peroid_size是以frame为单位，frame = channels * sample_size
    // chunk_bytes = peroid_size * sample_length / 8，指单次从WAV读PCM数据的大小
    snd_pcm_hw_params_get_period_size(hwparams, &sndpcm->chunk_size, 0);     
    snd_pcm_hw_params_get_buffer_size(hwparams, &sndpcm->buffer_size);
    if (sndpcm->chunk_size == sndpcm->buffer_size)
    {
        fprintf(stderr, "Can't use period equal to buffer size(%lu == %lu)\r\n", sndpcm->chunk_size,sndpcm->buffer_size);
        return -1;
    }
    sndpcm->bits_per_sample = snd_pcm_format_physical_width(format); // 有符号16 bit Little Endian
    sndpcm->bits_per_frame = sndpcm->bits_per_sample * sndpcm->channels; // 计算一帧数据的大小
    sndpcm->chunk_bytes = sndpcm->chunk_size * sndpcm->bits_per_frame / 8;
    printf("sndpcm->chunk_bytes = %d\n", sndpcm->chunk_bytes);

    // 分配单次从PCM读取数据大小
    sndpcm->data_buf = (unsigned char*)malloc(sndpcm->chunk_bytes);
    if (!sndpcm->data_buf)
    {
        fprintf(stderr, "Error malloc: [data_buf]\r\n");
        return -1;
    }
    return 0;
}

void efficdata(char *pdata)
{
    mutex = 0;
    int16_t *inbuffer = (int16_t*)malloc(record_data_size_g);
	
	char *opt[6] = {"2\r\n","3\r\n"};
	char *open_curtain = "打开灯光";
	char *close_curtain = "关闭灯光";

    // 打开LED灯操作文件描述符
    int fd = open("/sys/class/leds/firefly:yellow:user/brightness", O_WRONLY);
    if(fd < 0) {
        printf("open failed, fd = %d\n", fd);
        return -1;
    }
	
    memcpy(inbuffer, pdata, record_data_size_g);
    free(pdata); // 及时释放分配的空间

	char *aiui_text = (unsigned char*)malloc(1024);
	memset(aiui_text,0,1024);
	
    // 进行语音识别
    aiui_text = AIUI_Audio2Text(inbuffer, record_data_size_g);
	
	if (strstr((char *)aiui_text, open_curtain) != NULL) {
		printf("执行命令：打开灯光\n\r");
        // 打开灯
		ret = write(fd, "1", 1); // 写0灯灭，写1灯亮
        if(ret < 0) {
            printf("write failed ret = %d\n", ret);
            return -1;
        } else {
            printf("write success, ret = %d\n", ret);
        }
	} else if (strstr((char *)aiui_text, close_curtain) != NULL) {
		printf("执行命令：关闭灯光\n\r");
		// 关闭灯
        ret = write(fd, "0", 1); // 写0灯灭，写1灯亮
        if(ret < 0) {
            printf("write failed ret = %d\n", ret);
            return -1;
        } else {
            printf("write success, ret = %d\n", ret);
        }
	}	
    close(fd); // 关闭灯光控制文件描述符
    free(aiui_text);
	free(inbuffer); // 释放空间
}

void *while_efficdata(void *p)
{
    while(1)
    {
        if (mutex == 0)
        {
            continue;
        }else{
            efficdata(prepare);
            break;
        }        
    }
}

int main(int argc, char *argv[])
{
    unsigned char *pcRecvBuffer = NULL;
    char pszRetBuffer[4096];
    PCMContainer_t record; // 定义录音需要的结构体对象    
    char *devicename = "default"; // 使用系统默认的录音设备
    int res;
	pthread_t pt;

    memset(&record,0x0,sizeof(record)); // 初始化清空结构体
    record.channels = DEFAULT_CHANNELS; // 单声道录音
    res = snd_output_stdio_attach(&record.log,stderr,0); // 把stderr的信息重定向到 log中
    if (res < 0) {
        fprintf(stderr, "Error snd_output_stdio_attach\r\n");
        return -1;
    }

    //参数：打开的pcm句柄；要打开的pcm设备名字；录音流或者播放流；打开pcm句柄时的打开模式，默认阻塞
	res = snd_pcm_open(&record.handle,devicename,SND_PCM_STREAM_CAPTURE,0);
    if (res < 0)
    {
        fprintf(stderr, "Error snd_pcm_open %s\r\n", devicename);
    }
	
	res = setparams(&record); // 设置录音设备相关参数
    if (res < 0) {
        fprintf(stderr, "Error setparams\r\n");
        return -1;
    }
    
    pthread_create(&pt,NULL,while_efficdata,NULL);  // while_efficdata线程函数主要对录音数据进行识别
	printf("请说命令\n\r");
    while(1) {
        records(&record); // 此线程主要完成音频数据采集，录音
        while(1);
    }

    // 释放分配的系统资源
    snd_pcm_drain(record.handle);
    free(record.data_buf);
 
    return 0;
}
