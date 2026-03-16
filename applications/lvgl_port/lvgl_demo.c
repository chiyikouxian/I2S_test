/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-10-20     First version
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_LVGL

#include <lvgl.h>
#include "stm32h7rsxx.h"

#define DBG_TAG "lvgl.demo"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 外部声明LTDC句柄 */
extern LTDC_HandleTypeDef hltdc;

/* 注意：UI对象由 lvgl_voice_ui.c 管理 */

/**
 * @brief 用户GUI初始化函数
 * @note  这个函数由LVGL的RT-Thread移植层（lv_rt_thread_port.c）调用
 *        在 lv_init(), lv_port_disp_init(), lv_port_indev_init() 之后执行
 */
void lv_user_gui_init(void)
{
    LOG_I("Starting user GUI initialization...");
    LOG_I("Screen active: %p", lv_screen_active());
    
#ifdef BSP_USING_LVGL_DEMO
    /* 启动选定的Demo */
    #ifdef BSP_USING_LVGL_WIDGETS_DEMO
        extern void lv_demo_widgets(void);
        lv_demo_widgets();
        LOG_I("LVGL widgets demo started");
    #endif
    
    #ifdef BSP_USING_LVGL_BENCHMARK_DEMO
        extern void lv_demo_benchmark(void);
        lv_demo_benchmark();
        LOG_I("LVGL benchmark demo started");
    #endif
    
    #ifdef BSP_USING_LVGL_MUSIC_DEMO
        extern void lv_demo_music(void);
        lv_demo_music();
        LOG_I("LVGL music demo started");
    #endif
    
    #ifdef BSP_USING_LVGL_STRESS_DEMO
        extern void lv_demo_stress(void);
        lv_demo_stress();
        LOG_I("LVGL stress demo started");
    #endif
    
    #ifdef BSP_USING_LVGL_RENDER_DEMO
        extern void lv_demo_render(void);
        lv_demo_render();
        LOG_I("LVGL render demo started");
    #endif
#else
    /* 使用 lvgl_voice_ui.c 提供的完整UI界面 */
    extern void lvgl_voice_ui_create(void);
    
    LOG_I("Creating voice assistant UI...");
    lvgl_voice_ui_create();
    
    LOG_I("Voice assistant UI created successfully");
#endif
    
    LOG_I("User GUI initialization complete");
}

/* 注意：UI更新函数已在 lvgl_voice_ui.c 中实现 */

#endif /* BSP_USING_LVGL */

