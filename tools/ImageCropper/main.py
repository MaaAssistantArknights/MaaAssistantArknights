import cv2
import os
import sys
import numpy as np
import colormatcher
from datetime import datetime
from roimage import Roimage

from maa.define import MaaAdbScreencapMethodEnum, MaaWin32ScreencapMethodEnum
from maa.controller import AdbController, Win32Controller, Controller
from maa.toolkit import Toolkit

# 初始化设备参数
# device_serial = "127.0.0.1:16384"
device_serial = None
adb_screencap_method = MaaAdbScreencapMethodEnum.Default
win32_screencap_method = MaaWin32ScreencapMethodEnum.DXGI_DesktopDup

# 初始窗口大小 (width, height)
# window_size = (720, 1280) # 竖屏
window_size = (1280, 720)  # 横屏


# 截图参数
def set_screenshot_target_side(c: Controller):
    # c.set_screenshot_target_long_side(1280)
    c.set_screenshot_target_short_side(720)


# ROI 放大方法
def amplify(rect: list[int]) -> list[int]:
    x, y, w, h = rect
    return [x - 50, y - 50, w + 100, h + 100]


# 颜色匹配方法
def match_color(image) -> tuple[int, int, list[tuple[list[int]]]]:
    # framework ColorMatch common method:
    #   4.COLOR_BGR2RGB 40.COLOR_BGR2HSV 6.COLOR_BGR2GRAY
    # colormatcher method:
    #   1.Simple 2.RGBDistance
    method = cv2.COLOR_BGR2RGB
    reverse = cv2.COLOR_RGB2BGR  # 不需要逆转时为None，用于显示 'MainColors' 窗口
    cluster_colors = colormatcher.kmeansClusterColors(image, method)
    return method, reverse, colormatcher.RGBDistance(cluster_colors)


# -----------------------------------------------

print("Usage: python3 main.py [device serial]\n"
      "Put the images under ./src, and run this script, it will be auto converted to target size.\n"
      "Hold down the left mouse button, drag mouse to select a ROI.\n"
      "Hold down the right mouse button, drag mouse to move the image.\n"
      "Use the mouse wheel to zoom the image.\n"
      "press 'S' or 'ENTER' to save ROIs.\n"
      "press 'F' to save a full standardized screenshot.\n"
      "press 'R' to output only the ROI ranges, not save.\n"
      "press 'c' or 'C' (with connected) to output the ROI ranges and colors, not save.\n"
      "press 'Z' or 'DELETE' or 'BACKSPACE' to remove the latest ROI.\n"
      "press '0' ~ '9' to resize the window.\n"
      "press 'Q' or 'ESC' to quit.\n"
      "The cropped images will be saved in ./dst.\n")


# 解析命令行参数
def parse_args() -> Controller:
    global device_serial
    if len(sys.argv) > 1:
        device_serial = sys.argv[1]
    if device_serial:
        return AdbController(
            adb_path="adb",
            address=device_serial,
        )

    t = int(input("1 | AdbController\n"
                  "2 | Win32Controller\n"
                  "Please select the controller type (ENTER to pass): "))
    if not 1 <= t <= 2:
        return None
    print("MaaToolkit search in progress...")

    if t == 1:
        device_list = Toolkit.find_adb_devices()
        if len(device_list):
            for i, d in enumerate(device_list):
                print(f"{i:>3} | {d.address:>21} | {d.name}")
            i = int(input("Please select the device (ENTER to pass): "))
            if 0 <= i < len(device_list):
                device_serial = device_list[i].address
                return AdbController(adb_path=device_list[i].adb_path,
                                    address=device_serial,
                                    screencap_methods=adb_screencap_method)
    elif t == 2:
        window_list = Toolkit.find_desktop_windows()
        if len(window_list):
            max_len = 40
            for i, w in enumerate(window_list):
                c = w.class_name[:max_len - 3] + '...' if len(w.class_name) > max_len else w.class_name
                print(f"{w.hwnd:>19} {c:>{max_len}} | {i:>3} | {w.window_name}")
            print(str.format("{:->19} {:->{}} | {:->3} | {}", " hWnd", " class name", max_len, "num", "window name"))
        i = int(input("Please select the window (ENTER to pass): "))
        if 0 <= i < len(window_list):
            device_serial = window_list[i].window_name
            return Win32Controller(hWnd=window_list[i].hwnd,
                                   screencap_method=win32_screencap_method)
    return None


controller = parse_args()
if controller:
    set_screenshot_target_side(controller)
    if controller.post_connection().failed:
        print(f"Failed to connect device({device_serial}).")

# 初始化 Roi
std_roimage: Roimage = Roimage(window_size[0], window_size[1])  # 标准化截图
win_roimage: Roimage = Roimage(0, 0, 0, 0, std_roimage)  # 相对 std_roimage ，窗口显示的区域
crop_list: list[Roimage] = []  # 相对 std_roimage ，需要裁剪的区域

# 初始化参数
win_name = "image"  # 窗口名
trackbars_name = "trackbars"  # 轨迹条窗口名
file_name = "image"  # 文件名
files = [f for f in os.listdir("./src") if f.endswith('.png')]


# -----------------------------------------------

# OpenCV 鼠标回调
# -events 鼠标事件（如按下鼠标左键，释放鼠标左键，鼠标移动等）
# -x x坐标
# -y y坐标
# -flags params 其他参数
def mouse(event, x, y, flags, param) -> None:
    global crop_end
    crop_end = Roimage(0, 0, x, y, win_roimage).getRoiInRoot()
    crop(event, x, y, flags, param)
    zoom(event, x, y, flags, param)
    move(event, x, y, flags, param)
    show_roi(crop_end)


# 显示 Roi
trackbars_img = np.ones((100, 400, 3), dtype=np.uint8) * 255


def show_roi(roi: Roimage):
    trackbars_img.fill(255)
    cv2.putText(trackbars_img, f'{roi.rectangle}', (0, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 0), 2)
    cv2.imshow(trackbars_name, trackbars_img)


# 计算绘图四边形坐标
# -rectPts 相对于 std_roimage 的两点坐标 ((left,top),(right,bottom))
def count_draw_coordinate(rect_pts):
    z = win_roimage.zoom
    x, y = win_roimage.point
    return (
        (int(rect_pts[0][0] * z - x), int(rect_pts[0][1] * z - y)),
        (int(rect_pts[1][0] * z - x), int(rect_pts[1][1] * z - y)))


# 绘图
# -rois 相对于 std_roimage 的 crop_roimage 列表
def draw(rois: list[Roimage]) -> None:
    img = win_roimage.image.copy()
    for roi in rois:
        pt1, pt2 = count_draw_coordinate(roi.rectanglePoints)
        cv2.rectangle(img, pt1, pt2, (0, 255, 0), 2)
    cv2.imshow(win_name, img)


# 左键裁剪ROI区域
crop_start: Roimage = None  # 相对 win_roimage ，正在裁剪的区域
crop_end = Roimage(0, 0, 0, 0, win_roimage).getRoiInRoot()


def crop(event, x, y, flags, param) -> None:
    global crop_start, crop_end
    if event == cv2.EVENT_LBUTTONDOWN:  # 按下左键
        # 记录（x,y）坐标
        crop_start = Roimage(0, 0, x, y, win_roimage)
    elif crop_start is not None and (event == cv2.EVENT_LBUTTONUP or (
        event == cv2.EVENT_MOUSEMOVE and (flags & cv2.EVENT_FLAG_LBUTTON))):  # 释放左键 或 按住左键拖曳
        # 修正（x,y）坐标
        w, h = win_roimage.size
        x = max(0, min(w - 1, x))
        y = max(0, min(h - 1, y))
        # 记录（x,y）坐标
        crop_end = crop_start.getCropRoi(x, y).getRoiInRoot()
        # 绘图
        rois: list[Roimage] = crop_list.copy()
        rois.append(crop_end)
        draw(rois)
        # 保存 crop Roi
        if event == cv2.EVENT_LBUTTONUP:
            crop_list.append(crop_end.copy(std_roimage))
            crop_start = None


# 计算缩放倍数
# -flag 鼠标滚轮上移或下移的标识
# -zoom 缩放倍数
# -step 缩放系数，滚轮每步缩放0.1
def count_zoom(flag, zoom: float, step: float = 0.1):
    if flag > 0:  # 滚轮上移
        zoom += step
        if zoom > 3:  # 最多只能放大到 g_image 三倍大
            zoom = 3
    else:  # 滚轮下移
        zoom -= step
        if zoom < 1:  # 最多只能缩小到 g_image 大小
            zoom = 1
    return zoom


# 滚轮放大
def zoom(event, x, y, flags, param) -> None:
    global win_roimage
    if event == cv2.EVENT_MOUSEWHEEL:  # 滚轮
        z = count_zoom(flags, win_roimage.zoom)
        wx, wy, w, h = win_roimage.rectangle
        x = int((wx + x) * z / win_roimage.zoom - x)
        y = int((wy + y) * z / win_roimage.zoom - y)
        win_roimage = Roimage(w, h, x, y, win_roimage.parent, z)
        draw(crop_list)


# 计算移动后的坐标
# -pt0 win_roimage 的原始坐标
# -pt1 鼠标按下右键时的坐标
# -pt2 鼠标当前坐标
def count_move_coordinate(pt0, pt1, pt2):
    return pt0[0] + pt1[0] - pt2[0], pt0[1] + pt1[1] - pt2[1]


# 右键拖曳
move_start = (0, 0)
move_start_roi = (0, 0)


def move(event, x, y, flags, param) -> None:
    global move_start, move_start_roi, win_roimage
    if event == cv2.EVENT_RBUTTONDOWN:
        move_start = (x, y)
        move_start_roi = win_roimage.point
    elif event == cv2.EVENT_RBUTTONUP or (event == cv2.EVENT_MOUSEMOVE and (flags & cv2.EVENT_FLAG_RBUTTON)):
        x, y = count_move_coordinate(move_start_roi, move_start, (x, y))
        win_roimage = Roimage(win_roimage.width, win_roimage.height, x, y, std_roimage, win_roimage.zoom)
        draw(crop_list)


# 轨迹条回调
# -pos 轨迹条位置
def trackbar_change(pos) -> None:
    pos = pos / 100  # get a scaling factor from trackbar pos
    w = int(std_roimage.width * pos)  # scale w
    h = int(std_roimage.height * pos)  # scale h
    cv2.resizeWindow(win_name, w, h)  # resize a window


# 读取文件
# -file 文件名
def readfile(file: str):
    print("src:", f"{os.getcwd()}\src\{file}")
    return cv2.imread("./src/" + file)


# 截图
def screenshot() -> np.ndarray:
    if not controller:
        return None
    print("Screenshot in progress...")
    controller.post_screencap().wait()
    image = controller.cached_image
    print("Screenshot completed.")
    return image


# 获取标准化的 Roimage
def get_std_roimage() -> Roimage:
    global file_name
    if len(files):
        file_name = files.pop(0)
        image = readfile(file_name)
        file_name = file_name.split(".")[0]
    else:
        image = screenshot()
        file_name = datetime.now().strftime('%H%M%S')  # '%Y%m%d%H%M%S'
    if image is None:
        return None
    height, width, _ = image.shape
    roimage = Roimage(width, height)
    roimage.image = image
    return roimage


# 获取放大后的 Roi 四边形
# -roi: 需要放大的 Roi
def get_amplified_roi_rectangle(roi: Roimage) -> list[int]:
    x, y, w, h = amplify(roi.rectangle)
    return Roimage(w, h, x, y, roi.parent).rectangle


def is_window_visible(win_name: str, destroyed: bool = False) -> bool:
    if destroyed:
        return False
    try:
        return cv2.getWindowProperty(win_name, cv2.WND_PROP_VISIBLE) > 0
    except cv2.error:
        return False


# 初始化 cv2 窗口
cv2.namedWindow(win_name, cv2.WINDOW_NORMAL)
cv2.setWindowProperty(win_name, cv2.WND_PROP_TOPMOST, 1)
cv2.setMouseCallback(win_name, mouse)

cv2.namedWindow(trackbars_name, cv2.WINDOW_NORMAL)
cv2.setWindowProperty(trackbars_name, cv2.WND_PROP_TOPMOST, 1)
cv2.createTrackbar('Scale', trackbars_name, 100, 200, trackbar_change)

cropping = False
destroyedMainColors = True
while True:
    if not cropping:
        std_roimage = get_std_roimage()
        if std_roimage is None:
            break
        win_roimage = Roimage(0, 0, 0, 0, std_roimage)
        crop_list.clear()
    draw(crop_list)

    key = cv2.waitKey(0) & 0xFF
    cropping = True
    # q Q esc
    if key in [ord("q"), ord("Q"), 27]:
        cv2.destroyAllWindows()
        exit()
    # 0
    if key == 48:
        cv2.setTrackbarPos('Scale', trackbars_name, 100)
        continue
    # 1 ~ 5
    if key in range(49, 54):
        cv2.setTrackbarPos('Scale', trackbars_name, 100 + 15 * (key - 48))
        continue
    # 6 ~ 9
    if key in range(54, 58):
        cv2.setTrackbarPos('Scale', trackbars_name, 100 - 15 * (58 - key))
        continue
    # z Z delete backspace
    if key in [ord("z"), ord("Z"), 0, 8]:
        if len(crop_list):
            crop_list.pop()
        continue

    cropping = False
    needSave = True
    needColorMatch = False
    connected = False
    mains = []
    # r R
    if key in [ord("r"), ord("R")]:
        needSave = False
    # c C
    elif key in [ord("c")]:
        needSave = False
        needColorMatch = True
        connected = False
    elif key in [ord("C")]:
        needSave = False
        needColorMatch = True
        connected = True
    # f F
    elif key in [ord("f"), ord("F")]:
        crop_list.append(Roimage(0, 0, 0, 0, std_roimage))
    # s S enter
    elif key not in [ord("s"), ord("S"), ord("\r"), ord("\n")]:
        continue

    for roi in crop_list:
        print("")
        img = roi.image

        if needSave:
            x1, y1, w1, h1 = roi.rectangle
            x2, y2, w2, h2 = get_amplified_roi_rectangle(roi)
            dst_filename: str = f'{file_name}_{x1}_{y1}_{w1}_{h1}__{x2}_{y2}_{w2}_{h2}.png'
            print(f"dst: {os.getcwd()}\dst\{dst_filename}")
            cv2.imwrite('./dst/' + dst_filename, roi.image)

        print(f"original roi: {roi.rectangle}\n"
              f"amplified roi: {get_amplified_roi_rectangle(roi)}")

        if needColorMatch:
            method, reverse, colors = match_color(img)
            ret = {"recognition": "ColorMatch", "roi": [], "method": method, "lower": [], "upper": [], "count": [],
                   "connected": connected}
            mainColors = []
            for center, lower, upper in colors:
                count = colormatcher.getCount(img, lower, upper, connected, method)
                ret["lower"].append(lower)
                ret["upper"].append(upper)
                ret["count"].append(count)
                mainColor = np.zeros((80, 200, 3), dtype=np.uint8)
                mainColor[:28, :] = lower
                mainColor[28: 58, :] = upper
                mainColor[58:, :] = center
                if reverse is not None:
                    mainColor = cv2.cvtColor(mainColor, reverse)
                cv2.putText(mainColor, f'{lower}', (0, 20), cv2.FONT_HERSHEY_SIMPLEX, .75, (0, 0, 0), 2)
                cv2.putText(mainColor, f'{upper}', (0, 50), cv2.FONT_HERSHEY_SIMPLEX, .75, (255, 255, 255), 2)
                cv2.putText(mainColor, f'{count}', (66, 76), cv2.FONT_HERSHEY_SIMPLEX, .75, (127, 127, 127), 2)
                mainColors.append(mainColor)
            mains.append(cv2.hconcat(mainColors))
            cv2.namedWindow('MainColors', cv2.WINDOW_NORMAL)
            cv2.imshow('MainColors', cv2.vconcat(mains))
            destroyedMainColors = False
            print(f"ColorMatch: {ret}"
                  .replace("'", '"')
                  .replace("False", "false")
                  .replace("True", "true"))
        elif is_window_visible('MainColors', destroyedMainColors):
            cv2.destroyWindow('MainColors')
            destroyedMainColors = True

        print("")

print("Press any key to exit...")
cv2.waitKey(0)
cv2.destroyAllWindows()
