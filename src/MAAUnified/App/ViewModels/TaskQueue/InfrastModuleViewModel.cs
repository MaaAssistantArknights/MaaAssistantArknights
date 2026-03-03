using System.Collections.ObjectModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class InfrastModuleViewModel : TaskModuleSettingsViewModelBase
{
    private int _mode;
    private string _drones = "Money";
    private int _dormThresholdPercent = 30;
    private string _customFilePath = string.Empty;
    private int _selectedPlanIndex = -1;
    private PlanOption? _selectedPlan;
    private string _facilityText = "Mfg\nTrade\nPower\nControl\nReception\nOffice\nDorm\nProcessing\nTraining";
    private bool _continueTraining;
    private bool _dormTrustEnabled = true;
    private bool _dormNotStationedEnabled = true;
    private bool _replenish = true;
    private bool _receptionMessageBoard = true;
    private bool _receptionClueExchange = true;
    private bool _receptionSendClue = true;

    public InfrastModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, TaskModuleTypes.Infrast)
    {
    }

    public ObservableCollection<PlanOption> PlanOptions { get; } = [];

    public IReadOnlyList<ModeOption> ModeOptions { get; } =
    [
        new(0, "Infrast.Mode.Normal"),
        new(10000, "Infrast.Mode.Custom"),
        new(20000, "Infrast.Mode.Rotation"),
    ];

    public IReadOnlyList<string> DroneOptions { get; } =
    [
        "_NotUse",
        "Money",
        "SyntheticJade",
        "CombatRecord",
        "PureGold",
        "OriginStone",
        "Chip",
    ];

    public int Mode
    {
        get => _mode;
        set
        {
            if (!SetProperty(ref _mode, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public string Drones
    {
        get => _drones;
        set
        {
            if (!SetProperty(ref _drones, value))
            {
                return;
            }

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
            if (!SetProperty(ref _customFilePath, value))
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

            var nextIndex = value?.Index ?? -1;
            if (_selectedPlanIndex != nextIndex)
            {
                _selectedPlanIndex = nextIndex;
                OnPropertyChanged(nameof(SelectedPlanIndex));
            }

            QueuePersist();
        }
    }

    public string FacilityText
    {
        get => _facilityText;
        set
        {
            if (!SetProperty(ref _facilityText, value))
            {
                return;
            }

            QueuePersist();
        }
    }

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

    public async Task ReloadPlansAsync(CancellationToken cancellationToken = default)
    {
        PlanOptions.Clear();
        if (Mode != 10000 || string.IsNullOrWhiteSpace(CustomFilePath))
        {
            LastErrorMessage = string.Empty;
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

            var hasTimedPlan = false;
            var index = 0;
            foreach (var node in plansArray.OfType<JsonObject>())
            {
                var name = node["name"]?.GetValue<string?>() ?? string.Format(
                    Texts.GetOrDefault("Infrast.Plan.DefaultName", "Plan {0}"),
                    index + 1);
                var periodCount = 0;
                if (node["period"] is JsonArray periodArray)
                {
                    periodCount = periodArray.Count;
                    hasTimedPlan |= periodCount > 0;
                }

                var display = string.Format(
                    Texts.GetOrDefault("Infrast.Plan.Display", "{0} ({1} period)"),
                    name,
                    periodCount);
                PlanOptions.Add(new PlanOption(index, display));
                index++;
            }

            if (hasTimedPlan)
            {
                PlanOptions.Insert(0, new PlanOption(-1, Texts["Infrast.Plan.Auto"]));
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

    public void SelectAllFacility()
    {
        FacilityText = string.Join(Environment.NewLine, new[]
        {
            "Mfg", "Trade", "Power", "Control", "Reception", "Office", "Dorm", "Processing", "Training",
        });
    }

    public void ClearFacility()
    {
        FacilityText = string.Empty;
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
        FacilityText = string.Join(Environment.NewLine, model.Facility);
        await ReloadPlansAsync(cancellationToken);
    }

    protected override JsonObject BuildParameters()
    {
        var facilities = FacilityText
            .Split(new[] { '\r', '\n', ';', ',' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Distinct(StringComparer.OrdinalIgnoreCase)
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

    public sealed record PlanOption(int Index, string Display);

    public sealed record ModeOption(int Value, string LabelKey);
}
