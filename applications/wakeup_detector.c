/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant first version - Wakeup Word Detector Implementation
 */

#include <rtthread.h>
#include <string.h>
#include "wakeup_detector.h"
#include "audio_capture.h"
#include "ai_cloud_service.h"

#define DBG_TAG "wakeup"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define WAKEUP_BUFFER_SIZE  (16000 * 2 * 2)  /* 2秒音频缓冲 */

/* 唤醒词检测控制结构 */
static struct {
    char wakeup_word[32];
    rt_bool_t running;
    rt_thread_t detector_thread;
    wakeup_callback callback;
} wakeup_ctrl = {
    .running = RT_FALSE,
    .detector_thread = RT_NULL,
    .callback = RT_NULL
};

/* 简单的文本匹配检测 */
rt_bool_t wakeup_detector_check_text(const char *text, const char *wakeup_word)
{
    if (text == RT_NULL || wakeup_word == RT_NULL)
    {
        return RT_FALSE;
    }
    
    /* 移除空格后比较 */
    char text_no_space[128] = {0};
    char word_no_space[32] = {0};
    int text_idx = 0, word_idx = 0;
    
    /* 移除文本中的空格、标点 */
    for (int i = 0; text[i] && text_idx < 127; i++)
    {
        /* 跳过空格和常见标点符号 */
        if (text[i] != ' ' && text[i] != ',' && text[i] != '.')
        {
            /* 跳过中文标点（UTF-8编码，占3字节）*/
            if ((unsigned char)text[i] == 0xE3 && 
                (unsigned char)text[i+1] == 0x80 && 
                ((unsigned char)text[i+2] == 0x81 || (unsigned char)text[i+2] == 0x82))
            {
                /* 中文逗号（，）或句号（。）*/
                i += 2; /* 跳过剩余2字节 */
                continue;
            }
            text_no_space[text_idx++] = text[i];
        }
    }
    
    /* 移除唤醒词中的空格、标点 */
    for (int i = 0; wakeup_word[i] && word_idx < 31; i++)
    {
        /* 跳过空格和标点 */
        if (wakeup_word[i] != ' ' && wakeup_word[i] != ',' && wakeup_word[i] != '.')
        {
            /* 跳过中文逗号 */
            if ((unsigned char)wakeup_word[i] == 0xE3 && 
                (unsigned char)wakeup_word[i+1] == 0x80 && 
                (unsigned char)wakeup_word[i+2] == 0x81)
            {
                i += 2;
                continue;
            }
            word_no_space[word_idx++] = wakeup_word[i];
        }
    }
    
    /* 转换为小写比较（ASCII字符）*/
    for (int i = 0; text_no_space[i]; i++)
    {
        if (text_no_space[i] >= 'A' && text_no_space[i] <= 'Z')
            text_no_space[i] += 32;
    }
    for (int i = 0; word_no_space[i]; i++)
    {
        if (word_no_space[i] >= 'A' && word_no_space[i] <= 'Z')
            word_no_space[i] += 32;
    }
    
    /* 检查是否包含唤醒词 */
    if (strstr(text_no_space, word_no_space) != RT_NULL)
    {
        LOG_I("Wakeup word detected in text: %s", text);
        return RT_TRUE;
    }
    
    return RT_FALSE;
}

/* 唤醒词检测线程 */
static void wakeup_detector_thread_entry(void *parameter)
{
    uint8_t *audio_buffer = RT_NULL;
    ai_response_t ai_response;
    int ret;
    
    LOG_I("Wakeup detector thread started, listening for: %s", wakeup_ctrl.wakeup_word);
    
    /* 分配音频缓冲区 */
    audio_buffer = (uint8_t *)rt_malloc(WAKEUP_BUFFER_SIZE);
    if (audio_buffer == RT_NULL)
    {
        LOG_E("Failed to allocate wakeup buffer");
        return;
    }
    
    /* 开始音频采集 */
    audio_capture_start();
    
    while (wakeup_ctrl.running)
    {
        /* 读取2秒音频数据进行检测 */
        rt_memset(audio_buffer, 0, WAKEUP_BUFFER_SIZE);
        uint32_t total_read = 0;
        
        while (total_read < WAKEUP_BUFFER_SIZE && wakeup_ctrl.running)
        {
            int read_size = audio_capture_read(audio_buffer + total_read,
                                                WAKEUP_BUFFER_SIZE - total_read,
                                                500);
            if (read_size > 0)
            {
                total_read += read_size;
            }
        }
        
        if (!wakeup_ctrl.running)
        {
            break;
        }
        
        if (total_read < 1000)
        {
            continue;
        }
        
        /* 发送到AI进行识别 */
        rt_memset(&ai_response, 0, sizeof(ai_response_t));
        ret = ai_cloud_service_speech_to_text(audio_buffer, total_read, &ai_response);
        
        if (ret == RT_EOK && ai_response.text_result != RT_NULL)
        {
            LOG_D("Recognized: %s", ai_response.text_result);
            
            /* 检查是否包含唤醒词 */
            if (wakeup_detector_check_text(ai_response.text_result, wakeup_ctrl.wakeup_word))
            {
                LOG_I("=== Wakeup word detected! ===");
                rt_kprintf("\n[Wakeup] 检测到唤醒词: %s\n", wakeup_ctrl.wakeup_word);
                
                /* 触发回调 */
                if (wakeup_ctrl.callback)
                {
                    wakeup_ctrl.callback();
                }
            }
        }
        
        ai_cloud_service_free_response(&ai_response);
        
        /* 短暂延时，避免过于频繁的识别 */
        rt_thread_mdelay(500);
    }
    
    audio_capture_stop();
    rt_free(audio_buffer);
    
    LOG_I("Wakeup detector thread stopped");
}

/* 初始化唤醒词检测 */
int wakeup_detector_init(const char *wakeup_word)
{
    if (wakeup_word == RT_NULL)
    {
        LOG_E("Invalid wakeup word");
        return -RT_EINVAL;
    }
    
    strncpy(wakeup_ctrl.wakeup_word, wakeup_word, sizeof(wakeup_ctrl.wakeup_word) - 1);
    
    LOG_I("Wakeup detector initialized with word: %s", wakeup_ctrl.wakeup_word);
    
    return RT_EOK;
}

/* 启动唤醒词检测 */
int wakeup_detector_start(void)
{
    if (wakeup_ctrl.running)
    {
        LOG_W("Wakeup detector already running");
        return RT_EOK;
    }
    
    wakeup_ctrl.running = RT_TRUE;
    
    /* 创建检测线程 */
    wakeup_ctrl.detector_thread = rt_thread_create("wakeup",
                                                    wakeup_detector_thread_entry,
                                                    RT_NULL,
                                                    4096,
                                                    12,
                                                    10);
    if (wakeup_ctrl.detector_thread == RT_NULL)
    {
        LOG_E("Failed to create wakeup detector thread");
        wakeup_ctrl.running = RT_FALSE;
        return -RT_ERROR;
    }
    
    rt_thread_startup(wakeup_ctrl.detector_thread);
    
    LOG_I("Wakeup detector started");
    rt_kprintf("\n[Wakeup] 唤醒词检测已启动，请说: %s\n", wakeup_ctrl.wakeup_word);
    
    return RT_EOK;
}

/* 停止唤醒词检测 */
int wakeup_detector_stop(void)
{
    if (!wakeup_ctrl.running)
    {
        return RT_EOK;
    }
    
    wakeup_ctrl.running = RT_FALSE;
    
    /* 等待线程结束 */
    rt_thread_mdelay(1000);
    
    LOG_I("Wakeup detector stopped");
    
    return RT_EOK;
}

/* 设置唤醒回调 */
int wakeup_detector_set_callback(wakeup_callback callback)
{
    wakeup_ctrl.callback = callback;
    return RT_EOK;
}

/* 导出MSH命令 */
#ifdef FINSH_USING_MSH
static int cmd_wakeup_start(int argc, char **argv)
{
    const char *word = "Hi小石";
    
    if (argc > 1)
    {
        word = argv[1];
    }
    
    wakeup_detector_init(word);
    return wakeup_detector_start();
}
MSH_CMD_EXPORT_ALIAS(cmd_wakeup_start, wakeup_start, Start wakeup detection);

static int cmd_wakeup_stop(int argc, char **argv)
{
    return wakeup_detector_stop();
}
MSH_CMD_EXPORT_ALIAS(cmd_wakeup_stop, wakeup_stop, Stop wakeup detection);
#endif

