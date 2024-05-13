---
order: 1
icon: ri:windows-fill
---

# Windows Emulator Support TODO

::: tip
If you encounter problems, please refer to the [FAQ](../FAQ.md)
:::

The following emulators are displayed randomly in no particular order

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
### ✅ [Bluestacks 5](https://www.bluestacks.com/)
Fully compatible. Need to turn on \`Allow ADB connections\` in the emulator \`Settings\` - \`Engine Settings\`.

- Recommended to download [Offline Installer](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer) to avoid slow and bundled installation; recommend installing [Android 11](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer#:~:text=To%20install%20BlueStacks%205%20Android%2011) version; to uninstall it, please use the official [ Uninstall Tool](https://support.bluestacks.com/hc/en-us/articles/360057724751-How-to-uninstall-BlueStacks-5-BlueStacks-X-and-BlueStacks-Services-completely-from-your-PC) to get rid of the residue.
- If the adb port number keeps changing irregularly and is different every time you start it, it may be because your computer has [Hyper-V](https://support.bluestacks.com/hc/en-us/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11) enabled. MAA will now try to automatically read the port number within the Blue Stacker emulator configuration file, if this does not work/you have a need to multi-open/have more than one emulator kernel installed, please refer to the [Frequently Asked Questions] (... /FAQ.html#Bluestack emulator port number is different every time you start-hyper-v) to make changes. Since Hyper-V runs as administrator, operations that don't involve adb such as auto-shutdown of the emulator, auto-detect connection, etc. also need to run MAA as administrator.
`,
`
### ✅ [MuMu Emulator 12](https://mumu.163.com/)

Fully compatible, with additional support for the exclusive Extreme Control Mode.

- The 'Exit emulator when done’ function may occasionally be abnormal, if you encounter it, please contact MuMu official for feedback;
- If you are using MuMu 12 version 3.5.4 ~ 3.5.7, please disable the 'Keep alive in the background’ function in MuMu 12 Settings - Others. 'Keep alive while hanging in the background’ (see [Official Announcement](https://mumu.163.com/help/20230802/35047_1102450.html) for details);
- You need to check the port information of the corresponding instance through the ADB button of MuMu 12 Multiple Opener when you open more than one instance, and change the port number of the connection address in MAA \`Settings\` - \`Connection Settings\` to the corresponding port.

#### MuMu Screenshot Enhanced Mode

You need to use the official MuMu 12 3.8.13 and later versions, and close the background live. Ark Edition and International Edition are not supported at the moment.

##### Connection Settings

1. Settings → Connection Settings, tick \`Enable MuMu Screenshot Enhanced Mode\`. 2.

2. \`MuMu Emulator Path\` Fill in the path to the \`MuMuPlayer-12.0\` folder, e.g. \`C:\`Program Files\\Netease\\MuMuPlayer-12.0\`.

3. \`Instance Number\` Fill in the serial number of the corresponding emulator in MuMu Multiplayer, e.g. \`0\` for Main Multiplayer.

4. \`Instance Screen\` Fill in \`0\`.

##### About background keep alive

It is recommended to turn it off, at this time the instance screen is always \`0\`.

When it is on, the order of MuMu emulator tabs should be the serial number of the instance screen, e.g. \`0\` for emulator desktop, \`1\` for Tomorrow's Ark client.

Adaptation for backend live is very imperfect, there are always all kinds of inexplicable problems, it is very not recommended.

`,
`
### ✅ [LDPlayer Emulator](https://www.ldmnq.com/)

Fully compatible.

- Raiden 9 is recommended to use version 9.0.57 and above; Raiden 5 is recommended to use version 5.0.67 and above;
- For versions lower than the above, you need to run \`Forced ADB Replacement\` in \`Settings\` - \`Connection\` in order to use efficient touch modes such as Minitouch, MaaTouch, and so on;
`,
`
### ✅ [Nox Emulator](https://www.yeshen.com/)

Fully compatible, but only tested 7 and 9.
`,
`
### ✅ [MEmu Emulator](https://www.xyaz.cn/)

Fully compatible, but less tested.
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [MuMu Emulator 6](https://mumu.163.com/update/win/)

Support has been dropped since MAA v5.1.0 and NetEase has stopped maintaining it on 15-08-2023.

- No longer support auto-detect connection, need to use generic connection configuration and manually configure adb path and connection address;
- Need to run \`Forced Replacement of ADB\` in \`Settings\` - \`Connection\` to use efficient touch modes such as Minitouch, MaaTouch;
- You need to run MAA with administrator privileges to use the ‘Exit Emulator When Done’ function;
- MuMu 6 default resolution is not supported, you need to change it to \`1280x720\`, \`1920x1080\`, \`2560x1440\` and other 16:9 ratio;
- MuMu 6 multi-open uses the same adb port, so it can't support multi-open MuMu 6.
`,
`
### ⚠️ [Windows Subsystem for Android™️](https://learn.microsoft.com/en-us/windows/android/wsa/)

Support has been dropped since MAA v5.2.0 and will be discontinued by Microsoft on 05-03-2025.

- Requires the use of [custom connection](../details.html#Custom connection) is required;
- WSA 2204 or later (the version number is in the \`About\` page of the subsystem settings), select \`Common Configuration\` for the connection configuration;
- WSA 2203 or older (the version number is at the top of the subsystem setup page), for the connection configuration select \`WSA Older Versions\`;
- Since this software only supports 720p or higher \`16:9\` resolution better, please manually drag the window size as close to the 16:9 ratio as possible. (If your monitor is 16:9, you can press \`F11\` to go full screen);
- Please try to make sure that Tomorrow's Ark is in the foreground and no other Android apps are running in the foreground at the same time, otherwise it may cause the game to pause or the task recognition error;
- WSA's screenshots often somehow capture a white screen, resulting in recognition of abnormalities, or not recommended to use.
`,
`
### ⚠️ [AVD](https://developer.android.com/studio/run/managing-avds)

Theoretical support.

- Starting from Android 10, Minitouch is no longer available when SELinux is in \`Enforcing\` mode, please switch to other touch modes, or switch SELinux **temporary** to \`Permissive\` mode.
- AVD is made for debugging, it is more recommended to use other emulators designed for gaming.
`,
])

const notSupport = shuffleArray([
`
### 🚫 MuMu Mobile Assistant (Nebula Engine)

Not supported, adb port is not open.
`,
`
### 🚫 Tencent Handy Game Assistant

Not supported, adb port is not open.
`,
`
### 🚫 [Google Play Games Beta](https://play.google.com/googleplaygames)

Not supported, [Player Client](https://developer.android.com/games/playgames/pg-emulator#installing-game-consumer) adb port is not open.
`,
])

const md = (new MarkdownIt()).use(MarkdownItAnchor, { permalink: MarkdownItAnchor.permalink.linkInsideHeader()})

const fullySupportHtml = md.render(fullySupport.join(''))
const partiallySupportHtml = md.render(particallySupport.join(''))
const notSupportHtml = md.render(notSupport.join(''))

</script>

## ✅ Perfectly supported

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ Partially supported

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 Unsupported

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>
