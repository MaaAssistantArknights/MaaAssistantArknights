adb connect 127.0.0.1:5555
adb -s 127.0.0.1:5555 shell screencap /sdcard/screen.png
adb -s 127.0.0.1:5555 pull /sdcard/screen.png .