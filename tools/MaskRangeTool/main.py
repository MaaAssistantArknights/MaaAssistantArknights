import cv2
from utils import generate_mask_ranges, show_image_mask, calc_mask_from_ranges, compare_2_image_with_mask_ranges
from pathlib import Path

cur_dir = Path(__file__).parent.resolve()
maa_dir = cur_dir.parent.parent


if __name__ == '__main__':
    luv_base_mask_range_ignore_light = [[[0, 0, 0], [230, 255, 255]]] # 忽略亮色背景
    luv_base_mask_range_ignore_dark = [[[20, 0, 0], [255, 255, 255]]] # 忽略暗色背景
    luv_base_mask_range_ignore_light_dark = [[[20, 0, 0], [230, 255, 255]]] # 忽略亮色暗色背景
    hsv_base_mask_range_ignore_light = [[[0, 0, 0], [180, 254, 230]]] # 忽略亮色背景
    hsv_base_mask_range_ignore_dark = [[[0, 1, 40], [180, 255, 255]]] # 忽略暗色背景
    rgb_base_mask_range_ignore_light = [[[0, 0, 0], [200, 200, 200]]] # 忽略亮色背景
    rgb_base_mask_range_ignore_dark = [[[20, 20, 20], [255, 255, 255]]] # 忽略暗色背景

    # 自动判断图片的合适 mask_ranges
    # image = cv2.imread(str(maa_dir / "resource" / "template" / "BattleQuickFormationRole-Pioneer.png"))
    # generate_mask_ranges(image, 'luv', luv_base_mask_range_ignore_light_dark)
    # generate_mask_ranges(image, 'hsv', hsv_base_mask_range_ignore_dark)
    # generate_mask_ranges(image, 'rgb', rgb_base_mask_range_ignore_dark)

    # 在给定的 mask_ranges 下展示一张图
    image = cv2.imread(str(maa_dir / "resource" / "template" / "BattleQuickFormationRole-Pioneer.png"))
    mask_ranges = [[[90, 90, 90], [150, 150, 150]]]
    show_image_mask(image, calc_mask_from_ranges(image, mask_ranges, 'rgb', True), 'rgb')
    # mask_ranges = [[[93, 81, 130], [102, 97, 142]], [[128, 100, 164], [134, 110, 169]], [[95, 85, 145], [105, 95, 155]]]
    # show_image_mask(image, calc_mask_from_ranges(image, mask_ranges, 'luv', True), 'luv')

    # 在给定的 mask_ranges 下比较两张图
    image1 = cv2.imread(str(maa_dir / "resource" / "template" / "BattleQuickFormationRole-Pioneer.png"))
    image2 = cv2.imread(str(maa_dir / "resource" / "template" / "BattleQuickFormationRole-Tank.png"))
    mask_ranges = [[[90, 90, 90], [150, 150, 150]]]
    compare_2_image_with_mask_ranges(image1, image2, mask_ranges, "rgb")
    compare_2_image_with_mask_ranges(image1, image2, mask_ranges, "rgb", True)
    # mask_ranges = [[[93, 81, 125], [102, 97, 150]], [[95, 85, 145], [105, 95, 155]]]
    # compare_2_image_with_mask_ranges(image1, image2, mask_ranges, "luv")
    # compare_2_image_with_mask_ranges(image1, image2, mask_ranges, "luv", True)
