@echo off
cd /d %~dp0
set javaPath=%cd%
cd ..
::显示上级目录路径

::设置要临时加入到path环境变量中的路径
set My_PATH=%cd%
  
set PATH=%PATH%;%My_PATH%

cd %javaPath%
  
::下面写你其它脚本命令

java -jar .\Maa-HTTP-0.0.1.jar -port=8848
  
pause
