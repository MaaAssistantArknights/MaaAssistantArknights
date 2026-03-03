using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class FightTaskModuleViewModel : TypedTaskModuleViewModelBase<FightTaskParamsDto>
{
    private string _stage = string.Empty;
    private bool _useMedicine;
    private int _medicine;
    private bool _useStone;
    private int _stone;
    private bool _enableTimesLimit;
    private int _times = int.MaxValue;
    private int _series = 1;
    private bool _isDrGrandet;
    private bool _useExpiringMedicine;
    private bool _enableTargetDrop;
    private string _dropId = string.Empty;
    private int _dropCount = 1;
    private bool _useCustomAnnihilation;
    private string _annihilationStage = "Annihilation";
    private bool _useAlternateStage;
    private bool _hideUnavailableStage = true;
    private string _stageResetMode = "Current";

    public FightTaskModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Fight")
    {
    }

    public IReadOnlyList<string> StageResetModeOptions { get; } = ["Current", "Ignore", "Reset"];

    public string Stage
    {
        get => _stage;
        set => SetTrackedProperty(ref _stage, value);
    }

    public bool UseMedicine
    {
        get => _useMedicine;
        set => SetTrackedProperty(ref _useMedicine, value);
    }

    public int Medicine
    {
        get => _medicine;
        set => SetTrackedProperty(ref _medicine, Math.Max(0, value));
    }

    public bool UseStone
    {
        get => _useStone;
        set => SetTrackedProperty(ref _useStone, value);
    }

    public int Stone
    {
        get => _stone;
        set => SetTrackedProperty(ref _stone, Math.Max(0, value));
    }

    public bool EnableTimesLimit
    {
        get => _enableTimesLimit;
        set => SetTrackedProperty(ref _enableTimesLimit, value);
    }

    public int Times
    {
        get => _times;
        set => SetTrackedProperty(ref _times, Math.Max(0, value));
    }

    public int Series
    {
        get => _series;
        set => SetTrackedProperty(ref _series, value);
    }

    public bool IsDrGrandet
    {
        get => _isDrGrandet;
        set => SetTrackedProperty(ref _isDrGrandet, value);
    }

    public bool UseExpiringMedicine
    {
        get => _useExpiringMedicine;
        set => SetTrackedProperty(ref _useExpiringMedicine, value);
    }

    public bool EnableTargetDrop
    {
        get => _enableTargetDrop;
        set => SetTrackedProperty(ref _enableTargetDrop, value);
    }

    public string DropId
    {
        get => _dropId;
        set => SetTrackedProperty(ref _dropId, value);
    }

    public int DropCount
    {
        get => _dropCount;
        set => SetTrackedProperty(ref _dropCount, Math.Max(1, value));
    }

    public bool UseCustomAnnihilation
    {
        get => _useCustomAnnihilation;
        set => SetTrackedProperty(ref _useCustomAnnihilation, value);
    }

    public string AnnihilationStage
    {
        get => _annihilationStage;
        set => SetTrackedProperty(ref _annihilationStage, value);
    }

    public bool UseAlternateStage
    {
        get => _useAlternateStage;
        set => SetTrackedProperty(ref _useAlternateStage, value);
    }

    public bool HideUnavailableStage
    {
        get => _hideUnavailableStage;
        set => SetTrackedProperty(ref _hideUnavailableStage, value);
    }

    public string StageResetMode
    {
        get => _stageResetMode;
        set => SetTrackedProperty(ref _stageResetMode, value);
    }

    protected override Task<UiOperationResult<FightTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetFightParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, FightTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveFightParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(FightTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileFight(dto, profile, config);
    }

    protected override void ApplyDto(FightTaskParamsDto dto)
    {
        Stage = dto.Stage;
        UseMedicine = dto.UseMedicine;
        Medicine = dto.Medicine;
        UseStone = dto.UseStone;
        Stone = dto.Stone;
        EnableTimesLimit = dto.EnableTimesLimit;
        Times = dto.Times;
        Series = dto.Series;
        IsDrGrandet = dto.IsDrGrandet;
        UseExpiringMedicine = dto.UseExpiringMedicine;
        EnableTargetDrop = dto.EnableTargetDrop;
        DropId = dto.DropId;
        DropCount = dto.DropCount;
        UseCustomAnnihilation = dto.UseCustomAnnihilation;
        AnnihilationStage = dto.AnnihilationStage;
        UseAlternateStage = dto.UseAlternateStage;
        HideUnavailableStage = dto.HideUnavailableStage;
        StageResetMode = dto.StageResetMode;
    }

    protected override FightTaskParamsDto BuildDto()
    {
        return new FightTaskParamsDto
        {
            Stage = Stage.Trim(),
            UseMedicine = UseMedicine,
            Medicine = Math.Max(0, Medicine),
            UseStone = UseStone,
            Stone = Math.Max(0, Stone),
            EnableTimesLimit = EnableTimesLimit,
            Times = EnableTimesLimit ? Math.Max(0, Times) : int.MaxValue,
            Series = Series,
            IsDrGrandet = IsDrGrandet,
            UseExpiringMedicine = UseExpiringMedicine,
            EnableTargetDrop = EnableTargetDrop,
            DropId = DropId.Trim(),
            DropCount = Math.Max(1, DropCount),
            UseCustomAnnihilation = UseCustomAnnihilation,
            AnnihilationStage = string.IsNullOrWhiteSpace(AnnihilationStage) ? "Annihilation" : AnnihilationStage.Trim(),
            UseAlternateStage = UseAlternateStage,
            HideUnavailableStage = HideUnavailableStage,
            StageResetMode = string.IsNullOrWhiteSpace(StageResetMode) ? "Current" : StageResetMode.Trim(),
        };
    }
}
