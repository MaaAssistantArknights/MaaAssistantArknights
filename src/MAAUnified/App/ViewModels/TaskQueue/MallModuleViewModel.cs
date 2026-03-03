using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class MallModuleViewModel : TaskModuleSettingsViewModelBase
{
    private bool _creditFight;
    private bool _creditFightOnceADay = true;
    private int _formationIndex;
    private bool _visitFriends = true;
    private bool _visitFriendsOnceADay;
    private bool _shopping = true;
    private string _buyFirstText = string.Empty;
    private string _blacklistText = string.Empty;
    private bool _forceShoppingIfCreditFull;
    private bool _onlyBuyDiscount;
    private bool _reserveMaxCredit;

    public MallModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
        : base(runtime, texts, TaskModuleTypes.Mall)
    {
    }

    public bool CreditFight
    {
        get => _creditFight;
        set
        {
            if (!SetProperty(ref _creditFight, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool CreditFightOnceADay
    {
        get => _creditFightOnceADay;
        set
        {
            if (!SetProperty(ref _creditFightOnceADay, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public int FormationIndex
    {
        get => _formationIndex;
        set
        {
            var normalized = Math.Clamp(value, 0, 4);
            if (!SetProperty(ref _formationIndex, normalized))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool VisitFriends
    {
        get => _visitFriends;
        set
        {
            if (!SetProperty(ref _visitFriends, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool VisitFriendsOnceADay
    {
        get => _visitFriendsOnceADay;
        set
        {
            if (!SetProperty(ref _visitFriendsOnceADay, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool Shopping
    {
        get => _shopping;
        set
        {
            if (!SetProperty(ref _shopping, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public string BuyFirstText
    {
        get => _buyFirstText;
        set
        {
            if (!SetProperty(ref _buyFirstText, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public string BlacklistText
    {
        get => _blacklistText;
        set
        {
            if (!SetProperty(ref _blacklistText, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool ForceShoppingIfCreditFull
    {
        get => _forceShoppingIfCreditFull;
        set
        {
            if (!SetProperty(ref _forceShoppingIfCreditFull, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool OnlyBuyDiscount
    {
        get => _onlyBuyDiscount;
        set
        {
            if (!SetProperty(ref _onlyBuyDiscount, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    public bool ReserveMaxCredit
    {
        get => _reserveMaxCredit;
        set
        {
            if (!SetProperty(ref _reserveMaxCredit, value))
            {
                return;
            }

            QueuePersist();
        }
    }

    protected override Task LoadFromParametersAsync(JsonObject parameters, CancellationToken cancellationToken)
    {
        var model = MallParams.FromJson(parameters);
        CreditFight = model.CreditFight;
        CreditFightOnceADay = model.CreditFightOnceADay;
        FormationIndex = model.FormationIndex;
        VisitFriends = model.VisitFriends;
        VisitFriendsOnceADay = model.VisitFriendsOnceADay;
        Shopping = model.Shopping;
        BuyFirstText = string.Join(";", model.BuyFirst);
        BlacklistText = string.Join(";", model.Blacklist);
        ForceShoppingIfCreditFull = model.ForceShoppingIfCreditFull;
        OnlyBuyDiscount = model.OnlyBuyDiscount;
        ReserveMaxCredit = model.ReserveMaxCredit;
        return Task.CompletedTask;
    }

    protected override JsonObject BuildParameters()
    {
        var model = new MallParams
        {
            CreditFight = CreditFight,
            CreditFightOnceADay = CreditFightOnceADay,
            FormationIndex = FormationIndex,
            VisitFriends = VisitFriends,
            VisitFriendsOnceADay = VisitFriendsOnceADay,
            Shopping = Shopping,
            BuyFirst = SplitList(BuyFirstText),
            Blacklist = SplitList(BlacklistText),
            ForceShoppingIfCreditFull = ForceShoppingIfCreditFull,
            OnlyBuyDiscount = OnlyBuyDiscount,
            ReserveMaxCredit = ReserveMaxCredit,
        };

        return model.ToJson();
    }

    private static List<string> SplitList(string text)
    {
        return text
            .Replace('；', ';')
            .Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Where(t => !string.IsNullOrWhiteSpace(t))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
    }
}
