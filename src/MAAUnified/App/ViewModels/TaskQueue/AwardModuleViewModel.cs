using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class AwardModuleViewModel : TaskModuleSettingsViewModelBase
{
    private bool _award = true;
    private bool _mail;
    private bool _freeGacha;
    private bool _orundum;
    private bool _mining;
    private bool _specialAccess;
    private bool _pendingFreeGachaConfirmation;
    private bool _allowFreeGachaEnable;

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

    public bool FreeGacha
    {
        get => _freeGacha;
        set
        {
            if (value && !_freeGacha && !_allowFreeGachaEnable)
            {
                PendingFreeGachaConfirmation = true;
                StatusMessage = Texts["Award.GachaWarning"];
                return;
            }

            _allowFreeGachaEnable = false;
            PendingFreeGachaConfirmation = false;

            if (!SetProperty(ref _freeGacha, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    // Backward-compatible alias used by existing tests.
    public bool Recruit
    {
        get => FreeGacha;
        set => FreeGacha = value;
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

    public bool PendingFreeGachaConfirmation
    {
        get => _pendingFreeGachaConfirmation;
        private set => SetProperty(ref _pendingFreeGachaConfirmation, value);
    }

    // Backward-compatible alias used by existing tests.
    public bool PendingRecruitConfirmation => PendingFreeGachaConfirmation;

    public void ConfirmFreeGachaEnable()
    {
        _allowFreeGachaEnable = true;
        FreeGacha = true;
    }

    public void CancelFreeGachaEnable()
    {
        PendingFreeGachaConfirmation = false;
        _allowFreeGachaEnable = false;
        StatusMessage = string.Empty;
    }

    // Backward-compatible aliases used by existing tests/event handlers.
    public void ConfirmRecruitEnable() => ConfirmFreeGachaEnable();

    public void CancelRecruitEnable() => CancelFreeGachaEnable();

    protected override Task LoadFromParametersAsync(JsonObject parameters, CancellationToken cancellationToken)
    {
        var model = AwardParams.FromJson(parameters);
        _allowFreeGachaEnable = model.Recruit;
        Award = model.Award;
        Mail = model.Mail;
        FreeGacha = model.Recruit;
        Orundum = model.Orundum;
        Mining = model.Mining;
        SpecialAccess = model.SpecialAccess;
        PendingFreeGachaConfirmation = false;
        _allowFreeGachaEnable = false;
        return Task.CompletedTask;
    }

    protected override JsonObject BuildParameters()
    {
        return new AwardParams
        {
            Award = Award,
            Mail = Mail,
            Recruit = FreeGacha,
            Orundum = Orundum,
            Mining = Mining,
            SpecialAccess = SpecialAccess,
        }.ToJson();
    }
}
