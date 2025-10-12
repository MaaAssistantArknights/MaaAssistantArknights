import cv2
import numpy as np

def kmeansClusterColors(img, method: int = -1, K: int = 3, criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.2)) -> list:
    '''
    将图片颜色类聚
    
    Args:
        img:
            图片
        method: int = -1
            颜色匹配方式。即 cv::ColorConversionCodes。可选，默认 4 (RGB)。
            常用值：4 (RGB, 3 通道), 40 (HSV, 3 通道), 6 (GRAY, 1 通道)。
            默认不进行转换。
        K: int = 3
            颜色聚类个数
        criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 100, 0.2)
            K-means 参数，迭代停止的模式选择
    Return:
        [( center color, array([color, ...]), ... )]
    '''
    if method >= 0:
        img = cv2.cvtColor(img, method)
    # 将图像数据转换为一维数组，保留颜色通道
    pixels = img.reshape((-1, img.shape[-1]))
    # 聚类
    _, labels, centers = cv2.kmeans(pixels.astype(np.float32), K, None, criteria, 10, cv2.KMEANS_RANDOM_CENTERS)
    # 将颜色值转换为原来的颜色通道类型
    centers = centers.astype(img.dtype)
    # __show(centers[labels.reshape(img.shape[:-1])])
    # 返回结果
    ret = []
    for i, center in enumerate(centers):
        colors = pixels[(labels == i).all(-1)]
        ret.append((center, colors))
    return ret

def getCount(img, lower, upper, connected: bool, method: int = -1) -> int:
    '''
    获取匹配成功的数量
    
    Args:
        img:
            图片
        lower:
            颜色下限值
        upper:
            颜色上限值
        connected: bool
            是否是相连的点才会被计数
        method: int = -1
            颜色匹配方式。即 cv::ColorConversionCodes。可选，默认 4 (RGB)。
            常用值：4 (RGB, 3 通道), 40 (HSV, 3 通道), 6 (GRAY, 1 通道)。
            默认不进行转换。
    Return:
        int
    '''
    # https://github.com/MaaXYZ/MaaFramework/blob/main/source/MaaFramework/Vision/ColorMatcher.cpp
    # ColorMatcher::color_match
    if method >= 0:
        img = cv2.cvtColor(img, method)
    bin = cv2.inRange(img, np.array(lower, img.dtype), np.array(upper, img.dtype))
    # __show(img)
    # __show(bin)
    if connected:
        return __count_non_zero_with_connected(bin)
    else:
        return cv2.countNonZero(bin)

def __count_non_zero_with_connected(bin):
    number, labels, stats, centroids = cv2.connectedComponentsWithStats(bin, connectivity=8, ltype=cv2.CV_16U)
    count = 0
    for i in range(1, number):
        x = stats[i][cv2.CC_STAT_LEFT]
        y = stats[i][cv2.CC_STAT_TOP]
        width = stats[i][cv2.CC_STAT_WIDTH]
        height = stats[i][cv2.CC_STAT_HEIGHT]
        count = max(count, cv2.countNonZero(bin[
                int(y): int(y + height),
                int(x): int(x + width)
            ]))
    return count

def showClusterColors(cluster_colors):
    '''debug用'''
    for center, colors in cluster_colors:
        # 创建一个用于显示颜色的图像
        img = np.zeros((100, len(colors), 3), dtype=np.uint8)
        # 填充颜色
        img[:50, :] = center
        for i, color in enumerate(colors):
            img[50:, i] = color
        # 显示图像
        __show(img)

def __show(img):
    '''debug用'''
    cv2.imshow('debug show', img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def __getBoxPlotValues(img, threshold: float = 1.5):
    '''return: minimum, lower quartile, median, upper quartile, maximum'''
    channel = img.shape[-1]
    colors = img.reshape((-1, channel))
    ret = ([], [], [], [], [])
    for i in range(channel):
        ccs = colors[:, i]
        # 计算各项数据
        q1 = np.percentile(ccs, 25)
        q2 = np.percentile(ccs, 50)
        q3 = np.percentile(ccs, 75)
        # 计算上下边界
        iqr = q3 - q1
        lower_bound = q1 - iqr * threshold
        upper_bound = q3 + iqr * threshold
        # 排除异常值
        q0 = max(ccs.min(), lower_bound)
        q4 = min(ccs.max(), upper_bound)
        # 返回结果
        ret[0].append(q0.astype(img.dtype))
        ret[1].append(q1.astype(img.dtype))
        ret[2].append(q2.astype(img.dtype))
        ret[3].append(q3.astype(img.dtype))
        ret[4].append(q4.astype(img.dtype))
    return ret

# 简易方法
def Simple(cluster_colors) -> list[tuple[list[int]]]:
    '''
    基于四分位数的一种简易匹配方法

    Args:
        cluster_colors
            方法 clusterColors() 返回的结果

    Return:
        [(center, lower, upper), ...]
    '''
    ret = []
    for center, colors in cluster_colors:
        _, lower, _, upper, _ = __getBoxPlotValues(colors)
        ret.append((list(center), list(lower), list(upper)))
    return ret

def RGBDistance(cluster_colors, threshold: int = 50) -> list[tuple[list[int]]]:
    '''
    基于 RGB 通道的一种加权欧式距离匹配方法
    
    Args:
        cluster_colors
            方法 clusterColors() 返回的结果
        threshold: int = 50
            阈值 0 - 765 (255 * 3)
    
    Return:
        [(center, lower, upper), ...]
    '''
    # https://www.compuphase.com/cmetric.htm
    ret = []
    for center, colors in cluster_colors:
        center = center.astype(np.int32)
        colors = colors.astype(np.int32)
        rmean = ((colors[:, 0] + center[0]) / 2).astype(np.int32)
        r = colors[:, 0] - center[0]
        g = colors[:, 1] - center[1]
        b = colors[:, 2] - center[2]
        distances = np.sqrt((((512+rmean)*r**2)>>8) + 4*g**2 + (((767-rmean)*b**2)>>8))
        matched = colors[distances < threshold]
        lower, _, _, _, upper = __getBoxPlotValues(matched)
        ret.append((list(center), list(lower), list(upper)))
    return ret
