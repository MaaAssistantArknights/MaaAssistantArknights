1. 下载最新版 MAA
	1. [MAA软件下载页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/)
	2. 打开软件下载页面，找到最新的安装包，比如，名字可能是 `MeoAssistantArknights_v4.0.0.zip`
	3. 把压缩包解压，就已经安装好了
2. 查看使用说明，尝试运行起 MAA
	1. [MAA使用说明](https://github.com/MaaAssistantArknights/MaaAssistantArknights#%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E)
	2. [MAA使用说明-详细版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E8%AF%A6%E7%BB%86%E4%BB%8B%E7%BB%8D.md)
	3. [常见问题](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98.md)
	4. 上述教程看完还是解决不了问题的，请加 Q 群，[自动战斗 JSON 作业分享群](https://jq.qq.com/?_wv=1027&k=1giyMpPb)
	5. 在点击 LinkStart！之后，MAA 能帮你打游戏，就说明配置成功了
3. 最常见的问题，连接不上模拟器
	1. 在问别人之前，请先看 [连接错误、不知道 adb 路径怎么填写](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98.md#%E8%BF%9E%E6%8E%A5%E9%94%99%E8%AF%AF%E4%B8%8D%E7%9F%A5%E9%81%93-adb-%E8%B7%AF%E5%BE%84%E6%80%8E%E4%B9%88%E5%A1%AB%E5%86%99)
	2. 也可查看 [MAA adb疑难杂症及其解决办法](https://docs.qq.com/sheet/DS0Fhb1VUYllkbHRY)（不推荐）
	3. 另外玛丽正在想办法把连接功能改成全自动，等一两个版本就好
	4. 顺便写几个常见的解决办法
		1. 模拟器没开 adb 调试（仅限蓝叠）
		2. 没有使用管理员权限打开 MAA 
4. 使用 MAA 的自动抄作业功能
	1. 下载作业配置文件，途径有
		1. MAA 作业分享网站（绝赞开发中！）
		2. 加群，群号同上
	2. 打开 MAA，点击“自动战斗”标签页
	3. 点击“选择作业”，选择一个你想要使用的作业配置文件
	4. 在游戏内，手动前往，存在“开始行动”按钮的界面，比如
		1. 对于危机合约，就是选 tag 界面
		2. 对于普通关，就是点击了关卡名，右边划出关卡信息
		3. 注意事项 1：必须自己移动到这个页面，maa 不会从其他地方前往这个页面
		4. 注意事项 2: 不是点击了“开始行动”之后的编队页面！
	5. 点击“开始”，好了，接下来看 MAA 帮你打关
5. 自己制作作业文件，造福大家
	1. 方法一：使用可视化作业生成器
		1. 打开你的 MAA 所在文件夹
		2. 打开“作业制作器”文件夹
		3. 双击“自动战斗作业生成器”
		4. 好了，你可以开始写全新的作业了！
		5. 你可以上传别人的作业配置文件，参考一下怎么写
		6. 更详细的说明，可以参考[战斗流程协议.md](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E6%88%98%E6%96%97%E6%B5%81%E7%A8%8B%E5%8D%8F%E8%AE%AE.md)
		7. 还是不懂，加群，群号同上
	2. 方法二：直接写 JSON 文件
		1. 推荐有一定编程基础的朋友使用这个方法
		2. 格式参考 [战斗流程协议.md](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/docs/%E6%88%98%E6%96%97%E6%B5%81%E7%A8%8B%E5%8D%8F%E8%AE%AE.md)
		3. 一个例子 [GA-EX8-raid.json](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/GA-EX8-raid.json)
		4. 注：方法一的可视化作业生成器就是基于“战斗流程协议”开发的。以战斗流程协议为主。作业生成器暂时还不能支持所有功能。
	3. 方法三（备用）：使用最新作业生成器
		1. 此方法和方法一相同，区别是方法一的生成器可能不是最新的。
		2. 要想获取最新作业生成器，使用如下方法，前提是你可以上Github
		3. 前往 [MAA自动战斗作业生成器 主页](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/tools/CopilotDesinger)，找到“自动战斗作业生成器”，点击进入
		4. 找到 `Raw` 这个按钮，如果找不到，按 ctrl/command + F，使用浏览器的搜索功能，找到这个按钮。
		5. 右键 `Raw` 按钮，点击“保存链接为”（“save link as”），把它下载到你的电脑
		6. 双击你刚刚下载的文件，会打开一个网页，好了，你可以开始制作一份作业了。
		7. 可以导入别人的作业配置文件作为参考。有不懂的可以加群。群号如上。
		8. 最后，感谢大佬 [LYZhelloworld](https://github.com/LYZhelloworld) 开发的自动作业生成器
