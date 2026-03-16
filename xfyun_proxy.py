#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
讯飞星火大模型HTTP代理服务器
用途：将设备的HTTP请求转发为HTTPS请求到讯飞星火API

使用方法：
1. 安装依赖: pip install flask requests
2. 运行: python xfyun_proxy.py
3. 设备连接到: http://你的PC_IP:8080/v2/app/conversation
"""

from flask import Flask, request, jsonify
import requests
import json
import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s'
)
logger = logging.getLogger(__name__)

app = Flask(__name__)

# 配置Flask不对JSON进行ASCII转义，确保中文正常显示
app.config['JSON_AS_ASCII'] = False

# ==================== 配置区域 ====================
# 讯飞星火大模型配置
# 使用简单的HTTP API

# 讯飞星火凭证
# 获取方法：
# 1. 访问讯飞开放平台：https://console.xfyun.cn/
# 2. 注册并创建应用
# 3. 在"我的应用"中获取凭证
# 注意：HTTP接口和WebSocket接口使用不同的认证方式！

# HTTP接口认证（用于HTTP API）
# APIPassword格式: APIKey:APISecret
XFYUN_API_PASSWORD = "WpSrgFhnjCkyixwkuBGS:MhpCPllxVLJSQrkciRNt"

# 从APIPassword中解析HTTP接口的APIKey和APISecret
XFYUN_HTTP_API_KEY = XFYUN_API_PASSWORD.split(':')[0]      # WpSrgFhnjCkyixwkuBGS
XFYUN_HTTP_API_SECRET = XFYUN_API_PASSWORD.split(':')[1]   # MhpCPllxVLJSQrkciRNt

# WebSocket接口认证（不同的凭证，仅用于WebSocket）
XFYUN_APPID = "f1c70741"
XFYUN_API_KEY = "95ce069baa8841fd97e7707c0c4a8560"
XFYUN_API_SECRET = "MWI5YzllYzBlZDk4ZmY5MGNiY2ZlZDgx"

# 讯飞星火API配置
AI_PROVIDER = "xfyun"  # 使用讯飞星火
XFYUN_API_URL = "https://spark-api-open.xf-yun.com/v1/chat/completions"  # Spark Ultra-32K
XFYUN_MODEL = "4.0Ultra"  # 模型版本（重要：必须使用 4.0Ultra，不是 generalv3.5）

# 当前API URL
current_api_url = XFYUN_API_URL
# ==================================================

@app.route('/')
def index():
    """首页 - 显示状态"""
    return """
    <html>
    <head><title>讯飞星火代理服务器</title></head>
    <body>
        <h1>✅ 讯飞星火代理服务器运行中</h1>
        <p><strong>当前API端点:</strong> {}</p>
        <p><strong>设备访问地址:</strong> http://{}:8080/v2/app/conversation</p>
        <h2>测试命令:</h2>
        <pre>curl -X POST http://localhost:8080/v2/app/conversation \
  -H "Content-Type: application/json" \
  -d '{{"messages":[{{"role":"user","content":"你是谁"}}],"stream":false}}'</pre>
        <h2>实时日志:</h2>
        <p>查看终端输出</p>
    </body>
    </html>
    """.format(current_api_url, get_local_ip())

@app.route('/v2/app/conversation', methods=['POST', 'OPTIONS'])
def proxy_conversation():
    """代理对话请求 - 讯飞星火"""
    
    # 处理CORS预检请求
    if request.method == 'OPTIONS':
        return '', 200
    
    try:
        # 获取设备发送的数据
        device_data = request.get_json()
        logger.info("=" * 60)
        logger.info("📥 收到设备请求")
        logger.info(f"请求数据: {json.dumps(device_data, ensure_ascii=False, indent=2)}")
        
        # 提取用户消息
        user_message = ""
        if 'messages' in device_data and len(device_data['messages']) > 0:
            user_message = device_data['messages'][0].get('content', '')
        
        logger.info(f"用户问题: {user_message}")
        
        # 构造讯飞星火API请求
        # 根据讯飞HTTP API文档，可能需要在请求体中包含更多参数
        xfyun_data = {
            "model": XFYUN_MODEL,
            "messages": [
                {
                    "role": "system",
                    "content": "你是小石，一个友好的智能助手。请用简短的中文回答问题，不超过50字。回复中只使用中英文、数字和标点符号，不要使用表情符号、emoji或其他特殊符号。"
                },
                {
                    "role": "user", 
                    "content": user_message
                }
            ],
            "stream": False,
            "max_tokens": 1024,
            "temperature": 0.5
        }
        
        # 构造认证Header（讯飞HTTP API - 按照官方文档）
        # 参考: https://www.xfyun.cn/doc/spark/HTTP调用文档.html
        # 只需要简单的 Bearer + APIPassword，不需要复杂的HMAC签名！
        headers = {
            'Authorization': f'Bearer {XFYUN_API_PASSWORD}',
            'content-type': 'application/json'
        }
        
        logger.info(f"🔄 请求讯飞星火API: {XFYUN_API_URL}")
        logger.info(f"模型: {XFYUN_MODEL}")
        logger.info(f"请求头: Authorization: Bearer {XFYUN_API_PASSWORD[:20]}...")
        
        try:
            # 发送请求到讯飞API
            response = requests.post(
                XFYUN_API_URL,
                json=xfyun_data,
                headers=headers,
                timeout=30
            )
            
            logger.info(f"📤 讯飞响应状态: {response.status_code}")
            logger.info(f"响应内容: {response.text[:500]}...")
            
            if response.status_code == 200:
                logger.info("✅ 请求成功！")
                
                # 解析讯飞返回的数据
                result = response.json()
                
                # 讯飞返回格式: {"choices": [{"message": {"content": "回复"}}]}
                # 转换为兼容格式
                if 'choices' in result and len(result['choices']) > 0:
                    reply = result['choices'][0]['message']['content']
                    logger.info(f"AI回复: {reply}")
                    
                    # 返回兼容百度格式的响应
                    response_data = {
                        "result": reply,
                        "usage": result.get('usage', {})
                    }
                    logger.info(f"返回给设备的响应: {json.dumps(response_data, ensure_ascii=False)}")
                    return jsonify(response_data), 200
                else:
                    logger.error("响应格式错误")
                    return jsonify({
                        'error': '响应格式错误',
                        'detail': result
                    }), 500
            
            elif response.status_code == 401:
                logger.error("⚠️ 认证失败！请检查APPID、APIKey、APISecret是否正确")
                return jsonify({
                    'error': 'Authentication failed',
                    'message': '请检查讯飞凭证是否正确'
                }), 401
            
            else:
                logger.error(f"❌ API返回错误: {response.status_code}")
                return jsonify({
                    'error': f'XFyun API error: {response.status_code}',
                    'detail': response.text
                }), response.status_code
                
        except requests.exceptions.RequestException as e:
            logger.error(f"❌ 请求失败: {str(e)}")
            return jsonify({
                'error': 'Request failed',
                'detail': str(e)
            }), 500
    
    except Exception as e:
        logger.error(f"❌ 代理错误: {str(e)}")
        import traceback
        traceback.print_exc()
        return jsonify({'error': str(e)}), 500

@app.route('/test', methods=['GET', 'POST'])
def test():
    """测试端点"""
    if request.method == 'POST':
        data = request.get_json()
        logger.info(f"测试请求: {data}")
        return jsonify({
            'status': 'ok',
            'echo': data,
            'message': '代理服务器工作正常'
        })
    else:
        return jsonify({
            'status': 'ok',
            'message': '代理服务器工作正常',
            'api_url': current_api_url
        })

@app.after_request
def after_request(response):
    """添加CORS头"""
    response.headers.add('Access-Control-Allow-Origin', '*')
    response.headers.add('Access-Control-Allow-Headers', 'Content-Type,Authorization')
    response.headers.add('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS')
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
    except:
        return 'localhost'

def print_startup_info():
    """打印启动信息"""
    local_ip = get_local_ip()
    print("\n" + "=" * 70)
    print("🚀 讯飞星火大模型HTTP代理服务器已启动！")
    print("=" * 70)
    print(f"\n📍 本机IP地址: {local_ip}")
    print(f"📍 监听端口: 8080")
    print(f"\n🔗 设备访问地址:")
    print(f"   http://{local_ip}:8080/v2/app/conversation")
    print(f"\n🔗 浏览器访问:")
    print(f"   http://{local_ip}:8080/")
    print(f"\n💡 设备配置:")
    print(f"   修改 applications/ai_dialog_tool.c 中的 api_url 为:")
    print(f'   "http://{local_ip}:8080/v2/app/conversation"')
    print(f"\n🧪 测试命令:")
    print(f"   curl http://{local_ip}:8080/test")
    print("\n" + "=" * 70)
    print("按 Ctrl+C 停止服务器\n")

if __name__ == '__main__':
    print_startup_info()
    
    # 启动Flask服务器
    # host='0.0.0.0' 允许局域网内其他设备访问
    app.run(
        host='0.0.0.0',
        port=8080,
        debug=False,
        threaded=True
    )

