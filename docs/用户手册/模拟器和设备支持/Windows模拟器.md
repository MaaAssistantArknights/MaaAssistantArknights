---
order: 1
icon: ri:windows-fill
---

# Windows 模拟器支持

::: tip
遇到问题请先参阅 [常见问题](../常见问题.md)
:::

以下模拟器排序为随机生成，排名不分先后

<script setup>
import MarkdownIt from 'markdown-it'
import MarkdownItAnchor from 'markdown-it-anchor'

const shuffleArray = (array) => {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
}

const fullySupport = shuffleArray([
`
### ✅ [蓝叠模拟器 5](https://www.bluestacks.cn/)
完美支持。需要在模拟器 \`设置\` - \`引擎设置\` 中打开 \`允许ADB连接\`。
`,
`
### ✅ [蓝叠模拟器 5 国际版](https://www.bluestacks.com/tw/index.html)（最稳定👍）

完美支持。需要在模拟器 \`设定\` - \`进阶\` 中打开 \`Android调试桥\`。

- 推荐下载 [离线安装包](https://support.bluestacks.com/hc/zh-tw/articles/4402611273485-BlueStacks-5-%E9%9B%A2%E7%B7%9A%E5%AE%89%E8%A3%9D%E7%A8%8B%E5%BC%8F)，避免安装慢和捆绑安装；
- 若 adb 端口号不断的无规律变动，每次启动都不相同，可能是因为您的电脑开启了 [Hyper-V](https://support.bluestacks.com/hc/zh-tw/articles/4415238471053-BlueStacks-5-%E6%94%AF%E6%8F%B4-Hyper-V-%E7%9A%84-Windows-10-%E5%92%8C-11-%E4%B8%8A%E7%9A%84%E9%9B%BB%E8%85%A6%E8%A6%8F%E6%A0%BC%E9%9C%80%E6%B1%82)。MAA 现在会尝试自动读取蓝叠模拟器配置文件内的端口号，若该功能失效/你有多开需求/安装了多个模拟器核心，请参考 [常见问题](../常见问题.html#蓝叠模拟器每次启动端口号都不一样-hyper-v) 做出修改。由于 Hyper-V 以管理员身份运行，如自动关闭模拟器、自动检测连接等不涉及 adb 的操作同样需要以管理员身份运行 MAA。
`,
`
### ✅ [MuMu 模拟器 12](https://mumu.163.com/)（最流畅👍）

完美支持。

- “完成后退出模拟器”功能可能偶现异常，如果遇到请向 MuMu 官方反馈；
- 3.5.4 ~ 3.5.7 版本 MuMu 12 的“后台保活”功能会导致截图失败，推荐使用更新后的版本；若您正在使用 3.5.4 ~ 3.5.7 版本的 MuMu 12，请确认 MuMu 12 设置 - 其他 中将“后台挂机时保活运行”功能关闭（详见[官方公告](https://mumu.163.com/help/20230802/35047_1102450.html)）；
- 多开时需通过 MuMu 12 多开器的 ADB 按钮查看对应实例的端口信息，将 MAA \`设置\` - \`连接设置\` 的连接地址的端口号修改为对应的端口。
`,
`
### ✅ [雷电模拟器](https://www.ldmnq.com/)

完美支持。

- 雷电 9 推荐使用 9.0.57 及以上版本；雷电 5 推荐使用 5.0.67 及以上版本；
- 低于上述版本则需要在 \`设置\` - \`连接设置\` 中运行 \`强制替换 ADB\`，才能使用 Minitouch, MaaTouch 等高效的触控模式；
`,
`
### ✅ [夜神模拟器](https://www.yeshen.com/)

完美支持，但仅测试了 7 和 9。
`,
`
### ✅ [逍遥模拟器](https://www.xyaz.cn/)

完美支持，但测试较少。
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [MuMu 模拟器 6](https://mumu.163.com/update/win/)

停止支持，官方已在 2023.8.15 停止维护。

- 不支持自动检测连接；
- 需要在 \`设置\` - \`连接设置\` 中运行 \`强制替换 ADB\`，才能使用 Minitouch, MaaTouch 等高效的触控模式；
- 需要使用管理员权限运行 MAA 才能使用“完成后退出模拟器”相关功能；
- 不支持使用 MuMu 6 默认的几个奇葩分辨率，需要改成 \`1280x720\`，\`1920x1080\`，\`2560x1440\` 等 16:9 比例；
- MuMu 6 多开使用的是同一个 adb 端口，所以无法支持多开的 MuMu 6。
`,
`
### ⚠️ [适用于 Android™️ 的 Windows 子系统](https://docs.microsoft.com/zh-cn/windows/android/wsa/)

勉强支持。

- 需要使用 [自定义连接](../详细介绍.html#自定义连接) 的方式来连接；
- WSA 2204 或更高版本（版本号在子系统设置的 \`关于\` 页面中），连接配置选择 \`通用配置\`；
- WSA 2203 或更老版本（版本号在子系统设置页面的上方），连接配置选择 \`WSA 旧版本\`；
- 由于本软件仅对 720p 以上 \`16:9\` 分辨率支持较好，所以请手动拖动窗口大小，尽量贴近 16:9 比例。（如果你的显示器是 16:9 的，可以直接按 \`F11\` 全屏）；
- 任务运行过程中请尽量保证明日方舟在前台且无其他安卓应用同时在前台运行，否则可能导致游戏暂停运行或任务识别错误；
- WSA 的截图经常莫名其妙截出来一个白屏，导致识别异常，还是不推荐使用。
`,
`
### ⚠️ [AVD](https://developer.android.com/studio/run/managing-avds)

理论支持。

- 从 Android 10 开始，Minitouch 在 SELinux 为 \`Enforcing\` 模式时不再可用，请切换至其他触控模式，或将 SELinux **临时**切换为 \`Permissive\` 模式。
- AVD 是为调试而生的，更建议使用其他为游戏而设计的模拟器。
`,
])

const notSupport = shuffleArray([
`
### 🚫 MuMu 手游助手（星云引擎）

不支持，未开放 adb 端口。
`,
`
### 🚫 腾讯手游助手

不支持，未开放 adb 端口。
`,
`
### 🚫 [Google Play 游戏 Beta](https://play.google.com/googleplaygames)

不支持，[玩家客户端](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)未开放 adb 端口。
`,
])

const md = (new MarkdownIt()).use(MarkdownItAnchor, { permalink: MarkdownItAnchor.permalink.linkInsideHeader()})

const fullySupportHtml = md.render(fullySupport.join(''))
const partiallySupportHtml = md.render(particallySupport.join(''))
const notSupportHtml = md.render(notSupport.join(''))

</script>

## ✅ 完美支持

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ 部分支持

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 不支持

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>
