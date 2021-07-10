#include "WinMacro.h"

#include <vector>
#include <utility>
#include <ctime>

#include <stdint.h>
#include <WinUser.h>

#ifdef _DEBUG
#include <iostream>
#endif

using namespace MeoAssistance;

WinMacro::WinMacro(HandleType type)
    : m_handle_type(type),
    m_rand_engine(time(NULL))
{
}

bool WinMacro::findHandle()
{
    switch (m_handle_type) {
    case HandleType::BlueStacksControl: {
        HWND temp_handle = ::FindWindow(L"BS2CHINAUI", L"BlueStacks App Player");
        temp_handle = ::FindWindowEx(temp_handle, NULL, L"BS2CHINAUI", L"HOSTWND");
        m_handle = ::FindWindowEx(temp_handle, NULL, L"WindowsForms10.Window.8.app.0.34f5582_r6_ad1", L"BlueStacks Android PluginAndroid");
    } break;
    case HandleType::BlueStacksView:
    case HandleType::BlueStacksWindow:
        m_handle = ::FindWindow(L"BS2CHINAUI", L"BlueStacks App Player");
        break;
    default:
        std::cerr << "handle type error! " << static_cast<int>(m_handle_type) << std::endl;
        break;
    }

#ifdef _DEBUG
    std::cout << "handle: " << m_handle << std::endl;
#endif

    if (m_handle != NULL) {
        return true;
    }
    else {
        return false;
    }
}

bool WinMacro::resizeWindow(int width, int height)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Window))) {
        return false;
    }

    return ::MoveWindow(m_handle, 0, 0, width, height, true);
}

bool WinMacro::click(Point p)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Control))) {
        return false;
    }

#ifdef _DEBUG
    std::cout << "click: " << p.x << ", " << p.y << std::endl;
#endif

	LPARAM lparam = MAKELPARAM(p.x, p.y);

	::SendMessage(m_handle, WM_LBUTTONDOWN, MK_LBUTTON, lparam);
	::SendMessage(m_handle, WM_LBUTTONUP, 0, lparam);

    return true;
}

bool WinMacro::clickRange(Rect rect)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::Control))) {
        return false;
    }

    int x = 0, y = 0;
    if (rect.width == 0) {
        x = rect.x;
    }
    else {
        std::poisson_distribution<int> x_rand(rect.width);
        x = x_rand(m_rand_engine) + rect.x;

    }
    if (rect.height == 0) {
        y = rect.y;
    }
    else {
        std::poisson_distribution<int> y_rand(rect.height);
        int y = y_rand(m_rand_engine) + rect.y;
    }

    return click({ x, y });
}

bool WinMacro::getImage(Rect rect)
{
    if (!(static_cast<int>(m_handle_type) & static_cast<int>(HandleType::View))) {
        return false;
    }

    HDC pDC;// 源DC
    //判断是不是窗口句柄如果是的话不能使用GetDC来获取DC 不然截图会是黑屏
    if (m_handle == ::GetDesktopWindow())
    {
        pDC = CreateDCA("DISPLAY", NULL, NULL, NULL);
    }
    else
    {
        pDC = ::GetDC(m_handle);//获取屏幕DC(0为全屏，句柄则为窗口)
    }
    int BitPerPixel = ::GetDeviceCaps(pDC, BITSPIXEL);//获得颜色模式
    if (rect.width == 0 && rect.height == 0)//默认宽度和高度为全屏
    {
        rect.width = ::GetDeviceCaps(pDC, HORZRES); //设置图像宽度全屏
        rect.height = ::GetDeviceCaps(pDC, VERTRES); //设置图像高度全屏
    }
    HDC memDC;//内存DC
    memDC = ::CreateCompatibleDC(pDC);
    HBITMAP memBitmap, oldmemBitmap;//建立和屏幕兼容的bitmap
    memBitmap = ::CreateCompatibleBitmap(pDC, rect.width, rect.height);
    oldmemBitmap = (HBITMAP)::SelectObject(memDC, memBitmap);//将memBitmap选入内存DC
    if (m_handle == ::GetDesktopWindow())
    {
        BitBlt(memDC, 0, 0, rect.width, rect.height, pDC, rect.x, rect.y, SRCCOPY);//图像宽度高度和截取位置
    }
    else
    {
        bool bret = ::PrintWindow(m_handle, memDC, PW_CLIENTONLY);
        if (!bret)
        {
            BitBlt(memDC, 0, 0, rect.width, rect.height, pDC, rect.x, rect.y, SRCCOPY);//图像宽度高度和截取位置
        }
    }

#ifdef _DEBUG
    //以下代码保存memDC中的位图到文件
    BITMAP bmp;
    ::GetObject(memBitmap, sizeof(BITMAP), &bmp);;//获得位图信息
    FILE* fp;
    fopen_s(&fp, "D:\\Code\\MeoAssistance\\Debug\\test.bmp", "w+b");//图片保存路径和方式

    BITMAPINFOHEADER bih = { 0 };//位图信息头
    bih.biBitCount = bmp.bmBitsPixel;//每个像素字节大小
    bih.biCompression = BI_RGB;
    bih.biHeight = bmp.bmHeight;//高度
    bih.biPlanes = 1;
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;//图像数据大小
    bih.biWidth = bmp.bmWidth;//宽度

    BITMAPFILEHEADER bfh = { 0 };//位图文件头
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);//到位图数据的偏移量
    bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight;//文件总的大小
    bfh.bfType = (WORD)0x4d42;

    fwrite(&bfh, 1, sizeof(BITMAPFILEHEADER), fp);//写入位图文件头
    fwrite(&bih, 1, sizeof(BITMAPINFOHEADER), fp);//写入位图信息头
    byte* p = new byte[bmp.bmWidthBytes * bmp.bmHeight];//申请内存保存位图数据
    GetDIBits(memDC, (HBITMAP)memBitmap, 0, rect.height, p,
        (LPBITMAPINFO)&bih, DIB_RGB_COLORS);//获取位图数据
    fwrite(p, 1, bmp.bmWidthBytes * bmp.bmHeight, fp);//写入位图数据
    delete[] p;
    fclose(fp);
#endif

    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(m_handle, pDC);

    return true;
}