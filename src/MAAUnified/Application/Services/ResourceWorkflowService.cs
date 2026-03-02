using MAAUnified.Application.Models;
using MAAUnified.CoreBridge;
using System.Text.Json.Nodes;

namespace MAAUnified.Application.Services;

public sealed class ResourceWorkflowService
{
    private readonly string _baseDirectory;
    private readonly IMaaCoreBridge _bridge;
    private readonly UiLogService _logService;

    public ResourceWorkflowService(string baseDirectory, IMaaCoreBridge bridge, UiLogService logService)
    {
        _baseDirectory = baseDirectory;
        _bridge = bridge;
        _logService = logService;
    }

    public async Task<CoreResult<CoreInitializeInfo>> InitializeCoreAsync(
        UnifiedConfig config,
        CancellationToken cancellationToken = default)
    {
        var clientType = ResolveClientType(config);
        var result = await _bridge.InitializeAsync(
            new CoreInitializeRequest(_baseDirectory, clientType),
            cancellationToken);

        if (result.Success)
        {
            _logService.Info($"Core initialized. lib={result.Value?.LibraryPath}, version={result.Value?.CoreVersion}, client={clientType}");
        }
        else
        {
            _logService.Error($"Core initialize failed: {result.Error?.Code} {result.Error?.Message}");
        }

        return result;
    }

    private static string? ResolveClientType(UnifiedConfig config)
    {
        if (config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
            && profile.Values.TryGetValue("ClientType", out var clientTypeNode))
        {
            if (clientTypeNode is JsonValue value && value.TryGetValue(out string? clientType))
            {
                return clientType;
            }
        }

        return null;
    }
}
