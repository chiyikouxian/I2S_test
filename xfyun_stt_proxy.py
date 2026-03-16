#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
讯飞语音听写(IAT) HTTP→WebSocket 代理服务器

用途：
  STM32设备不支持HTTPS/WebSocket，通过本代理将设备的HTTP请求
  转发到讯飞WebSocket语音听写API (wss://iat-api.xfyun.cn/v2/iat)

数据流：
  设备 --HTTP POST /stt--> 本代理 --WebSocket--> 讯飞IAT API
  设备 <--HTTP 200 JSON--- 本代理 <--WebSocket--- 讯飞IAT API

使用方法：
  1. pip install flask websocket-client
  2. python xfyun_stt_proxy.py
  3. 设备发送 POST http://<PC_IP>:8080/stt
"""

from flask import Flask, request, jsonify
import websocket
import hashlib
import hmac
import base64
import json
import time
import logging
import threading
from datetime import datetime
from urllib.parse import urlencode, quote
from wsgiref.handlers import format_date_time
from time import mktime

# ==================== 讯飞 IAT 凭证 ====================
# 从讯飞控制台获取（语音听写服务）
XFYUN_APP_ID = "c050bb08"
XFYUN_API_KEY = "c934cd65a7a2da392cf718c1b7687b11"
XFYUN_API_SECRET = "ZTg4YzEwM2M0ZTIzMzIzZjE1ZmFmYWU5"

# IAT WebSocket 地址
IAT_WS_URL = "wss://iat-api.xfyun.cn/v2/iat"

# 每次发送的音频帧大小（字节），讯飞建议不超过 13000
FRAME_SIZE = 8000
# ==================================================

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)
app.config['JSON_AS_ASCII'] = False


def create_iat_auth_url():
    """
    构造讯飞IAT WebSocket鉴权URL

    鉴权方式：HMAC-SHA256 签名
    参考：https://www.xfyun.cn/doc/asr/voicedictation/API.html
    """
    host = "iat-api.xfyun.cn"
    path = "/v2/iat"

    # RFC1123 格式日期
    now = datetime.now()
    date = format_date_time(mktime(now.timetuple()))

    # 构造签名原文
    signature_origin = f"host: {host}\ndate: {date}\nGET {path} HTTP/1.1"

    # HMAC-SHA256 签名
    signature_sha = hmac.new(
        XFYUN_API_SECRET.encode('utf-8'),
        signature_origin.encode('utf-8'),
        digestmod=hashlib.sha256
    ).digest()
    signature = base64.b64encode(signature_sha).decode('utf-8')

    # 构造 authorization
    authorization_origin = (
        f'api_key="{XFYUN_API_KEY}", '
        f'algorithm="hmac256", '
        f'headers="host date request-line", '
        f'signature="{signature}"'
    )
    authorization = base64.b64encode(
        authorization_origin.encode('utf-8')
    ).decode('utf-8')

    # 拼接最终 URL
    params = {
        "authorization": authorization,
        "date": date,
        "host": host
    }
    ws_url = f"{IAT_WS_URL}?{urlencode(params)}"
    return ws_url


def do_iat_recognize(audio_bytes):
    """
    调用讯飞IAT WebSocket API 进行语音识别

    参数：
        audio_bytes: PCM 原始音频数据 (16kHz, 16-bit, mono)

    返回：
        识别结果文本，失败返回 None
    """
    ws_url = create_iat_auth_url()

    results = []       # 收集识别结果片段
    error_info = [None] # 记录错误
    done_event = threading.Event()

    def on_message(ws, message):
        """收到讯飞返回的识别结果"""
        try:
            data = json.loads(message)
            code = data.get("code", -1)

            if code != 0:
                error_info[0] = f"讯飞返回错误 code={code}, msg={data.get('message', '')}"
                logger.error(error_info[0])
                done_event.set()
                return

            # 解析识别结果
            result = data.get("data", {}).get("result", {})
            if result:
                ws_list = result.get("ws", [])
                for ws_item in ws_list:
                    cw_list = ws_item.get("cw", [])
                    for cw in cw_list:
                        word = cw.get("w", "")
                        if word:
                            results.append(word)

            # 检查是否为最后一帧结果
            status = data.get("data", {}).get("status", 0)
            if status == 2:
                done_event.set()

        except Exception as e:
            logger.error(f"解析响应失败: {e}")
            error_info[0] = str(e)
            done_event.set()

    def on_error(ws, error):
        logger.error(f"WebSocket 错误: {error}")
        error_info[0] = str(error)
        done_event.set()

    def on_close(ws, close_status_code, close_msg):
        logger.info(f"WebSocket 已关闭 (code={close_status_code})")
        done_event.set()

    def on_open(ws):
        """连接建立后，分帧发送音频数据"""
        def send_audio():
            try:
                total = len(audio_bytes)
                offset = 0
                frame_idx = 0

                logger.info(f"开始发送音频，总大小: {total} 字节")

                while offset < total:
                    end = min(offset + FRAME_SIZE, total)
                    chunk = audio_bytes[offset:end]

                    # status: 0=第一帧, 1=中间帧, 2=最后一帧
                    if offset == 0:
                        status = 0
                    elif end >= total:
                        status = 2
                    else:
                        status = 1

                    # 构造帧数据
                    frame_data = {
                        "common": {"app_id": XFYUN_APP_ID},
                        "business": {
                            "language": "zh_cn",
                            "domain": "iat",
                            "accent": "mandarin",
                            "vad_eos": 3000,
                            "dwa": "wpgs",
                            "ptt": 0
                        },
                        "data": {
                            "status": status,
                            "format": "audio/L16;rate=16000",
                            "encoding": "raw",
                            "audio": base64.b64encode(chunk).decode('utf-8')
                        }
                    }

                    # 第一帧之后不需要 common 和 business
                    if status != 0:
                        del frame_data["common"]
                        del frame_data["business"]

                    ws.send(json.dumps(frame_data))
                    frame_idx += 1
                    offset = end

                    # 模拟实时发送间隔（讯飞建议40ms/帧）
                    time.sleep(0.04)

                logger.info(f"音频发送完毕，共 {frame_idx} 帧")

            except Exception as e:
                logger.error(f"发送音频失败: {e}")
                error_info[0] = str(e)
                done_event.set()

        # 在子线程中发送音频
        t = threading.Thread(target=send_audio)
        t.daemon = True
        t.start()

    # 创建 WebSocket 连接
    ws = websocket.WebSocketApp(
        ws_url,
        on_open=on_open,
        on_message=on_message,
        on_error=on_error,
        on_close=on_close
    )

    # 在子线程中运行 WebSocket
    wst = threading.Thread(target=ws.run_forever)
    wst.daemon = True
    wst.start()

    # 等待识别完成（最多 30 秒）
    done_event.wait(timeout=30)
    ws.close()

    if error_info[0]:
        logger.error(f"识别失败: {error_info[0]}")
        return None, error_info[0]

    recognized_text = "".join(results)
    logger.info(f"识别结果: {recognized_text}")
    return recognized_text, None


@app.route('/')
def index():
    """首页状态"""
    local_ip = get_local_ip()
    return f"""
    <html>
    <head><title>讯飞语音听写代理</title></head>
    <body>
        <h1>讯飞语音听写(IAT) HTTP代理服务器运行中</h1>
        <p><b>APP_ID:</b> {XFYUN_APP_ID}</p>
        <p><b>设备请求地址:</b> http://{local_ip}:8080/stt</p>
        <h2>设备端配置:</h2>
        <pre>XFYUN_STT_URL = "http://{local_ip}:8080/stt"</pre>
        <h2>测试:</h2>
        <pre>curl http://{local_ip}:8080/test</pre>
    </body>
    </html>
    """


@app.route('/stt', methods=['POST'])
def stt_proxy():
    """
    接收设备的语音识别请求，转发到讯飞IAT WebSocket API

    设备发送的JSON格式：
    {
        "common": {"app_id": "..."},
        "business": {"language": "zh_cn", "domain": "iat", "accent": "mandarin"},
        "data": {"status": 2, "format": "audio/L16;rate=16000", "encoding": "raw", "audio": "BASE64..."}
    }

    返回格式（兼容设备端解析）：
    {"result": ["识别文本"]}
    """
    try:
        device_data = request.get_json()

        logger.info("=" * 60)
        logger.info("收到设备语音识别请求")

        # 提取 Base64 音频数据
        audio_base64 = device_data.get("data", {}).get("audio", "")
        if not audio_base64:
            logger.error("请求中没有音频数据")
            return jsonify({"error": "No audio data"}), 400

        # Base64 解码为原始 PCM 字节
        audio_bytes = base64.b64decode(audio_base64)
        logger.info(f"音频数据: Base64 长度={len(audio_base64)}, PCM 大小={len(audio_bytes)} 字节")
        logger.info(f"音频时长: {len(audio_bytes) / (16000 * 2):.1f} 秒 (16kHz/16bit/mono)")

        # 调用讯飞 IAT WebSocket API
        text, error = do_iat_recognize(audio_bytes)

        if text is not None:
            logger.info(f"识别成功: {text}")
            # 返回格式与设备端 ai_cloud_service.c 解析逻辑匹配
            # 设备端查找 "result" 字段中的第一个字符串
            return jsonify({"result": [text]}), 200
        else:
            logger.error(f"识别失败: {error}")
            return jsonify({"error": error or "Recognition failed"}), 500

    except Exception as e:
        logger.error(f"代理错误: {e}")
        import traceback
        traceback.print_exc()
        return jsonify({"error": str(e)}), 500


@app.route('/test', methods=['GET'])
def test():
    """测试端点"""
    return jsonify({
        "status": "ok",
        "service": "讯飞语音听写(IAT)代理",
        "app_id": XFYUN_APP_ID,
        "message": "代理服务器工作正常"
    })


@app.after_request
def after_request(response):
    """添加 CORS 头"""
    response.headers.add('Access-Control-Allow-Origin', '*')
    response.headers.add('Access-Control-Allow-Headers', 'Content-Type')
    response.headers.add('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')
    return response


def get_local_ip():
    """获取本机局域网IP"""
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return 'localhost'


if __name__ == '__main__':
    local_ip = get_local_ip()

    print()
    print("=" * 60)
    print("  讯飞语音听写(IAT) HTTP代理服务器")
    print("=" * 60)
    print(f"  APP_ID:    {XFYUN_APP_ID}")
    print(f"  API_KEY:   {XFYUN_API_KEY[:16]}...")
    print(f"  本机IP:    {local_ip}")
    print(f"  监听端口:  8080")
    print()
    print(f"  设备配置 XFYUN_STT_URL:")
    print(f"    http://{local_ip}:8080/stt")
    print()
    print(f"  测试: curl http://{local_ip}:8080/test")
    print("=" * 60)
    print("按 Ctrl+C 停止服务器")
    print()

    app.run(host='0.0.0.0', port=8080, debug=False, threaded=True)
