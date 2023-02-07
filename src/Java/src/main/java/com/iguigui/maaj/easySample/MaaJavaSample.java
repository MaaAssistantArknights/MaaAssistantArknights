package com.iguigui.maaj.easySample;

import com.sun.jna.Native;
import com.sun.jna.Pointer;

import java.util.Scanner;

public class MaaJavaSample {

    /**
     * Maa的Java调用实例实现，最简示例仅需依赖第三方JNA库。
     * 仅在Windows下进行过测试，未在Linux下测试过(相信强大的Linux用户自己可以解决问题！
     *
     * --------特别提醒----------
     * 1. 版本兼容性： >= v3.9.0-alpha。
     * 2. 需要手动在 我的电脑 - 高级系统设置 - 环境变量中，给Path增加Maa的路径。
     * --------特别提醒----------
     *
     * @param args
     */
    public static void main(String[] args) {
        //设定JNA寻找maa的dll的地址，同时也要把这个路径添加到环境变量中的Path中！
        String maaPath = "C:\\Users\\gui\\Desktop\\MeoAssistantArknights";
        System.setProperty("jna.library.path", maaPath);
        //adb地址，自己看着办
        String adbPath = maaPath + "/platform-tools/adb.exe";

        //第一步，加载Maa调用实例，加载失败请检查jna.library.path和path参数
        MaaCore instance = Native.load("MaaCore", MaaCore.class);
        System.out.println("获取Maa版本号:" + instance.AsstGetVersion());

        //第二步，加载Maa资源初始化
        boolean b = instance.AsstLoadResource(maaPath);
        System.out.println("Maa加载:" + b);

        //第三步，创建实例
        //这一步可以多次创建多个实例连接如果你有多个模拟器
//        Pointer pointer = instance.AsstCreate();
        Pointer pointer = instance.AsstCreateEx((msg, detail_json, custom_arg) -> {
            System.out.printf("回调msg : %s , 回调 detail_json : %s ,回调 custom_arg : %s \n",msg,detail_json,custom_arg);
        },"瞎写的参数建议可以弄个自己生成的实例ID");

        //第四步，发起连接
        boolean adb = instance.AsstConnect(pointer, adbPath, "127.0.0.1:62001", "");
        System.out.println("Maa 尝试连接:" + adb);

        //第五步，添加任务，详细参数请参考Maa集成文档，此处示例为清理智，执行上次战斗
        instance.AsstAppendTask(pointer, "Fight", "{\"stage\":\"LastBattle\"}");

        //开始执行
        instance.AsstStart(pointer);
        new Scanner(System.in).next();

        //停止执行，会清空taskChain
        instance.AsstStop(pointer);

        //销毁实例，释放连接
        instance.AsstDestroy(pointer);
    }


}
