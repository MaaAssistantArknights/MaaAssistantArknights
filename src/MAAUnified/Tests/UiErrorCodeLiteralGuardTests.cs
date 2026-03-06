using System.Text.RegularExpressions;

namespace MAAUnified.Tests;

public sealed class UiErrorCodeLiteralGuardTests
{
    private static readonly Regex FailLiteralCodePattern = new(
        "UiOperationResult(?:<[^>]+>)?\\.Fail\\(\\s*\\\"",
        RegexOptions.Compiled | RegexOptions.Singleline);

    private static readonly Regex FallbackLiteralCodePattern = new(
        "(?:Error\\?\\.Code|ErrorCode)\\s*\\?\\?\\s*\\\"",
        RegexOptions.Compiled | RegexOptions.Singleline);

    [Fact]
    public void ProductionCode_ShouldNotUseLiteralUiErrorCodes()
    {
        var repoRoot = ResolveRepoRoot();
        var targets = new[]
        {
            Path.Combine(repoRoot, "src", "MAAUnified", "App", "ViewModels"),
            Path.Combine(repoRoot, "src", "MAAUnified", "Application"),
        };

        var failLiteralViolations = new List<string>();
        var fallbackLiteralViolations = new List<string>();

        foreach (var directory in targets)
        {
            foreach (var file in Directory.EnumerateFiles(directory, "*.cs", SearchOption.AllDirectories))
            {
                var content = File.ReadAllText(file);
                if (FailLiteralCodePattern.IsMatch(content))
                {
                    failLiteralViolations.Add(Path.GetRelativePath(repoRoot, file));
                }

                if (FallbackLiteralCodePattern.IsMatch(content))
                {
                    fallbackLiteralViolations.Add(Path.GetRelativePath(repoRoot, file));
                }
            }
        }

        Assert.True(
            failLiteralViolations.Count == 0,
            "Found UiOperationResult.Fail literal code usages:\n" + string.Join("\n", failLiteralViolations));

        Assert.True(
            fallbackLiteralViolations.Count == 0,
            "Found fallback literal code usages (Error?.Code ?? \"...\"):\n" + string.Join("\n", fallbackLiteralViolations));
    }

    private static string ResolveRepoRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "src", "MAAUnified", "App");
            var applicationDir = Path.Combine(current.FullName, "src", "MAAUnified", "Application");
            if (Directory.Exists(appDir) && Directory.Exists(applicationDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Repository root not found from test base directory.");
    }
}
