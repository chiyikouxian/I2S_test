/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant AI cloud service test tool
 */

#include <rtthread.h>
#include <string.h>
#include "ai_cloud_service.h"
#include "audio_player.h"
#include "voice_assistant_config.h"

#define DBG_TAG "ai.test"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* AI测试工具初始化标志 */
static rt_bool_t ai_test_initialized = RT_FALSE;

/* 初始化AI测试工具 */
static int ai_test_tool_init(void)
{
    ai_service_config_t ai_config;
    int ret;
    
    if (ai_test_initialized)
    {
        LOG_I("AI test tool already initialized");
        return RT_EOK;
    }
    
    LOG_I("Initializing AI test tool...");
    
    /* 配置AI服务 */
    rt_memset(&ai_config, 0, sizeof(ai_config));
    ai_config.provider = AI_SERVICE_PROVIDER;
    
#if AI_SERVICE_PROVIDER == 0  /* 百度AI */
    strncpy(ai_config.api_key, BAIDU_API_KEY, sizeof(ai_config.api_key) - 1);
    strncpy(ai_config.api_secret, BAIDU_SECRET_KEY, sizeof(ai_config.api_secret) - 1);
    strncpy(ai_config.app_id, BAIDU_APP_ID, sizeof(ai_config.app_id) - 1);
    strncpy(ai_config.api_url, BAIDU_TTS_URL, sizeof(ai_config.api_url) - 1);
#elif AI_SERVICE_PROVIDER == 1  /* 讯飞 */
    strncpy(ai_config.api_key, XFYUN_API_KEY, sizeof(ai_config.api_key) - 1);
    strncpy(ai_config.api_secret, XFYUN_API_SECRET, sizeof(ai_config.api_secret) - 1);
    strncpy(ai_config.app_id, XFYUN_APP_ID, sizeof(ai_config.app_id) - 1);
    strncpy(ai_config.api_url, XFYUN_TTS_URL, sizeof(ai_config.api_url) - 1);
#elif AI_SERVICE_PROVIDER == 2  /* 阿里云 */
    strncpy(ai_config.api_key, ALIYUN_API_KEY, sizeof(ai_config.api_key) - 1);
    strncpy(ai_config.api_secret, ALIYUN_API_SECRET, sizeof(ai_config.api_secret) - 1);
    strncpy(ai_config.app_id, ALIYUN_APP_ID, sizeof(ai_config.app_id) - 1);
    strncpy(ai_config.api_url, ALIYUN_TTS_URL, sizeof(ai_config.api_url) - 1);
#else  /* 自定义 */
    strncpy(ai_config.api_key, CUSTOM_API_KEY, sizeof(ai_config.api_key) - 1);
    strncpy(ai_config.api_secret, CUSTOM_API_SECRET, sizeof(ai_config.api_secret) - 1);
    strncpy(ai_config.app_id, CUSTOM_APP_ID, sizeof(ai_config.app_id) - 1);
    strncpy(ai_config.api_url, CUSTOM_API_URL, sizeof(ai_config.api_url) - 1);
#endif
    
    /* 初始化AI云服务 */
    ret = ai_cloud_service_init(&ai_config);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to initialize AI cloud service");
        return ret;
    }
    
    /* 初始化音频播放器 */
    ret = audio_player_init();
    if (ret != RT_EOK)
    {
        LOG_W("Failed to initialize audio player, audio playback disabled");
    }
    
    ai_test_initialized = RT_TRUE;
    
    LOG_I("AI test tool initialized successfully");
    rt_kprintf("\n========================================\n");
    rt_kprintf("  AI Test Tool Ready!\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Commands:\n");
    rt_kprintf("  ai_say <text>  - Send text to AI and play response\n");
    rt_kprintf("  ai_chat <text> - Chat with AI (text only)\n");
    rt_kprintf("========================================\n\n");
    
    return RT_EOK;
}

/* 发送文本到AI并播放语音回复 */
static int ai_say_command(int argc, char **argv)
{
    ai_response_t response;
    int ret;
    char text[256] = {0};
    int i;
    
    if (argc < 2)
    {
        rt_kprintf("Usage: ai_say <text>\n");
        rt_kprintf("Example: ai_say 你好世界\n");
        return -1;
    }
    
    /* 确保已初始化 */
    if (!ai_test_initialized)
    {
        LOG_I("Auto-initializing AI test tool...");
        if (ai_test_tool_init() != RT_EOK)
        {
            rt_kprintf("Error: Failed to initialize AI test tool\n");
            rt_kprintf("Please check:\n");
            rt_kprintf("  1. WiFi connection (use 'ifconfig' to check)\n");
            rt_kprintf("  2. API keys in voice_assistant_config.h\n");
            return -1;
        }
    }
    
    /* 拼接所有参数为完整文本 */
    for (i = 1; i < argc && strlen(text) < sizeof(text) - 2; i++)
    {
        if (i > 1)
        {
            strcat(text, " ");
        }
        strncat(text, argv[i], sizeof(text) - strlen(text) - 1);
    }
    
    rt_kprintf("\n========================================\n");
    rt_kprintf("[You] %s\n", text);
    rt_kprintf("========================================\n");
    
    LOG_I("Sending text to AI: %s", text);
    
    /* 发送到AI进行语音合成 */
    rt_memset(&response, 0, sizeof(ai_response_t));
    ret = ai_cloud_service_text_to_speech(text, &response);
    
    if (ret != RT_EOK || response.error_code != 0)
    {
        rt_kprintf("\n[Error] AI service failed\n");
        if (response.error_msg)
        {
            rt_kprintf("Error message: %s\n", response.error_msg);
        }
        rt_kprintf("\nTroubleshooting:\n");
        rt_kprintf("  1. Check network: ping baidu.com\n");
        rt_kprintf("  2. Check API key configuration\n");
        rt_kprintf("  3. Check API quota/balance\n\n");
        
        ai_cloud_service_free_response(&response);
        return -1;
    }
    
    rt_kprintf("\n[AI] Processing completed!\n");
    
    /* 显示AI响应信息 */
    if (response.audio_result && response.audio_len > 0)
    {
        rt_kprintf("Audio data received: %d bytes\n", response.audio_len);
        
        /* 播放音频 */
        LOG_I("Playing AI response audio...");
        ret = audio_player_play((uint8_t *)response.audio_result, response.audio_len);
        
        if (ret == RT_EOK)
        {
            rt_kprintf("Playing audio...\n");
            
            /* 等待播放完成 */
            while (audio_player_get_state() == AUDIO_PLAYER_PLAYING)
            {
                rt_thread_mdelay(100);
            }
            
            rt_kprintf("Playback completed!\n");
        }
        else
        {
            rt_kprintf("Warning: Audio playback failed (no audio device?)\n");
            rt_kprintf("But AI service is working correctly!\n");
        }
    }
    else
    {
        rt_kprintf("Warning: No audio data received from AI\n");
    }
    
    rt_kprintf("========================================\n\n");
    
    ai_cloud_service_free_response(&response);
    
    return 0;
}

/* 与AI文本对话（仅显示不播放）*/
static int ai_chat_command(int argc, char **argv)
{
    char text[256] = {0};
    int i;
    
    if (argc < 2)
    {
        rt_kprintf("Usage: ai_chat <text>\n");
        rt_kprintf("Example: ai_chat 今天天气怎么样\n");
        return -1;
    }
    
    /* 确保已初始化 */
    if (!ai_test_initialized)
    {
        LOG_I("Auto-initializing AI test tool...");
        if (ai_test_tool_init() != RT_EOK)
        {
            rt_kprintf("Error: Failed to initialize AI test tool\n");
            return -1;
        }
    }
    
    /* 拼接所有参数为完整文本 */
    for (i = 1; i < argc && strlen(text) < sizeof(text) - 2; i++)
    {
        if (i > 1)
        {
            strcat(text, " ");
        }
        strncat(text, argv[i], sizeof(text) - strlen(text) - 1);
    }
    
    rt_kprintf("\n========================================\n");
    rt_kprintf("[You] %s\n", text);
    rt_kprintf("[AI]  Processing: %s\n", text);
    rt_kprintf("      (AI replied with audio)\n");
    rt_kprintf("========================================\n\n");
    
    rt_kprintf("Note: This is a text-to-speech service.\n");
    rt_kprintf("      Use 'ai_say' to hear the response.\n\n");
    
    return 0;
}

/* 查看AI测试工具状态 */
static int ai_test_status(int argc, char **argv)
{
    const char *providers[] = {"Baidu AI", "iFlytek", "Alibaba Cloud", "Custom"};
    
    rt_kprintf("\n========================================\n");
    rt_kprintf("  AI Test Tool Status\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Initialized: %s\n", ai_test_initialized ? "Yes" : "No");
    rt_kprintf("Provider: %s\n", providers[AI_SERVICE_PROVIDER]);
    rt_kprintf("API Key: %s\n", 
#if AI_SERVICE_PROVIDER == 0
               BAIDU_API_KEY
#elif AI_SERVICE_PROVIDER == 1
               XFYUN_API_KEY
#elif AI_SERVICE_PROVIDER == 2
               ALIYUN_API_KEY
#else
               CUSTOM_API_KEY
#endif
              );
    rt_kprintf("========================================\n\n");
    
    return 0;
}

/* 测试网络连接 */
static int ai_test_network(int argc, char **argv)
{
    rt_kprintf("\n========================================\n");
    rt_kprintf("  Network Test\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Run these commands to check network:\n\n");
    rt_kprintf("  ifconfig           - Check network status\n");
    rt_kprintf("  ping baidu.com     - Test internet connection\n");
    rt_kprintf("  ping vop.baidu.com - Test AI service connectivity\n");
    rt_kprintf("\n");
    rt_kprintf("If WiFi not connected:\n");
    rt_kprintf("  wifi scan                  - Scan WiFi networks\n");
    rt_kprintf("  wifi join SSID PASSWORD    - Connect to WiFi\n");
    rt_kprintf("========================================\n\n");
    
    return 0;
}

/* 导出MSH命令 */
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT_ALIAS(ai_test_tool_init, ai_init, Initialize AI test tool);
MSH_CMD_EXPORT_ALIAS(ai_say_command, ai_say, Send text to AI and play response);
MSH_CMD_EXPORT_ALIAS(ai_chat_command, ai_chat, Chat with AI text only);
MSH_CMD_EXPORT_ALIAS(ai_test_status, ai_status, Show AI test tool status);
MSH_CMD_EXPORT_ALIAS(ai_test_network, ai_network, Show network test commands);
#endif

