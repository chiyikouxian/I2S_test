/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 * 2024-10-16     AI Assistant add voice assistant support
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "voice_assistant.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define LED_PIN GET_PIN(O, 5)

/* 语音助手自动启动标志 - 禁用以节省内存 */
#define VOICE_ASSISTANT_AUTO_START  0

int main(void)
{
    rt_uint32_t count = 0;

    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    LOG_I("Desktop Voice Assistant Starting...");
    LOG_I("RT-Thread Version: 5.1.0");

#if VOICE_ASSISTANT_AUTO_START
    /* 等待网络就绪 */
    rt_thread_mdelay(3000);
    
    LOG_I("Initializing voice assistant...");
    
    /* 初始化语音助手 */
    if (voice_assistant_init() == RT_EOK)
    {
        /* 启动语音助手 */
        if (voice_assistant_start() == RT_EOK)
        {
            LOG_I("Voice assistant started successfully");
            rt_kprintf("\n");
            rt_kprintf("===========================================\n");
            rt_kprintf("   Welcome to Desktop Voice Assistant!\n");
            rt_kprintf("===========================================\n");
            rt_kprintf("Commands:\n");
            rt_kprintf("  va_trigger - Trigger voice recognition\n");
            rt_kprintf("  va_status  - Show status\n");
            rt_kprintf("  va_stop    - Stop assistant\n");
            rt_kprintf("===========================================\n\n");
        }
        else
        {
            LOG_E("Failed to start voice assistant");
        }
    }
    else
    {
        LOG_E("Failed to initialize voice assistant");
    }
#endif

    /* LED闪烁任务 */
    while(1)
    {
        count++;
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN, PIN_LOW);
        rt_thread_mdelay(500);
        
        /* 每10秒输出一次状态 */
        if (count % 10 == 0)
        {
            LOG_D("System running, uptime: %d seconds", count);
        }
    }
    
    return RT_EOK;
}

#include "stm32h7rsxx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal XSPI2_BASE */
    SCB->VTOR = XSPI2_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);

