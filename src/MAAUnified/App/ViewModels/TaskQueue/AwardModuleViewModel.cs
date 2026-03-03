using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class AwardModuleViewModel : TaskModuleSettingsViewModelBase
{
    private bool _award = true;
    private bool _mail;
    private bool _recruit;
    private bool _orundum;
    private bool _mining;
    private bool _specialAccess;
    private bool _pendingRecruitConfirmation;
    private bool _allowRecruitEnable;

    public AwardModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, TaskModuleTypes.Award)
    {
    }

    public bool Award
    {
        get => _award;
        set
        {
            if (!SetProperty(ref _award, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Mail
    {
        get => _mail;
        set
        {
            if (!SetProperty(ref _mail, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Recruit
    {
        get => _recruit;
        set
        {
            if (value && !_recruit && !_allowRecruitEnable)
            {
                PendingRecruitConfirmation = true;
                StatusMessage = Texts["Award.RecruitTip"];
                return;
            }

            _allowRecruitEnable = false;
            PendingRecruitConfirmation = false;

            if (!SetProperty(ref _recruit, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Orundum
    {
        get => _orundum;
        set
        {
            if (!SetProperty(ref _orundum, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Mining
    {
        get => _mining;
        set
        {
            if (!SetProperty(ref _mining, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool SpecialAccess
    {
        get => _specialAccess;
        set
        {
            if (!SetProperty(ref _specialAccess, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool PendingRecruitConfirmation
    {
        get => _pendingRecruitConfirmation;
        private set => SetProperty(ref _pendingRecruitConfirmation, value);
    }

    public void ConfirmRecruitEnable()
    {
        _allowRecruitEnable = true;
        Recruit = true;
    }

    public void CancelRecruitEnable()
    {
        PendingRecruitConfirmation = false;
        _allowRecruitEnable = false;
        StatusMessage = string.Empty;
    }

    protected override Task LoadFromParametersAsync(JsonObject parameters, CancellationToken cancellationToken)
    {
        var model = AwardParams.FromJson(parameters);
        _allowRecruitEnable = model.Recruit;
        Award = model.Award;
        Mail = model.Mail;
        Recruit = model.Recruit;
        Orundum = model.Orundum;
        Mining = model.Mining;
        SpecialAccess = model.SpecialAccess;
        PendingRecruitConfirmation = false;
        _allowRecruitEnable = false;
        return Task.CompletedTask;
    }

    protected override JsonObject BuildParameters()
    {
        return new AwardParams
        {
            Award = Award,
            Mail = Mail,
            Recruit = Recruit,
            Orundum = Orundum,
            Mining = Mining,
            SpecialAccess = SpecialAccess,
        }.ToJson();
    }
}
