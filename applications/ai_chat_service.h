/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant AI Chat Service (ChatGPT/文心一言等)
 */

#ifndef __AI_CHAT_SERVICE_H__
#define __AI_CHAT_SERVICE_H__

#include <rtthread.h>

/* 对话AI服务提供商 */
typedef enum {
    AI_CHAT_OPENAI = 0,        /* OpenAI ChatGPT */
    AI_CHAT_BAIDU_WENXIN,      /* 百度文心一言 */
    AI_CHAT_XFYUN_XINGHUO,     /* 讯飞星火 */
    AI_CHAT_ALIYUN_TONGYI,     /* 阿里通义千问 */
    AI_CHAT_CUSTOM             /* 自定义对话API */
} ai_chat_provider_t;

/* 对话AI配置 */
typedef struct {
    ai_chat_provider_t provider;
    char api_key[256];        /* V1:access_token, V2:IAM Access Key ID */
    char api_secret[128];     /* V2: Secret Access Key */
    char app_id[64];          /* V2: 应用身份ID，如 app-PrhUxYCW */
    char model[64];           /* 模型名称，如 gpt-3.5-turbo */
    char api_url[256];
    char system_prompt[256];  /* 系统提示词 */
    rt_bool_t use_v2;         /* 是否使用V2协议 */
} ai_chat_config_t;

/* 对话响应 */
typedef struct {
    int error_code;
    char *reply_text;         /* AI回复的文本 */
    char *error_msg;          /* 错误信息 */
} ai_chat_response_t;

/* 对话AI接口 */
int ai_chat_service_init(ai_chat_config_t *config);
int ai_chat_service_chat(const char *user_message, ai_chat_response_t *response);
void ai_chat_service_free_response(ai_chat_response_t *response);

/* 设置系统提示词（定义AI的角色和行为）*/
int ai_chat_service_set_system_prompt(const char *prompt);

#endif /* __AI_CHAT_SERVICE_H__ */

