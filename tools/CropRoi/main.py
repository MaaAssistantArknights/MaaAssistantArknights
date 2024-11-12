import cv2
from pathlib import Path


print("Usage:\n"
      "Put the 16:9 images under ./src, and run this script, it will be auto converted to 720p.\n"
      "Drag mouse to select ROI, press 'S' to save, press 'Q' to quit.\n"
      "The cropped images will be saved in ./dst\n")


# 初始化参考点列表和布尔值标志：是否正在执行裁剪
refPt = []
cropping = False


# 点击并裁剪ROI区域
# -events 鼠标事件（如按下鼠标左键，释放鼠标左键，鼠标移动等）
# -x x坐标
# -y y坐标
# -flages params 其他参数
def click_and_crop(event, x, y, flags, param):
    # 获取全局变量的引用
    global refPt, cropping

    # 如果鼠标左被单击，记录（x,y）坐标并显示裁剪正在进行
    if event == cv2.EVENT_LBUTTONDOWN:
        refPt = [(x, y)]
        cropping = True
    # 检测鼠标左键是否释放
    elif event == cv2.EVENT_LBUTTONUP:
        # 记录结束（x,y）坐标，并显示裁剪结束
        refPt.append((x, y))
        cropping = False

        draw = image.copy()
        cv2.rectangle(draw, refPt[0], refPt[1], (0, 255, 0), 2)
        cv2.imshow("image", draw)


std_width: int = 1280
std_height: int = 720
std_ratio = std_width / std_height

cv2.namedWindow("image")
cv2.setMouseCallback("image", click_and_crop)

path = Path(__file__).parent.resolve()
src_path = Path(f"{path}/src")
dst_path = Path(f"{path}/dst")

for filename in src_path.glob("*.png"):
    print("src:", filename)
    image = cv2.imread(str(filename))

    cur_ratio = image.shape[1] / image.shape[0]

    if cur_ratio >= std_ratio:  # 说明是宽屏或默认16:9，按照高度计算缩放
        dsize_width: int = (int)(cur_ratio * std_height)
        dsize_height: int = std_height
    else:                       # 否则可能是偏正方形的屏幕，按宽度计算
        dsize_width: int = std_width
        dsize_height: int = std_width / cur_ratio

    dsize = (dsize_width, dsize_height)
    image = cv2.resize(image, dsize, interpolation=cv2.INTER_AREA)

    while True:
        cv2.imshow("image", image)
        key = cv2.waitKey(0) & 0xFF
        if key == ord("s"):
            break
        elif key == ord("q"):
            exit()

    # 如果参考点列表里有俩个点，则裁剪区域并展示
    if len(refPt) == 2:
        if refPt[0][0] > refPt[1][0] or refPt[0][1] > refPt[1][1]:
            refPt[0], refPt[1] = refPt[1], refPt[0]
        left = refPt[0][0]
        right = refPt[1][0]
        top = refPt[0][1]
        bottom = refPt[1][1]
        if left > right:
            left, right = right, left
        if top > bottom:
            top, bottom = bottom, top

        roi = image[top:bottom, left:right]

        horizontal_expansion = 100
        vertical_expansion = 100

        filename_x: int = (int)(left - horizontal_expansion / 2)
        if filename_x < 0:
            filename_x = 0
        filename_y: int = (int)(top - vertical_expansion / 2)
        if filename_y < 0:
            filename_y = 0
        filename_w: int = (right - left) + horizontal_expansion
        if filename_x + filename_w > dsize_width:
            filename_w = dsize_width - filename_x
        filename_h: int = (bottom - top) + vertical_expansion
        if filename_y + filename_h > dsize_height:
            filename_h = dsize_height - filename_y

        dst_filename: str = f'{filename.stem}_{filename_x},{filename_y},{filename_w},{filename_h}.png'
        print('dst:', dst_filename)

        print(f"original roi: {left}, {top}, {right - left}, {bottom - top}, \n"
              f"amplified roi: {filename_x}, {filename_y}, {filename_w}, {filename_h}\n\n")
        cv2.imwrite(str(dst_path / dst_filename), roi)

    refPt = []
    cropping = False

# 关闭所有打开的窗口
cv2.destroyAllWindows()
