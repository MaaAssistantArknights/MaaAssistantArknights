using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class RecruitTaskModuleViewModel : TypedTaskModuleViewModelBase<RecruitTaskParamsDto>
{
    private int _times = 4;
    private bool _refresh = true;
    private bool _forceRefresh = true;
    private bool _useExpedited;
    private bool _skipRobot = true;
    private int _extraTagsMode;
    private string _firstTagsText = string.Empty;
    private bool _chooseLevel3 = true;
    private bool _chooseLevel4 = true;
    private bool _chooseLevel5;
    private int _level3Time = 540;
    private int _level4Time = 540;
    private int _level5Time = 540;
    private bool _setTime = true;

    public RecruitTaskModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, "TaskQueue.Recruit")
    {
    }

    public int Times
    {
        get => _times;
        set => SetTrackedProperty(ref _times, Math.Max(0, value));
    }

    public bool Refresh
    {
        get => _refresh;
        set
        {
            if (!SetTrackedProperty(ref _refresh, value))
            {
                return;
            }

            if (!_refresh)
            {
                ForceRefresh = false;
            }
        }
    }

    public bool ForceRefresh
    {
        get => _forceRefresh;
        set => SetTrackedProperty(ref _forceRefresh, value);
    }

    public bool UseExpedited
    {
        get => _useExpedited;
        set => SetTrackedProperty(ref _useExpedited, value);
    }

    public bool SkipRobot
    {
        get => _skipRobot;
        set => SetTrackedProperty(ref _skipRobot, value);
    }

    public int ExtraTagsMode
    {
        get => _extraTagsMode;
        set => SetTrackedProperty(ref _extraTagsMode, value);
    }

    public string FirstTagsText
    {
        get => _firstTagsText;
        set => SetTrackedProperty(ref _firstTagsText, value);
    }

    public bool ChooseLevel3
    {
        get => _chooseLevel3;
        set => SetTrackedProperty(ref _chooseLevel3, value);
    }

    public bool ChooseLevel4
    {
        get => _chooseLevel4;
        set => SetTrackedProperty(ref _chooseLevel4, value);
    }

    public bool ChooseLevel5
    {
        get => _chooseLevel5;
        set => SetTrackedProperty(ref _chooseLevel5, value);
    }

    public int Level3Time
    {
        get => _level3Time;
        set => SetTrackedProperty(ref _level3Time, value);
    }

    public int Level4Time
    {
        get => _level4Time;
        set => SetTrackedProperty(ref _level4Time, value);
    }

    public int Level5Time
    {
        get => _level5Time;
        set => SetTrackedProperty(ref _level5Time, value);
    }

    public bool SetTime
    {
        get => _setTime;
        set => SetTrackedProperty(ref _setTime, value);
    }

    protected override Task<UiOperationResult<RecruitTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetRecruitParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, RecruitTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveRecruitParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(RecruitTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileRecruit(dto, profile, config);
    }

    protected override void ApplyDto(RecruitTaskParamsDto dto)
    {
        Times = dto.Times;
        Refresh = dto.Refresh;
        ForceRefresh = dto.ForceRefresh;
        UseExpedited = dto.UseExpedited;
        SkipRobot = dto.SkipRobot;
        ExtraTagsMode = dto.ExtraTagsMode;
        FirstTagsText = string.Join(Environment.NewLine, dto.FirstTags);
        ChooseLevel3 = dto.ChooseLevel3;
        ChooseLevel4 = dto.ChooseLevel4;
        ChooseLevel5 = dto.ChooseLevel5;
        Level3Time = dto.Level3Time;
        Level4Time = dto.Level4Time;
        Level5Time = dto.Level5Time;
        SetTime = dto.SetTime;
    }

    protected override RecruitTaskParamsDto BuildDto()
    {
        var firstTags = FirstTagsText
            .Split(new[] { '\r', '\n', ';', ',' }, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();

        return new RecruitTaskParamsDto
        {
            Times = Math.Max(0, Times),
            Refresh = Refresh,
            ForceRefresh = Refresh && ForceRefresh,
            UseExpedited = UseExpedited,
            SkipRobot = SkipRobot,
            ExtraTagsMode = ExtraTagsMode,
            FirstTags = firstTags,
            ChooseLevel3 = ChooseLevel3,
            ChooseLevel4 = ChooseLevel4,
            ChooseLevel5 = ChooseLevel5,
            Level3Time = Level3Time,
            Level4Time = Level4Time,
            Level5Time = Level5Time,
            SetTime = SetTime,
        };
    }
}
