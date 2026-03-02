using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using MAAUnified.App.ViewModels;
using MAAUnified.App.Views;
using MAAUnified.Application.Services;

namespace MAAUnified.App;

public partial class App : Avalonia.Application
{
    public static MAAUnifiedRuntime Runtime { get; private set; } = null!;

    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        Runtime = MAAUnifiedRuntimeFactory.Create(AppContext.BaseDirectory);

        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var vm = new MainViewModel(Runtime);
            desktop.MainWindow = new MainWindow
            {
                DataContext = vm,
            };
            desktop.Exit += (_, _) => Runtime.DisposeAsync().AsTask().GetAwaiter().GetResult();

            _ = vm.InitializeAsync();
        }

        base.OnFrameworkInitializationCompleted();
    }
}
