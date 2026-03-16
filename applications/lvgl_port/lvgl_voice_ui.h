/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-22     First version - Voice Assistant UI API
 */

#ifndef LVGL_VOICE_UI_H
#define LVGL_VOICE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建语音助手UI界面
 */
void lvgl_voice_ui_create(void);

/**
 * @brief 更新状态显示
 * @param status 状态文本，例如："Voice Assistant - Listening..."
 */
void lvgl_update_status(const char *status);

/**
 * @brief 更新唤醒指示器颜色
 * @param state 状态：0=空闲, 1=唤醒, 2=听音, 3=思考, 4=播放, 5=错误
 */
void lvgl_update_wakeup_indicator(int state);

/**
 * @brief 显示用户语音识别结果
 * @param text 识别的文本
 */
void lvgl_show_recognition(const char *text);

/**
 * @brief 显示AI回复
 * @param text AI回复文本
 */
void lvgl_show_ai_reply(const char *text);

/**
 * @brief 清除对话内容
 */
void lvgl_clear_dialog(void);

/**
 * @brief 设置音量显示
 * @param volume 音量值 (0-100)
 */
void lvgl_set_volume(int volume);

/**
 * @brief 显示错误信息
 * @param error 错误信息
 */
void lvgl_show_error(const char *error);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_VOICE_UI_H */
