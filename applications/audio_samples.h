/*
 * 预先合成的音频样本（直接编译到固件中）
 * 生成方法：运行 python test_tts.py，然后转换PCM为C数组
 */

#ifndef __AUDIO_SAMPLES_H__
#define __AUDIO_SAMPLES_H__

#include <rtthread.h>

/* 
 * 使用方法：
 * 1. 在PC上运行：python test_tts.py
 * 2. 生成 tts_output/你好.pcm
 * 3. 使用工具转换PCM为C数组（或手动用脚本转换）
 * 4. 把数组数据粘贴到下面
 */

/* 示例：空的音频数据（需要替换为实际数据）*/
/* 
const uint8_t audio_sample_nihao[] = {
    // PCM数据会在这里
    // 格式：16bit PCM, 16kHz, 单声道
    0x00, 0x00, 0x01, 0x02, ...
};
const uint32_t audio_sample_nihao_len = sizeof(audio_sample_nihao);
*/

/* 测试用：440Hz正弦波（500ms）*/
/* 这是一个参考示例，可以直接使用 */

#endif /* __AUDIO_SAMPLES_H__ */

