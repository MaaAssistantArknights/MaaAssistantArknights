from __future__ import annotations

from numpy import ndarray
from roi import Roi
import cv2

class Roimage(Roi):
    def __init__(self, width: float, height: float, x: float = 0 , y: float = 0, parent: Roimage = None, zoom: float = 1) -> None:
        Roi.__init__(self, width, height, x, y, parent, zoom)
        self.__image: ndarray = None
        self.zoom_image_cache = {}

    @property
    def image(self):
        '''
        获取当前 Roi 的图片，如果没有则从父 Roi 裁剪

        或设置当前 Roi 的图片
        '''
        if self.__image is not None:
            return self.__image

        if self.zoom == 1:
            img = self.parent.image
        else:
            parent_img: ndarray = self.parent.image
            cache = self.parent.zoom_image_cache
            # 缓存过期
            if (parent_img != cache.get(self.parent.size)).any():
                cache.clear()
                cache[self.parent.size] = parent_img.copy()
            # 命中缓存
            size = (int(self.parent.width * self.zoom), int(self.parent.height * self.zoom))
            img = cache.get(size)
            # 没命中
            if img is None:
                img = cache.setdefault(
                    size,
                    cv2.resize(parent_img, size, interpolation=cv2.INTER_AREA))
        return img[
                int(self.y): int(self.y + self.height),
                int(self.x): int(self.x + self.width)
            ]

    @image.setter
    def image(self, value):
        self.__image = value
