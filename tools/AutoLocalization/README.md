# auto_localization

这个命令行工具可以帮助您自动翻译本地化目录中的不同语言文档。

## TODO

- [x] 调用git api获取中文文档旧版本完整文本内容
- [ ] githook
- [ ] 完成后格式化xaml

## 配置

在使用前，您需要配置OpenAI API密钥。运行init子命令将会提示您输入API密钥和文档根目录，并将其存储在.env文件中。

## 参数

```bash
python main.py -h
```

```
options:
   -h, --help            show this help message and exit

positional arguments:
   
   {init,create,update}
   
   init                初始化工具
    
   create              初始化其他语言的文档
      -f, --force       强制覆盖已有的部分
      -t, --test        跳过chatgpt翻译进行测试
      -l, --language    指定语言，可选参数：en-us, zh-tw, ja-jp, ko-kr
      
   update              更新本地化翻译
      -t, --test        跳过chatgpt翻译进行测试
      -l, --language    指定语言，可选参数：en-us, zh-tw, ja-jp, ko-kr

```

## 依赖

运行

```bash
pip install -r requirements.txt
```

Enjoy!

## 要求

自动本地化有几点要求看看能不能做到：

1. 不要破坏缩进（包括空行不带空格）
2. `<ResourceDictionary x:Uid="KeyName">` 如果能在资源字典找到Uid对应的Key，把对应的Value作为注释写入，像这样（注意注释中的空格）：
    ```xml
            <!--  设置  -->
    <ResourceDictionary x:Uid="Settings">
    </ResourceDictionary>
    ```
3. 由于字典被拆分了，重复的键要交给脚本检查
4. 因为zh-cn可能会混入一些中文梗，所以ja-jp、ko-kr尽量由en-us翻译

    ```code
    check duplicate Key()
    if exist duplicate Key:
        throw (or raise) an exception
    
    zh-cn -> zh-tw
    if exist en-us:
        en-us -> ja-jp
        en-us -> ko-kr
    else:
        zh-cn -> ja-jp
        zh-cn -> ko-kr
        zh-cn -> en-us
    ```