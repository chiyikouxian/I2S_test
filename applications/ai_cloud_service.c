/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-16     AI Assistant first version - AI Cloud Service Implementation
 */

#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include "ai_cloud_service.h"
#include "web_client.h"

#define DBG_TAG "ai.cloud"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* AI服务配置 */
static ai_service_config_t g_ai_config = {0};
static rt_bool_t g_ai_initialized = RT_FALSE;

/* Base64编码表 */
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Base64编码 */
static char *base64_encode(const uint8_t *data, uint32_t data_len)
{
    uint32_t encoded_len = ((data_len + 2) / 3) * 4;
    char *encoded = (char *)rt_malloc(encoded_len + 1);
    
    if (encoded == RT_NULL)
    {
        return RT_NULL;
    }
    
    uint32_t i, j;
    for (i = 0, j = 0; i < data_len;)
    {
        uint32_t octet_a = i < data_len ? data[i++] : 0;
        uint32_t octet_b = i < data_len ? data[i++] : 0;
        uint32_t octet_c = i < data_len ? data[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        encoded[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded[j++] = base64_table[triple & 0x3F];
    }
    
    /* 添加填充 */
    for (i = 0; i < (3 - data_len % 3) % 3; i++)
    {
        encoded[encoded_len - 1 - i] = '=';
    }
    
    encoded[encoded_len] = '\0';
    return encoded;
}

/* Base64解码 */
static uint8_t *base64_decode(const char *encoded, uint32_t *decoded_len)
{
    uint32_t encoded_len = strlen(encoded);
    uint32_t output_len = encoded_len / 4 * 3;
    
    if (encoded[encoded_len - 1] == '=') output_len--;
    if (encoded[encoded_len - 2] == '=') output_len--;
    
    uint8_t *decoded = (uint8_t *)rt_malloc(output_len);
    if (decoded == RT_NULL)
    {
        return RT_NULL;
    }
    
    uint32_t i, j;
    for (i = 0, j = 0; i < encoded_len;)
    {
        uint32_t sextet_a = encoded[i] == '=' ? 0 & i++ : strchr(base64_table, encoded[i++]) - base64_table;
        uint32_t sextet_b = encoded[i] == '=' ? 0 & i++ : strchr(base64_table, encoded[i++]) - base64_table;
        uint32_t sextet_c = encoded[i] == '=' ? 0 & i++ : strchr(base64_table, encoded[i++]) - base64_table;
        uint32_t sextet_d = encoded[i] == '=' ? 0 & i++ : strchr(base64_table, encoded[i++]) - base64_table;
        
        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;
        
        if (j < output_len) decoded[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) decoded[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) decoded[j++] = triple & 0xFF;
    }
    
    *decoded_len = output_len;
    return decoded;
}

/* 初始化AI云服务 */
int ai_cloud_service_init(ai_service_config_t *config)
{
    if (config == RT_NULL)
    {
        LOG_E("Invalid configuration");
        return -RT_EINVAL;
    }
    
    rt_memcpy(&g_ai_config, config, sizeof(ai_service_config_t));
    g_ai_initialized = RT_TRUE;
    
    LOG_I("AI cloud service initialized (Provider: %d)", config->provider);
    
    return RT_EOK;
}

/* 语音识别（Speech to Text）*/
int ai_cloud_service_speech_to_text(const uint8_t *audio_data, uint32_t audio_len,
                                     ai_response_t *response)
{
    http_response_t http_resp;
    char *json_data = RT_NULL;
    char *audio_base64 = RT_NULL;
    int ret = -RT_ERROR;
    
    if (!g_ai_initialized)
    {
        LOG_E("AI service not initialized");
        return -RT_ERROR;
    }
    
    if (audio_data == RT_NULL || audio_len == 0 || response == RT_NULL)
    {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }
    
    rt_memset(response, 0, sizeof(ai_response_t));
    
    LOG_I("Starting speech to text (audio_len: %d bytes)", audio_len);
    
    /* 将音频数据编码为Base64 */
    audio_base64 = base64_encode(audio_data, audio_len);
    if (audio_base64 == RT_NULL)
    {
        LOG_E("Failed to encode audio data");
        return -RT_ERROR;
    }
    
    /* 构造JSON请求数据 */
    /* 注意：这里需要根据具体的AI服务提供商构造不同的JSON格式 */
    /* 以下是一个通用的示例格式 */
    json_data = (char *)rt_malloc(audio_len * 2 + 512);
    if (json_data == RT_NULL)
    {
        LOG_E("Failed to allocate JSON buffer");
        rt_free(audio_base64);
        return -RT_ERROR;
    }
    
    if (g_ai_config.provider == AI_SERVICE_BAIDU)
    {
        /* 百度AI格式 */
        rt_snprintf(json_data, audio_len * 2 + 512,
                    "{\"format\":\"pcm\",\"rate\":16000,\"channel\":1,"
                    "\"cuid\":\"%s\",\"token\":\"%s\",\"speech\":\"%s\",\"len\":%d}",
                    g_ai_config.app_id, g_ai_config.api_key, audio_base64, audio_len);
    }
    else if (g_ai_config.provider == AI_SERVICE_XFYUN)
    {
        /* 讯飞格式 */
        rt_snprintf(json_data, audio_len * 2 + 512,
                    "{\"common\":{\"app_id\":\"%s\"},\"business\":{\"language\":\"zh_cn\","
                    "\"domain\":\"iat\",\"accent\":\"mandarin\"},\"data\":{\"status\":2,"
                    "\"format\":\"audio/L16;rate=16000\",\"encoding\":\"raw\","
                    "\"audio\":\"%s\"}}",
                    g_ai_config.app_id, audio_base64);
    }
    else
    {
        /* 通用格式 */
        rt_snprintf(json_data, audio_len * 2 + 512,
                    "{\"audio_data\":\"%s\",\"audio_len\":%d,\"format\":\"pcm\","
                    "\"sample_rate\":16000,\"channels\":1}",
                    audio_base64, audio_len);
    }
    
    /* 发送HTTP POST请求 */
    ret = web_client_post(g_ai_config.api_url, json_data, strlen(json_data),
                          "application/json", &http_resp);
    
    if (ret == RT_EOK && http_resp.status_code == 200)
    {
        LOG_I("Speech to text successful");
        
        /* 解析JSON响应 */
        /* 注意：这里需要一个完整的JSON解析器，简化处理只提取关键字段 */
        /* 建议集成cJSON或jsmn等JSON库 */
        
        /* 临时简化处理：假设返回格式为 {"result":["识别结果"]} */
        char *result_start = strstr(http_resp.body, "\"result\"");
        if (result_start)
        {
            char *text_start = strchr(result_start, '[');
            if (text_start)
            {
                text_start = strchr(text_start, '\"');
                if (text_start)
                {
                    text_start++;
                    char *text_end = strchr(text_start, '\"');
                    if (text_end)
                    {
                        int text_len = text_end - text_start;
                        response->text_result = (char *)rt_malloc(text_len + 1);
                        if (response->text_result)
                        {
                            rt_memcpy(response->text_result, text_start, text_len);
                            response->text_result[text_len] = '\0';
                            response->error_code = 0;
                            
                            LOG_I("Recognized text: %s", response->text_result);
                        }
                    }
                }
            }
        }
        
        if (response->text_result == RT_NULL)
        {
            LOG_W("Failed to parse response, raw: %s", http_resp.body);
            response->error_code = -1;
            response->error_msg = rt_strdup("Failed to parse response");
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
    
    rt_free(audio_base64);
    rt_free(json_data);
    
    return ret;
}

/* 语音合成（Text to Speech）*/
int ai_cloud_service_text_to_speech(const char *text, ai_response_t *response)
{
    http_response_t http_resp;
    char *json_data = RT_NULL;
    char proxy_url[128];
    int ret = -RT_ERROR;
    
    if (!g_ai_initialized)
    {
        LOG_E("AI service not initialized");
        return -RT_ERROR;
    }
    
    if (text == RT_NULL || response == RT_NULL)
    {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }
    
    rt_memset(response, 0, sizeof(ai_response_t));
    
    LOG_I("Starting text to speech: %s", text);
    
    /* 分配JSON数据缓冲区（需要足够大以容纳长文本）*/
    json_data = (char *)rt_malloc(512 + rt_strlen(text));
    if (json_data == RT_NULL)
    {
        LOG_E("Failed to allocate JSON buffer");
        return -RT_ERROR;
    }
    
    /* 根据服务提供商选择不同的处理方式 */
    if (g_ai_config.provider == AI_SERVICE_XFYUN)
    {
        /* 讯飞：使用本地代理服务器（简化鉴权）*/
        rt_snprintf(json_data, 512 + rt_strlen(text),
                    "{\"text\":\"%s\",\"vcn\":\"aisjiuxu\",\"speed\":50,\"volume\":60}",
                    text);  /* aisjiuxu=许久（男声，标准普通话）*/
        
        /* 代理服务器地址（PC的IP地址）*/
        rt_snprintf(proxy_url, sizeof(proxy_url), "http://192.168.1.6:8081/tts");
        
        ret = web_client_post(proxy_url, json_data, strlen(json_data),
                              "application/json", &http_resp);
    }
    else if (g_ai_config.provider == AI_SERVICE_BAIDU)
    {
        /* 百度AI格式 */
        rt_snprintf(json_data, 512 + rt_strlen(text),
                    "{\"tex\":\"%s\",\"tok\":\"%s\",\"cuid\":\"%s\","
                    "\"ctp\":1,\"lan\":\"zh\",\"spd\":5,\"pit\":5,\"vol\":5,\"per\":0}",
                    text, g_ai_config.api_key, g_ai_config.app_id);
        
        ret = web_client_post(g_ai_config.api_url, json_data, strlen(json_data),
                              "application/json", &http_resp);
    }
    else
    {
        /* 通用格式 */
        rt_snprintf(json_data, 512 + rt_strlen(text),
                    "{\"text\":\"%s\",\"format\":\"pcm\",\"sample_rate\":16000}",
                    text);
        
        ret = web_client_post(g_ai_config.api_url, json_data, strlen(json_data),
                              "application/json", &http_resp);
    }
    
    if (ret == RT_EOK && http_resp.status_code == 200)
    {
        LOG_I("Text to speech successful (response_len: %d)", http_resp.body_len);
        
        /* 解析JSON响应，提取音频数据 */
        if (http_resp.body_len > 0)
        {
            /* 查找 "audio" 字段 */
            char *audio_start = strstr(http_resp.body, "\"audio\":\"");
            if (audio_start)
            {
                audio_start += 9;  /* 跳过 "audio":" */
                char *audio_end = strchr(audio_start, '\"');
                
                if (audio_end && (audio_end > audio_start))
                {
                    /* 提取Base64编码的音频数据 */
                    int base64_len = audio_end - audio_start;
                    char *audio_base64 = (char *)rt_malloc(base64_len + 1);
                    
                    if (audio_base64)
                    {
                        rt_memcpy(audio_base64, audio_start, base64_len);
                        audio_base64[base64_len] = '\0';
                        
                        LOG_D("Audio Base64 length: %d", base64_len);
                        
                        /* Base64解码 */
                        uint32_t decoded_len;
                        response->audio_result = (char *)base64_decode(audio_base64, &decoded_len);
                        response->audio_len = decoded_len;
                        response->error_code = 0;
                        
                        rt_free(audio_base64);
                        
                        LOG_I("Audio decoded: %d bytes", decoded_len);
                    }
                    else
                    {
                        LOG_E("Failed to allocate audio Base64 buffer");
                        response->error_code = -1;
                        response->error_msg = rt_strdup("Memory allocation failed");
                    }
                }
                else
                {
                    LOG_E("Invalid audio field in JSON response");
                    response->error_code = -1;
                    response->error_msg = rt_strdup("Invalid JSON format");
                }
            }
            else
            {
                /* 可能直接返回了原始音频数据 */
                LOG_W("No 'audio' field found, trying raw audio data");
                
                if (http_resp.body[0] == 0xFF || http_resp.body[0] == 0x00)
                {
                    /* 看起来像PCM数据 */
                    response->audio_result = (char *)rt_malloc(http_resp.body_len);
                    if (response->audio_result)
                    {
                        rt_memcpy(response->audio_result, http_resp.body, http_resp.body_len);
                        response->audio_len = http_resp.body_len;
                        response->error_code = 0;
                        LOG_I("Using raw audio data: %d bytes", http_resp.body_len);
                    }
                }
                else
                {
                    LOG_E("Unknown response format");
                    LOG_D("Response preview: %.50s", http_resp.body);
                    response->error_code = -1;
                    response->error_msg = rt_strdup("Unknown response format");
                }
            }
        }
        else
        {
            LOG_E("Empty response body");
            response->error_code = -1;
            response->error_msg = rt_strdup("Empty response");
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

/* 全双工语音交互（语音输入 -> 识别 -> AI处理 -> 语音输出）*/
int ai_cloud_service_full_duplex(const uint8_t *audio_data, uint32_t audio_len,
                                  ai_response_t *response)
{
    ai_response_t stt_response;
    int ret;
    
    if (!g_ai_initialized)
    {
        LOG_E("AI service not initialized");
        return -RT_ERROR;
    }
    
    if (audio_data == RT_NULL || audio_len == 0 || response == RT_NULL)
    {
        LOG_E("Invalid parameters");
        return -RT_EINVAL;
    }
    
    rt_memset(response, 0, sizeof(ai_response_t));
    rt_memset(&stt_response, 0, sizeof(ai_response_t));
    
    LOG_I("Starting full duplex interaction");
    
    /* 步骤1：语音识别 */
    ret = ai_cloud_service_speech_to_text(audio_data, audio_len, &stt_response);
    if (ret != RT_EOK || stt_response.text_result == RT_NULL)
    {
        LOG_E("Speech to text failed");
        response->error_code = stt_response.error_code;
        response->error_msg = rt_strdup("Speech recognition failed");
        ai_cloud_service_free_response(&stt_response);
        return -RT_ERROR;
    }
    
    LOG_I("Recognized text: %s", stt_response.text_result);
    
    /* 步骤2：语音合成AI回复 */
    /* 注意：这里可以添加额外的AI对话处理，比如调用ChatGPT等 */
    /* 为了简化，这里直接将识别的文本合成语音作为回复 */
    char reply_text[256];
    rt_snprintf(reply_text, sizeof(reply_text), "您说的是：%s", stt_response.text_result);
    
    ret = ai_cloud_service_text_to_speech(reply_text, response);
    if (ret != RT_EOK)
    {
        LOG_E("Text to speech failed");
        response->error_code = -1;
        response->error_msg = rt_strdup("Speech synthesis failed");
    }
    
    /* 保留识别的文本 */
    response->text_result = stt_response.text_result;
    stt_response.text_result = RT_NULL;  /* 防止被释放 */
    
    ai_cloud_service_free_response(&stt_response);
    
    LOG_I("Full duplex interaction completed");
    
    return ret;
}

/* 释放响应数据 */
void ai_cloud_service_free_response(ai_response_t *response)
{
    if (response)
    {
        if (response->text_result)
        {
            rt_free(response->text_result);
            response->text_result = RT_NULL;
        }
        if (response->audio_result)
        {
            rt_free(response->audio_result);
            response->audio_result = RT_NULL;
        }
        if (response->error_msg)
        {
            rt_free(response->error_msg);
            response->error_msg = RT_NULL;
        }
        response->audio_len = 0;
        response->error_code = 0;
    }
}

/* 导出MSH命令 */
#ifdef FINSH_USING_MSH
static int cmd_ai_test(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: ai_test [init|stt|tts]\n");
        rt_kprintf("  init <provider> <api_key> <app_id> <api_url>\n");
        rt_kprintf("  tts <text>\n");
        return -1;
    }
    
    if (strcmp(argv[1], "init") == 0)
    {
        if (argc < 6)
        {
            rt_kprintf("Usage: ai_test init <provider> <api_key> <app_id> <api_url>\n");
            return -1;
        }
        
        ai_service_config_t config;
        config.provider = atoi(argv[2]);
        strncpy(config.api_key, argv[3], sizeof(config.api_key) - 1);
        strncpy(config.app_id, argv[4], sizeof(config.app_id) - 1);
        strncpy(config.api_url, argv[5], sizeof(config.api_url) - 1);
        
        return ai_cloud_service_init(&config);
    }
    else if (strcmp(argv[1], "tts") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Usage: ai_test tts <text>\n");
            return -1;
        }
        
        ai_response_t response;
        int ret = ai_cloud_service_text_to_speech(argv[2], &response);
        
        if (ret == RT_EOK)
        {
            rt_kprintf("TTS successful, audio_len: %d\n", response.audio_len);
            
            /* 播放TTS音频 */
            if (response.audio_result && response.audio_len > 0)
            {
                /* 需要包含audio_player.h */
                extern int audio_player_play(const uint8_t *data, uint32_t size);
                
                LOG_I("Playing TTS audio...");
                ret = audio_player_play((uint8_t *)response.audio_result, response.audio_len);
                
                if (ret == RT_EOK)
                {
                    /* 等待播放完成 */
                    extern int audio_player_get_state(void);
                    while (audio_player_get_state() == 1)  /* AUDIO_PLAYER_PLAYING = 1 */
                    {
                        rt_thread_mdelay(100);
                    }
                    LOG_I("TTS playback completed");
                }
                else
                {
                    LOG_E("Failed to play TTS audio");
                }
            }
        }
        else
        {
            rt_kprintf("TTS failed: %s\n", response.error_msg);
        }
        
        ai_cloud_service_free_response(&response);
        return ret;
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        return -1;
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_ai_test, ai_test, AI cloud service test commands);
#endif

