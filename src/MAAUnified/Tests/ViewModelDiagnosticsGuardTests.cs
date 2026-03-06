namespace MAAUnified.Tests;

public sealed class ViewModelDiagnosticsGuardTests
{
    [Fact]
    public void CoreViewModels_ShouldUseDiagnosticsWrappers_InsteadOfDirectCalls()
    {
        var repoRoot = ResolveRepoRoot();
        var targets = new[]
        {
            Path.Combine(repoRoot, "src", "MAAUnified", "App", "ViewModels", "MainShellViewModel.cs"),
            Path.Combine(repoRoot, "src", "MAAUnified", "App", "ViewModels", "TaskQueue", "TaskQueuePageViewModel.cs"),
            Path.Combine(repoRoot, "src", "MAAUnified", "App", "ViewModels", "Copilot", "CopilotPageViewModel.cs"),
            Path.Combine(repoRoot, "src", "MAAUnified", "App", "ViewModels", "Settings", "SettingsPageViewModel.cs"),
        };

        var violations = new List<string>();
        foreach (var file in targets)
        {
            var lines = File.ReadAllLines(file);
            for (var i = 0; i < lines.Length; i++)
            {
                var line = lines[i];
                if (!line.Contains("DiagnosticsService.Record", StringComparison.Ordinal))
                {
                    continue;
                }

                // MainShell keeps a tiny wrapper layer for diagnostics calls.
                if (file.EndsWith("MainShellViewModel.cs", StringComparison.Ordinal)
                    && line.Contains("return _runtime.DiagnosticsService.Record", StringComparison.Ordinal))
                {
                    continue;
                }

                violations.Add($"{Path.GetRelativePath(repoRoot, file)}:{i + 1}");
            }
        }

        Assert.True(
            violations.Count == 0,
            "Found direct diagnostics calls in guarded view models:\n" + string.Join("\n", violations));
    }

    private static string ResolveRepoRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "src", "MAAUnified", "App");
            var testsDir = Path.Combine(current.FullName, "src", "MAAUnified", "Tests");
            if (Directory.Exists(appDir) && Directory.Exists(testsDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Repository root not found from test base directory.");
    }
}
