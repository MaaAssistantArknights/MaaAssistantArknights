#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using Serilog;

namespace MaaWpfGui.Services.Web
{
    public class GameDataReportService
    {
        private static readonly ILogger _logger = Log.ForContext<GameDataReportService>();
        private static readonly string _penguinIoDomain = "https://penguin-stats.io";
        private static readonly string[] _backupDomains = { /*"https://penguin-stats.alvorna.com",*/ "https://penguin-stats.cn" };
        private const int MaxRetryPerDomain = 3;
        private const int InitialBackoffMs = 3000;

        public static async Task<bool> PostWithRetryAsync(string url, HttpContent content, Dictionary<string, string> headers, string subtask)
        {
            _logger.Information("Start PostWithRetryAsync, url: {Url}", url);
            var originalResponse = await TryPostAsync(url, content, headers);
            if (originalResponse != null && (int)originalResponse.StatusCode == 200)
            {
                return true;
            }

            _logger.Warning("Initial request failed, status code: {StatusCode}", originalResponse?.StatusCode);

            if (subtask != "ReportToPenguinStats" || !url.Contains(_penguinIoDomain))
            {
                // 一图流的上报失败不提示
                return true;
            }

            foreach (var backupDomain in _backupDomains)
            {
                var newUrl = url.Replace(_penguinIoDomain, backupDomain);
                _logger.Information("Trying backup domain {BackupDomain}, url: {NewUrl}", backupDomain, newUrl);
                var resp = await TryPostAsync(newUrl, content, headers);
                if (resp != null && (int)resp.StatusCode == 200)
                {
                    return true;
                }

                _logger.Warning("Request failed on backup domain {BackupDomain}, status code: {StatusCode}", backupDomain, resp?.StatusCode);
            }

            _logger.Error("All retries failed for URL: {Url}", url);
            return false;
        }

        private static async Task<HttpResponseMessage?> TryPostAsync(string targetUrl, HttpContent content, Dictionary<string, string> headers)
        {
            int currentBackoff = InitialBackoffMs;
            for (int attempt = 1; attempt <= MaxRetryPerDomain; attempt++)
            {
                var response = await Instances.HttpService.PostAsync(new(targetUrl), content, headers);

                switch ((int)response.StatusCode)
                {
                    case 200:
                        return response;
                    case >= 500 and < 600:
                        await Task.Delay(currentBackoff);
                        currentBackoff = (int)(currentBackoff * 1.5);
                        continue;
                    default:
                        return response;
                }
            }

            return null;
        }
    }
}
