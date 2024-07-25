import cv2
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

cur_dir = Path(__file__).parent.resolve()
maa_dir = cur_dir.parent.parent

def generate_mask_ranges(image, base_mask, thresholds):
    mask_ranges = []
    mask = cv2.inRange(image, (0, 0, 0), (0, 0, 0))
    for i, threshold in enumerate(thresholds):
        current_mask = cv2.bitwise_not(mask)
        if base_mask is not None:
            current_mask = cv2.bitwise_and(base_mask, current_mask)

        hist_0 = cv2.calcHist([image], [0], current_mask, [256], [0, 256])
        hist_1 = cv2.calcHist([image], [1], current_mask, [256], [0, 256])
        hist_2 = cv2.calcHist([image], [2], current_mask, [256], [0, 256])

        hue_threshold = threshold * hist_0.max()
        sat_threshold = threshold * hist_1.max()
        val_threshold = threshold * hist_2.max()

        try:
            l0 = int(np.where(hist_0 > hue_threshold)[0][0])
            u0 = int(np.where(hist_0 > hue_threshold)[0][-1])
            l1 = int(np.where(hist_1 > sat_threshold)[0][0])
            u1 = int(np.where(hist_1 > sat_threshold)[0][-1])
            l2 = int(np.where(hist_2 > val_threshold)[0][0])
            u2 = int(np.where(hist_2 > val_threshold)[0][-1])
        except:
            print(f"Failed at #{i}")
            break

        mask_ranges.append([[l0, l1, l2], [u0, u1, u2]])

        mask = cv2.bitwise_or(mask, cv2.inRange(image, (l0, l1, l2), (u0, u1, u2)))
    return mask_ranges, mask

def show_image_mask(image, mask, image_for_hist):
    image_with_mask = cv2.bitwise_and(image, image, mask=mask)

    fig, axs = plt.subplots(2, 3, figsize=(15, 8))

    hist_0 = cv2.calcHist([image_for_hist], [0], None, [256], [0, 256])
    hist_1 = cv2.calcHist([image_for_hist], [1], None, [256], [0, 256])
    hist_2 = cv2.calcHist([image_for_hist], [2], None, [256], [0, 256])

    axs[0, 0].plot(hist_0, color='r')
    axs[0, 0].set_title('Channel 0 Histogram')
    axs[0, 0].set_xlim([0, 256])

    axs[0, 1].plot(hist_1, color='g')
    axs[0, 1].set_title('Channel 1 Histogram')
    axs[0, 1].set_xlim([0, 256])

    axs[0, 2].plot(hist_2, color='b')
    axs[0, 2].set_title('Channel 2 Histogram')
    axs[0, 2].set_xlim([0, 256])

    # 原图
    axs[1, 0].imshow(cv2.cvtColor(image, cv2.COLOR_BGR2RGB))
    axs[1, 0].set_title('Original Image')
    axs[1, 0].axis('off')

    # 掩码后的图像
    axs[1, 1].imshow(cv2.cvtColor(image_with_mask, cv2.COLOR_BGR2RGB))
    axs[1, 1].set_title('Image with Recommand Mask Range')
    axs[1, 1].axis('off')

    # 显示掩码
    axs[1, 2].imshow(mask, cmap='gray')
    axs[1, 2].set_title('Recommand Impmortant Mask Range')
    axs[1, 2].axis('off')

    plt.tight_layout()
    plt.show()

def calc_mask_from_ranges(image, mask_ranges):
    mask = None
    for mask_range in mask_ranges:
        l0, l1, l2 = mask_range[0]
        u0, u1, u2 = mask_range[1]
        if mask is not None:
            mask = cv2.bitwise_or(mask, cv2.inRange(image, (l0, l1, l2), (u0, u1, u2)))
        else:
            mask = cv2.inRange(image, (l0, l1, l2), (u0, u1, u2))
    return mask

if __name__ == '__main__':
    image = cv2.imread(maa_dir / "resource" / "template" / "Sarkaz@Roguelike@StageFilterTruthEnter.png")
    hsv_image = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    show_image_mask(image, calc_mask_from_ranges(hsv_image, [[[23, 150, 40], [23, 230, 150]]]), hsv_image)

    # 自定义参数
    hsv_base_mask = None
    # hsv_base_mask = cv2.inRange(hsv_image, (0, 0, 0), (180, 254, 230)) # 忽略亮色背景
    hsv_base_mask = cv2.inRange(hsv_image, (0, 1, 40), (180, 255, 255)) # 忽略暗色背景
    hsv_thresholds = [0.6] * 3
    rgb_base_mask = None
    # rgb_base_mask = cv2.inRange(rgb_image, (0, 0, 0), (200, 200, 200)) # 忽略亮色背景
    rgb_base_mask = cv2.inRange(rgb_image, (20, 20, 20), (255, 255, 255)) # 忽略暗色背景
    rgb_thresholds = [0.6] * 3

    hsv_mask_ranges, hsv_mask = generate_mask_ranges(hsv_image, hsv_base_mask, hsv_thresholds)
    print(f'Recommand HSV Mask Range: {hsv_mask_ranges}')
    show_image_mask(image, hsv_mask, hsv_image)
    rgb_mask_ranges, rgb_mask = generate_mask_ranges(rgb_image, rgb_base_mask, rgb_thresholds)
    print(f'Recommand RGB Mask Range: {rgb_mask_ranges}')
    show_image_mask(image, rgb_mask, rgb_image)
