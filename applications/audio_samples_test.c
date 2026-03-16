/*
 * 本地音频样本播放测试
 * 播放预先合成并编译到固件中的音频
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <math.h>

#define DBG_TAG "audio.samples"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 预先合成的"你好"音频样本（示例）*/
/* 
 * 实际使用时，需要：
 * 1. 运行 python test_tts.py 生成 tts_output/你好.pcm
 * 2. 运行 python pcm_to_c_array.py 转换为C数组
 * 3. 把生成的数组代码复制到这里
 * 
 * 或者直接 #include "audio_samples_generated.h"
 */

/* 示例：440Hz测试音（1秒）*/
static void generate_test_audio(int16_t *buffer, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; i++)
    {
        float t = (float)i / 16000.0f;
        float value = sinf(2.0f * 3.14159f * 440.0f * t);
        buffer[i] = (int16_t)(value * 16000.0f);
    }
}

/* 测试命令：播放本地音频 */
static int cmd_play_local_audio(int argc, char **argv)
{
    rt_device_t audio_dev;
    int16_t *buffer;
    uint32_t samples = 16000;  /* 1秒 */
    uint32_t buffer_size = samples * sizeof(int16_t);
    
    LOG_I("Playing local audio sample...");
    
    /* 查找音频设备 */
    audio_dev = rt_device_find("audio0");
    if (audio_dev == RT_NULL)
    {
        LOG_E("Audio device not found");
        return -1;
    }
    
    /* 打开设备 */
    if (rt_device_open(audio_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK)
    {
        LOG_E("Failed to open audio device");
        return -1;
    }
    
    /* 分配缓冲区 */
    buffer = (int16_t *)rt_malloc(buffer_size);
    if (buffer == RT_NULL)
    {
        LOG_E("Failed to allocate buffer");
        rt_device_close(audio_dev);
        return -1;
    }
    
    /* 生成测试音（实际使用时替换为预存的音频数据）*/
    generate_test_audio(buffer, samples);
    
    LOG_I("Playing...");
    
    /* 播放音频 */
    rt_size_t written = rt_device_write(audio_dev, 0, buffer, buffer_size);
    
    if (written == buffer_size)
    {
        LOG_I("Local audio playback completed");
    }
    else
    {
        LOG_E("Audio write failed: %d/%d bytes", written, buffer_size);
    }
    
    /* 清理 */
    rt_free(buffer);
    rt_device_close(audio_dev);
    
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_play_local_audio, play_local, Play local audio sample);

/* 
 * 实际使用示例（当有预存音频时）：
 * 
 * #include "audio_samples_generated.h"
 * 
 * static int cmd_say_hello(int argc, char **argv)
 * {
 *     rt_device_t audio_dev = rt_device_find("audio0");
 *     rt_device_open(audio_dev, RT_DEVICE_OFLAG_WRONLY);
 *     rt_device_write(audio_dev, 0, audio_sample_nihao, audio_sample_nihao_len);
 *     rt_device_close(audio_dev);
 *     return 0;
 * }
 * MSH_CMD_EXPORT_ALIAS(cmd_say_hello, say_hello, Say hello);
 */

