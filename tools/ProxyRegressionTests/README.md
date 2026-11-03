# WPF proxy regression checks

Run in an isolated process:

```powershell
dotnet run --project tools/ProxyRegressionTests/ProxyRegressionTests.csproj -c Debug
```

The console harness links the production `HttpService.cs`, `IHttpService.cs`
and `UpdateMirrorSelection.cs`.
Only the UI, logging and filesystem dependencies are stubbed. It uses local
HTTP and SOCKS5 servers and overrides `HttpClient.DefaultProxy` only inside its
own process; it does not modify operating-system proxy settings.

Checks cover system/direct/manual/SOCKS5 routing, direct mode with an enabled
or throwing default proxy, saved direct mode, an empty manual address, invalid
input, proxy credential logging, and a response stream that remains active
while new requests switch to another proxy. Windows PAC and live operating-
system configuration changes remain delegated to the .NET runtime and need
separate platform integration checks.

Mirror checks cover system proxy scoring, system-without-proxy and per-URL
bypass rules, direct mode, empty or invalid manual input, SOCKS5 handshakes,
and proxy mode changes between candidates or during an in-flight probe.
Each batch retains one accepted client and reports its per-URL proxy policy;
mirror scoring never rereads the editable settings. Deterministic scores
exercise the production selector without relying on loopback timing.

System policy is resolved for each destination when its probe starts. The
operating system can still change PAC results or proxy failover while the
request is running; the policy flag is not packet-level routing telemetry.
