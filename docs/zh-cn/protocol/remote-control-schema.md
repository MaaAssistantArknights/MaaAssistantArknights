---
order: 8
icon: mdi:remote-desktop
---

# 远程控制协议

要实现对 MAA 的远程控制，你需要提供一个服务，该服务必须是 http(s) 服务，并且提供下面两个可匿名访问的端点（Endpoint）。这两个端点必须是 http(s) 协议的 web 端点。

::: warning
如果该端点为 http 协议，MAA 会在每次连接时发出不安全警告。**在公网部署明文传输服务是一种非常不推荐且危险的行为，仅供测试使用。**
:::

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

## 获取任务端点

MAA 会以 1 秒的间隔持续轮询这个端点，尝试获取他要执行的任务，并按照获取到的列表按顺序执行。

端点路径随意，但是必须是 http(s) 端点。比如：`https://your-control-host.net/maa/getTask`

被控 MAA 需要将该端点填写到 MAA 配置的`获取任务端点`文本框中。

该端点必需能够接受一个 `Content-Type=application/json` 的 POST 请求，并该请求必须可以接受下面这个 JSON 作为请求的 content：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // 用户在MAA设置中填写的用户标识符。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec"         // MAA自动生成的设备标识符。
    ...     // 如果你的这个端点还有其他用途，你可以自行添加可选的参数，但是MAA只会传递user和device
}
```

该端点必须返回一个 JSON 格式的 Response，并且至少要满足下列格式：

```json
{
    "tasks":                            // 需要让MAA执行的Task的列表，目前可以支持的类型如示例中所示，如果不存在tasks则视为连接无效。
    [
        // 顺序执行的任务：下面这些任务会按照下发的顺序排队执行
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "CaptureImage",                         //截图任务，会截取一张当前模拟器的截图，并以Base64字符串的形式放在汇报任务的payload里。如果你需要下发这种类型的任务，请务必注意你的端点可接受的最大请求大小，因为截图会有数十MB，会超过一般网关的默认大小限制。
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "LinkStart",                            //启动一键长草
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "LinkStart-Recruiting",                 //立即根据当前配置，单独执行一键长草中的对应子功能，无视主界面上该功能的勾选框。这一类Type的可选值详见下述
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "Toolbox-GachaOnce",                    //工具箱中的牛牛抽卡任务，该类Type的可选取值为：Toolbox-GachaOnce, Toolbox-GachaTenTimes
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "Settings-ConnectionAddress",           //修改配置项的任务，等同于执行ConfigurationHelper.SetValue("ConnectionAddress", params); 为了安全起见，不是每个配置都可以修改，能修改的配置详见下述。
            "params": "value"                               //要修改的值
        },
        // 立即执行任务：下面这些任务可以在顺序执行任务运行中执行，并且MAA保证下面的任何一个任务都会尽快返回结果，通常用于对远程控制功能本身的控制。
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "CaptureImageNow",                      //立刻截图任务，和上面的截图任务是基本一样的，唯一的区别是这个任务会立刻被运行，而不会等待其他任务。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "StopTask",                             //"结束当前任务"任务，将会尝试结束当前运行的任务。如果任务列表还有其他任务会继续开始执行下一个。该任务不会等待并确认当前任务已停止才会返回，因此请使用心跳任务来确认停止命令是否已生效。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任务的唯一id，字符串类型，在汇报任务时会使用
            "type": "HeartBeat",                            //心跳任务，该任务会立即返回，并且将当前“顺序执行的任务”队列中正在执行的任务的Id作为Payload返回，如果当前没有任务执行，返回空字符串。
        },
    ],
    ...     // 如果你的这个端点还有其他用途，你可以自行添加可选的返回值，但是MAA只会读取tasks
}
```

这些任务会被按顺序执行，也就是说如果你先发下一个公招任务，再发下一个截图任务，则截图会在公招任务结束后执行。
该端点应当可以重入并且重复返回需要执行的任务，MAA 会自动记录任务 id，对于相同的 id，不会重复执行。

::: note

- LinkStart-[TaskName] 型的任务 type 的可选值为 LinkStart-Base，LinkStart-WakeUp，LinkStart-Combat，LinkStart-Recruiting，LinkStart-Mall，LinkStart-Mission，LinkStart-AutoRoguelike，LinkStart-Reclamation
- Settings-[SettingsName] 型的任务的 type 的可选值为 Settings-ConnectionAddress, Settings-Stage1
- Settings 系列任务仍然是要按顺序执行的，并不会在收到任务的时候立刻执行，而是排在上一个任务的后面
- 多个立即执行的任务也会按下发顺序执行，只不过这些任务的执行速度都很快，通常来说，并不需要关注他们的顺序。

:::

## 汇报任务端点

每当 MAA 执行完一个任务，他就会通过该端点将任务的执行结果汇报给远端。

端点路径随意，但是必须是 http(s) 端点。比如：`https://your-control-host.net/maa/reportStatus`

被控 MAA 需要将该端点填写到 MAA 配置的 `汇报任务端点` 文本框中。

该端点必需能够接受一个 `Content-Type=application/json` 的 POST 请求，并该请求必须可以接受下面这个 JSON 作为请求的 content：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // 用户在MAA设置中填写的用户标识符。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec",        // MAA自动生成的设备标识符。
    "task":"15be4725-5bd3-443d-8ae3-0a5ae789254c",          // 要汇报的任务的Id，和获取任务时的Id对应。
    "status":"SUCCESS",                                     // 任务执行结果，SUCCESS或者FAILED。一般不论任务执行成功与否只会返回SUCCESS，只有特殊情况才会返回FAILED，会返回FAILED的情况，会在上面的任务介绍时明确说明。
    "payload":"",                                           //汇报时携带的数据，字符串类型。具体取决于任务类型，比如截图任务汇报时，这里就会携带截图的Base64字符串。
    ...     // 如果你的这个端点还有其他用途，你可以自行添加可选的参数，但是MAA只会传递user和device
}
```

该端点的返回内容任意，但是如果你不返回 200OK，会在 MAA 端弹出一个 Notification，显示 `上传失败`

## 范例工作流-用 QQBot 控制 MAA

A 开发者想要用自己的 QQBot 控制 MAA，于是他开发了一个后端，暴露在公网上，提供两个端点：

```text
https://myqqbot.com/maa/getTask
https://myqqbot.com/maa/reportStatus
```

为了让用户用的更方便，他的 getTask 接口不管接收什么参数都默认返回 200OK 和一个空的 tasks 列表。
每次他接收到一个请求，他就去数据库里看一下有没有重复的 device，如果没有，他就将该 device 和 user 记录在数据库。
也就是说，在这个工作流下，这个接口同时还承担了用户注册的功能。

他在 QQBot 上提供了一条指令，供用户提交自己的 deviceId。

在它的 QQBot 的使用说明上，他告诉用户，在 MAA 的`用户标识符`中填写自己的 QQ 号，然后将`设备标识符`通过 QQ 聊天发送给 Bot。

QQBot 在收到标识符后，再根据消息中的用户 QQ 号，寻找数据库中是否有对应的数据，如果没有，则叫用户先去配置 MAA。

因为 MAA 在配置好后就会持续的发送请求，因此如果用户配置好了 MAA，在他通过 QQ 提交时，数据库内应该有匹配的记录。

这时 Bot 将数据库内的该记录设置一个已验证标记，未来 getTask 再使用这套 device 和 user 请求时，就会返回真正的任务列表。

当用户通过 QQBot 提交指令后，Bot 将一条任务写入数据库，这样稍后，getTask 就会返回这条任务。并且，该 QQbot 还很贴心的，在每次用户提交指令后，都默认再附加一个截图任务。

MAA 在任务执行完后，会调用 reportStatus 汇报结果，Bot 在收到结果后，在 QQ 端发送消息通知用户以及展示截图。

## 范例工作流-用网站控制 MAA

B 开发者写了一个网站，设想通过网站批量管理 MAA，因此，他拥有一套自己的用户管理系统。但是它的后端在公网上，提供两个可匿名访问的端点：

```text
https://mywebsite.com/maa/getTask
https://mywebsite.com/maa/reportStatus
```

在网站上，有个连接 MAA 实例的界面，会展示一个 B 开发者称之为 `用户密钥` 的随机字符串，并有一个填入设备 id 的文本框。

网站要求用户在 MAA 的 `用户标识符` 中填写自己的用户密钥，然后将 `设备标识符` 填入网站。

只有在网站上成功创建了 MAA 连接，getTask 才会返回 200OK，其他时候都返回 401Unauthorized。

因此如果用户在 MAA 上填错了，按下测试连接按钮，会得到测试失败的提示。

用户可以在网站上下发任务，为任务排队，查看截图等等，这些功能的实现和上面 QQBot 例子类似，都是通过 getTask 和 reportStatus 组合完成。
