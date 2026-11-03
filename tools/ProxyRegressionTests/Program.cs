// <copyright file="Program.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Text;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UserControl.Settings;

using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(90));
await using var systemProxy = new LoopbackServer("system", false, timeout.Token);
await using var manualProxy = new LoopbackServer("manual", false, timeout.Token);
await using var socksProxy = new LoopbackServer("socks5", true, timeout.Token);
await using var directServer = new LoopbackServer("direct", false, timeout.Token);
var originalDefaultProxy = HttpClient.DefaultProxy;
var settings = VersionUpdateSettingsUserControlModel.Instance;
var destination = new Uri("http://proxy-regression.invalid/probe");
var passed = 0;

try
{
    HttpClient.DefaultProxy = new WebProxy(systemProxy.Address);
    settings.Proxy = manualProxy.Address.Authority;
    settings.ProxyType = "System";
    var service = new HttpService();

    await ExpectRouteAsync("system", "System ignores a retained manual address");

    settings.ProxyType = "Http";
    await ExpectRouteAsync("manual", "The first System-to-HTTP switch takes effect");

    settings.ProxyType = "System";
    await ExpectRouteAsync("system", "HTTP-to-System clears the explicit proxy");

    settings.ProxyType = "Http";
    settings.Proxy = string.Empty;
    await ExpectRouteAsync("system", "Clearing a manual address restores implicit system behavior");

    settings.Proxy = manualProxy.Address.Authority;
    await ExpectRouteAsync("manual", "The first address edit takes effect");

    settings.Proxy = "not a valid proxy address !";
    await ExpectRouteAsync("manual", "Invalid URI input preserves the last working client");

    settings.Proxy = "ftp://127.0.0.1:1234";
    await ExpectRouteAsync("manual", "Unsupported proxy schemes preserve the last working client");

    settings.Proxy = $"http://proxy-test-user:proxy-test-password@{manualProxy.Address.Authority}";
    await ExpectRouteAsync("manual", "A valid absolute HTTP proxy URI works");
    Check(!Serilog.Log.Messages.Any(message =>
        message.Contains("proxy-test-user", StringComparison.Ordinal)
        || message.Contains("proxy-test-password", StringComparison.Ordinal)),
        "Proxy credentials are absent from service logs");

    settings.Proxy = socksProxy.Address.Authority;
    settings.ProxyType = "Socks5";
    await ExpectRouteAsync("socks5", "SOCKS5 uses a real SOCKS5 handshake");
    Check(socksProxy.SocksHandshakeCount > 0, "The SOCKS5 server received a CONNECT command");

    settings.ProxyType = "Http";
    settings.Proxy = manualProxy.Address.Authority;
    using (var response = await service.GetAsync(new Uri("http://proxy-regression.invalid/stream"), token: timeout.Token))
    {
        await manualProxy.StreamStarted.Task.WaitAsync(timeout.Token);
        await using var responseStream = await response.Content.ReadAsStreamAsync(timeout.Token);
        var first = new byte[3];
        await responseStream.ReadExactlyAsync(first, timeout.Token);
        Check(Encoding.ASCII.GetString(first) == "one", "The old download has started streaming");

        settings.ProxyType = "System";
        await ExpectRouteAsync("system", "New requests use System while the old response remains open");
        manualProxy.ReleaseStream.TrySetResult();
        using var reader = new StreamReader(responseStream, Encoding.ASCII);
        var remainder = await reader.ReadToEndAsync(timeout.Token);
        Check(remainder == "two", "Switching proxy does not cancel an in-flight download");
    }

    settings.ProxyType = "None";
    settings.Proxy = manualProxy.Address.Authority;
    await ExpectDirectAsync("None ignores both the system proxy and a saved manual address");

    settings.Proxy = string.Empty;
    await ExpectDirectAsync("None with an empty address does not fall back to System");

    settings.Proxy = "not a valid proxy address !";
    await ExpectDirectAsync("None ignores invalid retained manual input");

    HttpClient.DefaultProxy = new ThrowingProxy();
    await ExpectDirectAsync("None never resolves the default system or environment proxy");

    HttpClient.DefaultProxy = new WebProxy(systemProxy.Address);
    settings.ProxyType = "System";
    await ExpectRouteAsync("system", "None-to-System restores system proxy routing");

    settings.Proxy = manualProxy.Address.Authority;
    settings.ProxyType = "Http";
    await ExpectRouteAsync("manual", "None-to-HTTP can reuse the retained manual address");

    settings.ProxyType = "None";
    await ExpectDirectAsync("HTTP-to-None replaces the manual proxy with direct routing");

    settings.Proxy = socksProxy.Address.Authority;
    settings.ProxyType = "Socks5";
    await ExpectRouteAsync("socks5", "None-to-SOCKS5 restores the SOCKS5 handshake");

    settings.ProxyType = "None";
    var startupDirectService = new HttpService();
    Check(await startupDirectService.GetStringAsync(new Uri(directServer.Address, "/startup")) == "direct",
        "A service created with saved None settings starts in direct mode");

    HttpClient.DefaultProxy = new WebProxy();
    settings.ProxyType = "System";
    var direct = await service.GetStringAsync(new Uri(directServer.Address, "/direct"));
    Check(direct == "direct", "System with no configured proxy connects directly");

    await CheckMirrorProbesAsync();

    Console.WriteLine($"PASS: {passed} loopback proxy regression checks.");

    async Task CheckMirrorProbesAsync()
    {
        Uri[] mirrorUris =
        [
            new("http://s3.maa-org.net/update.zip"),
            new("http://github.test/update.zip"),
        ];
        var directUri = new Uri(directServer.Address, "/mirror");

        HttpClient.DefaultProxy = new WebProxy(systemProxy.Address);
        settings.ProxyType = "System";
        var systemResults = await service.ProbeAsync(mirrorUris);
        Check(systemResults.All(probe => probe.Latency >= 0 && probe.UsesProxy),
            "System probes report proxy usage for each destination");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris,
            [(100, systemResults[0].UsesProxy), (200, systemResults[1].UsesProxy)]) == 1,
            "A proxied domestic mirror receives the existing latency penalty");

        HttpClient.DefaultProxy = new WebProxy();
        settings.ProxyType = "System";
        var noSystemProxy = await service.ProbeAsync([directUri]);
        Check(noSystemProxy[0].Latency >= 0 && !noSystemProxy[0].UsesProxy,
            "System without a configured proxy reports direct routing");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris,
            [(100, noSystemProxy[0].UsesProxy), (200, false)]) == 0,
            "System without a proxy does not penalize a domestic mirror");

        HttpClient.DefaultProxy = new DestinationProxy(systemProxy.Address, directUri);
        settings.ProxyType = "System";
        var splitResults = await service.ProbeAsync([directUri, mirrorUris[0]]);
        Check(splitResults[0].Latency >= 0 && !splitResults[0].UsesProxy
            && splitResults[1].Latency >= 0 && splitResults[1].UsesProxy,
            "System bypass rules produce separate direct and proxied results in one batch");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris,
            [(100, splitResults[0].UsesProxy), (200, splitResults[1].UsesProxy)]) == 0,
            "A bypassed domestic mirror is not penalized by another URL's proxy route");

        HttpClient.DefaultProxy = new NullProxy();
        settings.ProxyType = "System";
        var nullProxy = await service.ProbeAsync([directUri]);
        Check(nullProxy[0].Latency >= 0 && !nullProxy[0].UsesProxy,
            "A null proxy destination means direct routing");

        HttpClient.DefaultProxy = new ThrowingProxy();
        settings.ProxyType = "None";
        var noneResults = await service.ProbeAsync([directUri]);
        Check(noneResults[0].Latency >= 0 && !noneResults[0].UsesProxy,
            "None probes never consult the system proxy");

        settings.Proxy = "not a valid proxy address !";
        settings.ProxyType = "Http";
        var invalidDirect = await service.ProbeAsync([directUri]);
        Check(invalidDirect[0].Latency >= 0 && !invalidDirect[0].UsesProxy,
            "Invalid manual input retains the direct client's routing and scoring policy");

        HttpClient.DefaultProxy = new WebProxy(systemProxy.Address);
        settings.Proxy = string.Empty;
        var emptyManual = await service.ProbeAsync(mirrorUris);
        Check(emptyManual.All(probe => probe.Latency >= 0 && probe.UsesProxy),
            "An empty manual address reports the effective system proxy");

        settings.Proxy = manualProxy.Address.Authority;
        settings.Proxy = "ftp://127.0.0.1:1234";
        var manualBefore = manualProxy.Requests.Count;
        var invalidManual = await service.ProbeAsync(mirrorUris);
        Check(invalidManual.All(probe => probe.Latency >= 0 && probe.UsesProxy)
            && manualProxy.Requests.Count == manualBefore + mirrorUris.Length,
            "Invalid manual input retains the previous manual client's route and scoring policy");

        var startupInvalidService = new HttpService();
        var startupInvalid = await startupInvalidService.ProbeAsync(mirrorUris);
        Check(startupInvalid.All(probe => probe.Latency >= 0 && probe.UsesProxy),
            "Invalid saved manual settings report the actual system fallback");

        settings.Proxy = socksProxy.Address.Authority;
        settings.ProxyType = "Socks5";
        var socksBefore = socksProxy.SocksHandshakeCount;
        var socksResults = await service.ProbeAsync(mirrorUris);
        Check(socksResults.All(probe => probe.Latency >= 0 && probe.UsesProxy)
            && socksProxy.SocksHandshakeCount == socksBefore + mirrorUris.Length,
            "SOCKS5 mirror probes use real handshakes and report proxy usage");

        settings.ProxyType = "System";
        var systemBefore = systemProxy.Requests.Count;
        var switchedResults = await service.ProbeAsync(ChangeSettingsBetweenUrls());
        Check(settings.ProxyType == "None"
            && switchedResults.All(probe => probe.Latency >= 0 && probe.UsesProxy)
            && systemProxy.Requests.Count == systemBefore + mirrorUris.Length,
            "All probes keep the captured client when settings change between candidate URLs");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris,
            [(100, switchedResults[0].UsesProxy), (200, switchedResults[1].UsesProxy)]) == 1,
            "Scoring uses the captured routes after the selected mode changes to None");

        settings.ProxyType = "System";
        var heldProbe = service.ProbeAsync([new Uri(destination, "/held-probe")]);
        await systemProxy.ProbeStarted.Task.WaitAsync(timeout.Token);
        settings.ProxyType = "None";
        systemProxy.ReleaseProbe.TrySetResult();
        var heldResult = await heldProbe;
        Check(heldResult[0].Latency >= 0 && heldResult[0].UsesProxy,
            "A mode change while a probe is in flight cannot change its scoring policy");
        var followingProbe = await service.ProbeAsync([directUri]);
        Check(followingProbe[0].Latency >= 0 && !followingProbe[0].UsesProxy,
            "The next probe batch uses the newly selected direct mode");

        var failedProbe = await service.ProbeAsync([new Uri(directServer.Address, "/not-found")]);
        Check(failedProbe[0].Latency == -1,
            "Unsuccessful HEAD responses remain unavailable candidates");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris, [failedProbe[0], (200, true)]) == 1,
            "Unavailable mirrors cannot win selection");
        Check(UpdateMirrorSelection.SelectFastest(mirrorUris, [(-1, false), (-1, true)]) == -1,
            "All failed probes return no selected mirror");
        Check((await service.ProbeAsync([])).Length == 0
            && UpdateMirrorSelection.SelectFastest([], []) == -1,
            "An empty mirror list returns no selected mirror");
        Check(UpdateMirrorSelection.SelectFastest(
            [new("http://maa-ota.annangela.cn/update.zip"), mirrorUris[1]],
            [(100, true), (200, true)]) == 1,
            "The second domestic mirror receives the same proxy penalty");
        Check(UpdateMirrorSelection.SelectFastest(
            [new("http://github.test/update.zip?mirror=s3.maa-org.net"), mirrorUris[1]],
            [(100, true), (200, true)]) == 0,
            "Domestic mirror classification checks the host rather than URL text");

        IEnumerable<Uri> ChangeSettingsBetweenUrls()
        {
            yield return mirrorUris[0];
            settings.ProxyType = "None";
            yield return mirrorUris[1];
        }
    }

    async Task ExpectDirectAsync(string description)
    {
        var actual = await service.GetStringAsync(new Uri(directServer.Address, "/direct"));
        Check(actual == "direct", $"{description} (expected direct, got {actual ?? "<null>"})");
    }

    async Task ExpectRouteAsync(string expected, string description)
    {
        var actual = await service.GetStringAsync(destination);
        Check(actual == expected, $"{description} (expected {expected}, got {actual ?? "<null>"})");
    }

    void Check(bool condition, string description)
    {
        if (!condition)
        {
            throw new InvalidOperationException(description);
        }

        passed++;
        Console.WriteLine($"PASS: {description}");
    }
}
finally
{
    HttpClient.DefaultProxy = originalDefaultProxy;
}

internal sealed class ThrowingProxy : IWebProxy
{
    public ICredentials? Credentials { get; set; }

    public Uri GetProxy(Uri destination) => throw new InvalidOperationException("None must not resolve a proxy.");

    public bool IsBypassed(Uri host) => throw new InvalidOperationException("None must not query proxy bypass.");
}

internal sealed class DestinationProxy(Uri proxyAddress, Uri bypassedDestination) : IWebProxy
{
    public ICredentials? Credentials { get; set; }

    public Uri GetProxy(Uri destination)
    {
        if (destination == bypassedDestination)
        {
            throw new InvalidOperationException("A bypassed destination must not resolve a proxy.");
        }

        return proxyAddress;
    }

    public bool IsBypassed(Uri host) => host == bypassedDestination;
}

internal sealed class NullProxy : IWebProxy
{
    public ICredentials? Credentials { get; set; }

    public Uri? GetProxy(Uri destination) => null;

    public bool IsBypassed(Uri host) => false;
}

internal sealed class LoopbackServer : IAsyncDisposable
{
    private readonly TcpListener _listener = new(IPAddress.Loopback, 0);
    private readonly CancellationTokenSource _stop;
    private readonly ConcurrentBag<Task> _connections = [];
    private readonly string _name;
    private readonly bool _socks;
    private readonly Task _acceptLoop;
    private int _socksHandshakeCount;

    public LoopbackServer(string name, bool socks, CancellationToken timeout)
    {
        _name = name;
        _socks = socks;
        _stop = CancellationTokenSource.CreateLinkedTokenSource(timeout);
        _listener.Start();
        Address = new Uri($"http://127.0.0.1:{((IPEndPoint)_listener.LocalEndpoint).Port}/");
        _acceptLoop = AcceptAsync();
    }

    public Uri Address { get; }

    public int SocksHandshakeCount => Volatile.Read(ref _socksHandshakeCount);

    public TaskCompletionSource StreamStarted { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public TaskCompletionSource ReleaseStream { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public TaskCompletionSource ProbeStarted { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public TaskCompletionSource ReleaseProbe { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);

    public ConcurrentQueue<string> Requests { get; } = new();

    public async ValueTask DisposeAsync()
    {
        await _stop.CancelAsync();
        _listener.Stop();
        try
        {
            await _acceptLoop;
            await Task.WhenAll(_connections);
        }
        catch (Exception exception) when (exception is OperationCanceledException or IOException or SocketException)
        {
        }

        _stop.Dispose();
    }

    private async Task AcceptAsync()
    {
        while (!_stop.IsCancellationRequested)
        {
            var client = await _listener.AcceptTcpClientAsync(_stop.Token);
            _connections.Add(HandleAsync(client));
        }
    }

    private async Task HandleAsync(TcpClient client)
    {
        using (client)
        {
            var stream = client.GetStream();
            if (_socks)
            {
                await NegotiateSocksAsync(stream);
            }

            var headers = await ReadHeadersAsync(stream);
            Requests.Enqueue(headers.Split("\r\n", 2)[0]);
            if (headers.Contains("/held-probe ", StringComparison.Ordinal))
            {
                ProbeStarted.TrySetResult();
                await ReleaseProbe.Task.WaitAsync(_stop.Token);
            }

            if (headers.Contains("/not-found ", StringComparison.Ordinal))
            {
                await WriteAsync(stream, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                return;
            }

            if (headers.Contains("/stream ", StringComparison.Ordinal))
            {
                await WriteAsync(stream, "HTTP/1.1 200 OK\r\nContent-Length: 6\r\nConnection: close\r\n\r\none");
                StreamStarted.TrySetResult();
                await ReleaseStream.Task.WaitAsync(_stop.Token);
                await WriteAsync(stream, "two");
                return;
            }

            await WriteAsync(stream, $"HTTP/1.1 200 OK\r\nContent-Length: {_name.Length}\r\nConnection: close\r\n\r\n{_name}");
        }
    }

    private async Task NegotiateSocksAsync(NetworkStream stream)
    {
        var greeting = await ReadAsync(stream, 2);
        if (greeting[0] != 5)
        {
            throw new InvalidDataException("Expected SOCKS5 greeting.");
        }

        await ReadAsync(stream, greeting[1]);
        await stream.WriteAsync(new byte[] { 5, 0 }, _stop.Token);
        var request = await ReadAsync(stream, 4);
        if (request[0] != 5 || request[1] != 1)
        {
            throw new InvalidDataException("Expected SOCKS5 CONNECT.");
        }

        var addressLength = request[3] switch
        {
            1 => 4,
            3 => (await ReadAsync(stream, 1))[0],
            4 => 16,
            _ => throw new InvalidDataException("Unsupported SOCKS5 address type."),
        };
        await ReadAsync(stream, addressLength + 2);
        Interlocked.Increment(ref _socksHandshakeCount);
        await stream.WriteAsync(new byte[] { 5, 0, 0, 1, 127, 0, 0, 1, 0, 0 }, _stop.Token);
    }

    private async Task<byte[]> ReadAsync(NetworkStream stream, int count)
    {
        var buffer = new byte[count];
        await stream.ReadExactlyAsync(buffer, _stop.Token);
        return buffer;
    }

    private async Task<string> ReadHeadersAsync(NetworkStream stream)
    {
        var bytes = new List<byte>();
        while (bytes.Count < 65_536)
        {
            bytes.Add((await ReadAsync(stream, 1))[0]);
            if (bytes.Count >= 4
                && bytes[^4] == '\r' && bytes[^3] == '\n'
                && bytes[^2] == '\r' && bytes[^1] == '\n')
            {
                return Encoding.ASCII.GetString(bytes.ToArray());
            }
        }

        throw new InvalidDataException("HTTP headers exceeded the test limit.");
    }

    private Task WriteAsync(NetworkStream stream, string value)
        => stream.WriteAsync(Encoding.ASCII.GetBytes(value), _stop.Token).AsTask();
}
