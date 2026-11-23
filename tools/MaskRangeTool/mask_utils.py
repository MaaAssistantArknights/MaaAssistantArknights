"""Mask range helpers for MaaCore template matching.

MaaCore uses two different mask flavours:

* ``maskRange`` (task JSON, ``array<int, 2>``): the template is converted to
  grayscale and ``cv::inRange(lower, upper)`` keeps the pixels in range.
* ``colorScales`` (``HSVCount`` / ``RGBCount``): three-channel ranges in RGB or
  HSV, applied to the template (and to the image in count mode).

Both are the union of all ranges, optionally closed with a 3x3 kernel.  MaaCore
masks the *template* unless ``mask_src`` is set; the previews here apply the
mask to the image instead, so their output is only a visual approximation.
"""

import cv2
import matplotlib.pyplot as plt
import numpy as np

GRAY_NAMES = ("gray", "grey")


def convert_color(image, color):
    if color.lower() == "luv":
        return cv2.cvtColor(image, cv2.COLOR_BGR2Luv)
    elif color.lower() == "hsv":
        return cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    elif color.lower() == "rgb":
        return cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    else:
        raise RuntimeError("Invalid param `color` in function convert_color")


def is_gray_range(mask_range):
    """Gray range is ``[lower, upper]``, color range is ``[[l0, l1, l2], [u0, u1, u2]]``."""
    return not isinstance(mask_range[0], (list, tuple, np.ndarray))


def to_gray(image):
    if image.ndim == 2:
        return image
    return cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)


def calc_mask_from_ranges(image, mask_ranges, color="gray", mask_close=False):
    """Build the union mask for MaaCore template matching.

    ``maskRange`` is a single ``[lower, upper]`` pair over the grayscale
    template, exactly like ``Hasher`` and ``Matcher``: grayscale conversion
    first, then ``cv2.inRange``.  Gray is therefore the default: pass the pairs
    directly (or as one ``[1, 255]``).  Three-element ranges select the
    ``colorScales`` path and require ``color`` to name the space (RGB / HSV /
    LUV).
    """
    if mask_ranges is None:
        return None

    # 允许直接粘贴任务里的 "maskRange": [1, 255]，与 [ [1, 255] ] 等价
    if len(mask_ranges) and not isinstance(mask_ranges[0], (list, tuple, np.ndarray)):
        mask_ranges = [mask_ranges]

    gray_ranges = [range_ for range_ in mask_ranges if is_gray_range(range_)]
    color_ranges = [range_ for range_ in mask_ranges if not is_gray_range(range_)]
    if gray_ranges and color_ranges:
        raise RuntimeError("Cannot mix gray and color mask ranges in one call")

    if gray_ranges:
        image_for_mask = to_gray(image)
        mask = np.zeros(image_for_mask.shape[:2], np.uint8)
        for lower, upper in gray_ranges:
            current_mask = cv2.inRange(image_for_mask, int(lower), int(upper))
            mask = cv2.bitwise_or(mask, current_mask)
    else:
        if color is None or color.lower() in GRAY_NAMES:
            raise RuntimeError(
                "Three-element color ranges need color='rgb' / 'hsv' / 'luv'"
            )
        image_for_mask = convert_color(image, color)
        mask = None
        for mask_range in color_ranges:
            l0, l1, l2 = mask_range[0]
            u0, u1, u2 = mask_range[1]
            if mask is not None:
                mask = cv2.bitwise_or(
                    mask, cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2))
                )
            else:
                mask = cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2))

    if mask_close and mask is not None:
        kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)

    return mask


def show_image_mask(image, mask, color="gray", hist_mask=None, save=None, show=True):
    is_gray = color.lower() in GRAY_NAMES
    image_for_hist = to_gray(image) if is_gray else convert_color(image, color)
    image_with_mask = cv2.bitwise_and(image, image, mask=mask)

    if is_gray:
        fig, axs = plt.subplots(1, 4, figsize=(18, 4))
        hist = cv2.calcHist([image_for_hist], [0], hist_mask, [256], [0, 256])
        axs[0].plot(hist, color="k")
        axs[0].set_title("Grayscale Histogram")
        axs[0].set_xlim([0, 255])
        panels = [
            (cv2.cvtColor(image, cv2.COLOR_BGR2RGB), "Original Image", None),
            (cv2.cvtColor(image_with_mask, cv2.COLOR_BGR2RGB), "Image with Mask", None),
            (mask, "Mask", "gray"),
        ]
        for ax, (panel_image, title, cmap) in zip(axs[1:], panels):
            ax.imshow(panel_image, cmap=cmap)
            ax.set_title(title)
            ax.axis("off")
    else:
        fig, axs = plt.subplots(2, 3, figsize=(15, 8))

        # channel ranges
        CHANNEL_RANGES = {
            "luv": [(0, 255), (0, 255), (0, 255)],
            "hsv": [(0, 179), (0, 255), (0, 255)],
            "rgb": [(0, 255), (0, 255), (0, 255)],
        }

        channel_ranges = CHANNEL_RANGES[color.lower()]

        hist_0 = cv2.calcHist(
            [image_for_hist],
            [0],
            hist_mask,
            [channel_ranges[0][1] - channel_ranges[0][0]],
            [channel_ranges[0][0], channel_ranges[0][1] + 1],
        )
        hist_1 = cv2.calcHist(
            [image_for_hist],
            [1],
            hist_mask,
            [channel_ranges[1][1] - channel_ranges[1][0]],
            [channel_ranges[1][0], channel_ranges[1][1] + 1],
        )
        hist_2 = cv2.calcHist(
            [image_for_hist],
            [2],
            hist_mask,
            [channel_ranges[2][1] - channel_ranges[2][0]],
            [channel_ranges[2][0], channel_ranges[2][1] + 1],
        )

        axs[0, 0].plot(hist_0, color="r")
        axs[0, 0].set_title("Channel 0 Histogram")
        axs[0, 0].set_xlim([channel_ranges[0][0], channel_ranges[0][1]])

        axs[0, 1].plot(hist_1, color="g")
        axs[0, 1].set_title("Channel 1 Histogram")
        axs[0, 1].set_xlim([channel_ranges[1][0], channel_ranges[1][1]])

        axs[0, 2].plot(hist_2, color="b")
        axs[0, 2].set_title("Channel 2 Histogram")
        axs[0, 2].set_xlim([channel_ranges[2][0], channel_ranges[2][1]])

        # 原图
        axs[1, 0].imshow(cv2.cvtColor(image, cv2.COLOR_BGR2RGB))
        axs[1, 0].set_title("Original Image")
        axs[1, 0].axis("off")

        # 掩模后的图像
        axs[1, 1].imshow(cv2.cvtColor(image_with_mask, cv2.COLOR_BGR2RGB))
        axs[1, 1].set_title("Image with Mask")
        axs[1, 1].axis("off")

        # 显示掩模
        axs[1, 2].imshow(mask, cmap="gray")
        axs[1, 2].set_title("Mask")
        axs[1, 2].axis("off")

    plt.tight_layout()
    # save 优先：保存预览图供无头调用（脚本/agent）读图复核，不弹窗
    if save:
        fig.savefig(save, bbox_inches="tight", dpi=120)
        plt.close(fig)
    elif show:
        plt.show()
    else:
        plt.close(fig)


def generate_mask_ranges(
    image, color="gray", base_mask_ranges=None, thresholds=None, show=True, save=None
):
    thresholds = thresholds or [0.6] * 3
    is_gray = color.lower() in GRAY_NAMES

    if is_gray:
        image_for_mask = to_gray(image)
        base_mask = (
            calc_mask_from_ranges(image, base_mask_ranges, color)
            if base_mask_ranges
            else None
        )
        usable_mask = (
            np.full(image_for_mask.shape[:2], 255, np.uint8)
            if base_mask is None
            else base_mask
        )
        hist_0 = cv2.calcHist([image_for_mask], [0], usable_mask, [256], [0, 256])
        threshold = thresholds[0] * hist_0.max()
        indices = np.where(hist_0 > threshold)[0]
        if indices.size == 0:
            print("Failed at #0")
            return []

        mask_ranges = [[int(indices[0]), int(indices[-1])]]
        print(f"Recommend {color.upper()} Mask Range: {mask_ranges}")
        if show or save:
            show_image_mask(
                image,
                calc_mask_from_ranges(image, mask_ranges, color),
                color,
                base_mask,
                save,
            )
        return mask_ranges

    image_for_mask = convert_color(image, color)

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
        mask = cv2.bitwise_or(
            mask, cv2.inRange(image_for_mask, (l0, l1, l2), (u0, u1, u2))
        )

    print(f"Recommend {color.upper()} Mask Range: {mask_ranges}")
    # 空结果（阈值全部失败）时无可预览内容，跳过 show/save 避免 imshow 收到 None 崩溃
    if mask_ranges and (show or save):
        show_image_mask(
            image,
            calc_mask_from_ranges(image, mask_ranges, color),
            color,
            base_mask,
            save,
        )

    return mask_ranges


def compare_2_image_with_mask_ranges(
    image1, image2, mask_ranges, color="gray", mask_close=False, show=True, save=None
):
    mask1 = calc_mask_from_ranges(image1, mask_ranges, color, mask_close)
    mask2 = calc_mask_from_ranges(image2, mask_ranges, color, mask_close)
    if mask1 is None or mask2 is None:
        raise RuntimeError("mask_ranges produced an empty mask, nothing to compare")

    image1_with_mask = cv2.bitwise_and(image1, image1, mask=mask1)
    image2_with_mask = cv2.bitwise_and(image2, image2, mask=mask2)

    fig, axs = plt.subplots(2, 3, figsize=(15, 8))

    # 原图
    axs[0, 0].imshow(cv2.cvtColor(image1, cv2.COLOR_BGR2RGB))
    axs[0, 0].set_title("Original Image")
    axs[0, 0].axis("off")

    # 掩模后的图像
    axs[0, 1].imshow(cv2.cvtColor(image1_with_mask, cv2.COLOR_BGR2RGB))
    axs[0, 1].set_title("Image with Mask")
    axs[0, 1].axis("off")

    # 显示掩模
    axs[0, 2].imshow(mask1, cmap="gray")
    axs[0, 2].set_title("Mask")
    axs[0, 2].axis("off")

    # 原图
    axs[1, 0].imshow(cv2.cvtColor(image2, cv2.COLOR_BGR2RGB))
    axs[1, 0].set_title("Original Image")
    axs[1, 0].axis("off")

    # 掩模后的图像
    axs[1, 1].imshow(cv2.cvtColor(image2_with_mask, cv2.COLOR_BGR2RGB))
    axs[1, 1].set_title("Image with Mask")
    axs[1, 1].axis("off")

    # 显示掩模
    axs[1, 2].imshow(mask2, cmap="gray")
    axs[1, 2].set_title("Mask")
    axs[1, 2].axis("off")

    plt.tight_layout()
    if save:
        fig.savefig(save, bbox_inches="tight", dpi=120)
        plt.close(fig)
    elif show:
        plt.show()
    else:
        plt.close(fig)
