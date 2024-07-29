import cv2
from utils import generate_mask_ranges, show_image_mask, calc_mask_from_ranges, compare_2_image_with_mask_ranges
from pathlib import Path

cur_dir = Path(__file__).parent.resolve()
maa_dir = cur_dir.parent.parent


if __name__ == '__main__':
    hsv_base_mask_range_ignore_light = [[[0, 0, 0], [180, 254, 230]]] # 忽略亮色背景
    hsv_base_mask_range_ignore_dark = [[[0, 1, 40], [180, 255, 255]]] # 忽略暗色背景
    rgb_base_mask_range_ignore_light = [[[0, 0, 0], [200, 200, 200]]] # 忽略亮色背景
    rgb_base_mask_range_ignore_dark = [[[20, 20, 20], [255, 255, 255]]] # 忽略暗色背景

    # 自动判断图片的合适 mask_ranges
    image = cv2.imread(maa_dir / "resource" / "template" / "Sarkaz@Roguelike@StageEmergencyTransportation.png")
    generate_mask_ranges(image, 'hsv', hsv_base_mask_range_ignore_dark)
    generate_mask_ranges(image, 'rgb', rgb_base_mask_range_ignore_dark)

    # 在给定的 mask_ranges 下展示一张图
    image = cv2.imread(maa_dir / "resource" / "template" / "Sarkaz@Roguelike@StageEmergencyTransportation.png")
    mask_ranges = [[[78, 60, 80], [93, 140, 120]]]
    show_image_mask(image, calc_mask_from_ranges(image, mask_ranges, 'hsv'), 'hsv')

    # 在给定的 mask_ranges 下比较两张图
    image1 = cv2.imread(maa_dir / "resource" / "template" / "Sarkaz@Roguelike@StageEmergencyTransportation.png")
    image2 = cv2.imread(maa_dir / "resource" / "template" / "Sarkaz@Roguelike@StageConfrontation.png")
    mask_ranges = [[[78, 60, 80], [93, 140, 120]], [[18, 90, 130], [23, 130, 150]], [[14, 130, 200], [16, 150, 220]]]
    compare_2_image_with_mask_ranges(image1, image2, mask_ranges, "hsv")
