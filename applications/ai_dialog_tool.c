/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant AI Dialog Tool - 完整对话功能
 */

#include <rtthread.h>
#include <string.h>
#include "ai_chat_service.h"
#include "ai_cloud_service.h"
#include "audio_player.h"

#define DBG_TAG "ai.dialog"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 对话工具初始化标志 */
static rt_bool_t dialog_initialized = RT_FALSE;

/* 初始化对话工具 */
static int ai_dialog_init(void)
{
    ai_chat_config_t chat_config;
    int ret;
    
    if (dialog_initialized)
    {
        LOG_I("AI dialog tool already initialized");
        return RT_EOK;
    }
    
    LOG_I("Initializing AI dialog tool...");
    
    /* 配置对话AI服务（使用V1旧版协议，通过HTTP代理）*/
    rt_memset(&chat_config, 0, sizeof(ai_chat_config_t));
    chat_config.provider = AI_CHAT_BAIDU_WENXIN;
    chat_config.use_v2 = RT_FALSE;  /* 使用V1协议（access_token方式）*/
    
    /* 百度文心一言配置 */
    /* 应用身份ID */
    strncpy(chat_config.app_id, "app-PrhUxYCW", sizeof(chat_config.app_id) - 1);
    
    /* 模型名称 */
    strncpy(chat_config.model, "ERNIE-Speed-8K", sizeof(chat_config.model) - 1);
    
    /* ⚠️ 重要：修改为你的PC IP地址！ */
    /* PC必须连接到与设备相同的网络（手机热点）！ */
    /* 设备在 172.20.10.2，所以PC也需要在 172.20.10.x 网段 */
    /* 在PC连接手机热点后，运行 ipconfig 查看实际IP */
    strncpy(chat_config.api_url, 
            "http://192.168.137.1:8080/v2/app/conversation",  /* 待PC连接热点后更新！ */
            sizeof(chat_config.api_url) - 1);
    
    /* 系统提示词 */
    strncpy(chat_config.system_prompt, 
            "你是小石，一个友好的智能助手。请用简短的中文回答问题，不超过50字。回复中只使用中英文、数字和标点符号，不要使用表情符号。",
            sizeof(chat_config.system_prompt) - 1);
    
    ret = ai_chat_service_init(&chat_config);
    if (ret != RT_EOK)
    {
        LOG_E("Failed to initialize chat service");
        return ret;
    }
    
    /* 初始化TTS服务（用于播放AI回复）*/
    /* TTS配置使用voice_assistant_config.h中的配置 */
    
    /* 初始化音频播放 */
    audio_player_init();
    
    dialog_initialized = RT_TRUE;
    
    LOG_I("AI dialog tool initialized successfully");
    rt_kprintf("\n========================================\n");
    rt_kprintf("  AI Dialog Tool Ready!\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Commands:\n");
    rt_kprintf("  ai_ask <question>  - Ask AI and get voice reply\n");
    rt_kprintf("  ai_text <question> - Ask AI (text only)\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Example:\n");
    rt_kprintf("  ai_ask 你是谁\n");
    rt_kprintf("  ai_ask 今天天气怎么样\n");
    rt_kprintf("========================================\n\n");
    
    return RT_EOK;
}

/* 与AI对话并播放语音回复 */
static int cmd_ai_ask(int argc, char **argv)
{
    ai_chat_response_t chat_resp;
    ai_response_t tts_resp;
    char question[256] = {0};
    int i, ret;
    
    /* 初始化所有响应结构体，避免未初始化的内存被free */
    rt_memset(&chat_resp, 0, sizeof(ai_chat_response_t));
    rt_memset(&tts_resp, 0, sizeof(ai_response_t));
    
    if (argc < 2)
    {
        rt_kprintf("Usage: ai_ask <question>\n");
        rt_kprintf("Example: ai_ask 你是谁？\n");
        return -1;
    }
    
    /* 确保已初始化 */
    if (!dialog_initialized)
    {
        LOG_I("Auto-initializing dialog tool...");
        if (ai_dialog_init() != RT_EOK)
        {
            rt_kprintf("Error: Failed to initialize dialog tool\n");
            return -1;
        }
    }
    
    /* 拼接问题 */
    for (i = 1; i < argc && strlen(question) < sizeof(question) - 2; i++)
    {
        if (i > 1) strcat(question, " ");
        strncat(question, argv[i], sizeof(question) - strlen(question) - 1);
    }
    
    rt_kprintf("\n========================================\n");
    rt_kprintf("[You] %s\n", question);
    rt_kprintf("========================================\n");
    rt_kprintf("AI is thinking...\n");
    
    /* 步骤1：发送问题给对话AI */
    ret = ai_chat_service_chat(question, &chat_resp);
    
    if (ret != RT_EOK || chat_resp.error_code != 0)
    {
        rt_kprintf("\n[Error] AI chat failed\n");
        if (chat_resp.error_msg)
        {
            rt_kprintf("Error: %s\n", chat_resp.error_msg);
        }
        rt_kprintf("\nNote: This requires ChatGPT or similar API\n");
        rt_kprintf("Please configure API key in ai_dialog_tool.c\n\n");
        
        ai_chat_service_free_response(&chat_resp);
        return -1;
    }
    
    /* 步骤2：显示AI回复 */
    rt_kprintf("\n[AI] %s\n", chat_resp.reply_text);
    rt_kprintf("========================================\n");
    
    /* 步骤3：将回复转换为语音并播放（暂时禁用，需要配置TTS服务）*/
    #if 0  /* 如需启用TTS，请先配置ai_cloud_service并取消此注释 */
    rt_kprintf("Converting to speech...\n");
    
    ret = ai_cloud_service_text_to_speech(chat_resp.reply_text, &tts_resp);
    
    if (ret == RT_EOK && tts_resp.audio_result && tts_resp.audio_len > 0)
    {
        rt_kprintf("Playing audio reply...\n");
        
        audio_player_play((uint8_t *)tts_resp.audio_result, tts_resp.audio_len);
        
        /* 等待播放完成 */
        while (audio_player_get_state() == AUDIO_PLAYER_PLAYING)
        {
            rt_thread_mdelay(100);
        }
        
        rt_kprintf("Done!\n");
    }
    else
    {
        rt_kprintf("Note: Text-to-speech not available\n");
    }
    #else
    rt_kprintf("Note: Text-to-speech disabled (TTS service not configured)\n");
    #endif
    
    rt_kprintf("========================================\n\n");
    
    ai_cloud_service_free_response(&tts_resp);
    ai_chat_service_free_response(&chat_resp);
    
    return 0;
}

/* 与AI对话（仅文本）*/
static int cmd_ai_text(int argc, char **argv)
{
    ai_chat_response_t chat_resp;
    char question[256] = {0};
    int i, ret;
    
    if (argc < 2)
    {
        rt_kprintf("Usage: ai_text <question>\n");
        rt_kprintf("Example: ai_text 你好吗？\n");
        return -1;
    }
    
    if (!dialog_initialized)
    {
        ai_dialog_init();
    }
    
    /* 拼接问题 */
    for (i = 1; i < argc && strlen(question) < sizeof(question) - 2; i++)
    {
        if (i > 1) strcat(question, " ");
        strncat(question, argv[i], sizeof(question) - strlen(question) - 1);
    }
    
    rt_kprintf("\n[You] %s\n", question);
    rt_kprintf("AI is thinking...\n");
    
    /* 发送问题给AI */
    rt_memset(&chat_resp, 0, sizeof(ai_chat_response_t));
    ret = ai_chat_service_chat(question, &chat_resp);
    
    if (ret == RT_EOK && chat_resp.reply_text)
    {
        rt_kprintf("[AI]  %s\n\n", chat_resp.reply_text);
    }
    else
    {
        rt_kprintf("[Error] AI chat failed\n");
        if (chat_resp.error_msg)
        {
            rt_kprintf("Error: %s\n\n", chat_resp.error_msg);
        }
    }
    
    ai_chat_service_free_response(&chat_resp);
    
    return 0;
}

/* 导出MSH命令 */
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT_ALIAS(ai_dialog_init, ai_dialog_init, Initialize AI dialog tool);
MSH_CMD_EXPORT_ALIAS(cmd_ai_ask, ai_ask, Ask AI a question with voice reply);
MSH_CMD_EXPORT_ALIAS(cmd_ai_text, ai_text, Ask AI a question text only);
#endif

