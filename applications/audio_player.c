/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-16     AI Assistant first version - Audio Player Implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <math.h>
#include "audio_player.h"

#define DBG_TAG "audio.player"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 音频播放控制结构 */
static struct {
    audio_player_state_t state;
    rt_device_t dac_dev;
    rt_thread_t player_thread;
    rt_mutex_t lock;
    rt_sem_t sem;
    audio_player_callback callback;
    uint8_t buffer[AUDIO_PLAY_BUFFER_SIZE];
    uint32_t buffer_size;
    uint32_t buffer_pos;
} audio_player_ctrl = {
    .state = AUDIO_PLAYER_IDLE,
    .dac_dev = RT_NULL,
    .player_thread = RT_NULL,
    .lock = RT_NULL,
    .sem = RT_NULL,
    .callback = RT_NULL,
    .buffer_size = 0,
    .buffer_pos = 0
};

/* 音频播放线程 */
static void audio_player_thread_entry(void *parameter)
{
    #define CHUNK_SIZE 4096  /* 每次传输4KB */
    
    LOG_I("Audio player thread started");
    
    if (!audio_player_ctrl.dac_dev)
    {
        LOG_E("Audio device not available");
        audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
        return;
    }
    
    uint32_t total_written = 0;
    
    while (audio_player_ctrl.state == AUDIO_PLAYER_PLAYING)
    {
        rt_mutex_take(audio_player_ctrl.lock, RT_WAITING_FOREVER);
        
        /* 检查是否还有数据需要播放 */
        if (audio_player_ctrl.buffer_pos < audio_player_ctrl.buffer_size)
        {
            /* 计算本次要写入的数据大小 */
            uint32_t remaining = audio_player_ctrl.buffer_size - audio_player_ctrl.buffer_pos;
            uint32_t chunk_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
            
            /* 写入一块数据 */
            rt_size_t written = rt_device_write(audio_player_ctrl.dac_dev, 0,
                                               audio_player_ctrl.buffer + audio_player_ctrl.buffer_pos,
                                               chunk_size);
            
            if (written > 0)
            {
                audio_player_ctrl.buffer_pos += written;
                total_written += written;
                LOG_D("Wrote %d bytes, total: %d/%d", written, total_written, audio_player_ctrl.buffer_size);
            }
            else
            {
                LOG_E("Failed to write audio data");
                audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
            }
            
            rt_mutex_release(audio_player_ctrl.lock);
        }
        else
        {
            /* 播放完成 */
            audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
            audio_player_ctrl.buffer_size = 0;
            audio_player_ctrl.buffer_pos = 0;
            
            rt_mutex_release(audio_player_ctrl.lock);
            
            /* 触发播放完成回调 */
            if (audio_player_ctrl.callback)
            {
                audio_player_ctrl.callback();
            }
            
            LOG_I("Audio playback completed (%d bytes total)", total_written);
            break;
        }
    }
    
    LOG_I("Audio player thread stopped");
}

/* 初始化音频播放 */
int audio_player_init(void)
{
    if (audio_player_ctrl.lock != RT_NULL)
    {
        LOG_W("Audio player already initialized");
        return RT_EOK;
    }
    
    /* 创建互斥锁 */
    audio_player_ctrl.lock = rt_mutex_create("player_lock", RT_IPC_FLAG_PRIO);
    if (audio_player_ctrl.lock == RT_NULL)
    {
        LOG_E("Failed to create player mutex");
        return -RT_ERROR;
    }
    
    /* 创建信号量 */
    audio_player_ctrl.sem = rt_sem_create("player_sem", 0, RT_IPC_FLAG_FIFO);
    if (audio_player_ctrl.sem == RT_NULL)
    {
        LOG_E("Failed to create player semaphore");
        rt_mutex_delete(audio_player_ctrl.lock);
        return -RT_ERROR;
    }
    
    /* 查找并打开音频设备（MAX98357A）*/
    /* 使用audio0设备（I2S音频设备）*/
    audio_player_ctrl.dac_dev = rt_device_find("audio0");
    if (audio_player_ctrl.dac_dev)
    {
        if (rt_device_open(audio_player_ctrl.dac_dev, RT_DEVICE_OFLAG_WRONLY) != RT_EOK)
        {
            LOG_W("Failed to open audio device, audio output disabled");
            audio_player_ctrl.dac_dev = RT_NULL;
        }
        else
        {
            LOG_I("Audio device opened: audio0 (MAX98357A)");
        }
    }
    else
    {
        LOG_W("Audio device not found, audio output disabled");
    }
    
    audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
    
    LOG_I("Audio player initialized (Sample rate: %d Hz, Channels: %d, Bits: %d)",
          AUDIO_PLAY_SAMPLE_RATE, AUDIO_PLAY_CHANNELS, AUDIO_PLAY_BITS_PER_SAMPLE);
    
    return RT_EOK;
}

/* 播放音频数据 */
int audio_player_play(const uint8_t *data, uint32_t size)
{
    if (audio_player_ctrl.lock == RT_NULL)
    {
        LOG_E("Audio player not initialized");
        return -RT_ERROR;
    }
    
    if (data == RT_NULL || size == 0)
    {
        LOG_E("Invalid audio data");
        return -RT_EINVAL;
    }
    
    if (size > AUDIO_PLAY_BUFFER_SIZE)
    {
        LOG_W("Audio data too large, will be truncated");
        size = AUDIO_PLAY_BUFFER_SIZE;
    }
    
    /* 如果正在播放，先停止 */
    if (audio_player_ctrl.state == AUDIO_PLAYER_PLAYING)
    {
        audio_player_stop();
        rt_thread_mdelay(100);
    }
    
    rt_mutex_take(audio_player_ctrl.lock, RT_WAITING_FOREVER);
    
    /* 拷贝音频数据到缓冲区 */
    rt_memcpy(audio_player_ctrl.buffer, data, size);
    
    /* 转换字节序：TTS返回的是小端序，I2S需要大端序 */
    uint16_t *samples = (uint16_t *)audio_player_ctrl.buffer;
    uint32_t sample_count = size / 2;
    
    for (uint32_t i = 0; i < sample_count; i++)
    {
        uint16_t sample = samples[i];
        samples[i] = (sample << 8) | (sample >> 8);  /* 交换高低字节 */
    }
    
    LOG_D("Converted %d samples (byte order swap)", sample_count);
    
    audio_player_ctrl.buffer_size = size;
    audio_player_ctrl.buffer_pos = 0;
    audio_player_ctrl.state = AUDIO_PLAYER_PLAYING;
    
    rt_mutex_release(audio_player_ctrl.lock);
    
    /* 创建播放线程 */
    audio_player_ctrl.player_thread = rt_thread_create("audio_play",
                                                        audio_player_thread_entry,
                                                        RT_NULL,
                                                        2048,
                                                        16,
                                                        10);
    if (audio_player_ctrl.player_thread == RT_NULL)
    {
        LOG_E("Failed to create audio player thread");
        audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
        return -RT_ERROR;
    }
    
    rt_thread_startup(audio_player_ctrl.player_thread);
    
    LOG_I("Audio playback started (size: %d bytes)", size);
    
    return RT_EOK;
}

/* 暂停播放 */
int audio_player_pause(void)
{
    if (audio_player_ctrl.state == AUDIO_PLAYER_PLAYING)
    {
        audio_player_ctrl.state = AUDIO_PLAYER_PAUSED;
        LOG_I("Audio playback paused");
        return RT_EOK;
    }
    return -RT_ERROR;
}

/* 恢复播放 */
int audio_player_resume(void)
{
    if (audio_player_ctrl.state == AUDIO_PLAYER_PAUSED)
    {
        audio_player_ctrl.state = AUDIO_PLAYER_PLAYING;
        LOG_I("Audio playback resumed");
        return RT_EOK;
    }
    return -RT_ERROR;
}

/* 停止播放 */
int audio_player_stop(void)
{
    if (audio_player_ctrl.state == AUDIO_PLAYER_IDLE)
    {
        return RT_EOK;
    }
    
    audio_player_ctrl.state = AUDIO_PLAYER_STOPPED;
    
    /* 等待线程结束 */
    if (audio_player_ctrl.player_thread)
    {
        rt_thread_mdelay(100);
        audio_player_ctrl.player_thread = RT_NULL;
    }
    
    rt_mutex_take(audio_player_ctrl.lock, RT_WAITING_FOREVER);
    audio_player_ctrl.buffer_size = 0;
    audio_player_ctrl.buffer_pos = 0;
    audio_player_ctrl.state = AUDIO_PLAYER_IDLE;
    rt_mutex_release(audio_player_ctrl.lock);
    
    LOG_I("Audio playback stopped");
    
    return RT_EOK;
}

/* 设置播放完成回调 */
int audio_player_set_callback(audio_player_callback callback)
{
    audio_player_ctrl.callback = callback;
    return RT_EOK;
}

/* 获取播放状态 */
audio_player_state_t audio_player_get_state(void)
{
    return audio_player_ctrl.state;
}

/* 获取空闲缓冲区大小 */
int audio_player_get_free_space(void)
{
    if (audio_player_ctrl.state == AUDIO_PLAYER_IDLE)
    {
        return AUDIO_PLAY_BUFFER_SIZE;
    }
    
    rt_mutex_take(audio_player_ctrl.lock, RT_WAITING_FOREVER);
    int free_space = AUDIO_PLAY_BUFFER_SIZE - audio_player_ctrl.buffer_size;
    rt_mutex_release(audio_player_ctrl.lock);
    
    return free_space;
}

/* 导出MSH命令 */
#ifdef FINSH_USING_MSH
static int cmd_audio_player_test(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: audio_play [init|start|stop|status]\n");
        return -1;
    }
    
    if (strcmp(argv[1], "init") == 0)
    {
        return audio_player_init();
    }
    else if (strcmp(argv[1], "start") == 0)
    {
        /* 生成测试音频数据（1秒的440Hz正弦波）*/
        static uint8_t test_audio[AUDIO_PLAY_SAMPLE_RATE * 2];
        for (int i = 0; i < AUDIO_PLAY_SAMPLE_RATE; i++)
        {
            int16_t sample = (int16_t)(32767 * 0.5 * sin(2 * 3.14159 * 440 * i / AUDIO_PLAY_SAMPLE_RATE));
            test_audio[i * 2] = (sample >> 8) & 0xFF;
            test_audio[i * 2 + 1] = sample & 0xFF;
        }
        return audio_player_play(test_audio, sizeof(test_audio));
    }
    else if (strcmp(argv[1], "stop") == 0)
    {
        return audio_player_stop();
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        rt_kprintf("Audio player state: %d\n", audio_player_get_state());
        rt_kprintf("Free buffer space: %d bytes\n", audio_player_get_free_space());
        return 0;
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        return -1;
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_audio_player_test, audio_play, Audio player test commands);
#endif

