using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Compat.Constants;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class InfrastModuleViewModel : TaskModuleSettingsViewModelBase
{
    public const int InfrastModeNormal = 0;
    public const int InfrastModeCustom = 10000;
    public const int InfrastModeRotation = 20000;
    public const string UserDefinedInfrast = "user_defined";

    private static readonly string[] DefaultFacilityOrder =
    [
        "Mfg",
        "Trade",
        "Control",
        "Power",
        "Reception",
        "Office",
        "Dorm",
        "Processing",
        "Training",
    ];

    private int _mode;
    private string _drones = "Money";
    private int _dormThresholdPercent = 30;
    private string _customFilePath = string.Empty;
    private int _selectedPlanIndex = -1;
    private PlanOption? _selectedPlan;
    private bool _continueTraining;
    private bool _dormTrustEnabled = true;
    private bool _dormNotStationedEnabled = true;
    private bool _replenish = true;
    private bool _receptionMessageBoard = true;
    private bool _receptionClueExchange = true;
    private bool _receptionSendClue = true;
    private IReadOnlyList<IntOption> _modeOptions = [];
    private IReadOnlyList<StringOption> _droneOptions = [];
    private IReadOnlyList<StringOption> _defaultInfrastOptions = [];
    private string _defaultInfrast = UserDefinedInfrast;
    private bool _isCustomFileReadOnly;
    private bool _suppressDefaultInfrastPersist;
    private bool _suppressFacilitySelectionChanged;
    private readonly List<ParsedPlan> _parsedPlans = [];

    public InfrastModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, TaskModuleTypes.Infrast)
    {
        Texts.PropertyChanged += OnTextsChanged;
        ApplyPersistentDefaultInfrast(ResolveDefaultInfrast());
        RebuildLocalizedOptions();
        ConfigureFacilityOptions(DefaultFacilityOrder);
    }

    public ObservableCollection<PlanOption> PlanOptions { get; } = [];

    public ObservableCollection<FacilityOption> FacilityOptions { get; } = [];

    public IReadOnlyList<IntOption> ModeOptions => _modeOptions;

    public IReadOnlyList<StringOption> DroneOptions => _droneOptions;

    public IReadOnlyList<StringOption> DefaultInfrastOptions => _defaultInfrastOptions;

    public IntOption? SelectedModeOption
    {
        get => ResolveSelectedOption(ModeOptions, Mode);
        set => Mode = value?.Value ?? InfrastModeNormal;
    }

    public StringOption? SelectedDroneOption
    {
        get => ResolveSelectedOption(DroneOptions, Drones);
        set => Drones = value?.Value ?? "Money";
    }

    public StringOption? SelectedDefaultInfrastOption
    {
        get => ResolveSelectedOption(DefaultInfrastOptions, DefaultInfrast);
        set => DefaultInfrast = value?.Value ?? UserDefinedInfrast;
    }

    public int Mode
    {
        get => _mode;
        set
        {
            var normalized = value switch
            {
                InfrastModeCustom => InfrastModeCustom,
                InfrastModeRotation => InfrastModeRotation,
                _ => InfrastModeNormal,
            };
            if (!SetProperty(ref _mode, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedModeOption));
            OnPropertyChanged(nameof(IsCustomMode));
            OnPropertyChanged(nameof(IsRotationMode));
            OnPropertyChanged(nameof(ShowCustomModeSettings));
            OnPropertyChanged(nameof(ShowRotationTip));
            OnPropertyChanged(nameof(ShowThresholdSettings));
            OnPropertyChanged(nameof(ShowDormFilterSettings));
            OnPropertyChanged(nameof(CanEditDrones));

            QueuePersist();
            if (!IsCustomMode)
            {
                PlanOptions.Clear();
                _parsedPlans.Clear();
                SelectedPlan = null;
                LastErrorMessage = string.Empty;
            }
        }
    }

    public string Drones
    {
        get => _drones;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? "Money" : value.Trim();
            if (!SetProperty(ref _drones, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedDroneOption));
            QueuePersist();
        }
    }

    public int DormThresholdPercent
    {
        get => _dormThresholdPercent;
        set
        {
            var normalized = Math.Clamp(value, 0, 100);
            if (!SetProperty(ref _dormThresholdPercent, normalized))
            {
                return;
            }

            QueuePersist();
        }
    }

    public string CustomFilePath
    {
        get => _customFilePath;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetProperty(ref _customFilePath, normalized))
            {
                return;
            }

            QueuePersist();
        }
    }

    public int SelectedPlanIndex
    {
        get => _selectedPlanIndex;
        set
        {
            if (!SetProperty(ref _selectedPlanIndex, value))
            {
                return;
            }

            if (SelectedPlan?.Index != value)
            {
                _selectedPlan = PlanOptions.FirstOrDefault(option => option.Index == value);
                OnPropertyChanged(nameof(SelectedPlan));
            }

            QueuePersist();
        }
    }

    public PlanOption? SelectedPlan
    {
        get => _selectedPlan;
        set
        {
            if (!SetProperty(ref _selectedPlan, value))
            {
                return;
            }

            var index = value?.Index ?? -1;
            if (_selectedPlanIndex != index)
            {
                _selectedPlanIndex = index;
                OnPropertyChanged(nameof(SelectedPlanIndex));
            }

            QueuePersist();
        }
    }

    public string DefaultInfrast
    {
        get => _defaultInfrast;
        set
        {
            var normalized = string.IsNullOrWhiteSpace(value) ? UserDefinedInfrast : value.Trim();
            if (!SetProperty(ref _defaultInfrast, normalized))
            {
                return;
            }

            var readOnly = !IsUserDefinedDefault(normalized);
            if (SetProperty(ref _isCustomFileReadOnly, readOnly, nameof(IsCustomFileReadOnly)))
            {
                // no-op
            }

            OnPropertyChanged(nameof(SelectedDefaultInfrastOption));

            if (readOnly)
            {
                var defaultPath = Path.Combine(AppContext.BaseDirectory, "resource", "custom_infrast", normalized);
                CustomFilePath = defaultPath;
            }

            if (!_suppressDefaultInfrastPersist)
            {
                _ = PersistDefaultInfrastAsync(normalized);
            }
        }
    }

    public bool IsCustomFileReadOnly => _isCustomFileReadOnly;

    public bool IsCustomMode => Mode == InfrastModeCustom;

    public bool IsRotationMode => Mode == InfrastModeRotation;

    public bool ShowCustomModeSettings => IsCustomMode;

    public bool ShowRotationTip => IsRotationMode;

    public bool ShowThresholdSettings => !IsRotationMode;

    public bool ShowDormFilterSettings => !IsRotationMode;

    public bool CanEditDrones => !IsCustomMode;

    public bool ContinueTraining
    {
        get => _continueTraining;
        set
        {
            if (!SetProperty(ref _continueTraining, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool DormTrustEnabled
    {
        get => _dormTrustEnabled;
        set
        {
            if (!SetProperty(ref _dormTrustEnabled, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool DormNotStationedEnabled
    {
        get => _dormNotStationedEnabled;
        set
        {
            if (!SetProperty(ref _dormNotStationedEnabled, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Replenish
    {
        get => _replenish;
        set
        {
            if (!SetProperty(ref _replenish, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool ReceptionMessageBoard
    {
        get => _receptionMessageBoard;
        set
        {
            if (!SetProperty(ref _receptionMessageBoard, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool ReceptionClueExchange
    {
        get => _receptionClueExchange;
        set
        {
            if (!SetProperty(ref _receptionClueExchange, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool ReceptionSendClue
    {
        get => _receptionSendClue;
        set
        {
            if (!SetProperty(ref _receptionSendClue, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public void SelectAllFacility()
    {
        _suppressFacilitySelectionChanged = true;
        try
        {
            foreach (var option in FacilityOptions)
            {
                option.IsSelected = true;
            }
        }
        finally
        {
            _suppressFacilitySelectionChanged = false;
        }

        QueuePersist();
    }

    public void ClearFacility()
    {
        _suppressFacilitySelectionChanged = true;
        try
        {
            foreach (var option in FacilityOptions)
            {
                option.IsSelected = false;
            }
        }
        finally
        {
            _suppressFacilitySelectionChanged = false;
        }

        QueuePersist();
    }

    public void SelectCustomFile(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return;
        }

        DefaultInfrast = UserDefinedInfrast;
        CustomFilePath = path;
    }

    public Task ReloadPersistentConfigAsync(CancellationToken cancellationToken = default)
    {
        ApplyPersistentDefaultInfrast(ResolveDefaultInfrast());
        return Task.CompletedTask;
    }

    public async Task ReloadPlansAsync(CancellationToken cancellationToken = default)
    {
        _parsedPlans.Clear();
        PlanOptions.Clear();
        if (!IsCustomMode || string.IsNullOrWhiteSpace(CustomFilePath))
        {
            LastErrorMessage = string.Empty;
            StatusMessage = string.Empty;
            return;
        }

        if (!File.Exists(CustomFilePath))
        {
            LastErrorMessage = string.Format(
                Texts.GetOrDefault("Infrast.Error.CustomFileNotFound", "Custom file not found: {0}"),
                CustomFilePath);
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                "Infrast.ParsePlan",
                UiOperationResult.Fail(UiErrorCode.InfrastPlanParseFailed, LastErrorMessage),
                cancellationToken);
            return;
        }

        try
        {
            var json = await File.ReadAllTextAsync(CustomFilePath, cancellationToken);
            var root = JsonNode.Parse(json) as JsonObject;
            if (root?["plans"] is not JsonArray plansArray)
            {
                throw new JsonException("`plans` section missing.");
            }

            var index = 0;
            foreach (var node in plansArray.OfType<JsonObject>())
            {
                var name = node["name"]?.GetValue<string?>() ?? string.Format(
                    Texts.GetOrDefault("Infrast.Plan.DefaultName", "Plan {0}"),
                    index + 1);
                var periods = ParsePlanPeriods(node["period"]);
                _parsedPlans.Add(new ParsedPlan(index, name, periods));
                index++;
            }

            if (_parsedPlans.Any(plan => plan.Periods.Count > 0))
            {
                PlanOptions.Add(new PlanOption(-1, BuildAutoPlanDisplayName()));
            }

            foreach (var plan in _parsedPlans)
            {
                PlanOptions.Add(new PlanOption(plan.Index, plan.Name));
            }

            SelectedPlan = PlanOptions.FirstOrDefault(option => option.Index == SelectedPlanIndex);
            if (SelectedPlan is null && SelectedPlanIndex >= 0)
            {
                var message = string.Format(
                    Texts.GetOrDefault("Infrast.Error.PlanOutOfRange", "Plan index {0} is out of range for `{1}`."),
                    SelectedPlanIndex,
                    CustomFilePath);
                LastErrorMessage = message;
                await Runtime.DiagnosticsService.RecordFailedResultAsync(
                    "Infrast.ParsePlan",
                    UiOperationResult.Fail(UiErrorCode.InfrastPlanOutOfRange, message),
                    cancellationToken);
            }
            else
            {
                LastErrorMessage = string.Empty;
            }

            StatusMessage = string.Format(
                Texts.GetOrDefault("Infrast.Status.LoadedPlans", "Loaded {0} plans."),
                PlanOptions.Count);
        }
        catch (Exception ex)
        {
            LastErrorMessage = Texts.GetOrDefault("Infrast.Error.ParseFailed", "Failed to parse custom infrast file.");
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                "Infrast.ParsePlan",
                UiOperationResult.Fail(UiErrorCode.InfrastPlanParseFailed, LastErrorMessage, ex.Message),
                cancellationToken);
        }
    }

    protected override async Task LoadFromParametersAsync(JsonObject parameters, CancellationToken cancellationToken)
    {
        var model = InfrastParams.FromJson(parameters);
        Mode = model.Mode;
        Drones = model.Drones;
        DormThresholdPercent = Math.Clamp((int)Math.Round(model.Threshold * 100), 0, 100);
        CustomFilePath = model.Filename;
        SelectedPlanIndex = model.PlanIndex;
        ContinueTraining = model.ContinueTraining;
        DormTrustEnabled = model.DormTrustEnabled;
        DormNotStationedEnabled = model.DormNotStationedEnabled;
        Replenish = model.Replenish;
        ReceptionMessageBoard = model.ReceptionMessageBoard;
        ReceptionClueExchange = model.ReceptionClueExchange;
        ReceptionSendClue = model.ReceptionSendClue;
        ConfigureFacilityOptions(model.Facility);

        await ReloadPlansAsync(cancellationToken);
    }

    protected override JsonObject BuildParameters()
    {
        var facilities = FacilityOptions
            .Where(option => option.IsSelected)
            .Select(option => option.Value)
            .ToList();

        var model = new InfrastParams
        {
            Mode = Mode,
            Facility = facilities,
            Drones = Drones,
            ContinueTraining = ContinueTraining,
            Threshold = DormThresholdPercent / 100.0,
            DormTrustEnabled = DormTrustEnabled,
            DormNotStationedEnabled = DormNotStationedEnabled,
            Replenish = Replenish,
            ReceptionMessageBoard = ReceptionMessageBoard,
            ReceptionClueExchange = ReceptionClueExchange,
            ReceptionSendClue = ReceptionSendClue,
            Filename = CustomFilePath,
            PlanIndex = SelectedPlanIndex,
        };

        return model.ToJson();
    }

    private void OnTextsChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName is not (nameof(LocalizedTextMap.Language) or "Item[]"))
        {
            return;
        }

        RebuildLocalizedOptions();
        if (PlanOptions.Count > 0 && PlanOptions[0].Index == -1)
        {
            PlanOptions[0] = new PlanOption(-1, BuildAutoPlanDisplayName());
            if (SelectedPlanIndex == -1)
            {
                SelectedPlan = PlanOptions[0];
            }
        }
    }

    private void RebuildLocalizedOptions()
    {
        _modeOptions =
        [
            new IntOption(InfrastModeNormal, Texts.GetOrDefault("Infrast.Mode.Normal", "Default")),
            new IntOption(InfrastModeRotation, Texts.GetOrDefault("Infrast.Mode.Rotation", "Rotation")),
            new IntOption(InfrastModeCustom, Texts.GetOrDefault("Infrast.Mode.Custom", "Custom")),
        ];

        _droneOptions =
        [
            new StringOption("_NotUse", Texts.GetOrDefault("Infrast.Drone.NotUse", "Do not use drones")),
            new StringOption("Money", Texts.GetOrDefault("Infrast.Drone.Money", "LMD")),
            new StringOption("SyntheticJade", Texts.GetOrDefault("Infrast.Drone.SyntheticJade", "Originium shard")),
            new StringOption("CombatRecord", Texts.GetOrDefault("Infrast.Drone.CombatRecord", "Battle record")),
            new StringOption("PureGold", Texts.GetOrDefault("Infrast.Drone.PureGold", "Pure gold")),
            new StringOption("OriginStone", Texts.GetOrDefault("Infrast.Drone.OriginStone", "Originium")),
            new StringOption("Chip", Texts.GetOrDefault("Infrast.Drone.Chip", "Chip")),
        ];

        _defaultInfrastOptions =
        [
            new StringOption(UserDefinedInfrast, Texts.GetOrDefault("Infrast.Default.UserDefined", "User defined")),
            new StringOption("153_layout_3_times_a_day.json", Texts.GetOrDefault("Infrast.Default.153Time3", "153 (3 shifts/day)")),
            new StringOption("153_layout_4_times_a_day.json", Texts.GetOrDefault("Infrast.Default.153Time4", "153 (4 shifts/day)")),
            new StringOption("243_layout_3_times_a_day.json", Texts.GetOrDefault("Infrast.Default.243Time3", "243 (3 shifts/day)")),
            new StringOption("243_layout_4_times_a_day.json", Texts.GetOrDefault("Infrast.Default.243Time4", "243 (4 shifts/day)")),
            new StringOption("333_layout_for_Orundum_3_times_a_day.json", Texts.GetOrDefault("Infrast.Default.333Time3", "333 (orundum, 3 shifts/day)")),
        ];

        OnPropertyChanged(nameof(ModeOptions));
        OnPropertyChanged(nameof(DroneOptions));
        OnPropertyChanged(nameof(DefaultInfrastOptions));
        OnPropertyChanged(nameof(SelectedModeOption));
        OnPropertyChanged(nameof(SelectedDroneOption));
        OnPropertyChanged(nameof(SelectedDefaultInfrastOption));
    }

    private void ConfigureFacilityOptions(IEnumerable<string> enabledFacilities)
    {
        var enabled = enabledFacilities
            .Where(name => !string.IsNullOrWhiteSpace(name))
            .Select(name => name.Trim())
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
        var enabledSet = enabled.ToHashSet(StringComparer.OrdinalIgnoreCase);
        var orderedNames = new List<string>(enabled);

        foreach (var room in DefaultFacilityOrder)
        {
            if (!orderedNames.Contains(room, StringComparer.OrdinalIgnoreCase))
            {
                orderedNames.Add(room);
            }
        }

        foreach (var item in FacilityOptions)
        {
            item.PropertyChanged -= OnFacilityOptionChanged;
        }

        _suppressFacilitySelectionChanged = true;
        try
        {
            FacilityOptions.Clear();
            foreach (var name in orderedNames)
            {
                var option = new FacilityOption(name, name)
                {
                    IsSelected = enabledSet.Contains(name),
                };
                option.PropertyChanged += OnFacilityOptionChanged;
                FacilityOptions.Add(option);
            }
        }
        finally
        {
            _suppressFacilitySelectionChanged = false;
        }
    }

    private void OnFacilityOptionChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressFacilitySelectionChanged || e.PropertyName != nameof(FacilityOption.IsSelected))
        {
            return;
        }

        QueuePersist();
    }

    private string ResolveDefaultInfrast()
    {
        if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile)
            && profile.Values.TryGetValue(ConfigurationKeys.DefaultInfrast, out var profileNode)
            && TryReadString(profileNode, out var profileValue)
            && !string.IsNullOrWhiteSpace(profileValue))
        {
            return profileValue.Trim();
        }

        var globals = Runtime.ConfigurationService.CurrentConfig.GlobalValues;
        if (!globals.TryGetValue(ConfigurationKeys.DefaultInfrast, out var node) || node is not JsonValue value)
        {
            return UserDefinedInfrast;
        }

        return value.TryGetValue(out string? resolved) && !string.IsNullOrWhiteSpace(resolved)
            ? resolved
            : UserDefinedInfrast;
    }

    private async Task PersistDefaultInfrastAsync(string value)
    {
        try
        {
            if (Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
            {
                profile.Values[ConfigurationKeys.DefaultInfrast] = JsonValue.Create(value);
            }
            else
            {
                Runtime.ConfigurationService.CurrentConfig.GlobalValues[ConfigurationKeys.DefaultInfrast] = JsonValue.Create(value);
            }

            await Runtime.ConfigurationService.SaveAsync();
        }
        catch (Exception ex)
        {
            await Runtime.DiagnosticsService.RecordErrorAsync(
                "Infrast.DefaultInfrast.Save",
                "Failed to persist default infrast preset.",
                ex);
        }
    }

    private static bool IsUserDefinedDefault(string value)
        => string.Equals(value, UserDefinedInfrast, StringComparison.OrdinalIgnoreCase);

    private void ApplyPersistentDefaultInfrast(string value)
    {
        var normalized = string.IsNullOrWhiteSpace(value) ? UserDefinedInfrast : value.Trim();
        var readOnly = !IsUserDefinedDefault(normalized);

        _suppressDefaultInfrastPersist = true;
        try
        {
            SetProperty(ref _defaultInfrast, normalized, nameof(DefaultInfrast));
            SetProperty(ref _isCustomFileReadOnly, readOnly, nameof(IsCustomFileReadOnly));
            OnPropertyChanged(nameof(SelectedDefaultInfrastOption));
        }
        finally
        {
            _suppressDefaultInfrastPersist = false;
        }
    }

    private static bool TryReadString(JsonNode? node, out string value)
    {
        if (node is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? text)
            && !string.IsNullOrWhiteSpace(text))
        {
            value = text;
            return true;
        }

        var raw = node?.ToString();
        if (!string.IsNullOrWhiteSpace(raw))
        {
            value = raw;
            return true;
        }

        value = string.Empty;
        return false;
    }

    private string BuildAutoPlanDisplayName()
    {
        var currentPlanName = ResolveCurrentPlanName() ?? "???";
        return string.Format(
            Texts.GetOrDefault("Infrast.Plan.AutoCurrent", "Auto by time ({0})"),
            currentPlanName);
    }

    private string? ResolveCurrentPlanName()
    {
        var now = TimeOnly.FromDateTime(DateTime.Now.ToLocalTime());
        var current = _parsedPlans.FirstOrDefault(plan => plan.Periods.Any(period => period.Contains(now)));
        current ??= _parsedPlans.FirstOrDefault();
        return current?.Name;
    }

    private static List<TimeRange> ParsePlanPeriods(JsonNode? node)
    {
        if (node is not JsonArray array)
        {
            return [];
        }

        var periods = new List<TimeRange>();
        foreach (var item in array)
        {
            if (TryParsePeriodNode(item, out var period))
            {
                periods.Add(period);
            }
        }

        return periods;
    }

    private static bool TryParsePeriodNode(JsonNode? node, out TimeRange period)
    {
        period = default;
        if (node is null)
        {
            return false;
        }

        if (node is JsonValue stringNode && stringNode.TryGetValue(out string? text))
        {
            return TryParseRangeString(text, out period);
        }

        if (node is not JsonArray rangeArray || rangeArray.Count < 2)
        {
            return false;
        }

        var start = rangeArray[0]?.GetValue<string?>();
        var end = rangeArray[1]?.GetValue<string?>();
        if (!TryParseTime(start, out var startTime) || !TryParseTime(end, out var endTime))
        {
            return false;
        }

        period = new TimeRange(startTime, endTime);
        return true;
    }

    private static bool TryParseRangeString(string? value, out TimeRange period)
    {
        period = default;
        if (string.IsNullOrWhiteSpace(value))
        {
            return false;
        }

        var segments = value.Split('-', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        if (segments.Length != 2)
        {
            return false;
        }

        if (!TryParseTime(segments[0], out var start) || !TryParseTime(segments[1], out var end))
        {
            return false;
        }

        period = new TimeRange(start, end);
        return true;
    }

    private static bool TryParseTime(string? value, out TimeOnly result)
    {
        if (!TimeOnly.TryParseExact(value, "HH:mm", CultureInfo.InvariantCulture, DateTimeStyles.None, out result))
        {
            return TimeOnly.TryParse(value, CultureInfo.InvariantCulture, DateTimeStyles.None, out result);
        }

        return true;
    }

    private static IntOption? ResolveSelectedOption(IReadOnlyList<IntOption> options, int value)
    {
        return options.FirstOrDefault(option => option.Value == value)
               ?? options.FirstOrDefault();
    }

    private static StringOption? ResolveSelectedOption(IReadOnlyList<StringOption> options, string value)
    {
        return options.FirstOrDefault(option => string.Equals(option.Value, value, StringComparison.OrdinalIgnoreCase))
               ?? options.FirstOrDefault();
    }

    public sealed record PlanOption(int Index, string Display);

    public sealed record IntOption(int Value, string DisplayName);

    public sealed record StringOption(string Value, string DisplayName);

    public sealed class FacilityOption : ObservableObject
    {
        private bool _isSelected;

        public FacilityOption(string value, string displayName)
        {
            Value = value;
            DisplayName = displayName;
        }

        public string Value { get; }

        public string DisplayName { get; }

        public bool IsSelected
        {
            get => _isSelected;
            set => SetProperty(ref _isSelected, value);
        }
    }

    private sealed record ParsedPlan(int Index, string Name, List<TimeRange> Periods);

    private readonly record struct TimeRange(TimeOnly Start, TimeOnly End)
    {
        public bool Contains(TimeOnly now)
        {
            return Start <= End
                ? now >= Start && now <= End
                : now >= Start || now <= End;
        }
    }
}
