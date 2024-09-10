import cv2
import numpy as np
import matplotlib.pyplot as plt


def convert_color(image, color):
    if color.lower() == 'luv':
        return cv2.cvtColor(image, cv2.COLOR_BGR2Luv)
    elif color.lower() == 'hsv':
        return cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    elif color.lower() == 'rgb':
        return cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    else:
        raise RuntimeError("Invalid param `color` in function convert_color")


def calc_mask_from_ranges(image, mask_ranges, color=None, mask_close=False):
    image_for_mask = image if color is None else convert_color(image, color)

    if mask_ranges is None:
        return None

    mask = None
    for mask_range in mask_ranges:
        l0, l1, l2 = mask_range[0]
        u0, u1, u2 = mask_range[1]
        if mask is not None:
            mask = cv2.bitwise_or(mask, cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2)))
        else:
            mask = cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2))

    if mask_close:
        kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

    return mask


def show_image_mask(image, mask, color, hist_mask=None):
    image_for_hist = convert_color(image, color)
    image_with_mask = cv2.bitwise_and(image, image, mask=mask)

    fig, axs = plt.subplots(2, 3, figsize=(15, 8))

    hist_0 = cv2.calcHist([image_for_hist], [0], hist_mask, [256], [0, 256])
    hist_1 = cv2.calcHist([image_for_hist], [1], hist_mask, [256], [0, 256])
    hist_2 = cv2.calcHist([image_for_hist], [2], hist_mask, [256], [0, 256])

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


def generate_mask_ranges(image, color, base_mask_ranges=None, thresholds=None):
    image_for_mask = convert_color(image, color)

    if thresholds is None:
        thresholds = [0.6] * 3

    base_mask = calc_mask_from_ranges(image, base_mask_ranges, color)
    mask_ranges = []
    mask = cv2.inRange(image_for_mask, (0, 0, 0), (0, 0, 0))
    for i, threshold in enumerate(thresholds):
        current_mask = cv2.bitwise_not(mask)
        if base_mask is not None:
            current_mask = cv2.bitwise_and(base_mask, current_mask)

        hist_0 = cv2.calcHist([image_for_mask], [0], current_mask, [256], [0, 256])
        hist_1 = cv2.calcHist([image_for_mask], [1], current_mask, [256], [0, 256])
        hist_2 = cv2.calcHist([image_for_mask], [2], current_mask, [256], [0, 256])

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
        mask = cv2.bitwise_or(mask, cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2)))

    print(f'Recommand {color.upper()} Mask Range: {mask_ranges}')
    show_image_mask(image, calc_mask_from_ranges(image, mask_ranges, color), color, base_mask)

    return mask_ranges


def compare_2_image_with_mask_ranges(image1, image2, mask_ranges, color, mask_close=False):
    image1_for_mask = convert_color(image1, color)
    image2_for_mask = convert_color(image2, color)

    mask1 = calc_mask_from_ranges(image1_for_mask, mask_ranges, None, mask_close)
    mask2 = calc_mask_from_ranges(image2_for_mask, mask_ranges, None, mask_close)

    image1_with_mask = cv2.bitwise_and(image1, image1, mask=mask1)
    image2_with_mask = cv2.bitwise_and(image2, image2, mask=mask2)

    fig, axs = plt.subplots(2, 3, figsize=(15, 8))

    # 原图
    axs[0, 0].imshow(cv2.cvtColor(image1, cv2.COLOR_BGR2RGB))
    axs[0, 0].set_title('Original Image')
    axs[0, 0].axis('off')

    # 掩码后的图像
    axs[0, 1].imshow(image1_for_mask)
    axs[0, 1].set_title('Image for Mask')
    axs[0, 1].axis('off')

    # 显示掩码
    axs[0, 2].imshow(mask1, cmap='gray')
    axs[0, 2].set_title('Recommand Impmortant Mask Range')
    axs[0, 2].axis('off')

    # 原图
    axs[1, 0].imshow(cv2.cvtColor(image2, cv2.COLOR_BGR2RGB))
    axs[1, 0].set_title('Original Image')
    axs[1, 0].axis('off')

    # 掩码后的图像
    axs[1, 1].imshow(image2_for_mask)
    axs[1, 1].set_title('Image for Mask')
    axs[1, 1].axis('off')

    # 显示掩码
    axs[1, 2].imshow(mask2, cmap='gray')
    axs[1, 2].set_title('Recommand Impmortant Mask Range')
    axs[1, 2].axis('off')

    plt.tight_layout()
    plt.show()
