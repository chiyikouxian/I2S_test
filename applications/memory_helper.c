/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant Memory helper utilities
 */

#include <rtthread.h>

#define DBG_TAG "mem.helper"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* 获取可用内存信息 */
static void show_memory_info(void)
{
    rt_size_t total, used, max_used;
    
    rt_memory_info(&total, &used, &max_used);
    
    rt_kprintf("\n========================================\n");
    rt_kprintf("  Memory Information\n");
    rt_kprintf("========================================\n");
    rt_kprintf("Total Memory : %d bytes (%.2f KB)\n", total, total / 1024.0);
    rt_kprintf("Used Memory  : %d bytes (%.2f KB)\n", used, used / 1024.0);
    rt_kprintf("Free Memory  : %d bytes (%.2f KB)\n", total - used, (total - used) / 1024.0);
    rt_kprintf("Max Used     : %d bytes (%.2f KB)\n", max_used, max_used / 1024.0);
    rt_kprintf("Usage        : %.1f%%\n", (used * 100.0) / total);
    rt_kprintf("========================================\n\n");
    
    if ((total - used) < 64 * 1024)
    {
        rt_kprintf("⚠️  Warning: Available memory is low!\n");
        rt_kprintf("    Consider:\n");
        rt_kprintf("    - Reducing buffer sizes\n");
        rt_kprintf("    - Closing unused features\n");
        rt_kprintf("    - Restarting the system\n\n");
    }
}

/* MSH命令 */
#ifdef FINSH_USING_MSH
static int cmd_meminfo(int argc, char **argv)
{
    show_memory_info();
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_meminfo, meminfo, Show detailed memory information);
#endif

