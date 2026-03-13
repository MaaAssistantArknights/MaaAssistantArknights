using System;
using System.Collections.Generic;
using System.Linq;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.App.ViewModels.Settings;

internal static class SettingsOptionCatalog
{
    private static readonly IReadOnlyDictionary<string, string> LanguageDisplayNames =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["zh-cn"] = "简体中文",
            ["zh-tw"] = "繁體中文",
            ["en-us"] = "English",
            ["ja-jp"] = "日本語",
            ["ko-kr"] = "한국어",
            ["pallas"] = "🍻🍻🍻🍻",
        };

    private static readonly IReadOnlyDictionary<string, IReadOnlyDictionary<string, string>> LocalizedTexts =
        new Dictionary<string, IReadOnlyDictionary<string, string>>(StringComparer.OrdinalIgnoreCase)
        {
            ["General"] = BuildText("通用模式", "General Mode", "通用模式", "一般モード", "일반 모드"),
            ["BlueStacks"] = BuildText("蓝叠模拟器", "BlueStacks", "藍疊模擬器", "BlueStacks", "BlueStacks"),
            ["MuMuEmulator12"] = BuildText("MuMu 模拟器", "MuMu Emulator 12", "MuMu 模擬器 12", "MuMu Player", "MuMu Player"),
            ["LDPlayer"] = BuildText("雷电模拟器", "LD Player", "雷電模擬器", "LDPlayer", "LD Player"),
            ["Nox"] = BuildText("夜神模拟器", "Nox", "夜神模擬器", "NoxPlayer", "Nox"),
            ["XYAZ"] = BuildText("逍遥模拟器", "MEmu", "逍遙模擬器", "MEmu", "MEmu"),
            ["PC"] = BuildText("PC 端", "PC Client", "PC 端", "PC クライアント", "PC 클라이언트"),
            ["WSA"] = BuildText("WSA 旧版本", "Old version of WSA", "WSA 舊版本", "古いバージョンのWSA", "WSA 레거시 버전"),
            ["Compatible"] = BuildText("兼容模式", "Compatible Mode", "相容模式", "互換モード", "호환 모드"),
            ["SecondResolution"] = BuildText("第二分辨率", "2nd Resolution", "第二解析度", "2番目の決議", "분할 화면"),
            ["GeneralWithoutScreencapErr"] = BuildText(
                "通用模式（屏蔽异常输出）",
                "General Mode (Blocked exception output)",
                "通用模式（阻擋異常輸出）",
                "一般モード（ブロックされた例外出力）",
                "일반 모드 (예외 출력 차단)"),

            ["Official"] = BuildText("官服", "Official (CN)", "官服", "CN-公式", "중국 (Official)"),
            ["Bilibili"] = BuildText("Bilibili服", "Bilibili", "Bilibili 服", "Bilibili", "중국 (Bilibili)"),
            ["YoStarEN"] = BuildText("国际服 (YostarEN)", "YostarEN", "國際服 (YostarEN)", "YostarEN", "글로벌 (Yostar)"),
            ["YoStarJP"] = BuildText("日服 (YostarJP)", "YostarJP", "日服 (YostarJP)", "YostarJP", "일본 (Yostar)"),
            ["YoStarKR"] = BuildText("韩服 (YostarKR)", "YostarKR", "韓服 (YostarKR)", "YostarKR", "한국 (Yostar)"),
            ["Txwy"] = BuildText("繁中服 (txwy)", "txwy", "繁中服 (txwy)", "txwy", "TXWY (TW)"),

            ["MiniTouchMode"] = BuildText("Minitouch（默认）", "Minitouch (Default)", "Minitouch（預設）", "Minitouch (標準)", "Minitouch (기본값)"),
            ["MaaTouchMode"] = BuildText("MaaTouch（实验功能）", "MaaTouch (Experimental)", "MaaTouch（實驗功能）", "MaaTouch (実験機能)", "MaaTouch (실험적)"),
            ["AdbTouchMode"] = BuildText("ADB Input（不推荐使用）", "ADB Input (Deprecated)", "ADB Input（不推薦使用）", "ADB Input (非推奨)", "ADB Input (비권장)"),
            ["AttachWindowScreencapFramePool"] = BuildText("FramePool（默认，后台）", "FramePool (Default, Background)", "FramePool（預設，背景）", "FramePool（既定、バックグラウンド）", "FramePool (기본, 백그라운드)"),
            ["AttachWindowScreencapPrintWindow"] = BuildText("PrintWindow（后台，备选 1）", "PrintWindow (Background, Backup 1)", "PrintWindow（背景，備選 1）", "PrintWindow（バックグラウンド、予備 1）", "PrintWindow (백그라운드, 대체 1)"),
            ["AttachWindowScreencapScreenDC"] = BuildText("ScreenDC（后台，备选 2）", "ScreenDC (Background, Backup 2)", "ScreenDC（背景，備選 2）", "ScreenDC（バックグラウンド、予備 2）", "ScreenDC (백그라운드, 대체 2)"),
            ["AttachWindowScreencapDesktopDupWindow"] = BuildText("DesktopWindow（前台，更稳定）", "DesktopWindow (Foreground, More Stable)", "DesktopWindow（前景，更穩定）", "DesktopWindow（フォアグラウンド、安定）", "DesktopWindow (포그라운드, 더 안정적)"),
            ["AttachWindowInputSeize"] = BuildText("Seize（前台，更稳定）", "Seize (Foreground, More Stable)", "Seize（前景，更穩定）", "Seize（フォアグラウンド、安定）", "Seize (포그라운드, 더 안정적)"),
            ["AttachWindowInputPostWithCursor"] = BuildText("PostMessageWithCursor（半后台）", "PostMessageWithCursor (Semi-background)", "PostMessageWithCursor（半背景）", "PostMessageWithCursor（半バックグラウンド）", "PostMessageWithCursor (준 백그라운드)"),
            ["AttachWindowInputSendWithCursor"] = BuildText("SendMessageWithCursor（半后台，备选）", "SendMessageWithCursor (Semi-background, Backup)", "SendMessageWithCursor（半背景，備選）", "SendMessageWithCursor（半バックグラウンド、予備）", "SendMessageWithCursor (준 백그라운드, 대체)"),
            ["AttachWindowInputPostWithWindowPos"] = BuildText("PostMessageWithWindowPos（后台窗口）", "PostMessageWithWindowPos (Background Window)", "PostMessageWithWindowPos（背景視窗）", "PostMessageWithWindowPos（バックグラウンドウィンドウ）", "PostMessageWithWindowPos (백그라운드 창)"),
            ["AttachWindowInputSendWithWindowPos"] = BuildText("SendMessageWithWindowPos（后台窗口，备选）", "SendMessageWithWindowPos (Background Window, Backup)", "SendMessageWithWindowPos（背景視窗，備選）", "SendMessageWithWindowPos（バックグラウンドウィンドウ、予備）", "SendMessageWithWindowPos (백그라운드 창, 대체)"),

            ["Light"] = BuildText("亮色", "Light", "淺色模式", "ライトモード", "라이트"),
            ["Dark"] = BuildText("暗色", "Dark", "深色模式", "ダークモード", "다크"),
            ["SyncWithOs"] = BuildText("与系统同步", "Sync with OS", "與系統同步", "OSと同期する", "시스템 연동"),

            ["BackgroundImageStretchModeNone"] = BuildText("无拉伸", "None", "保持原比例", "伸縮なし", "늘리지 않음"),
            ["BackgroundImageStretchModeFill"] = BuildText("拉伸填充", "Fill", "非等比例填滿", "引き伸ばし（フィル）", "늘려 채움"),
            ["BackgroundImageStretchModeUniform"] = BuildText("等比适应", "Uniform (Fit)", "等比例縮放", "アスペクト比保持（フィット）", "비율 유지 (맞춤)"),
            ["BackgroundImageStretchModeUniformToFill"] = BuildText("等比填充（裁剪）", "Uniform to Fill (Cover)", "等比例填滿（裁剪）", "アスペクト比保持（トリミング）", "비율 유지 (자르기)"),

            ["UpdateCheckNightly"] = BuildText("内测版", "Nightly Release", "內測版", "内部テスト版", "개발 버전"),
            ["UpdateCheckBeta"] = BuildText("公测版", "Beta Release", "公測版", "ベータ版", "베타 버전"),
            ["UpdateCheckStable"] = BuildText("正式版", "Stable Release", "穩定版", "安定版", "안정 버전"),
            ["GlobalSource"] = BuildText("海外源", "Global Source", "全域來源", "グローバルソース", "글로벌 소스"),
            ["MirrorChyan"] = BuildText("Mirror酱", "MirrorChyan", "MirrorChyan", "MirrorChyan", "MirrorChyan"),

            ["OperNameLanguageMAA"] = BuildText("跟随 MAA", "Follow MAA", "跟隨 MAA", "MAA をフォローする", "MAA 언어 따라가기"),
            ["OperNameLanguageClient"] = BuildText("跟随游戏客户端", "Follow game client", "跟隨遊戲用戶端", "ゲームクライアントをフォローする", "게임 클라이언트 따라가기"),
            ["Clear"] = BuildText("清空", "Clear", "清空", "解除", "모두 해제"),
            ["Inverse"] = BuildText("反选", "Invert", "反向選取", "逆選択", "선택 반전"),
            ["Switchable"] = BuildText("可切换", "Switchable", "可切換", "切り替え可能", "전환 가능"),
        };

    private static readonly IReadOnlyList<string> LogItemDateFormatOptions =
    [
        "HH:mm:ss",
        "MM-dd  HH:mm:ss",
        "MM/dd  HH:mm:ss",
        "MM.dd  HH:mm:ss",
        "dd-MM  HH:mm:ss",
        "dd/MM  HH:mm:ss",
        "dd.MM  HH:mm:ss",
    ];

    public static IReadOnlyList<DisplayValueOption> BuildThemeOptions(string language)
    {
        return
        [
            new DisplayValueOption(GetText("Light", language), "Light"),
            new DisplayValueOption(GetText("Dark", language), "Dark"),
            new DisplayValueOption(GetText("SyncWithOs", language), "SyncWithOs"),
        ];
    }

    public static IReadOnlyList<DisplayValueOption> BuildLanguageOptions()
    {
        return UiLanguageCatalog.Ordered
            .Select(language => new DisplayValueOption(GetLanguageDisplayName(language), language))
            .ToArray();
    }

    public static IReadOnlyList<DisplayValueOption> BuildBackgroundStretchOptions(string language)
    {
        return
        [
            new DisplayValueOption(GetText("BackgroundImageStretchModeNone", language), "None"),
            new DisplayValueOption(GetText("BackgroundImageStretchModeFill", language), "Fill"),
            new DisplayValueOption(GetText("BackgroundImageStretchModeUniform", language), "Uniform"),
            new DisplayValueOption(GetText("BackgroundImageStretchModeUniformToFill", language), "UniformToFill"),
        ];
    }

    public static IReadOnlyList<ConnectionGameOptionItem> BuildConnectConfigOptions(string language)
    {
        return
        [
            new ConnectionGameOptionItem("General", GetText("General", language)),
            new ConnectionGameOptionItem("BlueStacks", GetText("BlueStacks", language)),
            new ConnectionGameOptionItem("MuMuEmulator12", GetText("MuMuEmulator12", language)),
            new ConnectionGameOptionItem("LDPlayer", GetText("LDPlayer", language)),
            new ConnectionGameOptionItem("Nox", GetText("Nox", language)),
            new ConnectionGameOptionItem("XYAZ", GetText("XYAZ", language)),
            new ConnectionGameOptionItem("PC", GetText("PC", language)),
            new ConnectionGameOptionItem("WSA", GetText("WSA", language)),
            new ConnectionGameOptionItem("Compatible", GetText("Compatible", language)),
            new ConnectionGameOptionItem("SecondResolution", GetText("SecondResolution", language)),
            new ConnectionGameOptionItem("GeneralWithoutScreencapErr", GetText("GeneralWithoutScreencapErr", language)),
        ];
    }

    public static IReadOnlyList<ConnectionGameOptionItem> BuildClientTypeOptions(string language)
    {
        return
        [
            new ConnectionGameOptionItem("Official", GetText("Official", language)),
            new ConnectionGameOptionItem("Bilibili", GetText("Bilibili", language)),
            new ConnectionGameOptionItem("YoStarEN", GetText("YoStarEN", language)),
            new ConnectionGameOptionItem("YoStarJP", GetText("YoStarJP", language)),
            new ConnectionGameOptionItem("YoStarKR", GetText("YoStarKR", language)),
            new ConnectionGameOptionItem("txwy", GetText("Txwy", language)),
        ];
    }

    public static IReadOnlyList<ConnectionGameOptionItem> BuildTouchModeOptions(string language)
    {
        return
        [
            new ConnectionGameOptionItem("minitouch", GetText("MiniTouchMode", language)),
            new ConnectionGameOptionItem("maatouch", GetText("MaaTouchMode", language)),
            new ConnectionGameOptionItem("adb", GetText("AdbTouchMode", language)),
        ];
    }

    public static IReadOnlyList<ConnectionGameOptionItem> BuildAttachWindowScreencapOptions(string language)
    {
        return
        [
            new ConnectionGameOptionItem("2", GetText("AttachWindowScreencapFramePool", language)),
            new ConnectionGameOptionItem("16", GetText("AttachWindowScreencapPrintWindow", language)),
            new ConnectionGameOptionItem("32", GetText("AttachWindowScreencapScreenDC", language)),
            new ConnectionGameOptionItem("8", GetText("AttachWindowScreencapDesktopDupWindow", language)),
        ];
    }

    public static IReadOnlyList<ConnectionGameOptionItem> BuildAttachWindowInputOptions(string language)
    {
        return
        [
            new ConnectionGameOptionItem("1", GetText("AttachWindowInputSeize", language)),
            new ConnectionGameOptionItem("64", GetText("AttachWindowInputPostWithCursor", language)),
            new ConnectionGameOptionItem("32", GetText("AttachWindowInputSendWithCursor", language)),
            new ConnectionGameOptionItem("256", GetText("AttachWindowInputPostWithWindowPos", language)),
            new ConnectionGameOptionItem("128", GetText("AttachWindowInputSendWithWindowPos", language)),
        ];
    }

    public static IReadOnlyList<DisplayValueOption> BuildVersionTypeOptions(string language, bool allowNightly)
    {
        var options = new List<DisplayValueOption>();
        if (allowNightly)
        {
            options.Add(new DisplayValueOption(GetText("UpdateCheckNightly", language), "Nightly"));
        }

        options.Add(new DisplayValueOption(GetText("UpdateCheckBeta", language), "Beta"));
        options.Add(new DisplayValueOption(GetText("UpdateCheckStable", language), "Stable"));
        return options;
    }

    public static IReadOnlyList<DisplayValueOption> BuildVersionResourceSourceOptions(string language)
    {
        return
        [
            new DisplayValueOption(GetText("GlobalSource", language), "Github"),
            new DisplayValueOption(GetText("MirrorChyan", language), "MirrorChyan"),
        ];
    }

    public static IReadOnlyList<DisplayValueOption> BuildOperNameLanguageOptions(string language)
    {
        return
        [
            new DisplayValueOption(GetText("OperNameLanguageMAA", language), "OperNameLanguageMAA"),
            new DisplayValueOption(GetText("OperNameLanguageClient", language), "OperNameLanguageClient"),
        ];
    }

    public static IReadOnlyList<DisplayValueOption> BuildInverseClearModeOptions(string language)
    {
        return
        [
            new DisplayValueOption(GetText("Clear", language), "Clear"),
            new DisplayValueOption(GetText("Inverse", language), "Inverse"),
            new DisplayValueOption(GetText("Switchable", language), "ClearInverse"),
        ];
    }

    public static IReadOnlyList<string> GetLogItemDateFormatOptions()
    {
        return LogItemDateFormatOptions;
    }

    public static string GetLanguageDisplayName(string language)
    {
        return LanguageDisplayNames.TryGetValue(language, out var display)
            ? display
            : language;
    }

    public static string GetText(string key, string language)
    {
        if (!LocalizedTexts.TryGetValue(key, out var localized))
        {
            return key;
        }

        var normalizedLanguage = NormalizeLanguageForLookup(language);
        if (localized.TryGetValue(normalizedLanguage, out var text))
        {
            return text;
        }

        if (localized.TryGetValue(UiLanguageCatalog.FallbackLanguage, out var fallback))
        {
            return fallback;
        }

        return localized.Values.FirstOrDefault() ?? key;
    }

    private static IReadOnlyDictionary<string, string> BuildText(
        string zhCn,
        string enUs,
        string zhTw,
        string jaJp,
        string koKr)
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["zh-cn"] = zhCn,
            ["en-us"] = enUs,
            ["zh-tw"] = zhTw,
            ["ja-jp"] = jaJp,
            ["ko-kr"] = koKr,
            ["pallas"] = zhCn,
        };
    }

    private static string NormalizeLanguageForLookup(string language)
    {
        return UiLanguageCatalog.IsSupported(language)
            ? UiLanguageCatalog.Normalize(language)
            : UiLanguageCatalog.FallbackLanguage;
    }
}
