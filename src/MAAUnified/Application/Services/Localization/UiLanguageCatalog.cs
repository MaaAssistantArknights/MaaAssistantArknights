namespace MAAUnified.Application.Services.Localization;

public static class UiLanguageCatalog
{
    private static readonly string[] OrderedLanguages =
    [
        "zh-cn",
        "zh-tw",
        "en-us",
        "ja-jp",
        "ko-kr",
        "pallas",
    ];

    public const string DefaultLanguage = "zh-cn";

    public const string FallbackLanguage = "en-us";

    public static IReadOnlyList<string> Ordered => OrderedLanguages;

    public static bool IsSupported(string? language)
    {
        if (string.IsNullOrWhiteSpace(language))
        {
            return false;
        }

        return OrderedLanguages.Contains(language.Trim(), StringComparer.OrdinalIgnoreCase);
    }

    public static string Normalize(string? language)
    {
        if (!IsSupported(language))
        {
            return DefaultLanguage;
        }

        return OrderedLanguages.First(item => string.Equals(item, language, StringComparison.OrdinalIgnoreCase));
    }

    public static string NextInCycle(string? currentLanguage)
    {
        var normalized = Normalize(currentLanguage);
        var index = Array.FindIndex(
            OrderedLanguages,
            language => string.Equals(language, normalized, StringComparison.OrdinalIgnoreCase));
        if (index < 0)
        {
            return DefaultLanguage;
        }

        return OrderedLanguages[(index + 1) % OrderedLanguages.Length];
    }
}

public readonly record struct LocalizationFallbackInfo(
    string Scope,
    string Language,
    string Key,
    string FallbackSource);
