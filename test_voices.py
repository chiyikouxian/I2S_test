#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
讯飞TTS发音人对比测试
测试不同的发音人，找到最喜欢的声音
"""

import os
import sys
sys.path.insert(0, os.path.dirname(__file__))

from xfyun_tts_proxy import text_to_speech
import wave

# 测试文本
TEST_TEXT = "你好，我是智能语音助手，很高兴为您服务"

# 发音人列表
VOICES = {
    # 超拟人男声（高级版）?????
    "x5_lingfeiyi_flow": "聆飞逸（超拟人男声，最自然）?推荐",
    
    # 超拟人女声（高级版）
    "x5_lingxiaoxuan_flow": "聆小暄（超拟人女声）",
    "x5_lingxiaoyue_flow": "聆小玥（超拟人女声）",
    "x5_lingyuzhao_flow": "聆玉昭（超拟人女声）",
    
    # 普通男声（对比用）
    "aisjiuxu": "许久（普通男声）",
    "aisjinger": "小婧（普通男声，年轻）",
    
    # 普通女声（对比用）
    "xiaoyan": "小燕（普通女声）",
}

def save_as_wav(pcm_data, filename):
    """保存为WAV文件"""
    with wave.open(filename, 'wb') as wav_file:
        wav_file.setnchannels(1)  # 单声道
        wav_file.setsampwidth(2)  # 16bit = 2字节
        wav_file.setframerate(16000)  # 16kHz
        wav_file.writeframes(pcm_data)

def main():
    print("\n" + "="*60)
    print("??  讯飞TTS发音人对比测试")
    print("="*60)
    print(f"测试文本: {TEST_TEXT}")
    print("="*60)
    
    # 创建输出目录
    output_dir = "voice_samples"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    print("\n开始生成各发音人的语音样本...\n")
    
    success_count = 0
    
    for vcn, description in VOICES.items():
        print(f"? {vcn} - {description}")
        
        try:
            # 调用TTS
            audio_data = text_to_speech(TEST_TEXT, vcn=vcn, speed=50, volume=50)
            
            if audio_data:
                # 保存文件
                filename = os.path.join(output_dir, f"{vcn}_{description.split('（')[0]}.wav")
                save_as_wav(audio_data, filename)
                print(f"   ? 已保存: {filename}")
                print(f"   大小: {len(audio_data)} 字节\n")
                success_count += 1
            else:
                print(f"   ? 生成失败\n")
                
        except Exception as e:
            print(f"   ? 错误: {e}\n")
    
    # 显示结果
    print("="*60)
    print(f"? 完成！成功生成 {success_count}/{len(VOICES)} 个语音样本")
    print("="*60)
    
    if success_count > 0:
        print(f"\n? 语音样本已保存到: {output_dir}/")
        print("\n? 现在可以播放对比各个发音人的效果：")
        print(f"   双击 {output_dir}/ 目录下的 .wav 文件")
        
        print("\n推荐的男声发音人：")
        print("   1. aisjiuxu (许久) - 最自然的男声，推荐！?????")
        print("   2. aisjinger (小婧) - 年轻男声")
        
        print("\n? 找到喜欢的发音人后，记下代码（如 aisjiuxu）")
        print("   可以在板子代码中修改使用")

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n??  测试被中断")
    except Exception as e:
        print(f"\n? 发生错误: {e}")
        import traceback
        traceback.print_exc()

