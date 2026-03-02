using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels;

namespace MAAUnified.App.Views;

public partial class MainWindow : Window
{
    private readonly Dictionary<string, Func<Control>> _viewFactory = new(StringComparer.OrdinalIgnoreCase);

    public MainWindow()
    {
        InitializeComponent();
        BuildViewFactory();

        DataContextChanged += (_, _) =>
        {
            if (VM is not null)
            {
                VM.ModuleChanged -= OnModuleChanged;
                VM.ModuleChanged += OnModuleChanged;
                RenderModule(VM.SelectedModule);
            }
        };
    }

    private MainViewModel? VM => DataContext as MainViewModel;

    private async void OnConnectClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ConnectAsync();
        }
    }

    private async void OnAppendClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.AppendTasksAsync();
        }
    }

    private async void OnStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartAsync();
        }
    }

    private async void OnStopClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StopAsync();
        }
    }

    private async void OnImportClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ManualImportAsync();
        }
    }

    private async void OnRunModuleActionClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RunSelectedModuleActionAsync();
        }
    }

    private void OnModuleSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (VM is not null)
        {
            RenderModule(VM.SelectedModule);
        }
    }

    private void OnModuleChanged(FeatureModule? module)
    {
        RenderModule(module);
    }

    private void RenderModule(FeatureModule? module)
    {
        if (ModuleHost is null)
        {
            return;
        }

        if (module is null)
        {
            ModuleHost.Content = new TextBlock { Text = "未选择模块" };
            return;
        }

        if (_viewFactory.TryGetValue(module.Key, out var factory))
        {
            ModuleHost.Content = factory();
            return;
        }

        ModuleHost.Content = new TextBlock { Text = $"模块 {module.Title} 未注册页面" };
    }

    private void BuildViewFactory()
    {
        foreach (var module in FeatureManifest.All)
        {
            var fullName = $"MAAUnified.App.Features.{module.Group}.{module.ViewTypeName}View";
            _viewFactory[module.Key] = () =>
            {
                var type = typeof(MainWindow).Assembly.GetType(fullName);
                if (type is null)
                {
                    return new TextBlock { Text = $"未找到页面类型: {fullName}" };
                }

                if (Activator.CreateInstance(type) is Control control)
                {
                    return control;
                }

                return new TextBlock { Text = $"页面实例化失败: {fullName}" };
            };
        }
    }
}
