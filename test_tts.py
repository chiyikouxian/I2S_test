#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS功能测试脚本 - 无需板子，直接测试讯飞语音合成
"""

import requests
import json
import base64
import wave
import os

# 代理服务器地址（如果代理在运行）
PROXY_URL = "http://127.0.0.1:8081/tts"

# 测试文本列表
TEST_TEXTS = [
    "你好世界",
    "今天天气真不错",
    "我是语音助手小石",
    "欢迎使用RT-Thread智能语音助手"
]

def save_pcm_as_wav(pcm_data, output_file, sample_rate=16000, channels=1, sample_width=2):
    """
    将PCM数据保存为WAV文件（方便播放）
    :param pcm_data: PCM原始数据
    :param output_file: 输出文件名
    :param sample_rate: 采样率
    :param channels: 声道数
    :param sample_width: 采样位宽（字节）
    """
    with wave.open(output_file, 'wb') as wav_file:
        wav_file.setnchannels(channels)
        wav_file.setsampwidth(sample_width)
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(pcm_data)
    print(f"  ? 已保存为: {output_file}")

def test_tts_via_proxy(text, output_dir="tts_output"):
    """
    通过代理测试TTS
    :param text: 要合成的文本
    :param output_dir: 输出目录
    """
    print(f"\n{'='*60}")
    print(f"? 测试文本: {text}")
    print(f"{'='*60}")
    
    # 创建输出目录
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    try:
        # 发送请求
        print("? 正在调用代理...")
        response = requests.post(
            PROXY_URL,
            json={
                "text": text,
                "vcn": "aisjiuxu",  # 发音人：许久（男声）
                "speed": 50,        # 语速
                "volume": 50        # 音量
            },
            timeout=30
        )
        
        # 检查响应
        if response.status_code != 200:
            print(f"? 请求失败: HTTP {response.status_code}")
            print(f"   响应内容: {response.text}")
            return False
        
        # 解析JSON
        result = response.json()
        
        if result.get("code") != 0:
            print(f"? TTS失败: {result.get('message')}")
            return False
        
        # 提取音频数据
        audio_base64 = result.get("audio")
        audio_len = result.get("audio_len")
        
        if not audio_base64:
            print("? 返回数据中没有音频")
            return False
        
        print(f"? 合成成功！")
        print(f"   音频长度: {audio_len} 字节")
        print(f"   采样率: {result.get('sample_rate')} Hz")
        print(f"   格式: {result.get('format')}")
        
        # Base64解码
        audio_data = base64.b64decode(audio_base64)
        print(f"   解码后: {len(audio_data)} 字节")
        
        # 生成文件名
        safe_name = "".join(c if c.isalnum() else "_" for c in text[:20])
        pcm_file = os.path.join(output_dir, f"{safe_name}.pcm")
        wav_file = os.path.join(output_dir, f"{safe_name}.wav")
        
        # 保存PCM文件
        with open(pcm_file, 'wb') as f:
            f.write(audio_data)
        print(f"  ? 已保存PCM: {pcm_file}")
        
        # 保存为WAV文件（方便播放）
        save_pcm_as_wav(audio_data, wav_file)
        
        # 显示播放命令
        print(f"\n? 播放命令:")
        print(f"   # Windows (需要安装ffplay):")
        print(f"   ffplay -f s16le -ar 16000 -ac 1 {pcm_file}")
        print(f"   # 或者直接播放WAV:")
        print(f"   {wav_file}")
        
        return True
        
    except requests.exceptions.ConnectionError:
        print("? 连接失败！请确认代理服务器正在运行：")
        print("   python xfyun_tts_proxy.py")
        return False
    except Exception as e:
        print(f"? 错误: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_direct_tts(text, output_dir="tts_output"):
    """
    直接调用讯飞API测试TTS（不通过代理）
    这需要实现WebSocket和鉴权，这里简化为导入xfyun_tts_proxy的函数
    """
    print(f"\n{'='*60}")
    print(f"? 测试文本: {text}")
    print(f"{'='*60}")
    
    try:
        # 导入TTS函数
        import sys
        sys.path.insert(0, os.path.dirname(__file__))
        from xfyun_tts_proxy import text_to_speech
        
        print("? 正在调用讯飞API...")
        
        # 创建输出目录
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        # 调用TTS
        audio_data = text_to_speech(text, vcn="aisjiuxu", speed=50, volume=60)  # 许久（男声）
        
        if not audio_data:
            print("? TTS失败，返回数据为空")
            return False
        
        print(f"? 合成成功！")
        print(f"   音频长度: {len(audio_data)} 字节")
        
        # 生成文件名
        safe_name = "".join(c if c.isalnum() else "_" for c in text[:20])
        pcm_file = os.path.join(output_dir, f"{safe_name}.pcm")
        wav_file = os.path.join(output_dir, f"{safe_name}.wav")
        
        # 保存文件
        with open(pcm_file, 'wb') as f:
            f.write(audio_data)
        print(f"  ? 已保存PCM: {pcm_file}")
        
        save_pcm_as_wav(audio_data, wav_file)
        
        print(f"\n? 播放命令:")
        print(f"   {wav_file}")
        
        return True
        
    except Exception as e:
        print(f"? 错误: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """主测试函数"""
    print("\n" + "="*60)
    print("??  讯飞TTS功能测试")
    print("="*60)
    
    # 询问测试方式
    print("\n请选择测试方式：")
    print("  1. 通过代理测试（需要先启动 xfyun_tts_proxy.py）")
    print("  2. 直接调用讯飞API（推荐，无需代理）")
    
    choice = input("\n输入选择 [1/2，默认2]: ").strip() or "2"
    
    if choice == "1":
        print("\n使用代理模式")
        print("??  请确保代理服务器正在运行：")
        print("   python xfyun_tts_proxy.py\n")
        input("按回车键继续...")
        test_func = test_tts_via_proxy
    else:
        print("\n使用直接调用模式（推荐）")
        test_func = test_direct_tts
    
    # 运行测试
    success_count = 0
    total_count = 0
    
    for text in TEST_TEXTS:
        total_count += 1
        if test_func(text):
            success_count += 1
        else:
            print("? 测试失败")
    
    # 显示结果
    print("\n" + "="*60)
    print(f"? 测试结果: {success_count}/{total_count} 成功")
    print("="*60)
    
    if success_count > 0:
        print(f"\n? 音频文件已保存到: tts_output/")
        print("   可以用Windows Media Player或其他播放器打开 .wav 文件")
    
    print("\n测试完成！")

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n??  测试被中断")
    except Exception as e:
        print(f"\n? 发生错误: {e}")
        import traceback
        traceback.print_exc()

