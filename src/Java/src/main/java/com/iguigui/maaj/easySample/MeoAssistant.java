package com.iguigui.maaj.easySample;

import com.sun.jna.Pointer;
import com.sun.jna.win32.StdCallLibrary;


//本质上是对C接口的抽象层
//请参考C接口 https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/include/AsstCaller.h
public interface MaaCore extends StdCallLibrary {

    //回调接口定义
    interface AsstApiCallback extends StdCallCallback {
        void callback(int msg, String detail_json, String custom_arg);
    }

    //第一步，加载资源
    boolean AsstLoadResource(String path);

    //选一个你喜欢的create，搞不定回调就用普通create，又不是不能用
    Pointer AsstCreate();

    /**
     * 带回调的实例创建
     * @param callback 回调方法实例
     * @param custom_arg 用户参数，塞什么进去回头就给你回调的时候返回什么，例如填入自己生成的实例ID，回调回来就可以用于查询是哪个实例的回调信息
     * @return
     */
    Pointer AsstCreateEx(AsstApiCallback callback, String custom_arg);

    //销毁实例释放连接
    void AsstDestroy(Pointer handle);

    //连接安卓
    boolean AsstConnect(Pointer handle, String adb, String host, String config);

    //添加任务链
    //参考集成文档 https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/3.1-%E9%9B%86%E6%88%90%E6%96%87%E6%A1%A3.md
    int AsstAppendTask(Pointer handle, String type, String params);

    //运行时修改参数
    boolean AsstSetTaskParams(Pointer handle, int taskId, String params);

    //开跑！
    boolean AsstStart(Pointer handle);

    //爷不想跑了爷要自己玩
    boolean AsstStop(Pointer handle);

    /**
     * 获取最后一次截图的内容
     * (但是这个接口我就没成功用上过
     * @param handle    Maa实例
     * @param buff      图片PNG格式编码内容
     * @param buff_size byte[]的长度
     * @return 图片长度
     */
    long AsstGetImage(Pointer handle, byte[] buff, long buff_size);

    //模拟点击
    boolean AsstCtrlerClick(Pointer handle, int x, int y, boolean block);

    //获取版本号
    String AsstGetVersion();

    //向maa注入日志
    void AsstLog(String level, String message);

}
