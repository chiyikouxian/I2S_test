/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant AI Chat Service Implementation
 */

#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include "ai_chat_service.h"
#include "web_client.h"

#define DBG_TAG "ai.chat"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* AI对话服务配置 */
static ai_chat_config_t g_chat_config = {0};
static rt_bool_t g_chat_initialized = RT_FALSE;

/* 初始化对话AI服务 */
int ai_chat_service_init(ai_chat_config_t *config)
{
    if (config == RT_NULL)
    {
        LOG_E("Invalid configuration");
        return -RT_EINVAL;
    }
    
    rt_memcpy(&g_chat_config, config, sizeof(ai_chat_config_t));
    g_chat_initialized = RT_TRUE;
    
    LOG_I("AI chat service initialized (Provider: %d, Model: %s)", 
          config->provider, config->model);
    
    return RT_EOK;
}

/* 设置系统提示词 */
int ai_chat_service_set_system_prompt(const char *prompt)
{
    if (prompt == RT_NULL || !g_chat_initialized)
    {
        return -RT_EINVAL;
    }
    
    strncpy(g_chat_config.system_prompt, prompt, sizeof(g_chat_config.system_prompt) - 1);
    LOG_I("System prompt updated: %s", prompt);
    
    return RT_EOK;
}

/* 提取JSON字符串值（简化版JSON解析）*/
static char *extract_json_string(const char *json, const char *key)
{
    char search_key[128];
    char *result = RT_NULL;
    char *start, *end;
    int len;
    
    /* 构造搜索键 "key": */
    rt_snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    start = strstr(json, search_key);
    if (start == RT_NULL)
    {
        return RT_NULL;
    }
    
    /* 跳过搜索键，定位到冒号后 */
    start = start + strlen(search_key);
    
    /* 跳过可能的空格 */
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
    {
        start++;
    }
    
    /* 检查是否是字符串值（以引号开始）*/
    if (*start != '\"')
    {
        return RT_NULL;
    }
    
    start++; /* 跳过开始引号 */
    
    /* 找到结束引号 */
    end = start;
    while (*end && *end != '\"')
    {
        if (*end == '\\' && *(end + 1))
        {
            end += 2; /* 跳过转义字符 */
        }
        else
        {
            end++;
        }
    }
    
    if (*end != '\"')
    {
        return RT_NULL;
    }
    
    len = end - start;
    result = (char *)rt_malloc(len + 1);
    if (result)
    {
        rt_memcpy(result, start, len);
        result[len] = '\0';
        
        /* 解码Unicode转义序列 (\uXXXX) */
        char *read_pos = result;
        char *write_pos = result;
        
        while (*read_pos)
        {
            if (*read_pos == '\\' && *(read_pos + 1) == 'u' && 
                *(read_pos + 2) && *(read_pos + 3) && *(read_pos + 4) && *(read_pos + 5))
            {
                /* 解析\uXXXX格式的Unicode转义 */
                unsigned int unicode = 0;
                char hex[5] = {0};
                rt_memcpy(hex, read_pos + 2, 4);
                
                /* 转换为整数 */
                for (int i = 0; i < 4; i++)
                {
                    unicode *= 16;
                    if (hex[i] >= '0' && hex[i] <= '9')
                        unicode += hex[i] - '0';
                    else if (hex[i] >= 'a' && hex[i] <= 'f')
                        unicode += hex[i] - 'a' + 10;
                    else if (hex[i] >= 'A' && hex[i] <= 'F')
                        unicode += hex[i] - 'A' + 10;
                }
                
                /* 将Unicode码点转换为UTF-8 */
                if (unicode < 0x80)
                {
                    *write_pos++ = (char)unicode;
                }
                else if (unicode < 0x800)
                {
                    *write_pos++ = (char)(0xC0 | (unicode >> 6));
                    *write_pos++ = (char)(0x80 | (unicode & 0x3F));
                }
                else
                {
                    *write_pos++ = (char)(0xE0 | (unicode >> 12));
                    *write_pos++ = (char)(0x80 | ((unicode >> 6) & 0x3F));
                    *write_pos++ = (char)(0x80 | (unicode & 0x3F));
                }
                
                read_pos += 6; /* 跳过\uXXXX */
            }
            else
            {
                *write_pos++ = *read_pos++;
            }
        }
        
        *write_pos = '\0';
    }
    
    return result;
}

/* 对话功能 - OpenAI ChatGPT */
static int chat_with_openai(const char *user_message, ai_chat_response_t *response)
{
    http_response_t http_resp;
    char *json_data = RT_NULL;
    int ret = -RT_ERROR;
    
    /* 构造JSON请求 */
    json_data = (char *)rt_malloc(2048);
    if (json_data == RT_NULL)
    {
        LOG_E("Failed to allocate JSON buffer");
        return -RT_ERROR;
    }
    
    /* OpenAI API格式 */
    rt_snprintf(json_data, 2048,
                "{\"model\":\"%s\","
                "\"messages\":["
                "{\"role\":\"system\",\"content\":\"%s\"},"
                "{\"role\":\"user\",\"content\":\"%s\"}"
                "],\"max_tokens\":150,\"temperature\":0.7}",
                g_chat_config.model,
                g_chat_config.system_prompt,
                user_message);
    
    /* 发送HTTP POST请求 */
    ret = web_client_post(g_chat_config.api_url, json_data, strlen(json_data),
                          "application/json", &http_resp);
    
    if (ret == RT_EOK && http_resp.status_code == 200)
    {
        LOG_I("Chat successful");
        
        /* 解析JSON响应，提取回复文本 */
        response->reply_text = extract_json_string(http_resp.body, "content");
        
        if (response->reply_text)
        {
            response->error_code = 0;
            LOG_I("AI reply: %s", response->reply_text);
        }
        else
        {
            LOG_W("Failed to parse response");
            response->error_code = -1;
            response->error_msg = rt_strdup("Failed to parse AI response");
        }
        
        web_client_free_response(&http_resp);
    }
    else
    {
        LOG_E("HTTP request failed (status: %d)", http_resp.status_code);
        response->error_code = http_resp.status_code;
        response->error_msg = rt_strdup("HTTP request failed");
        web_client_free_response(&http_resp);
    }
    
    rt_free(json_data);
    
    return ret;
}

/* 对话功能 - 百度文心一言 */
static int chat_with_baidu_wenxin(const char *user_message, ai_chat_response_t *response)
{
    http_response_t http_resp;
    char *json_data = RT_NULL;
    char *custom_header = RT_NULL;
    int ret = -RT_ERROR;
    
    /* 构造JSON请求 */
    json_data = (char *)rt_malloc(2048);
    if (json_data == RT_NULL)
    {
        LOG_E("Failed to allocate JSON buffer");
        return -RT_ERROR;
    }
    
    /* 构造请求数据 */
    if (g_chat_config.use_v2)
    {
        /* V2协议格式 */
        LOG_I("Using AI Chat V2 protocol");
        rt_snprintf(json_data, 2048,
                    "{\"messages\":["
                    "{\"role\":\"user\",\"content\":\"%s\"}"
                    "],\"stream\":false}",
                    user_message);
        
        /* V2需要自定义Header进行IAM认证 */
        custom_header = (char *)rt_malloc(512);
        if (custom_header == RT_NULL)
        {
            LOG_E("Failed to allocate header buffer");
            rt_free(json_data);
            return -RT_ERROR;
        }
        
        /* 构造认证Header
         * 百度V2协议使用 X-Appbuilder-Authorization 
         * 格式: X-Appbuilder-Authorization: Bearer {access_key}\r\n
         */
        rt_snprintf(custom_header, 512,
                    "X-Appbuilder-Authorization: Bearer %s\r\n",
                    g_chat_config.api_secret);
        
        LOG_D("V2 Auth header: X-Appbuilder-Authorization: Bearer %s", g_chat_config.api_secret);
    }
    else
    {
        /* V1协议格式（通过HTTP代理，支持讯飞星火等）*/
        LOG_I("Using AI Chat via HTTP Proxy (XFyun Spark)");
        rt_snprintf(json_data, 2048,
                    "{\"messages\":["
                    "{\"role\":\"user\",\"content\":\"%s\"}"
                    "],\"stream\":false}",
                    user_message);
    }
    
    LOG_D("Request JSON: %s", json_data);
    
    /* 发送HTTP POST请求 */
    if (g_chat_config.use_v2)
    {
        /* V2使用自定义Header */
        ret = web_client_post_with_header(g_chat_config.api_url, json_data, strlen(json_data),
                                          "application/json", custom_header, &http_resp);
        rt_free(custom_header);
    }
    else
    {
        /* V1使用URL中的access_token */
        ret = web_client_post(g_chat_config.api_url, json_data, strlen(json_data),
                              "application/json", &http_resp);
    }
    
    if (ret == RT_EOK && http_resp.status_code == 200)
    {
        LOG_I("AI chat successful");
        
        /* 解析JSON响应，提取回复文本 */
        /* V2和V1的响应格式可能不同，先尝试V2格式 */
        response->reply_text = extract_json_string(http_resp.body, "answer");
        if (!response->reply_text)
        {
            /* 如果V2格式失败，尝试V1格式 */
            response->reply_text = extract_json_string(http_resp.body, "result");
        }
        
        if (response->reply_text)
        {
            response->error_code = 0;
            LOG_I("AI reply: %s", response->reply_text);
        }
        else
        {
            LOG_W("Failed to parse AI response");
            LOG_D("Response body: %s", http_resp.body);
            response->error_code = -1;
            response->error_msg = rt_strdup("Failed to parse AI response");
        }
        
        web_client_free_response(&http_resp);
    }
    else
    {
        LOG_E("HTTP request failed (status: %d)", http_resp.status_code);
        if (http_resp.body)
        {
            LOG_E("Response body: %s", http_resp.body);
        }
        response->error_code = http_resp.status_code;
        response->error_msg = rt_strdup("AI API request failed");
        web_client_free_response(&http_resp);
    }
    
    rt_free(json_data);
    
    return ret;
}

/* 通用对话接口 */
int ai_chat_service_chat(const char *user_message, ai_chat_response_t *response)
{
    if (!g_chat_initialized)
    {
        LOG_E("AI chat service not initialized");
        return -RT_ERROR;
    }
    
    if (user_message == RT_NULL || response == RT_NULL)
    {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }
    
    rt_memset(response, 0, sizeof(ai_chat_response_t));
    
    LOG_I("User: %s", user_message);
    
    /* 根据提供商调用不同的API */
    switch (g_chat_config.provider)
    {
        case AI_CHAT_OPENAI:
            return chat_with_openai(user_message, response);
            
        case AI_CHAT_BAIDU_WENXIN:
            return chat_with_baidu_wenxin(user_message, response);
            
        case AI_CHAT_CUSTOM:
            /* 自定义API实现 */
            LOG_W("Custom chat API not implemented");
            response->error_code = -1;
            response->error_msg = rt_strdup("Custom API not implemented");
            return -RT_ERROR;
            
        default:
            LOG_E("Unknown chat provider: %d", g_chat_config.provider);
            response->error_code = -1;
            response->error_msg = rt_strdup("Unknown provider");
            return -RT_ERROR;
    }
}

/* 释放响应数据 */
void ai_chat_service_free_response(ai_chat_response_t *response)
{
    if (response)
    {
        if (response->reply_text)
        {
            rt_free(response->reply_text);
            response->reply_text = RT_NULL;
        }
        if (response->error_msg)
        {
            rt_free(response->error_msg);
            response->error_msg = RT_NULL;
        }
        response->error_code = 0;
    }
}

