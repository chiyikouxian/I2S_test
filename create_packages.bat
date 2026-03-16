@echo off
echo ========================================
echo 创建Voice Assistant Packages结构
echo ========================================

cd /d %~dp0

echo.
echo [1/4] 创建audio-codec package...
mkdir packages\audio-codec\inc 2>nul
mkdir packages\audio-codec\src 2>nul
copy applications\audio_capture.h packages\audio-codec\inc\ >nul
copy applications\audio_capture.c packages\audio-codec\src\ >nul
copy applications\audio_player.h packages\audio-codec\inc\ >nul
copy applications\audio_player.c packages\audio-codec\src\ >nul
echo [OK] audio-codec package created

echo.
echo [2/4] 创建web-client package...
mkdir packages\web-client\inc 2>nul
mkdir packages\web-client\src 2>nul
copy applications\web_client.h packages\web-client\inc\ >nul
copy applications\web_client.c packages\web-client\src\ >nul
echo [OK] web-client package created

echo.
echo [3/4] 创建ai-cloud package...
mkdir packages\ai-cloud\inc 2>nul
mkdir packages\ai-cloud\src 2>nul
copy applications\ai_cloud_service.h packages\ai-cloud\inc\ >nul
copy applications\ai_cloud_service.c packages\ai-cloud\src\ >nul
echo [OK] ai-cloud package created

echo.
echo [4/4] 创建voice-assistant package...
mkdir packages\voice-assistant\inc 2>nul
mkdir packages\voice-assistant\src 2>nul
mkdir packages\voice-assistant\docs 2>nul
copy applications\voice_assistant.h packages\voice-assistant\inc\ >nul
copy applications\voice_assistant.c packages\voice-assistant\src\ >nul
copy applications\voice_assistant_config.h packages\voice-assistant\inc\ >nul
copy applications\wakeup_detector.h packages\voice-assistant\inc\ >nul
copy applications\wakeup_detector.c packages\voice-assistant\src\ >nul
copy applications\QUICK_START.md packages\voice-assistant\docs\ >nul
copy applications\WAKEUP_GUIDE.md packages\voice-assistant\docs\ >nul
echo [OK] voice-assistant package created

echo.
echo ========================================
echo 所有packages创建完成！
echo ========================================
echo.
echo 文件已复制到packages目录，原applications目录文件保持不变
echo.
echo 下一步：
echo 1. 查看 PACKAGE_STRUCTURE.md 了解详细结构
echo 2. 根据需要编辑各package的配置文件
echo 3. 运行 scons 编译项目
echo.
pause

