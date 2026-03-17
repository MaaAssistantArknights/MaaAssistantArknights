namespace MAAUnified.Tests;

public sealed class ConfigurationWriteEntryGuardTests
{
    [Fact]
    public void StoreSaveAsync_ShouldOnlyBeCalledInUnifiedConfigurationServiceSaveCore()
    {
        var repoRoot = ResolveRepoRoot();
        var sourceRoot = Path.Combine(repoRoot, "src", "MAAUnified");
        var files = EnumerateSourceFiles(sourceRoot, includeTests: false);

        var hits = new List<string>();
        foreach (var file in files)
        {
            var lines = File.ReadAllLines(file);
            for (var i = 0; i < lines.Length; i++)
            {
                if (!lines[i].Contains("_store.SaveAsync(", StringComparison.Ordinal))
                {
                    continue;
                }

                hits.Add($"{Path.GetRelativePath(repoRoot, file)}:{i + 1}");
            }
        }

        Assert.NotEmpty(hits);
        Assert.All(
            hits,
            hit => Assert.StartsWith(
                Path.Combine("src", "MAAUnified", "Application", "Services", "UnifiedConfigurationService.cs"),
                hit,
                StringComparison.Ordinal));
    }

    [Fact]
    public void AvaloniaJsonConfigStore_ShouldOnlyBeConstructedByRuntimeFactory()
    {
        var repoRoot = ResolveRepoRoot();
        var sourceRoot = Path.Combine(repoRoot, "src", "MAAUnified");
        var files = EnumerateSourceFiles(sourceRoot, includeTests: false);

        var hits = new List<string>();
        foreach (var file in files)
        {
            var lines = File.ReadAllLines(file);
            for (var i = 0; i < lines.Length; i++)
            {
                if (!lines[i].Contains("new AvaloniaJsonConfigStore(", StringComparison.Ordinal))
                {
                    continue;
                }

                hits.Add($"{Path.GetRelativePath(repoRoot, file)}:{i + 1}");
            }
        }

        var hit = Assert.Single(hits);
        Assert.StartsWith(
            Path.Combine("src", "MAAUnified", "Application", "Services", "MAAUnifiedRuntime.cs"),
            hit,
            StringComparison.Ordinal);
    }

    private static IReadOnlyList<string> EnumerateSourceFiles(string sourceRoot, bool includeTests)
    {
        return Directory
            .EnumerateFiles(sourceRoot, "*.cs", SearchOption.AllDirectories)
            .Where(file => !file.Contains($"{Path.DirectorySeparatorChar}bin{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
            .Where(file => !file.Contains($"{Path.DirectorySeparatorChar}obj{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
            .Where(file => includeTests || !file.Contains($"{Path.DirectorySeparatorChar}Tests{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
            .ToArray();
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
