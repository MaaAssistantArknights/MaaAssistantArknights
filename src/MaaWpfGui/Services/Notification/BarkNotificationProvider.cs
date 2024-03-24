using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.RightsManagement;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using Serilog;

namespace MaaWpfGui.Services.Notification
{
    public class BarkNotificationProvider : IExternalNotificationProvider
    {
        private readonly IHttpService _httpService;

        private readonly ILogger _logger = Log.ForContext<BarkNotificationProvider>();

        private readonly string _apiBase = "https://api.day.app";

        public BarkNotificationProvider(IHttpService httpService)
        {
            _httpService = httpService;
        }

        public async Task<bool> SendAsync(string title, string content)
        {
            var sendKey = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkSendKey, string.Empty);
            if (string.IsNullOrWhiteSpace(sendKey))
            {
                _logger.Warning("Failed to send Bark notification, Bark send key is empty");
                return false;
            }
            var response = await _httpService.PostAsJsonAsync(
                new Uri($"{_apiBase}/push"),
                new BarkPostContent { Title = title, Content = content, SendKey = sendKey });
            var data = JsonSerializer.Deserialize<BarkResponse>(response);
            if (data.Code != 200)
            {
                _logger.Warning("Failed to send Bark notification: {Code} {Message}", data.Code, data.Message);
                return false;
            }

            return true;
        }

        private class BarkPostContent
        {
            [JsonPropertyName("device_key")]
            public string SendKey { get; set; }

            [JsonPropertyName("title")]
            public string Title { get; set; }

            [JsonPropertyName("body")]
            public string Content { get; set; }

            [JsonPropertyName("icon")]
            public string Icon { get => "https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_256x256.png"; }
        }

        private class BarkResponse
        {
            [JsonPropertyName("code")]
            public int Code { get; set; }

            [JsonPropertyName("message")]
            public string Message { get; set; }

            [JsonPropertyName("timestamp")]
            public long Timestamp { get; set; }
        }
    }
}
