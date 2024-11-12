# Maa-HTTP 接口实现

基于 JNA (Java-Native-Access) 调用 Maa 核心的 HTTP + WebSocket 接口实现.
仅在 Windows 下进行过测试，未在 Linux 下测试过（相信强大的 Linux 用户自己可以解决问题！

## 接口实现情况

| 功能名称   | 接口路径(命令)      | HTTP实现 | WebSocket实现 |
|:-------|:--------------|:-------|:------------|
| 获取版本号  | getVersion    | Yes    | Yes |
| 创建连接   | connect       | Yes    | Yes |
| 添加任务   | appendTask    | Yes    | Yes |
| 设定任务参数 | setTaskParams | Yes    | Yes |
| 开始执行   | start         | Yes    | Yes |
| 停止执行   | stop          | Yes    | Yes |
| 销毁实例   | destroy       | Yes    | Yes |
| 列出实例连接 | listInstance  | Yes    | Yes |
| 消息回调通知 | callBack  | No     | Yes |
| 获取截图   | getImage  | Yes     | No |

## 环境依赖

1. JDK1.8 +
2. Maa v3.9.0-alpha +

## 如何编译

.\gradlew.bat jar

## 使用方法

1. 把他丢进 Maa 文件夹下,形成如下文件结构.备注: 启动脚本[Maa-HTTP-Server-startup.bat](Maa-HTTP-Server-startup.bat).

    ```text
    MaaCoreArknights_v3.9.0-beta.8
    │   MAA.exe
    │   MaaCore.dll  
    │   ...
    └───Java-HTTP
        │   Maa-HTTP-0.0.1.jar
        │   Maa-HTTP-Server-startup.bat
    ```

2. 双击Maa-HTTP-Server-startup.bat启动.
3. 默认端口号8848钛金端口,如需修改,请修改 Maa-HTTP-Server-startup.bat 中的-port=8848 .
4. 当你看到如下内容则启动成功.

```log
2022-05-31 22:20:03.000 [main] INFO  Application - Autoreload is disabled because the development mode is off.
2022-05-31 22:20:03.407 [main] INFO  Application - Application started in 0.458 seconds.
2022-05-31 22:20:03.407 [main] INFO  Application - Application started: io.ktor.server.application.Application@fac80
2022-05-31 22:20:03.717 [DefaultDispatcher-worker-1] INFO  Application - Responding at http://0.0.0.0:8848
```

## 接口文档

### HTTP 部分

提供了如下功能

| 功能名称   | 接口路径          |
|:-------|:--------------|
| 获取版本号  | getVersion    |
| 创建连接   | connect       |
| 添加任务   | appendTask    |
| 设定任务参数 | setTaskParams |
| 开始执行   | start         |
| 停止执行   | stop          |
| 销毁实例   | destroy       |
| 列出连接   | listInstance  |
| 获取截图   | getImage  |

---

#### 接口名称 获取版本号

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/getVersion>

###### 2) 调用方式: HTTP get

###### 3) 请求参数: 无

###### 4) 返回结果示例

```json
{
  "data": {
    "version": "v3.9.0-beta"
  },
  "code": 0,
  "message": "success"
}
```

---

##### 接口名称 创建连接

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/connect>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "adbPath": "C:\\MaaCoreArknights3\\platform-tools\\adb.exe",
  "host": "127.0.0.1:62001",
  "detailJson": ""
}
```

###### Post 参数

|  字段名称   | 字段说明            | 类型     | 必填  | 备注  |
|  ----  |-----------------|--------|-----|-----|
| adbPath  | ADB地址           | String | Y   | -   |
| host  | 实例地址            | String   | Y   | -   |
| detailJson  | 有什么用我也不知道，反正就是有 | String   | N   | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
    "result": true
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型      | 备注 |
|------|-------------|---------|----  |
| id   | 给连接实例创建的ID | String  | - |
| result    | 是否成功        | Boolean | - |

---

---

##### 接口名称 添加任务

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/appendTask>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
  "type": "Fight",
  "params": {
    "stage": "LastBattle"
  }
}
```

###### Post 参数

|  字段名称   | 字段说明                                                                                                                          | 类型     | 必填 | 备注  |
|  ----  |-------------------------------------------------------------------------------------------------------------------------------|--------|----  |-----|
| id  | 创建连接返回的实例ID                                                                                                                   | String | Y | -   |
| type  | 任务类型,此次示例为刷理智战斗,详情参考[集成文档](https://raw.githubusercontent.com/MaaAssistantArknights/MaaAssistantArknights/master/docs/集成文档.md) | String | Y | -   |
| params  | 任务参数                                                                                                                          | Json   | Y | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
    "taskId": 0
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明       | 类型     | 备注 |
|------|------------|--------|----  |
| id   | 实例ID       | String | - |
| taskId    | 任务ID,为0则失败 | Int    | - |

---

##### 接口名称 设定任务参数

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/setTaskParams>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
  "type": "Fight",
  "taskId": 0,
  "params": {
    "stage": "LastBattle"
  }
}
```

###### Post 参数

|  字段名称   | 字段说明                                                                                                                          | 类型     | 必填 | 备注  |
|  ----  |-------------------------------------------------------------------------------------------------------------------------------|--------|----  |-----|
| id  | 创建连接返回的实例ID                                                                                                                   | String | Y | -   |
| taskId  | 任务ID                                                                                                                   | Int    | Y | -   |
| type  | 任务类型,此次示例为刷理智战斗,详情参考[集成文档](https://raw.githubusercontent.com/MaaAssistantArknights/MaaAssistantArknights/master/docs/集成文档.md) | String | Y | -   |
| params  | 任务参数                                                                                                                          | Json   | Y | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
    "result": false
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型      | 备注 |
|------|-------------|---------|----  |
| id   | 给连接实例创建的ID | String  | - |
| result    | 是否成功        | Boolean | - |

---

##### 接口名称 开始执行

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/start>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3"
}
```

###### Post 参数

|  字段名称   | 字段说明                                                                                                                       | 类型     | 必填 | 备注  |
|  ----  |--------|--------|----  |-----|
| id  | 创建连接返回的实例ID   | String | Y | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3",
    "result": true
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型      | 备注 |
|------|-------------|---------|----  |
| id   | 给连接实例创建的ID | String  | - |
| result    | 是否成功        | Boolean | - |

---

##### 接口名称 停止执行，清空任务链

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/stop>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3"
}
```

###### Post 参数

|  字段名称   | 字段说明                                                                                                                       | 类型     | 必填 | 备注  |
|  ----  |--------|--------|----  |-----|
| id  | 创建连接返回的实例ID   | String | Y | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3",
    "result": true
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型      | 备注 |
|------|-------------|---------|----  |
| id   | 给连接实例创建的ID | String  | - |
| result    | 是否成功        | Boolean | - |

---

##### 接口名称 销毁实例

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/destroy>

###### 2) 调用方式: HTTP post

###### 3) 请求参数

```json
{
  "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3"
}
```

###### Post 参数

|  字段名称   | 字段说明                                                                                                                       | 类型     | 必填 | 备注  |
|  ----  |--------|--------|----  |-----|
| id  | 创建连接返回的实例ID   | String | Y | -   |

###### 4) 请求返回结果

```json
{
  "data": {
    "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3",
    "result": true
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型      | 备注 |
|------|-------------|---------|----  |
| id   | 给连接实例创建的ID | String  | - |
| result    | 是否成功        | Boolean | - |

---

##### 接口名称 列出实例连接

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/listInstance>

###### 2) 调用方式: HTTP get

###### 3) 请求参数: 无

###### 4) 请求返回结果

```json
{
  "data": {
    "list": [
      {
        "id": "ccd76e0c367511158ca774ff951a22e8bb62f5d3",
        "host": "127.0.0.1:62026",
        "adbPath": "C:\\Users\\atmzx\\Desktop\\MaaCoreArknights3\\platform-tools\\adb.exe",
        "uuid": "",
        "status": 0
      }
    ]
  },
  "code": 0,
  "message": "success"
}
```

###### 5) 请求返回结果参数说明

| 字段名称 | 字段说明        | 类型     | 备注 |
|------|-------------|--------|----  |
| id   | 实例ID        | String | - |
| host    | 连接的Host     | String | - |
| adbPath    | 使用的Adb地址    | String | - |
| uuid    | 设备备ID，保留    | String | - |
| status    | 0待机中,1任务执行中 | Int    | - |

---

##### 接口名称 获取截图

###### 1) 请求地址

> <http://127.0.0.1:8848/API/V1/getImage>

###### 2) 调用方式: HTTP get

###### 3) 请求参数

###### Get参数

|  字段名称   | 字段说明                                                                                                                       | 类型     | 必填 | 备注  |
|  ----  |--------|--------|----  |-----|
| id  | 创建连接返回的实例ID   | String | Y | -   |

###### 4) 请求返回结果: 图片内容, PNG 格式

### WebSocket 部分

请求地址(没错跟HTTP同样是8848钛金端口

> <ws://127.0.0.1:8848/API/V1>

提供了如下功能

| 功能名称   | 命令            |
|:-------|:--------------|
| 获取版本号  | getVersion    |
| 创建连接   | connect       |
| 添加任务   | appendTask    |
| 设定任务参数 | setTaskParams |
| 开始执行   | start         |
| 停止执行   | stop          |
| 销毁实例   | destroy       |
| 列出实例连接   | listInstance  |
| 消息回调通知 | callBack      |

所有的的WebSocket接口跟HTTP接口请求响应参数均完全一致

---

##### 接口名称 获取版本号

```json
{
  "command": "getVersion",
  "msgId": 114514,
  "data": {}
}
```

|  字段名称   | 字段说明                              | 类型     | 必填 | 备注  |
|  ----  |-----------------------------------|--------|----  |-----|
| command  | 操作命令                              | String |,Y | -   |
| msgId  | 消息ID，服务端收到此次请求，会用同一个ID答复，便于请求响应关联 | Int    | Y | -   |
| data  | 请求参数，请参考同名接口的HTTP接口文档，与HTTP接口完全一致 | Json   | Y | -   |

###### 请求返回结果

```json
{
  "data": {
    "version": "v3.9.0-beta"
  },
  "command": "getVersion",
  "msgId": 114514,
  "code": 0,
  "message": "success"
}
```

| 字段名称 | 字段说明                              | 类型     | 备注 |
|------|-----------------------------------|--------|----  |
| command   | 操作命令,与请求一致                        | String | - |
| msgId    | 消息ID，此次请求的消息ID为114514则响应也是114514  | Int    | - |
| data    | 响应内容,请参考同名接口的HTTP接口文档，与HTTP接口完全一致 | Json   | - |

---

##### 接口名称 创建连接

```json
{
  "command": "connect",
  "msgId": 114514,
  "data": {
    "adbPath": "C:\\Users\\atmzx\\Desktop\\MaaCoreArknights3\\platform-tools\\adb.exe",
    "host": "127.0.0.1:62001"
  }
}
```

|  字段名称   | 字段说明                            | 类型     | 必填 | 备注  |
|  ----  |---------------------------------|--------|----  |-----|
| command  | 操作命令                            | String |,Y | -   |
| msgId  | 消息ID，服务端收到此次请求，会用同一个ID答复，便于请求响应关联 | Int    | Y | -   |
| data  | 请求参数，请参考同名接口的HTTP文档，与HTTP接口完全一致 | Json   | Y | -   |

###### 请求返回结果

```json
{
  "data": {
    "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
    "result": true
  },
  "command": "connect",
  "msgId": 114514,
  "code": 0,
  "message": "success"
}
```

| 字段名称 | 字段说明                              | 类型     | 备注 |
|------|-----------------------------------|--------|----  |
| command   | 操作命令,与请求一致                        | String | - |
| msgId    | 消息ID，此次请求的消息ID为114514则响应也是114514  | Int    | - |
| data    | 响应内容,请参考同名接口的HTTP接口文档，与HTTP接口完全一致 | Json   | - |

appendTask setTaskParams start stop等接口不再描述，均可遵循以上规则，参考HTTP接口文档实现。

---

##### 接口名称 回调消息

###### 返回结果示例

```json
{
  "data": {
    "id": "46b9d5cd60382f100b336b17d6817f8eda255f73",
    "logId": 1,
    "msg": 2,
    "details": {
      "uuid": "",
      "details": {
        "adb": "C:\\MaaCoreArknights3\\platform-tools\\adb.exe",
        "address": "127.0.0.1:62001",
        "config": "General",
        "width": 1280,
        "height": 720
      },
      "what": "ResolutionGot",
      "why": ""
    }
  },
  "command": "callBack",
  "msgId": 0,
  "code": 0,
  "message": "success"
}
```

| 字段名称 | 字段说明                              | 类型     | 备注 |
|------|-----------------------------------|--------|----  |
| command   | 操作命令,callBack类型                        | String | - |
| msgId    | 消息ID，callBack 一律为0 | Int    | - |
| data    | 响应内容,详情参考[集成文档](https://raw.githubusercontent.com/MaaAssistantArknights/MaaAssistantArknights/master/docs/集成文档.md)   | Json   | - |
