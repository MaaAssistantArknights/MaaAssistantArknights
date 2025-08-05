---
order: 1
icon: ri:windows-fill
---

# Windows Emulators

The following emulators are displayed in random order with no particular ranking.

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
    {
        name: 'BlueStacks 5',
        link: 'https://www.bluestacks.com/',
        note: 'Fully compatible. Need to turn on ADB Connectivity in the emulator `Settings` → `Advanced` → `Android Debug Bridge` → `Enable`. Known to be compatible with Hyper-V.\n\n- Recommended to download [Offline Installer](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer) to avoid slow and bundled installation; recommend installing [Android 11](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer#:~:text=To%20install%20BlueStacks%205%20Android%2011) version; to uninstall it, please use the official [Uninstall Tool](https://support.bluestacks.com/hc/en-us/articles/360057724751-How-to-uninstall-BlueStacks-5-BlueStacks-X-and-BlueStacks-Services-completely-from-your-PC) to get rid of residues.\n- If the ADB port number keeps changing irregularly and is different every time you start it, it may be because your computer has [Hyper-V](https://support.bluestacks.com/hc/en-us/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11) enabled. MAA will now try to automatically read the port number from the BlueStacks emulator configuration file. If this doesn\'t work, or you need to use multiple instances, or have installed multiple emulator cores, please refer to [Connection Settings](../connection.html#bluestacks-emulator-hyper-v-port-number-changes-every-startup) to make adjustments. Since Hyper-V runs as administrator, operations that don\'t involve ADB such as automatic emulator shutdown or connection detection also need MAA to run as administrator.',
    },
    {
        name: 'MuMu Emulator 12',
        link: 'https://www.mumuglobal.com/',
        note: 'Fully compatible, with additional support for [Screenshot Enhanced Mode](../connection.html#mumu-screenshot-enhanced-mode). Known to be compatible with Hyper-V.\n\n- The "Exit emulator when done" function may occasionally have issues. If you encounter problems, please report them to MuMu\'s official support.\n- When running multiple instances, you need to check the port information of each instance through the ADB button in MuMu 12 Multi-Instance Manager, then change the port number in MAA `Settings` - `Connection Settings` to match the corresponding port.',
    },
    {
        name: 'LDPlayer',
        link: 'https://www.ldplayer.net/',
        note: 'Fully compatible, with additional support for [Screenshot Enhanced Mode](../connection.html#ld-screenshot-enhanced-mode). Known to be compatible with Hyper-V.\n\n- LDPlayer 9 installer will automatically and silently disable Hyper-V during the installation process. Please be aware of this if you need Hyper-V for other purposes.',
    },
    {
        name: 'Nox Player',
        link: 'https://www.bignox.com/',
        note: 'Fully compatible, but less thoroughly tested. Known to be compatible with Hyper-V.',
    },
    {
        name: 'MEmu Play',
        link: 'https://www.memuplay.com/',
        note: 'Fully compatible, but less thoroughly tested.',
    },
]);

const partiallySupport = shuffleArray([
    {
        name: 'MuMu Emulator 6',
        link: 'https://mumu.163.com/update/win/',
        note: 'Support has been dropped since MAA v5.1.0, and NetEase stopped maintaining it on August 15, 2023.\n\n- No longer supports automatic connection detection. You need to use general connection configuration and manually configure ADB path and connection address.\n- You need to run `Force Replace ADB` in `Settings` - `Connection Settings` to use efficient touch modes like Minitouch and MaaTouch.\n- You need to run MAA with administrator privileges to use the "Exit Emulator When Done" function.\n- MuMu 6\'s unusual default resolutions are not supported. You need to change it to standard 16:9 ratios like `1280x720` or `1920x1080`.\n- MuMu 6 multi-instance uses the same ADB port for all instances, so MAA cannot support multiple MuMu 6 instances.',
    },
    {
        name: 'Windows Subsystem for Android™',
        link: 'https://learn.microsoft.com/en-us/windows/android/wsa/',
        note: 'Support has been dropped since MAA v5.2.0, and Microsoft will discontinue it on March 5, 2025.\n\n- Requires using [custom connection](../connection.html) method.\n- For WSA 2204 or later versions (version number can be found in subsystem settings under `About`), select `General Configuration` for connection configuration.\n- For WSA 2203 or older versions (version number appears at the top of subsystem settings page), select `WSA Older Versions` for connection configuration.\n- Since MAA only properly supports `16:9` resolutions of 720p or higher, please manually adjust the window size to be as close to 16:9 ratio as possible. (If your monitor is 16:9, you can press `F11` for fullscreen).\n- During task execution, try to ensure Arknights remains in the foreground with no other Android apps running simultaneously, otherwise the game may pause or task recognition may fail.\n- WSA sometimes inexplicably captures white screens during screenshots, causing recognition issues. Not recommended for use.',
    },
    {
        name: 'AVD',
        link: 'https://developer.android.com/studio/run/managing-avds',
        note: 'Theoretically supported.\n\n- Starting from Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode. Please switch to other touch modes, or **temporarily** switch SELinux to `Permissive` mode.\n- AVD is designed for development debugging, not gaming. Other emulators specifically designed for gaming are recommended instead.',
    },
    {
        name: 'Google Play Games (Developer)',
        link: 'https://developer.android.com/games/playgames/emulator',
        note: 'Theoretically supported. Requires Hyper-V to be enabled and a Google account to be logged in.\n\n- You need to use [custom connection](../connection.html) to connect, with ADB port `6520`.\n- Due to SELinux policies in Android 10 and later, Minitouch cannot work properly. Please switch to other touch modes.\n- The first connection attempt after each emulator startup will fail. Enable `Attempt to kill and restart ADB process after connection failure`.',
    },
]);

const notSupport = shuffleArray([
    {
        name: 'Google Play Games',
        link: 'https://play.google.com/googleplaygames',
        note: 'Not supported. The [Consumer Client](https://developer.android.com/games/playgames/emulator#installing-game-consumer) cannot connect via ADB.',
    },
]);

const md = new MarkdownIt();
md.use(MarkdownItAnchor);

const fullySupportHtml = md.render(fullySupport.map(simulator => `
### ✅ ${simulator.link ? `[${simulator.name}](${simulator.link})` : simulator.name}
${simulator.note}
`).join(''));
const partiallySupportHtml = md.render(partiallySupport.map(simulator => `
### ⚠️ ${simulator.link ? `[${simulator.name}](${simulator.link})` : simulator.name}
${simulator.note}
`).join(''));
const notSupportHtml = md.render(notSupport.map(simulator => `
### 🚫 ${simulator.link ? `[${simulator.name}](${simulator.link})` : simulator.name}
${simulator.note}
`).join(''));
</script>

## ✅ Fully supported

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ Partially supported

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 Unsupported

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>
