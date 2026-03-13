using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Diagnostics;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using Avalonia.Threading;
using MAAUnified.Application.Models;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.CoreBridge;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.Copilot;

public sealed partial class CopilotPageViewModel
{
    private const string PrtsPlusUrl = "https://prts.plus";
    private const string MapPrtsUrl = "https://map.ark-nights.com/areas?coord_override=maa";
    private static readonly Regex InvalidNavigationStageNameRegex = new(
        "[:',\\.\\(\\)\\|\\[\\]\\?，。【】｛｝；：]",
        RegexOptions.Compiled);

    private readonly HashSet<CopilotItemViewModel> _trackedCopilotItems = new();
    private readonly ObservableCollection<CopilotFileItemViewModel> _fileItems = [];
    private readonly ObservableCollection<CopilotUserAdditionalItemViewModel> _userAdditionalItems = [];
    private bool _isFilePopupOpen;
    private string _displayFilename = string.Empty;
    private bool _useFormation;
    private int _formationIndex = 1;
    private int _supportUnitUsage = 1;
    private bool _ignoreRequirements;
    private bool _addUserAdditional;
    private string _userAdditional = string.Empty;
    private bool _isUserAdditionalPopupOpen;
    private bool _useCopilotList;
    private bool _useSanityPotion;
    private string _copilotTaskName = string.Empty;
    private bool _loop;
    private int _loopTimes = 1;
    private string _loadedSourcePath = string.Empty;
    private string _loadedInlinePayload = string.Empty;
    private string _loadedDisplayName = string.Empty;
    private string _loadedStageName = string.Empty;
    private string _loadedType = MainStageStoryCollectionSideStoryType;
    private int _loadedCopilotId;
    private bool _couldLikeWebJson;
    private string _copilotUrl = PrtsPlusUrl;
    private string _mapUrl = MapPrtsUrl;
    private bool _suppressAutoAddLoadedCopilot;

    public ObservableCollection<CopilotFileItemViewModel> FileItems => _fileItems;

    public ObservableCollection<CopilotUserAdditionalItemViewModel> UserAdditionalItems => _userAdditionalItems;

    public IReadOnlyList<IntOption> FormationOptions { get; } =
    [
        new(1, "1"),
        new(2, "2"),
        new(3, "3"),
        new(4, "4"),
    ];

    public IReadOnlyList<IntOption> SupportUnitUsageOptions { get; } =
    [
        new(1, "补漏"),
        new(3, "随机"),
    ];

    public IReadOnlyList<IntOption> ModuleOptions { get; } =
    [
        new(0, "不使用模组"),
        new(1, "χ"),
        new(2, "γ"),
        new(3, "α"),
        new(4, "Δ"),
    ];

    public string HelpText =>
        "小提示:\n\n" +
        "1. 使用前请确认作业与所选的关卡类型一致。\n\n" +
        "2. 主线、故事集、SideStory: 请在关卡界面的右下角存在「开始行动」按钮界面启动。\n\n" +
        "3. 保全派驻: resource/copilot 文件夹内置多份作业。请先手动编队，在右下角存在「开始部署」按钮界面启动，可配合「循环次数」。\n\n" +
        "4. 悖论模拟: 选好技能后，在技能选择界面存在「开始模拟」按钮界面启动，1/2 星干员（无技能）在右下角存在「开始模拟」按钮界面开始。若使用「战斗列表」，请从干员列表「等级/稀有度」筛选下启动。\n\n" +
        "5. 使用好友助战时，请关闭「自动编队」和「战斗列表」，手动选择干员后，在编队界面右下角存在「开始行动」按钮界面启动。\n\n" +
        "6. 干员若被标记为「特别关注」，可能影响「自动编队」的识别与选择。建议使用「自动编队」时移除关注，或在报错后关闭「自动编队」，根据提示手动补充缺失的干员。\n\n" +
        "7. Copilot 作业站的神秘代码可通过输入框右侧的粘贴按钮粘贴：\n" +
        "● 单击第 2 个按钮 = 添加作业。\n" +
        "● 单击第 3 个按钮 = 添加作业集。\n\n" +
        "8. 战斗列表:\n" +
        "● 选择作业后，检查下方关卡名是否正确 (例: CV-EX-1)。\n" +
        "● 添加: 左键 = 普通难度，右键 = 突袭难度。\n" +
        "● 清除: 左键 = 全部清空，右键 = 仅移除未激活任务。\n" +
        "● 请在能看到目标关卡名的界面启动，不支持跨章节导航。\n" +
        "● 遇到理智不足、战斗失败、未能三星结算时将自动中止。";

    public int CopilotTabIndex
    {
        get => SelectedTypeIndex;
        set => SelectedTypeIndex = value;
    }

    public bool CanEdit => !IsRunning;

    public bool ShowClipboardSetButton => CopilotTabIndex is 0 or 2;

    public bool Form
    {
        get => CopilotTabIndex is 1 or 2 ? false : AutoSquad;
        set
        {
            if (SetProperty(ref _autoSquad, value, nameof(AutoSquad)))
            {
                OnPropertyChanged(nameof(Form));
                RefreshVisibilityState();
            }
        }
    }

    public bool ShowFormationGroup => CopilotTabIndex is 0 or 3;

    public bool ShowUseFormation => ShowFormationGroup && Form;

    public bool UseFormation
    {
        get => _useFormation;
        set
        {
            if (SetProperty(ref _useFormation, value))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotSelectFormation, FormationIndex.ToString());
                OnPropertyChanged(nameof(SelectedFormationOption));
            }
        }
    }

    public IntOption? SelectedFormationOption
    {
        get => FormationOptions.FirstOrDefault(option => option.Value == FormationIndex);
        set => FormationIndex = value?.Value ?? 1;
    }

    public int FormationIndex
    {
        get => _formationIndex;
        set
        {
            var normalized = Math.Clamp(value, 1, 4);
            if (SetProperty(ref _formationIndex, normalized))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotSelectFormation, normalized.ToString());
                OnPropertyChanged(nameof(SelectedFormationOption));
            }
        }
    }

    public bool UseSupportUnitUsage
    {
        get => UseSupportUnit;
        set
        {
            if (SetProperty(ref _useSupportUnit, value, nameof(UseSupportUnit)))
            {
                OnPropertyChanged(nameof(UseSupportUnitUsage));
                RefreshVisibilityState();
            }
        }
    }

    public bool ShowSupportUnitUsage => ShowFormationGroup && Form;

    public IntOption? SelectedSupportUnitUsageOption
    {
        get => SupportUnitUsageOptions.FirstOrDefault(option => option.Value == SupportUnitUsage);
        set => SupportUnitUsage = value?.Value ?? 1;
    }

    public int SupportUnitUsage
    {
        get => _supportUnitUsage;
        set
        {
            var normalized = value == 3 ? 3 : 1;
            if (SetProperty(ref _supportUnitUsage, normalized))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotSupportUnitUsage, normalized.ToString());
                OnPropertyChanged(nameof(SelectedSupportUnitUsageOption));
            }
        }
    }

    public bool ShowIgnoreRequirements => ShowFormationGroup && Form;

    public bool IgnoreRequirements
    {
        get => _ignoreRequirements;
        set => SetProperty(ref _ignoreRequirements, value);
    }

    public bool ShowAddTrust => ShowFormationGroup && Form;

    public bool ShowAddUserAdditional => ShowFormationGroup && Form;

    public bool AddUserAdditional
    {
        get => _addUserAdditional;
        set
        {
            if (SetProperty(ref _addUserAdditional, value))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotAddUserAdditional, value.ToString());
            }
        }
    }

    public string UserAdditional
    {
        get => _userAdditional;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _userAdditional, normalized))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotUserAdditional, normalized);
            }
        }
    }

    public bool IsUserAdditionalPopupOpen
    {
        get => _isUserAdditionalPopupOpen;
        set => SetProperty(ref _isUserAdditionalPopupOpen, value);
    }

    public bool UseCopilotList
    {
        get => CopilotTabIndex is 1 or 3 ? false : _useCopilotList;
        set
        {
            if (value)
            {
                Form = true;
            }

            if (SetProperty(ref _useCopilotList, value))
            {
                RefreshVisibilityState();
            }
        }
    }

    public bool ShowUseCopilotList => CopilotTabIndex is 0 or 2;

    public bool ShowCopilotListPanel => UseCopilotList && ShowUseCopilotList;

    public bool ShowUseSanityPotion => UseCopilotList && CopilotTabIndex == 0;

    public bool UseSanityPotion
    {
        get => _useSanityPotion;
        set => SetProperty(ref _useSanityPotion, value);
    }

    public bool ShowLoopSetting => !UseCopilotList && CopilotTabIndex is 1 or 3;

    public bool Loop
    {
        get => _loop;
        set => SetProperty(ref _loop, value);
    }

    public int LoopTimes
    {
        get => _loopTimes;
        set
        {
            var normalized = Math.Max(1, value);
            if (SetProperty(ref _loopTimes, normalized))
            {
                PersistGlobalSetting(LegacyConfigurationKeys.CopilotLoopTimes, normalized.ToString());
            }
        }
    }

    public string CopilotTaskName
    {
        get => _copilotTaskName;
        set
        {
            var sanitized = InvalidNavigationStageNameRegex.Replace(value ?? string.Empty, string.Empty).Trim();
            SetProperty(ref _copilotTaskName, sanitized);
        }
    }

    public bool IsFilePopupOpen
    {
        get => _isFilePopupOpen;
        set
        {
            if (SetProperty(ref _isFilePopupOpen, value) && value)
            {
                LoadFileItems();
            }
        }
    }

    public string DisplayFilename
    {
        get => _displayFilename;
        set => SetProperty(ref _displayFilename, value ?? string.Empty);
    }

    public bool HasLoadedCopilot
        => !string.IsNullOrWhiteSpace(_loadedSourcePath) || !string.IsNullOrWhiteSpace(_loadedInlinePayload);

    public bool CouldLikeWebJson
    {
        get => _couldLikeWebJson;
        private set => SetProperty(ref _couldLikeWebJson, value);
    }

    public string CopilotUrl
    {
        get => _copilotUrl;
        private set => SetProperty(ref _copilotUrl, value);
    }

    public string MapUrl
    {
        get => _mapUrl;
        private set => SetProperty(ref _mapUrl, value);
    }

    public string ListSelectionHint
        => ShowCopilotListPanel
            ? "战斗列表中的勾选项会参与启动。"
            : "当前将直接启动输入框中的单个作业。";

    private void InitializeWpfParityState()
    {
        foreach (var item in Items)
        {
            TrackCopilotItem(item);
        }

        Items.CollectionChanged += OnItemsCollectionChangedForWpfParity;
        LoadPersistedWpfParitySettings();
        EnsureHelpLogPresent();
        RefreshVisibilityState();
    }

    private void EnsureHelpLogPresent()
    {
        if (Logs.Count > 0)
        {
            return;
        }

        AddLog(HelpText, showTime: false);
    }

    private void OnSelectedTypeIndexChanged()
    {
        OnPropertyChanged(nameof(CopilotTabIndex));
        OnPropertyChanged(nameof(Form));
        RefreshVisibilityState();
    }

    private void RefreshVisibilityState()
    {
        OnPropertyChanged(nameof(CanEdit));
        OnPropertyChanged(nameof(ShowClipboardSetButton));
        OnPropertyChanged(nameof(ShowFormationGroup));
        OnPropertyChanged(nameof(ShowUseFormation));
        OnPropertyChanged(nameof(ShowSupportUnitUsage));
        OnPropertyChanged(nameof(ShowIgnoreRequirements));
        OnPropertyChanged(nameof(ShowAddTrust));
        OnPropertyChanged(nameof(ShowAddUserAdditional));
        OnPropertyChanged(nameof(ShowUseCopilotList));
        OnPropertyChanged(nameof(ShowCopilotListPanel));
        OnPropertyChanged(nameof(ShowUseSanityPotion));
        OnPropertyChanged(nameof(ShowLoopSetting));
        OnPropertyChanged(nameof(ListSelectionHint));
        OnPropertyChanged(nameof(UseCopilotList));
    }

    private void LoadPersistedWpfParitySettings()
    {
        _formationIndex = ReadGlobalInt(LegacyConfigurationKeys.CopilotSelectFormation, 1, 1, 4);
        _supportUnitUsage = ReadGlobalInt(LegacyConfigurationKeys.CopilotSupportUnitUsage, 1, 1, 3) == 3 ? 3 : 1;
        _loopTimes = ReadGlobalInt(LegacyConfigurationKeys.CopilotLoopTimes, 1, 1, 9999);
        _addUserAdditional = ReadGlobalBool(LegacyConfigurationKeys.CopilotAddUserAdditional, false);
        _userAdditional = ReadGlobalString(LegacyConfigurationKeys.CopilotUserAdditional);
    }

    private int ReadGlobalInt(string key, int fallback, int min, int max)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node)
            || node is not JsonValue jsonValue)
        {
            return fallback;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            return Math.Clamp(parsedInt, min, max);
        }

        if (jsonValue.TryGetValue(out string? raw) && int.TryParse(raw, out parsedInt))
        {
            return Math.Clamp(parsedInt, min, max);
        }

        return fallback;
    }

    private bool ReadGlobalBool(string key, bool fallback)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node)
            || node is not JsonValue jsonValue)
        {
            return fallback;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            return parsedBool;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            return parsedInt != 0;
        }

        if (jsonValue.TryGetValue(out string? raw))
        {
            if (bool.TryParse(raw, out parsedBool))
            {
                return parsedBool;
            }

            if (int.TryParse(raw, out parsedInt))
            {
                return parsedInt != 0;
            }
        }

        return fallback;
    }

    private string ReadGlobalString(string key)
    {
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(key, out var node)
            || node is not JsonValue jsonValue
            || !jsonValue.TryGetValue(out string? raw))
        {
            return string.Empty;
        }

        return raw?.Trim() ?? string.Empty;
    }

    private void PersistGlobalSetting(string key, string value)
    {
        Runtime.ConfigurationService.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
        _ = PersistGlobalSettingCoreAsync(key, value);
    }

    private async Task PersistGlobalSettingCoreAsync(string key, string value)
    {
        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(key, value);
        if (!result.Success)
        {
            await RecordFailedResultAsync($"Config.{key}", result);
        }
    }

    private void OnItemsCollectionChangedForWpfParity(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.OldItems is not null)
        {
            foreach (var item in e.OldItems.OfType<CopilotItemViewModel>())
            {
                UntrackCopilotItem(item);
            }
        }

        if (e.NewItems is not null)
        {
            foreach (var item in e.NewItems.OfType<CopilotItemViewModel>())
            {
                TrackCopilotItem(item);
            }
        }
    }

    private void TrackCopilotItem(CopilotItemViewModel item)
    {
        if (!_trackedCopilotItems.Add(item))
        {
            return;
        }

        item.PropertyChanged += OnTrackedCopilotItemPropertyChanged;
    }

    private void UntrackCopilotItem(CopilotItemViewModel item)
    {
        if (!_trackedCopilotItems.Remove(item))
        {
            return;
        }

        item.PropertyChanged -= OnTrackedCopilotItemPropertyChanged;
    }

    private void OnTrackedCopilotItemPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not CopilotItemViewModel)
        {
            return;
        }

        if (e.PropertyName is nameof(CopilotItemViewModel.Status))
        {
            return;
        }

        _ = PersistItemsAsync(CancellationToken.None);
    }

    public void ToggleFilePopup()
    {
        if (!IsFilePopupOpen)
        {
            LoadFileItems();
        }

        IsFilePopupOpen = !IsFilePopupOpen;
    }

    public void LoadFileItems()
    {
        _fileItems.Clear();
        var root = ResolveCopilotResourceRoot();
        if (!Directory.Exists(root))
        {
            return;
        }

        foreach (var file in Directory.GetFiles(root, "*.json"))
        {
            _fileItems.Add(CreateFileNode(file, root));
        }

        CopilotFileItemViewModel? oldFolder = null;
        foreach (var directory in Directory.GetDirectories(root))
        {
            var node = CreateFolderNode(directory, root);
            if (node is null)
            {
                continue;
            }

            if (string.Equals(node.Name, "old", StringComparison.OrdinalIgnoreCase))
            {
                oldFolder = node;
            }
            else
            {
                _fileItems.Add(node);
            }
        }

        if (oldFolder is not null)
        {
            _fileItems.Add(oldFolder);
        }
    }

    public async Task OnFileSelectedAsync(CopilotFileItemViewModel? fileItem, CancellationToken cancellationToken = default)
    {
        if (fileItem is null || fileItem.IsFolder || string.IsNullOrWhiteSpace(fileItem.FullPath))
        {
            return;
        }

        IsFilePopupOpen = false;
        await LoadCurrentFromFileAsync(fileItem.FullPath, cancellationToken);
    }

    public async Task LoadCurrentFromDisplayInputAsync(CancellationToken cancellationToken = default)
    {
        var raw = (DisplayFilename ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(raw))
        {
            ClearLoadedCopilot();
            return;
        }

        var expanded = Environment.ExpandEnvironmentVariables(raw);
        var copilotRoot = ResolveCopilotResourceRoot();
        var absolute = Path.IsPathRooted(expanded)
            ? expanded
            : Path.Combine(copilotRoot, expanded);

        if (File.Exists(absolute))
        {
            await LoadCurrentFromFileAsync(absolute, cancellationToken);
            return;
        }

        if (File.Exists(expanded))
        {
            await LoadCurrentFromFileAsync(expanded, cancellationToken);
            return;
        }

        if (raw.StartsWith('{') || raw.StartsWith('['))
        {
            await LoadCurrentFromClipboardAsync(raw, cancellationToken);
            return;
        }

        FilePath = expanded;
        StatusMessage = "已更新作业输入。";
        LastErrorMessage = string.Empty;
    }

    public async Task LoadCurrentFromFileAsync(string filePath, CancellationToken cancellationToken = default)
    {
        var normalizedPath = ResolveStoredSourcePath(filePath);
        var result = await Runtime.CopilotFeatureService.ImportFromFileAsync(normalizedPath, cancellationToken);
        if (!result.Success)
        {
            StatusMessage = "读取作业失败。";
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("Copilot.LoadCurrent.File", result, cancellationToken);
            return;
        }

        string payload;
        try
        {
            payload = await File.ReadAllTextAsync(normalizedPath, cancellationToken);
        }
        catch (Exception ex)
        {
            StatusMessage = "读取作业失败。";
            LastErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Copilot.LoadCurrent.File",
                ex,
                UiErrorCode.CopilotFileReadFailed,
                "读取作业文件失败。",
                cancellationToken);
            return;
        }

        if (!TryReadLoadedCopilotDescriptor(payload, normalizedPath, out var descriptor, out var warning))
        {
            StatusMessage = "读取作业失败。";
            LastErrorMessage = warning;
            await RecordFailedResultAsync(
                "Copilot.LoadCurrent.File",
                UiOperationResult.Fail(UiErrorCode.CopilotPayloadInvalidType, warning),
                cancellationToken);
            return;
        }

        ApplyLoadedCopilot(normalizedPath, string.Empty, descriptor);
        StatusMessage = result.Message;
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.LoadCurrent.File", StatusMessage, cancellationToken);
        await TryAutoAddLoadedCopilotAsync(cancellationToken);
    }

    public async Task LoadCurrentFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        var normalized = (payload ?? string.Empty).Trim();
        var result = await Runtime.CopilotFeatureService.ImportFromClipboardAsync(normalized, cancellationToken);
        if (!result.Success)
        {
            StatusMessage = "读取剪贴板作业失败。";
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("Copilot.LoadCurrent.Clipboard", result, cancellationToken);
            return;
        }

        var pathCandidate = ResolveClipboardPathCandidate(normalized);
        if (!string.IsNullOrWhiteSpace(pathCandidate))
        {
            await LoadCurrentFromFileAsync(pathCandidate, cancellationToken);
            return;
        }

        if (!TryReadLoadedCopilotDescriptor(normalized, sourcePath: null, out var descriptor, out var warning))
        {
            StatusMessage = "读取剪贴板作业失败。";
            LastErrorMessage = warning;
            await RecordFailedResultAsync(
                "Copilot.LoadCurrent.Clipboard",
                UiOperationResult.Fail(UiErrorCode.CopilotPayloadInvalidType, warning),
                cancellationToken);
            return;
        }

        ApplyLoadedCopilot(string.Empty, normalized, descriptor);
        StatusMessage = result.Message;
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.LoadCurrent.Clipboard", StatusMessage, cancellationToken);
        await TryAutoAddLoadedCopilotAsync(cancellationToken);
    }

    public async Task LoadCurrentFromClipboardSetAsync(string payload, CancellationToken cancellationToken = default)
    {
        if (CopilotTabIndex is 1 or 3)
        {
            return;
        }

        JsonNode? root;
        try
        {
            root = JsonNode.Parse((payload ?? string.Empty).Trim());
        }
        catch (Exception ex)
        {
            StatusMessage = "读取作业集失败。";
            LastErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Copilot.LoadSet.Clipboard",
                ex,
                UiErrorCode.CopilotPayloadInvalidJson,
                "读取作业集失败。",
                cancellationToken);
            return;
        }

        if (root is not JsonArray array)
        {
            StatusMessage = "读取作业集失败。";
            LastErrorMessage = "作业集必须是 JSON 数组。";
            await RecordFailedResultAsync(
                "Copilot.LoadSet.Clipboard",
                UiOperationResult.Fail(UiErrorCode.CopilotPayloadInvalidType, LastErrorMessage),
                cancellationToken);
            return;
        }

        UseCopilotList = true;
        var added = 0;
        foreach (var node in array.OfType<JsonObject>())
        {
            var json = node.ToJsonString();
            if (!TryReadLoadedCopilotDescriptor(json, sourcePath: null, out var descriptor, out _))
            {
                continue;
            }

            var item = CreateListItemFromDescriptor(descriptor, string.Empty, json, isRaid: false);
            Items.Add(item);
            added++;
        }

        if (added == 0)
        {
            StatusMessage = "读取作业集失败。";
            LastErrorMessage = "作业集内没有可识别的作业条目。";
            return;
        }

        await PersistItemsAsync(cancellationToken);
        StatusMessage = $"已添加 {added} 个作业到战斗列表。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.LoadSet.Clipboard", StatusMessage, cancellationToken);
    }

    public async Task ImportFilesToListAsync(IReadOnlyList<string> filePaths, CancellationToken cancellationToken = default)
    {
        if (filePaths.Count == 0)
        {
            return;
        }

        var added = 0;
        foreach (var filePath in filePaths)
        {
            var normalizedPath = ResolveStoredSourcePath(filePath);
            var result = await Runtime.CopilotFeatureService.ImportFromFileAsync(normalizedPath, cancellationToken);
            if (!result.Success)
            {
                StatusMessage = "批量导入失败。";
                LastErrorMessage = result.Message;
                await RecordFailedResultAsync("Copilot.ImportFiles", result, cancellationToken);
                return;
            }

            var payload = await File.ReadAllTextAsync(normalizedPath, cancellationToken);
            if (!TryReadLoadedCopilotDescriptor(payload, normalizedPath, out var descriptor, out _))
            {
                continue;
            }

            Items.Add(CreateListItemFromDescriptor(descriptor, normalizedPath, string.Empty, isRaid: false));
            added++;
        }

        if (added == 0)
        {
            StatusMessage = "批量导入失败。";
            LastErrorMessage = "未找到可导入的作业文件。";
            return;
        }

        UseCopilotList = CopilotTabIndex is 0 or 2;
        await PersistItemsAsync(cancellationToken);
        StatusMessage = $"已批量导入 {added} 个作业。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.ImportFiles", StatusMessage, cancellationToken);
    }

    public async Task AddCurrentToListAsync(bool isRaid, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!HasLoadedCopilot)
        {
            StatusMessage = "添加作业失败。";
            LastErrorMessage = "请先选择一个作业。";
            await RecordFailedResultAsync(
                "Copilot.List.AddCurrent",
                UiOperationResult.Fail(UiErrorCode.CopilotFileMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        var descriptor = new LoadedCopilotDescriptor(
            ResolveListItemName(_loadedDisplayName, _loadedStageName),
            string.IsNullOrWhiteSpace(_loadedType) ? ResolveTypeDisplayNameForTab(CopilotTabIndex) : _loadedType,
            string.IsNullOrWhiteSpace(_loadedStageName) ? _loadedDisplayName : _loadedStageName,
            _loadedCopilotId);
        var item = CreateListItemFromDescriptor(descriptor, _loadedSourcePath, _loadedInlinePayload, isRaid);
        item.Name = ResolveListItemName(CopilotTaskName, descriptor.StageName);
        Items.Add(item);
        SetSelectedItemSilently(item);
        UseCopilotList = CopilotTabIndex is 0 or 2;
        await PersistItemsAsync(cancellationToken);
        StatusMessage = isRaid ? "已添加突袭作业到战斗列表。" : "已添加作业到战斗列表。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.List.AddCurrent", StatusMessage, cancellationToken);
    }

    public async Task DeleteListItemAsync(CopilotItemViewModel item, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!Items.Contains(item))
        {
            return;
        }

        Items.Remove(item);
        if (ReferenceEquals(SelectedItem, item))
        {
            SetSelectedItemSilently(Items.LastOrDefault());
        }

        await PersistItemsAsync(cancellationToken);
        StatusMessage = "已删除作业。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.List.Delete", StatusMessage, cancellationToken);
    }

    public async Task CleanInactiveListItemsAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var removed = Items.RemoveAll(item => !item.IsChecked);
        await PersistItemsAsync(cancellationToken);
        StatusMessage = removed == 0 ? "没有未激活作业需要清理。" : $"已清理 {removed} 个未激活作业。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.List.CleanInactive", StatusMessage, cancellationToken);
    }

    public async Task LoadListItemAsync(CopilotItemViewModel item, bool disableListMode, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        SetSelectedItemSilently(item);
        if (disableListMode)
        {
            UseCopilotList = false;
        }

        _suppressAutoAddLoadedCopilot = true;
        try
        {
            var resolvedSourcePath = ResolveStoredSourcePath(item.SourcePath);
            if (!string.IsNullOrWhiteSpace(resolvedSourcePath) && File.Exists(resolvedSourcePath))
            {
                await LoadCurrentFromFileAsync(resolvedSourcePath, cancellationToken);
                return;
            }

            if (!string.IsNullOrWhiteSpace(item.InlinePayload))
            {
                await LoadCurrentFromClipboardAsync(item.InlinePayload, cancellationToken);
                return;
            }
        }
        finally
        {
            _suppressAutoAddLoadedCopilot = false;
        }

        StatusMessage = "读取作业失败。";
        LastErrorMessage = "所选列表项缺少可用的作业来源。";
    }

    public void OpenUserAdditionalPopup()
    {
        _userAdditionalItems.Clear();
        foreach (var item in ParseUserAdditionalItems())
        {
            _userAdditionalItems.Add(item);
        }

        if (_userAdditionalItems.Count == 0)
        {
            _userAdditionalItems.Add(new CopilotUserAdditionalItemViewModel());
        }

        IsUserAdditionalPopupOpen = true;
    }

    public void AddUserAdditionalItem()
    {
        if (_userAdditionalItems.Any(item => string.IsNullOrWhiteSpace(item.Name)))
        {
            return;
        }

        _userAdditionalItems.Add(new CopilotUserAdditionalItemViewModel());
    }

    public void RemoveUserAdditionalItem(CopilotUserAdditionalItemViewModel item)
    {
        _userAdditionalItems.Remove(item);
        if (_userAdditionalItems.Count == 0)
        {
            _userAdditionalItems.Add(new CopilotUserAdditionalItemViewModel());
        }
    }

    public void SaveUserAdditional()
    {
        var payload = new JsonArray();
        foreach (var item in _userAdditionalItems.Where(item => !string.IsNullOrWhiteSpace(item.Name)))
        {
            payload.Add(new JsonObject
            {
                ["name"] = item.Name.Trim(),
                ["skill"] = Math.Clamp(item.Skill, 1, 3),
                ["module"] = Math.Clamp(item.Module, 0, 4),
            });
        }

        UserAdditional = payload.Count == 0 ? string.Empty : payload.ToJsonString();
        IsUserAdditionalPopupOpen = false;
    }

    public void CancelUserAdditionalEdit()
    {
        IsUserAdditionalPopupOpen = false;
    }

    public async Task SubmitLoadedFeedbackAsync(bool like, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var copilotId = _loadedCopilotId > 0
            ? _loadedCopilotId.ToString()
            : SelectedItem?.CopilotId > 0
                ? SelectedItem.CopilotId.ToString()
                : string.Empty;
        if (string.IsNullOrWhiteSpace(copilotId))
        {
            StatusMessage = "反馈失败。";
            LastErrorMessage = "当前作业没有可反馈的作业站 ID。";
            return;
        }

        var result = await Runtime.CopilotFeatureService.SubmitFeedbackAsync(copilotId, like, cancellationToken);
        if (!result.Success)
        {
            StatusMessage = "反馈失败。";
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("Copilot.Feedback.Loaded", result, cancellationToken);
            return;
        }

        StatusMessage = like ? "已提交点赞。" : "已提交点踩。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.Feedback.Loaded", StatusMessage, cancellationToken);
    }

    private async Task TryAutoAddLoadedCopilotAsync(CancellationToken cancellationToken)
    {
        if (!ShowCopilotListPanel || !HasLoadedCopilot || _suppressAutoAddLoadedCopilot)
        {
            return;
        }

        // Match WPF behavior: battle-list auto append only applies to clipboard/web payloads,
        // while local files stay as the currently loaded copilot.
        if (string.IsNullOrWhiteSpace(_loadedInlinePayload))
        {
            return;
        }

        await AddCurrentToListAsync(isRaid: false, cancellationToken);
    }

    private void ApplyLoadedCopilot(string sourcePath, string inlinePayload, LoadedCopilotDescriptor descriptor)
    {
        _loadedSourcePath = sourcePath;
        _loadedInlinePayload = inlinePayload;
        _loadedDisplayName = descriptor.DisplayName;
        _loadedStageName = descriptor.StageName;
        _loadedType = descriptor.Type;
        _loadedCopilotId = descriptor.CopilotId;
        FilePath = !string.IsNullOrWhiteSpace(sourcePath) ? sourcePath : inlinePayload;
        DisplayFilename = BuildDisplayFilename(sourcePath, inlinePayload);
        CopilotTaskName = descriptor.StageName;
        CopilotUrl = PrtsPlusUrl;
        MapUrl = MapPrtsUrl;
        CouldLikeWebJson = descriptor.CopilotId > 0;
        OnPropertyChanged(nameof(HasLoadedCopilot));
    }

    private void ClearLoadedCopilot()
    {
        _loadedSourcePath = string.Empty;
        _loadedInlinePayload = string.Empty;
        _loadedDisplayName = string.Empty;
        _loadedStageName = string.Empty;
        _loadedType = ResolveTypeDisplayNameForTab(CopilotTabIndex);
        _loadedCopilotId = 0;
        FilePath = string.Empty;
        DisplayFilename = string.Empty;
        CopilotTaskName = string.Empty;
        CouldLikeWebJson = false;
        OnPropertyChanged(nameof(HasLoadedCopilot));
    }

    private string BuildDisplayFilename(string sourcePath, string inlinePayload)
    {
        if (!string.IsNullOrWhiteSpace(sourcePath))
        {
            var root = ResolveCopilotResourceRoot();
            if (!string.IsNullOrWhiteSpace(root)
                && sourcePath.StartsWith(root, StringComparison.OrdinalIgnoreCase))
            {
                return Path.GetRelativePath(root, sourcePath);
            }

            return sourcePath;
        }

        return inlinePayload;
    }

    private static string ResolveCopilotResourceRoot()
    {
        return Path.Combine(AppContext.BaseDirectory, "resource", "copilot");
    }

    private CopilotFileItemViewModel CreateFileNode(string filePath, string root)
    {
        return new CopilotFileItemViewModel(
            Path.GetFileName(filePath),
            filePath,
            Path.GetRelativePath(root, filePath),
            isFolder: false);
    }

    private CopilotFileItemViewModel? CreateFolderNode(string directoryPath, string root)
    {
        var item = new CopilotFileItemViewModel(
            Path.GetFileName(directoryPath),
            directoryPath,
            Path.GetRelativePath(root, directoryPath),
            isFolder: true);

        foreach (var file in Directory.GetFiles(directoryPath, "*.json"))
        {
            item.Children.Add(CreateFileNode(file, root));
        }

        foreach (var childDirectory in Directory.GetDirectories(directoryPath))
        {
            var child = CreateFolderNode(childDirectory, root);
            if (child is not null)
            {
                item.Children.Add(child);
            }
        }

        return item.Children.Count == 0 ? null : item;
    }

    private static string ResolveStoredSourcePath(string sourcePath)
    {
        var raw = (sourcePath ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(raw))
        {
            return string.Empty;
        }

        if (Path.IsPathRooted(raw))
        {
            return raw;
        }

        return Path.Combine(AppContext.BaseDirectory, raw);
    }

    private static string ResolveTypeDisplayNameForTab(int tabIndex)
    {
        return tabIndex switch
        {
            1 => SecurityServiceStationType,
            2 => ParadoxSimulationType,
            3 => OtherActivityType,
            _ => MainStageStoryCollectionSideStoryType,
        };
    }

    private static string ResolveListItemName(string displayName, string stageName)
    {
        if (!string.IsNullOrWhiteSpace(stageName))
        {
            return stageName.Trim();
        }

        if (!string.IsNullOrWhiteSpace(displayName))
        {
            return displayName.Trim();
        }

        return $"Task-{DateTime.Now:HHmmss}";
    }

    private bool TryReadLoadedCopilotDescriptor(
        string payload,
        string? sourcePath,
        out LoadedCopilotDescriptor descriptor,
        out string warning)
    {
        descriptor = new LoadedCopilotDescriptor(
            Path.GetFileNameWithoutExtension(sourcePath ?? string.Empty),
            ResolveTypeDisplayNameForTab(CopilotTabIndex),
            string.Empty,
            0);
        warning = string.Empty;

        JsonNode? root;
        try
        {
            root = JsonNode.Parse(payload);
        }
        catch (Exception ex)
        {
            warning = ex.Message;
            return false;
        }

        if (root is not JsonObject obj)
        {
            warning = "当前作业必须是单个 JSON 对象。";
            return false;
        }

        var stageName = ReadJsonString(obj, "stage_name");
        var displayName = !string.IsNullOrWhiteSpace(stageName)
            ? stageName
            : Path.GetFileNameWithoutExtension(sourcePath ?? string.Empty);
        var type = ResolveTypeDisplayNameForTab(CopilotTabIndex);
        var rawType = ReadJsonString(obj, "type");
        if (string.Equals(rawType, "SSS", StringComparison.OrdinalIgnoreCase))
        {
            type = SecurityServiceStationType;
        }

        descriptor = new LoadedCopilotDescriptor(
            string.IsNullOrWhiteSpace(displayName) ? $"Imported-{DateTime.Now:HHmmss}" : displayName.Trim(),
            type,
            string.IsNullOrWhiteSpace(stageName) ? displayName.Trim() : stageName.Trim(),
            ReadJsonInt(obj, "copilot_id") ?? ReadJsonInt(obj, "id") ?? 0);
        return true;
    }

    private static string ReadJsonString(JsonObject obj, string key)
    {
        if (!obj.TryGetPropertyValue(key, out var node)
            || node is not JsonValue jsonValue
            || !jsonValue.TryGetValue(out string? raw))
        {
            return string.Empty;
        }

        return raw?.Trim() ?? string.Empty;
    }

    private static int? ReadJsonInt(JsonObject obj, string key)
    {
        if (!obj.TryGetPropertyValue(key, out var node)
            || node is not JsonValue jsonValue)
        {
            return null;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            return parsedInt;
        }

        if (jsonValue.TryGetValue(out string? raw) && int.TryParse(raw, out parsedInt))
        {
            return parsedInt;
        }

        return null;
    }

    private CopilotItemViewModel CreateListItemFromDescriptor(
        LoadedCopilotDescriptor descriptor,
        string sourcePath,
        string inlinePayload,
        bool isRaid)
    {
        return new CopilotItemViewModel(
            ResolveListItemName(descriptor.DisplayName, descriptor.StageName),
            descriptor.Type,
            sourcePath,
            inlinePayload)
        {
            CopilotId = Math.Max(0, descriptor.CopilotId),
            IsChecked = true,
            IsRaid = isRaid,
            TabIndex = CopilotTabIndex,
        };
    }

    private IEnumerable<CopilotUserAdditionalItemViewModel> ParseUserAdditionalItems()
    {
        if (string.IsNullOrWhiteSpace(UserAdditional))
        {
            return [];
        }

        JsonNode? root;
        try
        {
            root = JsonNode.Parse(UserAdditional);
        }
        catch
        {
            var fallbackItems = new List<CopilotUserAdditionalItemViewModel>();
            foreach (var segment in UserAdditional.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
            {
                var parts = segment.Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length == 0 || string.IsNullOrWhiteSpace(parts[0]))
                {
                    continue;
                }

                fallbackItems.Add(new CopilotUserAdditionalItemViewModel
                {
                    Name = parts[0],
                    Skill = parts.Length > 1 && int.TryParse(parts[1], out var skill) ? Math.Clamp(skill, 1, 3) : 1,
                    Module = 0,
                });
            }

            return fallbackItems;
        }

        if (root is not JsonArray array)
        {
            return [];
        }

        var parsedItems = new List<CopilotUserAdditionalItemViewModel>();
        foreach (var item in array.OfType<JsonObject>())
        {
            var name = ReadJsonString(item, "name");
            if (string.IsNullOrWhiteSpace(name))
            {
                continue;
            }

            parsedItems.Add(new CopilotUserAdditionalItemViewModel
            {
                Name = name,
                Skill = Math.Clamp(ReadJsonInt(item, "skill") ?? 1, 1, 3),
                Module = Math.Clamp(ReadJsonInt(item, "module") ?? 0, 0, 4),
            });
        }

        return parsedItems;
    }

    private JsonArray BuildUserAdditionalPayload()
    {
        var result = new JsonArray();
        if (!AddUserAdditional)
        {
            return result;
        }

        foreach (var item in ParseUserAdditionalItems())
        {
            result.Add(new JsonObject
            {
                ["name"] = item.Name.Trim(),
                ["skill"] = Math.Clamp(item.Skill, 1, 3),
                ["module"] = Math.Clamp(item.Module, 0, 4),
            });
        }

        return result;
    }

    private async Task<AppendPlan?> AppendConfiguredCopilotAsync(CancellationToken cancellationToken)
    {
        if (ShowCopilotListPanel)
        {
            var checkedItems = await ValidateCopilotListSelectionAsync(cancellationToken);
            if (checkedItems is null)
            {
                return null;
            }

            return await AppendCopilotListAsync(checkedItems, cancellationToken);
        }

        if (!await ValidateSingleSelectionAsync(cancellationToken))
        {
            return null;
        }

        if (HasLoadedCopilot)
        {
            return await AppendLoadedCopilotAsync(cancellationToken);
        }

        if (SelectedItem is not null)
        {
            return await AppendSingleItemAsync(SelectedItem, cancellationToken);
        }

        StatusMessage = "启动失败。";
        LastErrorMessage = "请选择要执行的作业。";
        await RecordFailedResultAsync(
            "Copilot.Start",
            UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
            cancellationToken);
        return null;
    }

    private async Task<bool> ValidateSingleSelectionAsync(CancellationToken cancellationToken)
    {
        var selectedType = HasLoadedCopilot
            ? _loadedType
            : SelectedItem?.Type;
        if (string.IsNullOrWhiteSpace(selectedType))
        {
            return true;
        }

        var isSss = string.Equals(ResolveCopilotTaskType(selectedType), "SSSCopilot", StringComparison.Ordinal);
        if ((isSss && CopilotTabIndex != 1) || (!isSss && CopilotTabIndex == 1))
        {
            return await FailStartValidationAsync(
                "Copilot.Start.Validate",
                "当前选择的作业与页签不匹配",
                UiErrorCode.TaskValidationFailed,
                cancellationToken);
        }

        return true;
    }

    private async Task<IReadOnlyList<CopilotItemViewModel>?> ValidateCopilotListSelectionAsync(CancellationToken cancellationToken)
    {
        var checkedItems = Items
            .Where(item => item.IsChecked)
            .ToList();
        if (checkedItems.Count == 0)
        {
            await FailStartValidationAsync(
                "Copilot.Start.List",
                "正在使用「战斗列表」，但未勾选任何作业。",
                UiErrorCode.CopilotSelectionMissing,
                cancellationToken);
            return null;
        }

        if (checkedItems.Any(item => item.TabIndex is null))
        {
            await FailStartValidationAsync(
                "Copilot.Start.List",
                "正在使用「战斗列表」，但列表包含旧版本条目（缺少页签信息），请在正确的页签重新添加这些作业后再启动",
                UiErrorCode.TaskValidationFailed,
                cancellationToken);
            return null;
        }

        var tabs = checkedItems
            .Select(item => item.TabIndex!.Value)
            .Distinct()
            .ToArray();
        if (tabs.Length > 1)
        {
            await FailStartValidationAsync(
                "Copilot.Start.List",
                "正在使用「战斗列表」，但不允许混用「主线/故事集/SideStory」与「悖论模拟」，请分别在对应页签建立列表后再启动",
                UiErrorCode.TaskValidationFailed,
                cancellationToken);
            return null;
        }

        var listTab = tabs[0];
        if (listTab != CopilotTabIndex)
        {
            await FailStartValidationAsync(
                "Copilot.Start.List",
                $"正在使用「战斗列表」，当前页签为「{GetCopilotTabDisplayName(CopilotTabIndex)}」，但列表来自「{GetCopilotTabDisplayName(listTab)}」，请切换到对应页签后再启动",
                UiErrorCode.TaskValidationFailed,
                cancellationToken);
            return null;
        }

        if (CopilotTabIndex != 2)
        {
            if (checkedItems.Count == 1)
            {
                AddLog("正在使用「战斗列表」执行单个作业, 不推荐此行为。 单个作业请直接运行", "WARN", showTime: false);
            }

            if (checkedItems.Any(item => string.IsNullOrWhiteSpace(item.Name)))
            {
                await FailStartValidationAsync(
                    "Copilot.Start.List",
                    "存在关卡名为空的作业",
                    UiErrorCode.TaskValidationFailed,
                    cancellationToken);
                return null;
            }
        }

        return checkedItems;
    }

    private async Task<bool> FailStartValidationAsync(
        string scope,
        string message,
        string errorCode,
        CancellationToken cancellationToken)
    {
        StatusMessage = "启动失败。";
        LastErrorMessage = message;
        AddLog(message, "ERROR", showTime: false);
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(errorCode, message),
            cancellationToken);
        return false;
    }

    private async Task<AppendPlan?> AppendLoadedCopilotAsync(CancellationToken cancellationToken)
    {
        var filePath = await ResolveExecutionFilePathAsync(
            _loadedSourcePath,
            _loadedInlinePayload,
            _loadedDisplayName,
            cancellationToken);
        if (filePath is null)
        {
            return null;
        }

        var displayName = string.IsNullOrWhiteSpace(_loadedDisplayName)
            ? Path.GetFileNameWithoutExtension(filePath)
            : _loadedDisplayName;
        var taskType = string.IsNullOrWhiteSpace(_loadedType)
            ? ResolveTypeDisplayNameForTab(CopilotTabIndex)
            : _loadedType;
        var request = BuildSingleTaskRequest(taskType, displayName, filePath);
        var appendResult = await Runtime.CoreBridge.AppendTaskAsync(request, cancellationToken);
        if (!appendResult.Success)
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = $"追加 Copilot 任务失败：{appendResult.Error?.Code} {appendResult.Error?.Message}";
            await RecordFailedResultAsync(
                "Copilot.Append.Loaded",
                UiOperationResult.Fail(UiErrorCode.CopilotFileReadFailed, LastErrorMessage),
                cancellationToken);
            return null;
        }

        await RecordEventAsync(
            "Copilot.Append.Loaded",
            $"Appended current copilot task #{appendResult.Value}: {displayName}",
            cancellationToken);
        return new AppendPlan(appendResult.Value, displayName, ResolveCopilotTaskChain(taskType));
    }

    private async Task<AppendPlan?> AppendSingleItemAsync(CopilotItemViewModel item, CancellationToken cancellationToken)
    {
        var filePath = await ResolveExecutionFilePathAsync(item, cancellationToken);
        if (filePath is null)
        {
            return null;
        }

        var request = BuildSingleTaskRequest(item.Type, item.Name, filePath);
        var appendResult = await Runtime.CoreBridge.AppendTaskAsync(request, cancellationToken);
        if (!appendResult.Success)
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = $"追加 Copilot 任务失败：{appendResult.Error?.Code} {appendResult.Error?.Message}";
            await RecordFailedResultAsync(
                "Copilot.Append",
                UiOperationResult.Fail(UiErrorCode.CopilotFileReadFailed, LastErrorMessage),
                cancellationToken);
            return null;
        }

        await RecordEventAsync(
            "Copilot.Append",
            $"Appended copilot task #{appendResult.Value}: {item.Name}",
            cancellationToken);
        return new AppendPlan(appendResult.Value, item.Name, ResolveCopilotTaskChain(item.Type));
    }

    private async Task<AppendPlan?> AppendCopilotListAsync(
        IReadOnlyList<CopilotItemViewModel> checkedItems,
        CancellationToken cancellationToken)
    {
        if (CopilotTabIndex == 2)
        {
            var list = new JsonArray();
            foreach (var item in checkedItems)
            {
                var filePath = await ResolveExecutionFilePathAsync(item, cancellationToken);
                if (filePath is null)
                {
                    return null;
                }

                list.Add(filePath);
            }

            var paradoxRequest = new CoreTaskRequest(
                "ParadoxCopilot",
                checkedItems[0].Name,
                true,
                new JsonObject
                {
                    ["list"] = list,
                }.ToJsonString());
            return await AppendListRequestAsync(paradoxRequest, checkedItems[0].Name, "ParadoxCopilot", cancellationToken);
        }

        var tasks = new JsonArray();
        foreach (var item in checkedItems)
        {
            var filePath = await ResolveExecutionFilePathAsync(item, cancellationToken);
            if (filePath is null)
            {
                return null;
            }

            tasks.Add(new JsonObject
            {
                ["filename"] = filePath,
                ["stage_name"] = item.Name,
                ["is_raid"] = item.IsRaid,
            });
        }

        var payload = new JsonObject
        {
            ["copilot_list"] = tasks,
            ["formation"] = Form,
            ["support_unit_usage"] = UseSupportUnitUsage ? SupportUnitUsage : 0,
            ["add_trust"] = AddTrust,
            ["ignore_requirements"] = IgnoreRequirements,
            ["use_sanity_potion"] = ShowUseSanityPotion && UseSanityPotion,
        };
        if (UseFormation)
        {
            payload["formation_index"] = FormationIndex;
        }

        var userAdditional = BuildUserAdditionalPayload();
        if (userAdditional.Count > 0)
        {
            payload["user_additional"] = userAdditional;
        }

        var request = new CoreTaskRequest("Copilot", checkedItems[0].Name, true, payload.ToJsonString());
        return await AppendListRequestAsync(request, checkedItems[0].Name, "Copilot", cancellationToken);
    }

    private async Task<AppendPlan?> AppendListRequestAsync(
        CoreTaskRequest request,
        string activeItemName,
        string taskChain,
        CancellationToken cancellationToken)
    {
        var appendResult = await Runtime.CoreBridge.AppendTaskAsync(request, cancellationToken);
        if (!appendResult.Success)
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = $"追加 Copilot 任务失败：{appendResult.Error?.Code} {appendResult.Error?.Message}";
            await RecordFailedResultAsync(
                "Copilot.Append.List",
                UiOperationResult.Fail(UiErrorCode.CopilotFileReadFailed, LastErrorMessage),
                cancellationToken);
            return null;
        }

        await RecordEventAsync(
            "Copilot.Append.List",
            $"Appended copilot list task #{appendResult.Value}: {activeItemName}",
            cancellationToken);
        return new AppendPlan(appendResult.Value, activeItemName, taskChain);
    }

    private CoreTaskRequest BuildSingleTaskRequest(string type, string displayName, string filePath)
    {
        var taskType = ResolveCopilotTaskType(type);
        if (string.Equals(taskType, "ParadoxCopilot", StringComparison.Ordinal))
        {
            return new CoreTaskRequest(
                taskType,
                displayName,
                true,
                new JsonObject
                {
                    ["filename"] = filePath,
                }.ToJsonString());
        }

        var payload = new JsonObject
        {
            ["filename"] = filePath,
            ["formation"] = Form,
            ["support_unit_usage"] = UseSupportUnitUsage ? SupportUnitUsage : 0,
            ["add_trust"] = AddTrust,
            ["ignore_requirements"] = IgnoreRequirements,
            ["loop_times"] = ShowLoopSetting && Loop ? LoopTimes : 1,
            ["use_sanity_potion"] = false,
        };
        if (UseFormation)
        {
            payload["formation_index"] = FormationIndex;
        }

        var userAdditional = BuildUserAdditionalPayload();
        if (userAdditional.Count > 0)
        {
            payload["user_additional"] = userAdditional;
        }

        return new CoreTaskRequest(taskType, displayName, true, payload.ToJsonString());
    }

    private async Task<string?> ResolveExecutionFilePathAsync(
        string sourcePath,
        string inlinePayload,
        string displayName,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var resolvedSourcePath = ResolveStoredSourcePath(sourcePath);
        if (!string.IsNullOrWhiteSpace(resolvedSourcePath))
        {
            if (!File.Exists(resolvedSourcePath))
            {
                StatusMessage = "启动失败。";
                LastErrorMessage = $"作业文件不存在：{resolvedSourcePath}";
                await RecordFailedResultAsync(
                    "Copilot.Start.Input",
                    UiOperationResult.Fail(UiErrorCode.CopilotFileNotFound, LastErrorMessage),
                    cancellationToken);
                return null;
            }

            return resolvedSourcePath;
        }

        if (string.IsNullOrWhiteSpace(inlinePayload))
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = "当前作业缺少可执行来源（文件路径或 JSON 内容）。";
            await RecordFailedResultAsync(
                "Copilot.Start.Input",
                UiOperationResult.Fail(UiErrorCode.CopilotFileMissing, LastErrorMessage),
                cancellationToken);
            return null;
        }

        var debugDirectory = Path.GetDirectoryName(Runtime.DiagnosticsService.EventLogPath)
            ?? Path.Combine(AppContext.BaseDirectory, "debug");
        var directory = Path.Combine(debugDirectory, "copilot-cache");
        Directory.CreateDirectory(directory);
        var baseName = string.IsNullOrWhiteSpace(displayName) ? "copilot-inline" : displayName;
        var fileName = string.Concat(baseName.Select(ch => Path.GetInvalidFileNameChars().Contains(ch) ? '_' : ch));
        var filePath = Path.Combine(directory, $"{fileName}-{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}.json");
        await File.WriteAllTextAsync(filePath, inlinePayload, cancellationToken);
        return filePath;
    }

    private static string GetCopilotTabDisplayName(int tabIndex)
    {
        return tabIndex switch
        {
            1 => "保全派驻",
            2 => "悖论模拟",
            3 => "其他活动",
            _ => "主线/故事集/SideStory",
        };
    }

    private int ResolveTabIndexFromType(string type)
    {
        var normalized = NormalizeTypeDisplayName(type);
        for (var i = 0; i < Types.Count; i++)
        {
            if (string.Equals(Types[i], normalized, StringComparison.Ordinal))
            {
                return i;
            }
        }

        return 0;
    }

    private sealed record LoadedCopilotDescriptor(string DisplayName, string Type, string StageName, int CopilotId);

    private sealed record AppendPlan(int TaskId, string ActiveItemName, string TaskChain);

    public sealed class CopilotFileItemViewModel : ObservableObject
    {
        private bool _isExpanded;

        public CopilotFileItemViewModel(string name, string fullPath, string relativePath, bool isFolder)
        {
            Name = name;
            FullPath = fullPath;
            RelativePath = relativePath;
            IsFolder = isFolder;
        }

        public string Name { get; }

        public string FullPath { get; }

        public string RelativePath { get; }

        public bool IsFolder { get; }

        public bool CanSelect => !IsFolder;

        public bool HasChildren => Children.Count > 0;

        public bool IsExpanded
        {
            get => _isExpanded;
            set
            {
                if (SetProperty(ref _isExpanded, value))
                {
                    OnPropertyChanged(nameof(ExpandGlyph));
                }
            }
        }

        public string ExpandGlyph => IsExpanded ? "▼" : "▶";

        public ObservableCollection<CopilotFileItemViewModel> Children { get; } = [];

        public override string ToString() => Name;
    }

    public sealed class CopilotUserAdditionalItemViewModel : ObservableObject
    {
        private string _name = string.Empty;
        private int _skill = 1;
        private int _module;

        public string Name
        {
            get => _name;
            set => SetProperty(ref _name, value ?? string.Empty);
        }

        public int Skill
        {
            get => _skill;
            set => SetProperty(ref _skill, Math.Clamp(value, 1, 3));
        }

        public int Module
        {
            get => _module;
            set => SetProperty(ref _module, Math.Clamp(value, 0, 4));
        }
    }

    public sealed record IntOption(int Value, string DisplayName);
}

file static class CopilotCollectionExtensions
{
    public static int RemoveAll<T>(this ObservableCollection<T> collection, Func<T, bool> predicate)
    {
        var removed = 0;
        for (var i = collection.Count - 1; i >= 0; i--)
        {
            if (!predicate(collection[i]))
            {
                continue;
            }

            collection.RemoveAt(i);
            removed++;
        }

        return removed;
    }
}
