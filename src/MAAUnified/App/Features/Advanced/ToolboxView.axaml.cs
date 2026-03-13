using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Toolbox;

namespace MAAUnified.App.Features.Advanced;

public partial class ToolboxView : UserControl
{
    private readonly DispatcherTimer _gachaEmphasisTimer;
    private double _gachaEmphasisPhase;
    private TranslateTransform? _gachaGradientTransform;

    public ToolboxView()
    {
        InitializeComponent();
        _gachaEmphasisTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(90),
        };
        _gachaEmphasisTimer.Tick += (_, _) => AnimateGachaDisclaimerText();

        AttachedToVisualTree += (_, _) => _gachaEmphasisTimer.Start();
        DetachedFromVisualTree += (_, _) => _gachaEmphasisTimer.Stop();
    }

    private ToolboxPageViewModel? VM => DataContext as ToolboxPageViewModel;

    private async void OnRecruitStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartRecruitAsync();
        }
    }

    private async void OnOperBoxStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartOperBoxAsync();
        }
    }

    private async void OnOperBoxExportClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        await CopyTextAsync(VM.OperBoxExportText);
        VM.NotifyOperBoxExportCopied();
    }

    private async void OnDepotStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartDepotAsync();
        }
    }

    private async void OnDepotExportArkPlannerClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        await CopyTextAsync(VM.ArkPlannerResult);
        VM.NotifyDepotExportCopied("ArkPlanner 数据");
    }

    private async void OnDepotExportLoliconClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        await CopyTextAsync(VM.LoliconResult);
        VM.NotifyDepotExportCopied("一图流数据");
    }

    private async void OnGachaAgreeDisclaimerClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        var owner = TopLevel.GetTopLevel(this) as Window;
        if (owner is null)
        {
            return;
        }

        var dialog = new WarningConfirmDialogView();
        dialog.ApplyRequest(
            DialogTextCatalog.WarningDialogTitle(VM.DialogLanguage),
            VM.GachaWarningText,
            confirmText: DialogTextCatalog.WarningDialogConfirmButton(VM.DialogLanguage),
            cancelText: DialogTextCatalog.WarningDialogCancelButton(VM.DialogLanguage),
            language: VM.DialogLanguage);
        if (await dialog.ShowDialog<bool>(owner))
        {
            VM.AgreeGachaDisclaimer();
        }
    }

    private async void OnGachaOnceClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartGachaAsync(once: true);
        }
    }

    private async void OnGachaTenTimesClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartGachaAsync(once: false);
        }
    }

    private async void OnGachaPeepClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        if (VM.IsGachaInProgress)
        {
            await VM.StopActiveToolAsync();
            return;
        }

        await VM.TogglePeepAsync();
    }

    private async void OnPeepCommandClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        if (VM.IsGachaInProgress)
        {
            await VM.StopActiveToolAsync();
        }
        else
        {
            await VM.TogglePeepAsync();
        }
    }

    private async void OnMiniGameCommandClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        if (VM.IsMiniGameRunning)
        {
            await VM.StopActiveToolAsync();
            return;
        }

        await VM.StartMiniGameAsync();
    }

    private async Task CopyTextAsync(string text)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.Clipboard is null || string.IsNullOrWhiteSpace(text))
        {
            return;
        }

        await topLevel.Clipboard.SetTextAsync(text);
    }

    private void AnimateGachaDisclaimerText()
    {
        if (GachaDisclaimerEmphasisText is null)
        {
            return;
        }

        _gachaEmphasisPhase += 0.16d;
        if (GachaDisclaimerEmphasisText.Foreground is LinearGradientBrush gradientBrush)
        {
            gradientBrush.SpreadMethod = GradientSpreadMethod.Repeat;
            _gachaGradientTransform ??= gradientBrush.Transform as TranslateTransform ?? new TranslateTransform();
            if (!ReferenceEquals(gradientBrush.Transform, _gachaGradientTransform))
            {
                gradientBrush.Transform = _gachaGradientTransform;
            }

            _gachaGradientTransform.X = (_gachaEmphasisPhase * 24d) % 280d;
        }

        GachaDisclaimerEmphasisText.Opacity = 0.96d + (Math.Sin(_gachaEmphasisPhase * 0.75d) * 0.04d);
    }
}
