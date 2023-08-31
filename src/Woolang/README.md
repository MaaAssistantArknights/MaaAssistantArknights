# MAA Woolang 接口

~~Woolang是每个窝窝头都需要使用的语言，更适合窝窝头的体质！~~

目前MAA-Woolang接口仅是对C-API的简单包装；Woolang是一门静态类型的脚本语言，关于Woolang的更多信息，请看[这里](https://github.com/cinogama/woolang)

## 运行前准备

MAA-Woolang使用了 `woffi` 和 `filesystem`，因此需要使用 baozi 安装这些依赖：

```
baozi 是 Woolang 的包管理器，可以这里获取到Chief（由BiDuang提供）：
    https://github.com/BiDuang/Chief_Reloaded

这将帮助你在Windows操作系统下安装baozi和Woolang运行时。

对于Linux用户，可以直接从这个仓库直接获取 baozi 的源码和二进制文件：
    https://git.cinogama.net/cinogamaproject/woolangpackages/baozi
```

切换到Woolang包配置文件 `package.json` 所在路径，在命令行中输入：

```shell
baozi install
```

## 运行

由于窝窝头没有用过MAA(悲)，因此没法进行测试；理论上MAA-Woolang接口的运行和C-API类似（但是我也不知道C-API是咋跑的）。假设一切顺利，应该可以通过：

```shell
woodriver demo.wo
```

运行示例项目。



