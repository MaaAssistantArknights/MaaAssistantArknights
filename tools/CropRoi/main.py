try:
    import cv2
    import numpy as np
except ImportError:
    print("Please install OpenCV and NumPy to run this script.")
    print("You can install them using:")
    print("    pip install opencv-python numpy")
    exit(1)

from pathlib import Path
from typing import Tuple, Optional


class RoiCropper:
    """ROI裁剪工具类，支持实时预览和交互式选择"""

    def __init__(self, std_width: int = 1280, std_height: int = 720):
        self.std_width = std_width
        self.std_height = std_height
        self.std_ratio = std_width / std_height

        # ROI选择状态
        self.start_point: Optional[Tuple[int, int]] = None
        self.end_point: Optional[Tuple[int, int]] = None
        self.cropping = False
        self.current_mouse_pos: Optional[Tuple[int, int]] = None

        # 当前处理的图像
        self.original_image: Optional[np.ndarray] = None
        self.display_image: Optional[np.ndarray] = None
        self.current_size: Optional[Tuple[int, int]] = None

        # 扩展参数
        self.horizontal_expansion = 100
        self.vertical_expansion = 100

        # 颜色定义
        self.PREVIEW_COLOR = (255, 255, 0)  # 预览
        self.SELECTED_COLOR = (0, 255, 0)  # 已选择
        self.EXPANDED_COLOR = (0, 0, 255)  # 扩展区域

        self._setup_window()

    def _setup_window(self):
        """设置OpenCV窗口"""
        cv2.namedWindow("ROI Cropper", cv2.WINDOW_AUTOSIZE)
        cv2.setMouseCallback("ROI Cropper", self._mouse_callback)

    def _mouse_callback(self, event, x, y, flags, param):
        """鼠标事件回调函数"""
        if event == cv2.EVENT_LBUTTONDOWN:
            self.start_point = (x, y)
            self.end_point = None
            self.cropping = True
            self._update_display()

        elif event == cv2.EVENT_MOUSEMOVE:
            self.current_mouse_pos = (x, y)
            if self.cropping and self.start_point:
                self._update_display()

        elif event == cv2.EVENT_LBUTTONUP:
            if self.start_point:
                self.end_point = (x, y)
                self.cropping = False
                self._update_display()

    def _update_display(self):
        """更新显示图像"""
        if self.original_image is None:
            return

        display = self.original_image.copy()

        # 显示当前ROI选择
        if self.start_point:
            if self.cropping and self.current_mouse_pos:
                # 实时预览 - 黄色虚线
                end_pos = self.current_mouse_pos
                self._draw_rectangle(
                    display,
                    self.start_point,
                    end_pos,
                    self.PREVIEW_COLOR,
                    2,
                    is_dashed=True,
                )
            elif self.end_point:
                # 最终选择 - 绿色实线
                self._draw_rectangle(
                    display, self.start_point, self.end_point, self.SELECTED_COLOR, 2
                )
                # 显示扩展区域 - 红色虚线
                expanded_rect = self._calculate_expanded_rect()
                if expanded_rect:
                    self._draw_rectangle(
                        display,
                        (expanded_rect[0], expanded_rect[1]),
                        (
                            expanded_rect[0] + expanded_rect[2],
                            expanded_rect[1] + expanded_rect[3],
                        ),
                        self.EXPANDED_COLOR,
                        1,
                        is_dashed=True,
                    )

        # 添加操作提示
        #self._draw_instructions(display)

        cv2.imshow("ROI Cropper", display)

    def _draw_rectangle(self, img, pt1, pt2, color, thickness, is_dashed=False):
        """绘制矩形（支持虚线）"""
        if is_dashed:
            # 绘制虚线矩形
            self._draw_dashed_rectangle(img, pt1, pt2, color, thickness)
        else:
            cv2.rectangle(img, pt1, pt2, color, thickness)

    def _draw_dashed_rectangle(self, img, pt1, pt2, color, thickness):
        """绘制虚线矩形"""
        x1, y1 = pt1
        x2, y2 = pt2

        # 确保坐标顺序正确
        x1, x2 = min(x1, x2), max(x1, x2)
        y1, y2 = min(y1, y2), max(y1, y2)

        dash_length = 10

        # 绘制四条边的虚线
        self._draw_dashed_line(
            img, (x1, y1), (x2, y1), color, thickness, dash_length
        )  # 上边
        self._draw_dashed_line(
            img, (x2, y1), (x2, y2), color, thickness, dash_length
        )  # 右边
        self._draw_dashed_line(
            img, (x2, y2), (x1, y2), color, thickness, dash_length
        )  # 下边
        self._draw_dashed_line(
            img, (x1, y2), (x1, y1), color, thickness, dash_length
        )  # 左边

    def _draw_dashed_line(self, img, pt1, pt2, color, thickness, dash_length):
        """绘制虚线"""
        x1, y1 = pt1
        x2, y2 = pt2

        dist = int(np.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2))
        dashes = dist // (dash_length * 2)

        if dashes == 0:
            cv2.line(img, pt1, pt2, color, thickness)
            return

        for i in range(dashes):
            start_ratio = (2 * i * dash_length) / dist
            end_ratio = ((2 * i + 1) * dash_length) / dist

            start_x = int(x1 + (x2 - x1) * start_ratio)
            start_y = int(y1 + (y2 - y1) * start_ratio)
            end_x = int(x1 + (x2 - x1) * end_ratio)
            end_y = int(y1 + (y2 - y1) * end_ratio)

            cv2.line(img, (start_x, start_y), (end_x, end_y), color, thickness)

    def _draw_instructions(self, img):
        """在图像上绘制操作说明"""
        instructions = [
            "Mouse: Drag to select ROI",
            "S: Save current ROI",
            "R: Reset selection",
            "Q: Next image / Quit",
            "Blue:  Preview",
            "Green: Original ORI",
            "Red:   Expanded ROI",
        ]

        font = cv2.FONT_HERSHEY_SIMPLEX
        font_scale = 0.6
        color = (255, 255, 255)
        thickness = 1

        for i, text in enumerate(instructions):
            y = 25 + i * 25
            # 绘制黑色背景
            (text_width, text_height), _ = cv2.getTextSize(
                text, font, font_scale, thickness
            )
            cv2.rectangle(
                img, (10, y - text_height - 5), (15 + text_width, y + 5), (0, 0, 0), -1
            )
            # 绘制白色文字
            cv2.putText(img, text, (10, y), font, font_scale, color, thickness)

    def _calculate_expanded_rect(self) -> Optional[Tuple[int, int, int, int]]:
        """计算扩展后的矩形区域"""
        if not (self.start_point and self.end_point and self.current_size):
            return None

        x1, y1 = self.start_point
        x2, y2 = self.end_point

        # 确保坐标顺序正确
        left = min(x1, x2)
        right = max(x1, x2)
        top = min(y1, y2)
        bottom = max(y1, y2)

        # 计算扩展区域
        expand_x = self.horizontal_expansion // 2
        expand_y = self.vertical_expansion // 2

        final_x = max(0, left - expand_x)
        final_y = max(0, top - expand_y)
        final_w = min(
            self.current_size[0] - final_x, (right - left) + self.horizontal_expansion
        )
        final_h = min(
            self.current_size[1] - final_y, (bottom - top) + self.vertical_expansion
        )

        return (final_x, final_y, final_w, final_h)

    def resize_image(self, image: np.ndarray) -> np.ndarray:
        """调整图像尺寸以符合标准比例"""
        current_ratio = image.shape[1] / image.shape[0]

        if current_ratio >= self.std_ratio:
            # 宽屏或16:9，按高度缩放
            new_height = self.std_height
            new_width = int(current_ratio * self.std_height)
        else:
            # 偏正方形屏幕，按宽度缩放
            new_width = self.std_width
            new_height = int(self.std_width / current_ratio)

        resized = cv2.resize(
            image, (new_width, new_height), interpolation=cv2.INTER_AREA
        )
        self.current_size = (new_width, new_height)
        return resized

    def reset_selection(self):
        """重置ROI选择"""
        self.start_point = None
        self.end_point = None
        self.cropping = False
        self.current_mouse_pos = None
        self._update_display()

    def has_valid_selection(self) -> bool:
        """检查是否有有效的ROI选择"""
        return self.start_point is not None and self.end_point is not None

    def get_roi_info(
        self,
    ) -> Optional[Tuple[Tuple[int, int, int, int], Tuple[int, int, int, int]]]:
        """获取ROI信息，返回(原始ROI, 扩展ROI)"""
        if not self.has_valid_selection():
            return None

        x1, y1 = self.start_point
        x2, y2 = self.end_point

        # 原始ROI
        left = min(x1, x2)
        right = max(x1, x2)
        top = min(y1, y2)
        bottom = max(y1, y2)
        original_roi = (left, top, right - left, bottom - top)

        # 扩展ROI
        expanded_rect = self._calculate_expanded_rect()
        if expanded_rect is None:
            return None

        return original_roi, expanded_rect

    def save_roi(self, image: np.ndarray, filename: Path, dst_path: Path) -> bool:
        """保存ROI图像"""
        roi_info = self.get_roi_info()
        if not roi_info:
            print("No valid ROI selected!")
            return False

        original_roi, expanded_roi = roi_info
        left, top, width, height = original_roi

        # 裁剪ROI区域
        roi_image = image[top : top + height, left : left + width]

        # 生成文件名
        exp_x, exp_y, exp_w, exp_h = expanded_roi
        dst_filename = f"{filename.stem}_{exp_x},{exp_y},{exp_w},{exp_h}.png"
        dst_file_path = dst_path / dst_filename

        # 确保目标目录存在
        dst_path.mkdir(exist_ok=True)

        # 保存图像
        success = cv2.imwrite(str(dst_file_path), roi_image)

        if success:
            print(f"✓ Saved: {dst_filename}")
            print(f"  Original ROI: {left}, {top}, {width}, {height}")
            print(f"  Expanded ROI: {exp_x}, {exp_y}, {exp_w}, {exp_h}")
        else:
            print(f"✗ Failed to save: {dst_filename}")

        return success

    def process_image(self, image: np.ndarray):
        """处理单张图像"""
        self.original_image = self.resize_image(image)
        self.reset_selection()
        self._update_display()

    def cleanup(self):
        """清理资源"""
        cv2.destroyAllWindows()


def main():
    print("=" * 60)
    print("ROI Cropping Tool")
    print("=" * 60)
    print("Instructions:")
    print("• Put 16:9 images under ./src")
    print("• Drag mouse to select ROI (real-time preview)")
    print("• Press 'S' to save current ROI")
    print("• Press 'R' to reset selection")
    print("• Press 'Q' to go to next image or quit")
    print("• Green: Original ORI, Red: Expanded ROI")
    print("• Cropped images will be saved in ./dst")
    print("=" * 60)
    # 初始化
    cropper = RoiCropper()

    # 设置路径
    base_path = Path(__file__).parent.resolve()
    src_path = base_path / "src"
    dst_path = base_path / "dst"

    # 检查源目录
    if not src_path.exists():
        print(f"❌ Source directory not found: {src_path}")
        print("Please create the 'src' directory and put your images there.")
        return

    # 获取图像文件
    image_files = (
        list(src_path.glob("*.png"))
        + list(src_path.glob("*.jpg"))
        + list(src_path.glob("*.jpeg"))
    )

    if not image_files:
        print(f"❌ No image files found in: {src_path}")
        print("Supported formats: PNG, JPG, JPEG")
        return

    print(f"📁 Found {len(image_files)} image(s) to process")
    print()

    try:
        # 处理每张图像
        for i, filename in enumerate(image_files, 1):
            print(f"📸 Processing ({i}/{len(image_files)}): {filename.name}")

            # 读取图像
            image = cv2.imread(str(filename))
            if image is None:
                print(f"❌ Failed to load image: {filename}")
                continue

            # 处理图像
            cropper.process_image(image)

            # 等待用户操作
            while True:
                key = cv2.waitKey(1) & 0xFF

                if key == ord("s") or key == ord("S"):
                    # 保存ROI
                    if cropper.has_valid_selection():
                        if cropper.save_roi(cropper.original_image, filename, dst_path):
                            print()
                            break
                        else:
                            print("❌ Failed to save ROI!")
                    else:
                        print("⚠️  Please select a ROI first!")

                elif key == ord("r") or key == ord("R"):
                    # 重置选择
                    cropper.reset_selection()
                    print("🔄 Selection reset")

                elif key == ord("q") or key == ord("Q"):
                    # 下一张图像或退出
                    if cropper.has_valid_selection():
                        print("⚠️  ROI selected but not saved. Moving to next image...")
                    else:
                        print("⏭️  Moving to next image...")
                    print()
                    break

                elif key == 27:  # ESC
                    print("🛑 Exiting...")
                    return

    except KeyboardInterrupt:
        print("\n🛑 Interrupted by user")
    finally:
        cropper.cleanup()
        print("✅ Done!")


if __name__ == "__main__":
    main()
