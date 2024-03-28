#nullable enable

using System;
using System.Collections.Generic;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class DiscordNotificationProvider : IExternalNotificationProvider
{
    private const string DiscordApiVersion = "v9";

    private readonly IHttpService _httpService;

    private readonly ILogger _logger = Log.ForContext<DiscordNotificationProvider>();

    public DiscordNotificationProvider(IHttpService httpService)
    {
        _httpService = httpService;
    }

    public async Task<bool> SendAsync(string title, string content)
    {
        var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordBotToken, string.Empty);
        var userId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordUserId, string.Empty);

        var channelId = await CreateDMChannel(botToken, userId);

        if (channelId is null)
        {
            return false;
        }

        return await SendMessageAsync(botToken, channelId, content);
    }

    private async Task<string?> CreateDMChannel(string token, string recipientId)
    {
        var uri = $"https://discord.com/api/{DiscordApiVersion}/users/@me/channels";

        var response = await _httpService.PostAsJsonAsync(
            new Uri(uri),
            new ChannelsCreation { RecipientId = recipientId },
            new Dictionary<string, string>
            {
                { "Authorization", $"Bot {token}" },
                { "User-Agent", "DiscordBot" }, // browser related user-agent is not allowed.
            });

        if (response is null)
        {
            _logger.Warning("Failed to create DM channel.");
            return null;
        }

        var responseData = JsonSerializer.Deserialize<JsonElement>(response);
        if (responseData.ValueKind != JsonValueKind.Undefined && responseData.TryGetProperty("id", out var channelId))
        {
            _logger.Debug($"DM Channel created successfully. Channel ID: {channelId}");
            return channelId.GetString();
        }

        return null;
    }

    public async Task<bool> SendMessageAsync(string token, string channelId, string message)
    {
        var uri = $"https://discord.com/api/{DiscordApiVersion}/channels/{channelId}/messages";

        var response = await _httpService.PostAsFormUrlEncodedAsync(
            new Uri(uri),
            new Dictionary<string, string?> { { "content", message } },
            new Dictionary<string, string>
            {
                { "Authorization", $"Bot {token}" },
                { "User-Agent", "DiscordBot" }, // browser related user-agent is not allowed.
            });

        if (response is null)
        {
            _logger.Warning("Failed to send message.");
            return false;
        }

        return true;
    }

    private class ChannelsCreation
    {
        [JsonPropertyName("recipient_id")]
        public string RecipientId { get; set; }

        [JsonPropertyName("access_tokens")]
        public string[]? AccessTokens { get; set; }

        [JsonPropertyName("nicks")]
        public string? Nicks { get; set; }
    }
}
