set port=7555

adb connect 127.0.0.1:%port%
adb -s 127.0.0.1:%port% shell screencap /sdcard/screen.png
adb -s 127.0.0.1:%port% pull /sdcard/screen.png .