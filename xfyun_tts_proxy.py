#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
讯飞语音合成（TTS）HTTP代理服务器
板子通过简单的HTTP请求调用，代理负责复杂的WebSocket和鉴权
"""

from flask import Flask, request, jsonify
import websocket
import json
import base64
import hmac
import hashlib
from datetime import datetime
from time import mktime
from wsgiref.handlers import format_date_time
from urllib.parse import urlencode
import ssl
import socket

# ==================== 讯飞API配置 ====================
# 请在这里填入你的讯飞密钥
# 获取方式：https://console.xfyun.cn/ -> 创建应用 -> 查看APPID/APIKey/APISecret
XFYUN_APPID = "f1c70741"          # 从xfyun_proxy.py复制
XFYUN_API_KEY = "95ce069baa8841fd97e7707c0c4a8560"      # 从xfyun_proxy.py复制
XFYUN_API_SECRET = "MWI5YzllYzBlZDk4ZmY5MGNiY2ZlZDgx" # 从xfyun_proxy.py复制
# =====================================================

app = Flask(__name__)

def get_local_ip():
    """获取本机IP地址"""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

def create_url():
    """生成讯飞TTS WebSocket URL（包含鉴权）"""
    # 生成RFC1123时间戳
    now = datetime.now()
    date = format_date_time(mktime(now.timetuple()))
    
    # 拼接签名原串
    signature_origin = f"host: tts-api.xfyun.cn\ndate: {date}\nGET /v2/tts HTTP/1.1"
    
    # HMAC-SHA256加密
    signature_sha = hmac.new(
        XFYUN_API_SECRET.encode('utf-8'),
        signature_origin.encode('utf-8'),
        digestmod=hashlib.sha256
    ).digest()
    
    # Base64编码
    signature_sha_base64 = base64.b64encode(signature_sha).decode('utf-8')
    
    # 构造鉴权参数
    authorization_origin = f'api_key="{XFYUN_API_KEY}", algorithm="hmac-sha256", headers="host date request-line", signature="{signature_sha_base64}"'
    authorization = base64.b64encode(authorization_origin.encode('utf-8')).decode('utf-8')
    
    # 构造完整URL
    params = {
        "authorization": authorization,
        "date": date,
        "host": "tts-api.xfyun.cn"
    }
    
    url = f"wss://tts-api.xfyun.cn/v2/tts?{urlencode(params)}"
    return url

def text_to_speech(text, vcn="xiaoyan", speed=50, volume=50):
    """
    调用讯飞TTS API
    :param text: 要合成的文本
    :param vcn: 发音人 (xiaoyan-小燕, aisjiuxu-许久, aisxping-小萍)
    :param speed: 语速 0-100
    :param volume: 音量 0-100
    :return: PCM音频数据
    """
    # 生成WebSocket URL
    ws_url = create_url()
    
    # 准备音频数据缓冲区
    audio_data = bytearray()
    error_msg = None
    
    # WebSocket回调函数
    def on_message(ws, message):
        nonlocal audio_data, error_msg
        try:
            data = json.loads(message)
            code = data.get("code")
            
            if code != 0:
                error_msg = f"讯飞返回错误: {code}, {data.get('message')}"
                print(f"[错误] {error_msg}")
                ws.close()
                return
            
            # 提取音频数据
            audio_base64 = data.get("data", {}).get("audio")
            if audio_base64:
                audio_chunk = base64.b64decode(audio_base64)
                audio_data.extend(audio_chunk)
                print(f"[接收] 音频数据: {len(audio_chunk)} 字节")
            
            # 检查是否结束
            status = data.get("data", {}).get("status")
            if status == 2:  # 合成完成
                print(f"[完成] 合成完成，总计: {len(audio_data)} 字节")
                ws.close()
        except Exception as e:
            error_msg = f"处理消息失败: {e}"
            print(f"[错误] {error_msg}")
            ws.close()
    
    def on_error(ws, error):
        nonlocal error_msg
        error_msg = f"WebSocket错误: {error}"
        print(f"[错误] {error_msg}")
    
    def on_open(ws):
        print(f"[连接] WebSocket已连接")
        try:
            # 构造请求参数
            request_data = {
                "common": {
                    "app_id": XFYUN_APPID
                },
                "business": {
                    "vcn": vcn,
                    "speed": speed,
                    "volume": volume,
                    "pitch": 50,
                    "aue": "raw",  # PCM格式
                    "auf": "audio/L16;rate=16000",  # 16kHz采样率
                    "tte": "UTF8"
                },
                "data": {
                    "text": base64.b64encode(text.encode('utf-8')).decode('utf-8'),
                    "status": 2
                }
            }
            ws.send(json.dumps(request_data))
            print(f"[发送] 已发送TTS请求: {text}")
        except Exception as e:
            print(f"[错误] 发送请求失败: {e}")
            ws.close()
    
    def on_close(ws, close_status_code, close_msg):
        print(f"[关闭] WebSocket已关闭")
    
    # 创建WebSocket连接
    try:
        print(f"[开始] 合成文本: {text}")
        ws = websocket.WebSocketApp(
            ws_url,
            on_message=on_message,
            on_error=on_error,
            on_open=on_open,
            on_close=on_close
        )
        
        ws.run_forever(sslopt={"cert_reqs": ssl.CERT_NONE})
        
        if error_msg:
            raise Exception(error_msg)
        
        return bytes(audio_data)
    
    except Exception as e:
        print(f"[错误] TTS失败: {e}")
        raise

@app.route('/tts', methods=['POST'])
def tts_endpoint():
    """HTTP端点：接收板子的TTS请求"""
    try:
        data = request.get_json()
        if not data:
            return jsonify({"error": "请求体必须是JSON格式"}), 400
        
        text = data.get('text', '')
        vcn = data.get('vcn', 'xiaoyan')  # 默认小燕
        speed = data.get('speed', 50)     # 默认语速
        volume = data.get('volume', 50)   # 默认音量
        
        if not text:
            return jsonify({"error": "缺少text参数"}), 400
        
        print(f"\n{'='*60}")
        print(f"[TTS请求]")
        print(f"  文本: {text}")
        print(f"  发音人: {vcn}")
        print(f"  语速: {speed}, 音量: {volume}")
        print(f"{'='*60}")
        
        # 调用讯飞TTS
        audio_data = text_to_speech(text, vcn, speed, volume)
        
        if not audio_data:
            return jsonify({"error": "语音合成失败，返回数据为空"}), 500
        
        # 返回音频数据（Base64编码）
        audio_base64 = base64.b64encode(audio_data).decode('utf-8')
        
        print(f"[成功] 返回音频: {len(audio_data)} 字节")
        print(f"{'='*60}\n")
        
        return jsonify({
            "code": 0,
            "message": "success",
            "audio": audio_base64,
            "audio_len": len(audio_data),
            "format": "pcm",
            "sample_rate": 16000,
            "channels": 1
        })
        
    except Exception as e:
        print(f"[错误] 处理请求失败: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500

@app.route('/test', methods=['GET'])
def test():
    """测试端点"""
    return jsonify({
        "status": "running",
        "service": "讯飞TTS代理",
        "appid": XFYUN_APPID[:4] + "****" if len(XFYUN_APPID) > 4 else "未配置",
        "version": "1.0.0"
    })

@app.route('/', methods=['GET'])
def index():
    """首页"""
    return """
    <html>
    <head><title>讯飞TTS代理</title></head>
    <body style="font-family: Arial; padding: 20px;">
        <h1>?? 讯飞TTS代理服务器</h1>
        <p>状态: <span style="color: green;">运行中</span></p>
        <h2>API接口：</h2>
        <ul>
            <li><b>POST /tts</b> - 语音合成
                <pre>{
  "text": "你好世界",
  "vcn": "xiaoyan",  // 可选: xiaoyan, aisjiuxu, aisxping
  "speed": 50,       // 可选: 0-100
  "volume": 50       // 可选: 0-100
}</pre>
            </li>
            <li><b>GET /test</b> - 测试服务</li>
        </ul>
        <h2>测试命令：</h2>
        <pre>curl -X POST http://""" + get_local_ip() + """:8081/tts \\
  -H "Content-Type: application/json" \\
  -d '{"text":"你好世界"}'</pre>
    </body>
    </html>
    """

if __name__ == '__main__':
    # 检查配置
    if XFYUN_APPID == "your_appid_here" or "your_" in XFYUN_APPID:
        print("\n" + "="*60)
        print("??  警告：请先配置讯飞密钥！")
        print("="*60)
        print("请编辑此文件，修改第20-22行：")
        print(f"  XFYUN_APPID      = \"你的APPID\"")
        print(f"  XFYUN_API_KEY    = \"你的APIKey\"")
        print(f"  XFYUN_API_SECRET = \"你的APISecret\"")
        print("\n获取密钥：https://console.xfyun.cn/")
        print("="*60 + "\n")
        exit(1)
    
    local_ip = get_local_ip()
    
    print("\n" + "="*60)
    print("? 讯飞TTS代理服务器已启动！")
    print("="*60)
    print(f"? 本机IP地址: {local_ip}")
    print(f"? 访问地址: http://{local_ip}:8081")
    print(f"? 测试页面: http://{local_ip}:8081/")
    print(f"? TTS接口: POST http://{local_ip}:8081/tts")
    print("\n请求示例：")
    print(f"  curl -X POST http://{local_ip}:8081/tts \\")
    print(f'    -H "Content-Type: application/json" \\')
    print(f'    -d \'{{"text":"你好世界"}}\'')
    print("\n配置信息：")
    print(f"  APPID: {XFYUN_APPID[:4]}****")
    print("="*60 + "\n")
    
    # 安装websocket-client提示
    try:
        import websocket
    except ImportError:
        print("??  缺少依赖：websocket-client")
        print("请运行：pip install websocket-client\n")
        exit(1)
    
    # 启动服务器
    app.run(host='0.0.0.0', port=8081, debug=False)

