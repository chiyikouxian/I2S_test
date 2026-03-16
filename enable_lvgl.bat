@echo off
echo ========================================
echo 启用LVGL配置脚本
echo ========================================
echo.

cd /d "%~dp0"

echo 正在打开配置菜单...
echo.
echo 请在菜单中选择：
echo   Hardware Drivers Config
echo     - On-chip Peripheral Drivers
echo       [*] Enable LCD (BSP_USING_LCD)
echo         (*) Enable LCD RGB (BSP_USING_LCD_800_RGB)
echo.
echo     - Onboard Peripheral Drivers
echo       [*] Enable LVGL for LCD (BSP_USING_LVGL)
echo         [*] Enable LVGL for LCD_RGB565
echo           [*] Enable Touch control
echo             (*) USING Touch IC GT9147
echo.
echo 保存后（按两次ESC，选择Yes）会自动更新软件包
echo.
pause

scons --menuconfig

echo.
echo 配置已保存，正在更新软件包...
echo.

echo 请手动运行: env pkgs --update
echo.
pause

