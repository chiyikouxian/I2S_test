/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-16     AI Assistant first version - Audio Player Module
 */

#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <rtthread.h>

/* 音频播放配置 */
#define AUDIO_PLAY_SAMPLE_RATE       16000   /* 采样率 16kHz */
#define AUDIO_PLAY_CHANNELS          1       /* 单声道 */
#define AUDIO_PLAY_BITS_PER_SAMPLE   16      /* 16位采样 */
#define AUDIO_PLAY_BUFFER_SIZE       (1024 * 32)  /* 32KB缓冲区（支持TTS音频）*/

/* 音频播放状态 */
typedef enum {
    AUDIO_PLAYER_IDLE = 0,
    AUDIO_PLAYER_PLAYING,
    AUDIO_PLAYER_PAUSED,
    AUDIO_PLAYER_STOPPED
} audio_player_state_t;

/* 音频播放回调函数类型 */
typedef void (*audio_player_callback)(void);

/* 音频播放接口 */
int audio_player_init(void);
int audio_player_play(const uint8_t *data, uint32_t size);
int audio_player_pause(void);
int audio_player_resume(void);
int audio_player_stop(void);
int audio_player_set_callback(audio_player_callback callback);
audio_player_state_t audio_player_get_state(void);
int audio_player_get_free_space(void);

#endif /* __AUDIO_PLAYER_H__ */

