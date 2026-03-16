#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将PCM音频文件转换为C数组
用于直接编译到固件中
"""

import sys
import os

def pcm_to_c_array(pcm_file, var_name="audio_sample", output_file=None):
    """
    将PCM文件转换为C数组
    
    Args:
        pcm_file: PCM文件路径
        var_name: C变量名
        output_file: 输出文件路径（可选）
    """
    # 读取PCM文件
    try:
        with open(pcm_file, 'rb') as f:
            pcm_data = f.read()
    except FileNotFoundError:
        print(f"错误：找不到文件 {pcm_file}")
        return
    
    file_size = len(pcm_data)
    print(f"文件大小: {file_size} 字节")
    print(f"音频时长: {file_size / (16000 * 2):.2f} 秒 (假设16kHz, 16bit)")
    
    # 生成C数组代码
    c_code = []
    c_code.append(f"/* 音频样本：{os.path.basename(pcm_file)} */")
    c_code.append(f"/* 大小：{file_size} 字节，时长：{file_size / (16000 * 2):.2f}秒 */")
    c_code.append(f"const uint8_t {var_name}[] = {{")
    
    # 每行16个字节
    for i in range(0, file_size, 16):
        line = "    "
        for j in range(16):
            if i + j < file_size:
                line += f"0x{pcm_data[i+j]:02X}, "
        c_code.append(line)
    
    c_code.append("};")
    c_code.append(f"const uint32_t {var_name}_len = {file_size};")
    c_code.append("")
    
    result = "\n".join(c_code)
    
    # 输出到文件或打印
    if output_file:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(result)
        print(f"\n? 已保存到: {output_file}")
    else:
        print("\n" + "="*60)
        print("生成的C代码（复制到audio_samples.h中）：")
        print("="*60)
        print(result)
        print("="*60)
    
    return result

def batch_convert(directory="tts_output"):
    """批量转换目录中的所有PCM文件"""
    if not os.path.exists(directory):
        print(f"错误：目录不存在 {directory}")
        return
    
    pcm_files = [f for f in os.listdir(directory) if f.endswith('.pcm')]
    
    if not pcm_files:
        print(f"错误：在 {directory} 中没有找到PCM文件")
        print("提示：先运行 python test_tts.py 生成音频文件")
        return
    
    print(f"找到 {len(pcm_files)} 个PCM文件")
    print()
    
    all_code = []
    all_code.append("/* 自动生成的音频样本 */")
    all_code.append("/* 生成工具：pcm_to_c_array.py */")
    all_code.append("")
    all_code.append("#ifndef __AUDIO_SAMPLES_H__")
    all_code.append("#define __AUDIO_SAMPLES_H__")
    all_code.append("")
    all_code.append("#include <rtthread.h>")
    all_code.append("")
    
    for pcm_file in pcm_files:
        file_path = os.path.join(directory, pcm_file)
        # 生成合法的C变量名
        var_name = "audio_sample_" + pcm_file.replace('.pcm', '').replace('-', '_').replace(' ', '_')
        
        print(f"转换: {pcm_file} -> {var_name}")
        
        c_code = pcm_to_c_array(file_path, var_name, output_file=None)
        all_code.append(c_code)
        all_code.append("")
    
    all_code.append("#endif /* __AUDIO_SAMPLES_H__ */")
    
    # 保存到文件
    output_file = "applications/audio_samples_generated.h"
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("\n".join(all_code))
    
    print(f"\n? 所有音频样本已保存到: {output_file}")
    print(f"\n使用方法：")
    print(f"  #include \"audio_samples_generated.h\"")
    print(f"  audio_player_play(audio_sample_xxx, audio_sample_xxx_len);")

def main():
    print("="*60)
    print("PCM音频文件转C数组工具")
    print("="*60)
    print()
    
    if len(sys.argv) > 1:
        # 单个文件转换
        pcm_file = sys.argv[1]
        var_name = sys.argv[2] if len(sys.argv) > 2 else "audio_sample"
        output_file = sys.argv[3] if len(sys.argv) > 3 else None
        
        pcm_to_c_array(pcm_file, var_name, output_file)
    else:
        # 批量转换
        print("批量转换模式（转换 tts_output/ 目录中的所有PCM文件）")
        print()
        batch_convert("tts_output")

if __name__ == '__main__':
    main()

