/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MAX98357A I2S Audio Driver Header
 */

#ifndef __DRV_AUDIO_MAX98357A_H__
#define __DRV_AUDIO_MAX98357A_H__

#include <rtthread.h>
#include <rtdevice.h>

/* 包含 STM32 HAL 库 */
#include "stm32h7rsxx_hal.h"

/* MAX98357A 配置 */
#define MAX98357A_SAMPLE_RATE    16000  /* 默认采样率 */
#define MAX98357A_CHANNELS       1      /* 单声道 */
#define MAX98357A_BITS           16     /* 16位 */

/* 初始化函数 */
int rt_hw_audio_max98357a_init(void);

#endif /* __DRV_AUDIO_MAX98357A_H__ */

