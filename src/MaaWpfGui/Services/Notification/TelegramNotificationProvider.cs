#nullable enable

using System;
using System.Collections.Generic;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class TelegramNotificationProvider : IExternalNotificationProvider
{
    private readonly IHttpService _httpService;

    private readonly ILogger _logger = Log.ForContext<TelegramNotificationProvider>();

    public TelegramNotificationProvider(IHttpService httpService)
    {
        _httpService = httpService;
    }

    public async Task<bool> SendAsync(string title, string content)
    {
        var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramBotToken, string.Empty);
        var chatId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramChatId, string.Empty);

        var uri = $"https://api.telegram.org/bot{botToken}/sendMessage";

        var response = await _httpService.PostAsJsonAsync(
            new Uri(uri),
            new TelegramPostContent { ChatId = chatId, Content = $"{title}: {content}"});

        if (response is null)
        {
            _logger.Warning("Failed to send message.");
            return false;
        }

        return true;
    }

    private class TelegramPostContent
    {
        [JsonPropertyName("chat_id")]
        public string ChatId { get; set; }

        [JsonPropertyName("text")]
        public string Content { get; set; }
    }
}
