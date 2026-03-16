/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-17     AI Assistant first version - Wakeup Word Detector
 */

#ifndef __WAKEUP_DETECTOR_H__
#define __WAKEUP_DETECTOR_H__

#include <rtthread.h>

/* 唤醒词检测回调函数类型 */
typedef void (*wakeup_callback)(void);

/* 唤醒词检测接口 */
int wakeup_detector_init(const char *wakeup_word);
int wakeup_detector_start(void);
int wakeup_detector_stop(void);
int wakeup_detector_set_callback(wakeup_callback callback);

/* 简单的文本匹配检测（用于识别结果中查找唤醒词）*/
rt_bool_t wakeup_detector_check_text(const char *text, const char *wakeup_word);

#endif /* __WAKEUP_DETECTOR_H__ */

