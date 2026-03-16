/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-20     First version - 语音助手UI界面
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_LVGL

#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#define DBG_TAG "lvgl.voice_ui"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* UI对象 */
static lv_obj_t *status_label = NULL;      /* 状态标签 */
static lv_obj_t *user_text = NULL;         /* 用户语音文本 */
static lv_obj_t *ai_text = NULL;           /* AI回复文本 */
static lv_obj_t *volume_slider = NULL;     /* 音量滑块 */
static lv_obj_t *wakeup_indicator = NULL;  /* 唤醒指示器 */

/* 状态颜色 */
#define COLOR_IDLE      lv_color_hex(0x808080)  /* 灰色 - 空闲 */
#define COLOR_WAKEUP    lv_color_hex(0x00FF00)  /* 绿色 - 唤醒 */
#define COLOR_LISTENING lv_color_hex(0x0000FF)  /* 蓝色 - 听音 */
#define COLOR_THINKING  lv_color_hex(0xFFFF00)  /* 黄色 - 思考 */
#define COLOR_SPEAKING  lv_color_hex(0xFF00FF)  /* 紫色 - 播放 */
#define COLOR_ERROR     lv_color_hex(0xFF0000)  /* 红色 - 错误 */

/* 音量滑块回调 */
static void volume_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    
    /* 这里可以调用音频播放器的音量设置函数 */
    LOG_I("Volume changed to: %d", (int)value);
    
    /* 示例：调用音频播放器接口
    #ifdef PKG_USING_AUDIO_PLAYER
    extern void audio_player_set_volume(int volume);
    audio_player_set_volume((int)value);
    #endif
    */
}

/**
 * @brief 创建语音助手UI界面
 */
void lvgl_voice_ui_create(void)
{
    /* 创建主容器 */
    lv_obj_t *main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_center(main_cont);
    
    /* ========== 顶部状态栏 ========== */
    lv_obj_t *status_bar = lv_obj_create(main_cont);
    lv_obj_set_size(status_bar, LV_HOR_RES_MAX - 20, 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x333333), 0);
    
    /* 状态文本 */
    status_label = lv_label_create(status_bar);
    lv_label_set_text(status_label, "Voice Assistant - Ready");
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    
    /* 唤醒指示器 */
    wakeup_indicator = lv_obj_create(status_bar);
    lv_obj_set_size(wakeup_indicator, 20, 20);
    lv_obj_align(wakeup_indicator, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(wakeup_indicator, COLOR_IDLE, 0);
    lv_obj_set_style_radius(wakeup_indicator, LV_RADIUS_CIRCLE, 0);
    
    /* ========== 中间对话区域 ========== */
    lv_obj_t *dialog_area = lv_obj_create(main_cont);
    lv_obj_set_size(dialog_area, LV_HOR_RES_MAX - 20, LV_VER_RES_MAX - 150);
    lv_obj_align(dialog_area, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(dialog_area, lv_color_hex(0xF0F0F0), 0);
    
    /* 用户语音文本框 */
    lv_obj_t *user_cont = lv_obj_create(dialog_area);
    lv_obj_set_size(user_cont, LV_HOR_RES_MAX - 40, 80);
    lv_obj_align(user_cont, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(user_cont, lv_color_hex(0xE3F2FD), 0);
    
    lv_obj_t *user_label = lv_label_create(user_cont);
    lv_label_set_text(user_label, "User:");
    lv_obj_align(user_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_style_text_color(user_label, lv_color_hex(0x1976D2), 0);
    
    user_text = lv_label_create(user_cont);
    lv_label_set_text(user_text, "Waiting for voice input...");
    lv_obj_align(user_text, LV_ALIGN_TOP_LEFT, 5, 25);
    lv_label_set_long_mode(user_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(user_text, LV_HOR_RES_MAX - 50);
    
    /* AI回复文本框 */
    lv_obj_t *ai_cont = lv_obj_create(dialog_area);
    lv_obj_set_size(ai_cont, LV_HOR_RES_MAX - 40, 120);
    lv_obj_align(ai_cont, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_bg_color(ai_cont, lv_color_hex(0xE8F5E9), 0);
    
    lv_obj_t *ai_label = lv_label_create(ai_cont);
    lv_label_set_text(ai_label, "AI Assistant:");
    lv_obj_align(ai_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_style_text_color(ai_label, lv_color_hex(0x388E3C), 0);
    
    ai_text = lv_label_create(ai_cont);
    lv_label_set_text(ai_text, "Ready to assist you!");
    lv_obj_align(ai_text, LV_ALIGN_TOP_LEFT, 5, 25);
    lv_label_set_long_mode(ai_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ai_text, LV_HOR_RES_MAX - 50);
    
    /* ========== 底部控制区 ========== */
    lv_obj_t *control_area = lv_obj_create(main_cont);
    lv_obj_set_size(control_area, LV_HOR_RES_MAX - 20, 80);
    lv_obj_align(control_area, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(control_area, lv_color_hex(0x424242), 0);
    
    /* 音量标签 */
    lv_obj_t *vol_label = lv_label_create(control_area);
    lv_label_set_text(vol_label, "Volume:");
    lv_obj_align(vol_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(vol_label, lv_color_white(), 0);
    
    /* 音量滑块 */
    volume_slider = lv_slider_create(control_area);
    lv_obj_set_size(volume_slider, LV_HOR_RES_MAX - 150, 20);
    lv_obj_align(volume_slider, LV_ALIGN_TOP_LEFT, 80, 10);
    lv_slider_set_range(volume_slider, 0, 100);
    lv_slider_set_value(volume_slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_slider, volume_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    /* 提示文本 */
    lv_obj_t *hint_label = lv_label_create(control_area);
    lv_label_set_text(hint_label, "Say '小智小智' to wake up");
    lv_obj_align(hint_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_text_color(hint_label, lv_color_hex(0xFFD700), 0);
    
    LOG_I("Voice UI created successfully");
}

/**
 * @brief 更新状态显示
 * @param status 状态字符串
 */
void lvgl_update_status(const char *status)
{
    if (status_label != NULL && status != NULL)
    {
        lv_label_set_text(status_label, status);
        LOG_D("Status updated: %s", status);
    }
}

/**
 * @brief 更新唤醒指示器颜色
 * @param state 状态：0=空闲, 1=唤醒, 2=听音, 3=思考, 4=播放, 5=错误
 */
void lvgl_update_wakeup_indicator(int state)
{
    if (wakeup_indicator == NULL)
        return;
    
    lv_color_t color;
    switch (state)
    {
        case 0: color = COLOR_IDLE; break;
        case 1: color = COLOR_WAKEUP; break;
        case 2: color = COLOR_LISTENING; break;
        case 3: color = COLOR_THINKING; break;
        case 4: color = COLOR_SPEAKING; break;
        case 5: color = COLOR_ERROR; break;
        default: color = COLOR_IDLE; break;
    }
    
    lv_obj_set_style_bg_color(wakeup_indicator, color, 0);
}

/**
 * @brief 显示用户语音识别结果
 * @param text 识别的文本
 */
void lvgl_show_recognition(const char *text)
{
    if (user_text != NULL && text != NULL)
    {
        lv_label_set_text(user_text, text);
        lvgl_update_wakeup_indicator(2); /* 听音状态 */
        LOG_I("Recognition: %s", text);
    }
}

/**
 * @brief 显示AI回复
 * @param text AI回复文本
 */
void lvgl_show_ai_reply(const char *text)
{
    if (ai_text != NULL && text != NULL)
    {
        lv_label_set_text(ai_text, text);
        lvgl_update_wakeup_indicator(4); /* 播放状态 */
        LOG_I("AI Reply: %s", text);
    }
}

/**
 * @brief 清除对话内容
 */
void lvgl_clear_dialog(void)
{
    if (user_text != NULL)
        lv_label_set_text(user_text, "Waiting for voice input...");
    
    if (ai_text != NULL)
        lv_label_set_text(ai_text, "Ready to assist you!");
    
    lvgl_update_wakeup_indicator(0); /* 空闲状态 */
}

/**
 * @brief 设置音量显示
 * @param volume 音量值 (0-100)
 */
void lvgl_set_volume(int volume)
{
    if (volume_slider != NULL)
    {
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        lv_slider_set_value(volume_slider, volume, LV_ANIM_ON);
    }
}

/**
 * @brief 显示错误信息
 * @param error 错误信息
 */
void lvgl_show_error(const char *error)
{
    if (ai_text != NULL && error != NULL)
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "[ERROR] %s", error);
        lv_label_set_text(ai_text, buf);
        lvgl_update_wakeup_indicator(5); /* 错误状态 */
        LOG_E("Error: %s", error);
    }
}

/* MSH命令：测试UI */
static int lvgl_test_ui(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: lvgl_test_ui <cmd>\n");
        rt_kprintf("  cmd: create, user, ai, clear, error, status, volume\n");
        return -1;
    }
    
    if (strcmp(argv[1], "create") == 0)
    {
        lvgl_voice_ui_create();
        rt_kprintf("Voice UI created\n");
    }
    else if (strcmp(argv[1], "user") == 0)
    {
        lvgl_show_recognition("今天天气怎么样？");
        rt_kprintf("User text updated\n");
    }
    else if (strcmp(argv[1], "ai") == 0)
    {
        lvgl_show_ai_reply("今天天气晴朗，温度25度，非常适合外出活动。");
        rt_kprintf("AI reply updated\n");
    }
    else if (strcmp(argv[1], "clear") == 0)
    {
        lvgl_clear_dialog();
        rt_kprintf("Dialog cleared\n");
    }
    else if (strcmp(argv[1], "error") == 0)
    {
        lvgl_show_error("Network connection failed");
        rt_kprintf("Error shown\n");
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        lvgl_update_status("Voice Assistant - Processing...");
        rt_kprintf("Status updated\n");
    }
    else if (strcmp(argv[1], "volume") == 0)
    {
        int vol = 75;
        if (argc > 2)
            vol = atoi(argv[2]);
        lvgl_set_volume(vol);
        rt_kprintf("Volume set to %d\n", vol);
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        return -1;
    }
    
    return 0;
}
MSH_CMD_EXPORT(lvgl_test_ui, Test LVGL voice UI);

#endif /* BSP_USING_LVGL */

