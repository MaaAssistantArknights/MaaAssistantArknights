using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Infrastructure;

public static class PlatformCapabilityTextMap
{
    private static readonly Dictionary<string, Dictionary<string, string>> Map = new(StringComparer.OrdinalIgnoreCase)
    {
        ["zh-cn"] = BuildZhCn(),
        ["en-us"] = BuildEnUs(),
        ["ja-jp"] = BuildJaJp(),
        ["ko-kr"] = BuildKoKr(),
        ["zh-tw"] = BuildZhTw(),
    };

    public static string FormatCapabilityLine(string language, string name, PlatformCapabilityStatus status)
    {
        var mode = status.Supported
            ? Get(language, "Status.Supported")
            : Get(language, "Status.Fallback");
        var fallback = status.HasFallback && !string.IsNullOrWhiteSpace(status.FallbackMode)
            ? string.Format(Get(language, "Capability.FallbackSuffix"), status.FallbackMode)
            : string.Empty;
        return string.Format(
            Get(language, "Capability.Line"),
            name,
            mode,
            status.Provider,
            fallback);
    }

    public static string FormatCapabilityLine(string language, PlatformCapabilityId capability, PlatformCapabilityStatus status)
    {
        return FormatCapabilityLine(language, GetCapabilityName(language, capability), status);
    }

    public static string FormatSnapshotUnavailable(string language, string message)
    {
        return string.Format(Get(language, "Snapshot.Unavailable"), message);
    }

    public static string FormatAutostartStatus(string language, bool enabled)
    {
        return enabled
            ? Get(language, "Autostart.Enabled")
            : Get(language, "Autostart.Disabled");
    }

    public static string FormatErrorCode(string language, string? errorCode, string fallbackMessage)
    {
        if (string.IsNullOrWhiteSpace(errorCode))
        {
            return fallbackMessage;
        }

        var key = $"Error.{errorCode}";
        var localized = Get(language, key);
        if (string.Equals(localized, key, StringComparison.Ordinal))
        {
            return fallbackMessage;
        }

        return localized;
    }

    public static string GetCapabilityName(string language, PlatformCapabilityId capability)
    {
        var key = capability switch
        {
            PlatformCapabilityId.Tray => "CapabilityName.Tray",
            PlatformCapabilityId.Notification => "CapabilityName.Notification",
            PlatformCapabilityId.Hotkey => "CapabilityName.Hotkey",
            PlatformCapabilityId.Autostart => "CapabilityName.Autostart",
            PlatformCapabilityId.Overlay => "CapabilityName.Overlay",
            _ => "CapabilityName.Unknown",
        };
        return Get(language, key);
    }

    public static TrayMenuText CreateTrayMenuText(string language)
    {
        return new TrayMenuText(
            Start: Get(language, "TrayMenu.Start"),
            Stop: Get(language, "TrayMenu.Stop"),
            ForceShow: Get(language, "TrayMenu.ForceShow"),
            HideTray: Get(language, "TrayMenu.HideTray"),
            ToggleOverlay: Get(language, "TrayMenu.ToggleOverlay"),
            Exit: Get(language, "TrayMenu.Exit"));
    }

    public static string GetUiText(string language, string key, string fallback)
    {
        var value = Get(language, key);
        return string.Equals(value, key, StringComparison.Ordinal) ? fallback : value;
    }

    private static string Get(string language, string key)
    {
        if (!Map.TryGetValue(language ?? string.Empty, out var langMap))
        {
            langMap = Map["en-us"];
        }

        if (langMap.TryGetValue(key, out var value))
        {
            return value;
        }

        if (Map["en-us"].TryGetValue(key, out value))
        {
            return value;
        }

        return key;
    }

    private static Dictionary<string, string> BuildZhCn()
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Status.Supported"] = "已支持",
            ["Status.Fallback"] = "已降级",
            ["Status.Failed"] = "失败",
            ["Snapshot.Unavailable"] = "能力快照不可用: {0}",
            ["Capability.FallbackSuffix"] = ", 降级={0}",
            ["Capability.Line"] = "{0}: {1} (provider={2}{3})",
            ["CapabilityName.Tray"] = "托盘",
            ["CapabilityName.Notification"] = "通知",
            ["CapabilityName.Hotkey"] = "全局热键",
            ["CapabilityName.Autostart"] = "自启动",
            ["CapabilityName.Overlay"] = "Overlay",
            ["CapabilityName.Unknown"] = "未知能力",
            ["TrayMenu.Start"] = "开始",
            ["TrayMenu.Stop"] = "停止",
            ["TrayMenu.ForceShow"] = "显示主窗口",
            ["TrayMenu.HideTray"] = "隐藏托盘",
            ["TrayMenu.ToggleOverlay"] = "切换 Overlay",
            ["TrayMenu.Exit"] = "退出",
            ["Ui.OverlayHostUnavailable"] = "Overlay 宿主句柄不可用",
            ["Ui.UnknownTrayCommand"] = "未知托盘命令: {0}",
            ["Autostart.Enabled"] = "开机启动：已启用",
            ["Autostart.Disabled"] = "开机启动：未启用",
            ["Error.TrayUnsupported"] = "当前环境不支持托盘",
            ["Error.TrayFallback"] = "托盘已降级到窗口菜单",
            ["Error.TrayInitFailed"] = "托盘初始化失败",
            ["Error.TrayMenuDispatchFailed"] = "托盘菜单事件派发失败",
            ["Error.TrayNotInitialized"] = "托盘尚未初始化",
            ["Error.NotificationUnsupported"] = "当前平台不支持系统通知",
            ["Error.NotificationFallback"] = "系统通知不可用，已降级",
            ["Error.NotificationSendFailed"] = "通知发送失败",
            ["Error.HotkeyUnsupported"] = "当前环境不支持全局热键",
            ["Error.HotkeyFallback"] = "热键已降级为窗口级",
            ["Error.HotkeyNameMissing"] = "热键名称为空",
            ["Error.HotkeyInvalidGesture"] = "热键格式非法",
            ["Error.HotkeyConflict"] = "热键冲突，已被占用",
            ["Error.HotkeyNotFound"] = "热键不存在",
            ["Error.HotkeyPermissionDenied"] = "热键权限不足",
            ["Error.HotkeyHookStartFailed"] = "全局热键钩子启动失败",
            ["Error.HotkeyTriggerDispatchFailed"] = "热键触发派发失败",
            ["Error.AutostartUnsupported"] = "当前平台不支持自启动",
            ["Error.AutostartQueryFailed"] = "查询自启动状态失败",
            ["Error.AutostartSetFailed"] = "设置自启动失败",
            ["Error.AutostartVerificationFailed"] = "自启动状态校验失败",
            ["Error.AutostartExecutableMissing"] = "找不到可执行文件，无法设置自启动",
            ["Error.OverlayUnsupported"] = "当前环境不支持 Overlay 附着",
            ["Error.OverlayPreviewMode"] = "Overlay 已切换为预览模式",
            ["Error.OverlayTargetInvalid"] = "Overlay 目标无效",
            ["Error.OverlayTargetGone"] = "Overlay 目标已失效",
            ["Error.OverlayQueryFailed"] = "Overlay 目标查询失败",
            ["Error.OverlayHostNotBound"] = "Overlay 宿主窗口未绑定",
            ["Error.OverlayAttachFailed"] = "Overlay 附着失败",
            ["Error.PostActionUnsupported"] = "后置动作不受支持，已降级为日志",
            ["Error.PostActionExecutionFailed"] = "后置动作执行失败",
            ["Error.PostActionPowerActionsDisabled"] = "系统电源动作已禁用",
        };
    }

    private static Dictionary<string, string> BuildEnUs()
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Status.Supported"] = "Supported",
            ["Status.Fallback"] = "Fallback",
            ["Status.Failed"] = "Failed",
            ["Snapshot.Unavailable"] = "Capability snapshot unavailable: {0}",
            ["Capability.FallbackSuffix"] = ", fallback={0}",
            ["Capability.Line"] = "{0}: {1} (provider={2}{3})",
            ["CapabilityName.Tray"] = "Tray",
            ["CapabilityName.Notification"] = "Notification",
            ["CapabilityName.Hotkey"] = "Global Hotkey",
            ["CapabilityName.Autostart"] = "Autostart",
            ["CapabilityName.Overlay"] = "Overlay",
            ["CapabilityName.Unknown"] = "Unknown Capability",
            ["TrayMenu.Start"] = "Start",
            ["TrayMenu.Stop"] = "Stop",
            ["TrayMenu.ForceShow"] = "Force Show",
            ["TrayMenu.HideTray"] = "Hide Tray",
            ["TrayMenu.ToggleOverlay"] = "Toggle Overlay",
            ["TrayMenu.Exit"] = "Exit",
            ["Ui.OverlayHostUnavailable"] = "Overlay host handle is unavailable.",
            ["Ui.UnknownTrayCommand"] = "Unknown tray command: {0}",
            ["Autostart.Enabled"] = "Autostart: Enabled",
            ["Autostart.Disabled"] = "Autostart: Disabled",
            ["Error.TrayUnsupported"] = "Tray is unsupported in this environment",
            ["Error.TrayFallback"] = "Tray degraded to window menu",
            ["Error.TrayInitFailed"] = "Tray initialization failed",
            ["Error.TrayMenuDispatchFailed"] = "Tray command dispatch failed",
            ["Error.TrayNotInitialized"] = "Tray is not initialized",
            ["Error.NotificationUnsupported"] = "System notification is unsupported",
            ["Error.NotificationFallback"] = "System notification degraded to fallback",
            ["Error.NotificationSendFailed"] = "Notification send failed",
            ["Error.HotkeyUnsupported"] = "Global hotkey is unsupported in this environment",
            ["Error.HotkeyFallback"] = "Hotkey degraded to window-scoped mode",
            ["Error.HotkeyNameMissing"] = "Hotkey name is missing",
            ["Error.HotkeyInvalidGesture"] = "Invalid hotkey gesture",
            ["Error.HotkeyConflict"] = "Hotkey conflict",
            ["Error.HotkeyNotFound"] = "Hotkey not found",
            ["Error.HotkeyPermissionDenied"] = "Hotkey permission denied",
            ["Error.HotkeyHookStartFailed"] = "Global hotkey hook failed to start",
            ["Error.HotkeyTriggerDispatchFailed"] = "Hotkey trigger dispatch failed",
            ["Error.AutostartUnsupported"] = "Autostart is unsupported on this platform",
            ["Error.AutostartQueryFailed"] = "Failed to query autostart status",
            ["Error.AutostartSetFailed"] = "Failed to set autostart",
            ["Error.AutostartVerificationFailed"] = "Autostart verification failed",
            ["Error.AutostartExecutableMissing"] = "Executable path is unavailable for autostart",
            ["Error.OverlayUnsupported"] = "Overlay attachment is unsupported",
            ["Error.OverlayPreviewMode"] = "Overlay switched to preview mode",
            ["Error.OverlayTargetInvalid"] = "Overlay target is invalid",
            ["Error.OverlayTargetGone"] = "Overlay target is unavailable",
            ["Error.OverlayQueryFailed"] = "Failed to query overlay targets",
            ["Error.OverlayHostNotBound"] = "Overlay host window not bound",
            ["Error.OverlayAttachFailed"] = "Overlay attach failed",
            ["Error.PostActionUnsupported"] = "Post action is unsupported and downgraded to logging",
            ["Error.PostActionExecutionFailed"] = "Post action execution failed",
            ["Error.PostActionPowerActionsDisabled"] = "Power actions are disabled",
        };
    }

    private static Dictionary<string, string> BuildJaJp()
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Status.Supported"] = "対応",
            ["Status.Fallback"] = "フォールバック",
            ["Status.Failed"] = "失敗",
            ["Snapshot.Unavailable"] = "機能スナップショットを取得できません: {0}",
            ["Capability.FallbackSuffix"] = ", フォールバック={0}",
            ["Capability.Line"] = "{0}: {1} (provider={2}{3})",
            ["CapabilityName.Tray"] = "トレイ",
            ["CapabilityName.Notification"] = "通知",
            ["CapabilityName.Hotkey"] = "グローバルホットキー",
            ["CapabilityName.Autostart"] = "自動起動",
            ["CapabilityName.Overlay"] = "Overlay",
            ["CapabilityName.Unknown"] = "不明な機能",
            ["TrayMenu.Start"] = "開始",
            ["TrayMenu.Stop"] = "停止",
            ["TrayMenu.ForceShow"] = "メイン画面を表示",
            ["TrayMenu.HideTray"] = "トレイを隠す",
            ["TrayMenu.ToggleOverlay"] = "Overlay 切替",
            ["TrayMenu.Exit"] = "終了",
            ["Ui.OverlayHostUnavailable"] = "Overlay ホストハンドルを取得できません。",
            ["Ui.UnknownTrayCommand"] = "不明なトレイコマンド: {0}",
            ["Autostart.Enabled"] = "自動起動: 有効",
            ["Autostart.Disabled"] = "自動起動: 無効",
            ["Error.TrayUnsupported"] = "この環境ではトレイがサポートされていません",
            ["Error.TrayFallback"] = "トレイはウィンドウメニューにフォールバックしました",
            ["Error.TrayInitFailed"] = "トレイの初期化に失敗しました",
            ["Error.TrayMenuDispatchFailed"] = "トレイコマンドの配信に失敗しました",
            ["Error.TrayNotInitialized"] = "トレイが初期化されていません",
            ["Error.NotificationUnsupported"] = "この環境では通知がサポートされていません",
            ["Error.NotificationFallback"] = "通知はフォールバック経路に切り替わりました",
            ["Error.NotificationSendFailed"] = "通知の送信に失敗しました",
            ["Error.HotkeyUnsupported"] = "この環境ではグローバルホットキーがサポートされていません",
            ["Error.HotkeyFallback"] = "ホットキーはウィンドウ内モードにフォールバックしました",
            ["Error.HotkeyNameMissing"] = "ホットキー名が未指定です",
            ["Error.HotkeyInvalidGesture"] = "ホットキー形式が不正です",
            ["Error.HotkeyConflict"] = "ホットキーが競合しています",
            ["Error.HotkeyNotFound"] = "ホットキーが見つかりません",
            ["Error.HotkeyPermissionDenied"] = "ホットキーの権限が不足しています",
            ["Error.HotkeyHookStartFailed"] = "グローバルホットキーの開始に失敗しました",
            ["Error.HotkeyTriggerDispatchFailed"] = "ホットキートリガーの配信に失敗しました",
            ["Error.AutostartUnsupported"] = "このプラットフォームでは自動起動に非対応です",
            ["Error.AutostartQueryFailed"] = "自動起動状態の取得に失敗しました",
            ["Error.AutostartSetFailed"] = "自動起動設定の更新に失敗しました",
            ["Error.AutostartVerificationFailed"] = "自動起動の検証に失敗しました",
            ["Error.AutostartExecutableMissing"] = "自動起動に必要な実行ファイルが見つかりません",
            ["Error.OverlayUnsupported"] = "この環境では Overlay がサポートされていません",
            ["Error.OverlayPreviewMode"] = "Overlay はプレビューモードに切り替わりました",
            ["Error.OverlayTargetInvalid"] = "Overlay ターゲットが不正です",
            ["Error.OverlayTargetGone"] = "Overlay ターゲットが利用できません",
            ["Error.OverlayQueryFailed"] = "Overlay ターゲットの取得に失敗しました",
            ["Error.OverlayHostNotBound"] = "Overlay ホストウィンドウが未バインドです",
            ["Error.OverlayAttachFailed"] = "Overlay のアタッチに失敗しました",
            ["Error.PostActionUnsupported"] = "後処理は非対応のためログ動作にフォールバックしました",
            ["Error.PostActionExecutionFailed"] = "後処理の実行に失敗しました",
            ["Error.PostActionPowerActionsDisabled"] = "電源アクションは無効化されています",
        };
    }

    private static Dictionary<string, string> BuildKoKr()
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Status.Supported"] = "지원됨",
            ["Status.Fallback"] = "폴백",
            ["Status.Failed"] = "실패",
            ["Snapshot.Unavailable"] = "기능 스냅샷을 가져올 수 없음: {0}",
            ["Capability.FallbackSuffix"] = ", 폴백={0}",
            ["Capability.Line"] = "{0}: {1} (provider={2}{3})",
            ["CapabilityName.Tray"] = "트레이",
            ["CapabilityName.Notification"] = "알림",
            ["CapabilityName.Hotkey"] = "전역 단축키",
            ["CapabilityName.Autostart"] = "자동 시작",
            ["CapabilityName.Overlay"] = "Overlay",
            ["CapabilityName.Unknown"] = "알 수 없는 기능",
            ["TrayMenu.Start"] = "시작",
            ["TrayMenu.Stop"] = "중지",
            ["TrayMenu.ForceShow"] = "메인 창 표시",
            ["TrayMenu.HideTray"] = "트레이 숨기기",
            ["TrayMenu.ToggleOverlay"] = "Overlay 전환",
            ["TrayMenu.Exit"] = "종료",
            ["Ui.OverlayHostUnavailable"] = "Overlay 호스트 핸들을 사용할 수 없습니다.",
            ["Ui.UnknownTrayCommand"] = "알 수 없는 트레이 명령: {0}",
            ["Autostart.Enabled"] = "자동 시작: 사용",
            ["Autostart.Disabled"] = "자동 시작: 사용 안 함",
            ["Error.TrayUnsupported"] = "현재 환경에서는 트레이를 지원하지 않습니다",
            ["Error.TrayFallback"] = "트레이가 창 메뉴 폴백으로 전환되었습니다",
            ["Error.TrayInitFailed"] = "트레이 초기화 실패",
            ["Error.TrayMenuDispatchFailed"] = "트레이 명령 전달 실패",
            ["Error.TrayNotInitialized"] = "트레이가 초기화되지 않았습니다",
            ["Error.NotificationUnsupported"] = "시스템 알림을 지원하지 않습니다",
            ["Error.NotificationFallback"] = "시스템 알림이 폴백으로 전환되었습니다",
            ["Error.NotificationSendFailed"] = "알림 전송 실패",
            ["Error.HotkeyUnsupported"] = "현재 환경에서는 전역 단축키를 지원하지 않습니다",
            ["Error.HotkeyFallback"] = "단축키가 창 범위 모드로 폴백되었습니다",
            ["Error.HotkeyNameMissing"] = "단축키 이름이 비어 있습니다",
            ["Error.HotkeyInvalidGesture"] = "단축키 형식이 잘못되었습니다",
            ["Error.HotkeyConflict"] = "단축키 충돌",
            ["Error.HotkeyNotFound"] = "단축키를 찾을 수 없습니다",
            ["Error.HotkeyPermissionDenied"] = "단축키 권한 부족",
            ["Error.HotkeyHookStartFailed"] = "전역 단축키 훅 시작 실패",
            ["Error.HotkeyTriggerDispatchFailed"] = "단축키 트리거 전달 실패",
            ["Error.AutostartUnsupported"] = "이 플랫폼에서는 자동 시작을 지원하지 않습니다",
            ["Error.AutostartQueryFailed"] = "자동 시작 상태 조회 실패",
            ["Error.AutostartSetFailed"] = "자동 시작 설정 실패",
            ["Error.AutostartVerificationFailed"] = "자동 시작 검증 실패",
            ["Error.AutostartExecutableMissing"] = "자동 시작 설정에 필요한 실행 파일이 없습니다",
            ["Error.OverlayUnsupported"] = "현재 환경에서는 Overlay를 지원하지 않습니다",
            ["Error.OverlayPreviewMode"] = "Overlay가 미리보기 모드로 전환되었습니다",
            ["Error.OverlayTargetInvalid"] = "Overlay 대상이 잘못되었습니다",
            ["Error.OverlayTargetGone"] = "Overlay 대상을 사용할 수 없습니다",
            ["Error.OverlayQueryFailed"] = "Overlay 대상 조회 실패",
            ["Error.OverlayHostNotBound"] = "Overlay 호스트 창이 바인딩되지 않았습니다",
            ["Error.OverlayAttachFailed"] = "Overlay 부착 실패",
            ["Error.PostActionUnsupported"] = "후처리 동작이 지원되지 않아 로그로 폴백되었습니다",
            ["Error.PostActionExecutionFailed"] = "후처리 동작 실행 실패",
            ["Error.PostActionPowerActionsDisabled"] = "전원 동작이 비활성화되어 있습니다",
        };
    }

    private static Dictionary<string, string> BuildZhTw()
    {
        return new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Status.Supported"] = "已支援",
            ["Status.Fallback"] = "已降級",
            ["Status.Failed"] = "失敗",
            ["Snapshot.Unavailable"] = "能力快照不可用: {0}",
            ["Capability.FallbackSuffix"] = ", 降級={0}",
            ["Capability.Line"] = "{0}: {1} (provider={2}{3})",
            ["CapabilityName.Tray"] = "托盤",
            ["CapabilityName.Notification"] = "通知",
            ["CapabilityName.Hotkey"] = "全域熱鍵",
            ["CapabilityName.Autostart"] = "自啟動",
            ["CapabilityName.Overlay"] = "Overlay",
            ["CapabilityName.Unknown"] = "未知能力",
            ["TrayMenu.Start"] = "開始",
            ["TrayMenu.Stop"] = "停止",
            ["TrayMenu.ForceShow"] = "顯示主視窗",
            ["TrayMenu.HideTray"] = "隱藏托盤",
            ["TrayMenu.ToggleOverlay"] = "切換 Overlay",
            ["TrayMenu.Exit"] = "退出",
            ["Ui.OverlayHostUnavailable"] = "Overlay 宿主句柄不可用",
            ["Ui.UnknownTrayCommand"] = "未知托盤命令: {0}",
            ["Autostart.Enabled"] = "開機啟動：已啟用",
            ["Autostart.Disabled"] = "開機啟動：未啟用",
            ["Error.TrayUnsupported"] = "目前環境不支援托盤",
            ["Error.TrayFallback"] = "托盤已降級為視窗選單",
            ["Error.TrayInitFailed"] = "托盤初始化失敗",
            ["Error.TrayMenuDispatchFailed"] = "托盤命令派發失敗",
            ["Error.TrayNotInitialized"] = "托盤尚未初始化",
            ["Error.NotificationUnsupported"] = "目前平台不支援系統通知",
            ["Error.NotificationFallback"] = "系統通知已降級",
            ["Error.NotificationSendFailed"] = "通知發送失敗",
            ["Error.HotkeyUnsupported"] = "目前環境不支援全域熱鍵",
            ["Error.HotkeyFallback"] = "熱鍵已降級為視窗級模式",
            ["Error.HotkeyNameMissing"] = "熱鍵名稱為空",
            ["Error.HotkeyInvalidGesture"] = "熱鍵格式不合法",
            ["Error.HotkeyConflict"] = "熱鍵衝突",
            ["Error.HotkeyNotFound"] = "熱鍵不存在",
            ["Error.HotkeyPermissionDenied"] = "熱鍵權限不足",
            ["Error.HotkeyHookStartFailed"] = "全域熱鍵掛鉤啟動失敗",
            ["Error.HotkeyTriggerDispatchFailed"] = "熱鍵觸發派發失敗",
            ["Error.AutostartUnsupported"] = "目前平台不支援自啟動",
            ["Error.AutostartQueryFailed"] = "查詢自啟動狀態失敗",
            ["Error.AutostartSetFailed"] = "設定自啟動失敗",
            ["Error.AutostartVerificationFailed"] = "自啟動狀態校驗失敗",
            ["Error.AutostartExecutableMissing"] = "找不到可執行檔，無法設定自啟動",
            ["Error.OverlayUnsupported"] = "目前環境不支援 Overlay 附著",
            ["Error.OverlayPreviewMode"] = "Overlay 已切換至預覽模式",
            ["Error.OverlayTargetInvalid"] = "Overlay 目標無效",
            ["Error.OverlayTargetGone"] = "Overlay 目標已失效",
            ["Error.OverlayQueryFailed"] = "Overlay 目標查詢失敗",
            ["Error.OverlayHostNotBound"] = "Overlay 宿主視窗未綁定",
            ["Error.OverlayAttachFailed"] = "Overlay 附著失敗",
            ["Error.PostActionUnsupported"] = "後置動作不支援，已降級為日誌",
            ["Error.PostActionExecutionFailed"] = "後置動作執行失敗",
            ["Error.PostActionPowerActionsDisabled"] = "系統電源動作已停用",
        };
    }
}
