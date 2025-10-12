from __future__ import annotations

# 描述符基类,用于类型检查
class Typed(object):

    def __init__(self, name, expected_type):
        self.name = name
        self.expected_type = expected_type

    def __get__(self, instance, cls):
        if instance is None:
            return self
        else:
            return instance.__dict__[self.name]

    def __set__(self, instance, value):
        if not isinstance(value, self.expected_type):
            raise TypeError("Expected " + str(self.expected_type))
        instance.__dict__[self.name] = value

    def __delete__(self, instance):
        del instance.__dict__[self.name]

#类装饰器，用来装饰自定义的类, 且将类属性设置为某特定类型的Typed实例
def typeassert(**kwargs):
    def decorate(cls):
        for name, expected_type in kwargs.items():
            # 将名字name和期望类型创建一个Typed实例
            iTyped = Typed(name, expected_type)
            # 将名字name和iType，设置到cls中，作为对应cls.__dict__中的key，value
            setattr(cls, name, iTyped)
        return cls
    return decorate

@typeassert(width=(float, int), height=(float, int), x=(float, int), y=(float, int), zoom=(float, int))
class Roi(object):
    def __init__(self, width: float, height: float, x: float = 0 , y: float = 0, parent: Roi = None, zoom: float = 1) -> None:
        '''感兴趣区域

        相对于父 Roi，由宽、高、左上角顶点坐标所描述的一个感兴趣区域

        Args:
            width: float
                Roi 的长度，当值 <= 0 且不为根 Roi 时，自动计算为 parent.width - x
            height: float
                Roi 的高度，当值 <= 0 且不为根 Roi 时，自动计算为 parent.height - y
            x: float = 0
                Roi 左上角顶点的 x 坐标，当值小于 0 时，置为0
            y: float = 0
                Roi 左上角顶点的 y 坐标，当值小于 0 时，置为0
            parent: Roi = None
                Roi 的父 Roi
            zoom: float = 1
                缩放倍数（标记宽、高、左上角顶点坐标相对父 Roi 的倍数）
                使用 getZoomRoi 获得经过缩放后的 Roi

        Returns:
            Roi
        '''
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.zoom = zoom # 加了缩放系数之后，就开始烧脑了（
        self.__parent = parent
        self.__check()

    def __check(self) -> None:
        if self.isRoot:
            assert self.x == 0, self.x
            assert self.y == 0, self.y
            assert self.width > 0, self.width
            assert self.height > 0, self.height
            assert self.zoom == 1, self.zoom
            return
        self.x = max(0, min(self.x, self.parent.width * self.zoom - 1))
        self.y = max(0, min(self.y, self.parent.height * self.zoom - 1))
        if self.width <= 0:
            self.width = self.parent.width * self.zoom - self.x
            assert self.width > 0, self.width
        if self.x + self.width > self.parent.width * self.zoom:
            self.width = min(self.width, self.parent.width * self.zoom)
            self.x = self.parent.width * self.zoom - self.width
            assert self.x >= 0, self.x
        if self.height <= 0:
            self.height = self.parent.height * self.zoom - self.y
            assert self.height > 0, self.height
        if self.y + self.height > self.parent.height * self.zoom:
            self.height = min(self.height, self.parent.height * self.zoom)
            self.y = self.parent.height * self.zoom - self.height
            assert self.y >= 0, self.y

    @property
    def parent(self) -> Roi:
        '''Roi 父节点'''
        return self.__parent

    @property
    def isRoot(self) -> bool:
        '''是否是根节点'''
        return self.parent is None

    @property
    def point(self) -> tuple[int, int]:
        '''Roi 左上角顶点坐标 (x, y)'''
        return (int(self.x), int(self.y))

    @property
    def size(self) -> tuple[int, int]:
        '''Roi 大小 (width, height)'''
        return (int(self.width), int(self.height))

    @property
    def rectangle(self) -> list[int]:
        '''Roi 区域 [ x, y, width, height ]，由左上角顶点 (x, y) 和宽高描述'''
        return [int(self.x), int(self.y), int(self.width), int(self.height)]

    @property
    def rectanglePoints(self) -> tuple[tuple[int, int], tuple[int, int]]:
        '''Roi 区域 ( x1, y1, x2, y2 )，由左上角顶点 (x1, y1) 和右下角顶点 (x2, y2) 描述'''
        return ((int(self.x), int(self.y)), (int(self.x + self.width - 1), int(self.y + self.height - 1)))

    def getZoomRoi(self, zoom: float) -> Roi:
        '''
        获取相对于 parent，基于当前Roi，从 zoom 放大的一个新 Roi

        如果 self 为 root，则相对于 root

        Args:
            zoom: float
                缩放倍数

        Returns:
            Roi
        '''
        parent = self if self.isRoot else self.parent
        return self.__class__(self.width * zoom, self.height * zoom, self.x  * zoom, self.y * zoom, parent, zoom)

    def getCropRoi(self, x: float, y: float) -> Roi:
        '''
        获取相对于 parent，基于当前Roi，从坐标计算的一个新 Roi

        如果 self 为 root，则相对于 root

        Args:
            x: float
                x 轴坐标
            y: float
                y 轴坐标

        Returns:
            Roi
        '''
        parent = self if self.isRoot else self.parent
        if x > self.x:
            width = 1 + x - self.x
            x = self.x
        else:
            width = 1 - x + self.x
        if y > self.y:
            height = 1 + y - self.y
            y = self.y
        else:
            height = 1 - y + self.y
        return self.__class__(width, height, x, y, parent, self.zoom)

    def getRoiFromParent(self) -> Roi:
        '''
        获取相对于 parent.parent，基于当前Roi，从父 Roi 坐标计算的一个新 Roi

        若 self 为 root，则返回 self

        Returns:
            Roi
        '''
        if self.isRoot:
            return self
        if self.parent.isRoot and self.zoom == 1.0:
            return self
        x, y = self.parent.point
        p = self.parent if self.parent.isRoot else self.parent.parent
        return self.__class__(self.width / self.zoom, self.height / self.zoom, x + self.x / self.zoom, y + self.y / self.zoom, p, self.parent.zoom)

    def getRoiInRoot(self) -> Roi:
        '''
        获取相对于 root，基于 self，从 root 坐标计算的一个新 Roi

        若 self 为 root，则返回 self

        Returns:
            Roi
        '''
        roi = self.getRoiFromParent()
        return self if self is roi else roi.getRoiInRoot()
    
    def copy(self, parent: Roi = None, zoom: float = None) -> Roi:
        '''
        深复制 Roi

        Args:
            parent: Roi = None
                父节点
            zoom: float = None
                缩放倍数

        Returns:
            Roi
        '''
        parent = self.parent if parent is None else parent
        zoom = self.zoom if zoom is None else zoom
        return self.__class__(self.width, self.height, self.x, self.y, parent, zoom)
