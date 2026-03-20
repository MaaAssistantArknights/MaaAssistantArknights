using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using Microsoft.Win32;

namespace MAAUnified.Platform;

public sealed class WindowsGpuCapabilityService : IGpuCapabilityService
{
    private const int DxgiErrorNotFound = unchecked((int)0x887A0002);
    private const int SFalse = 1;
    private const int CrSuccess = 0;
    private const int CrBufferSmall = 0x1A;
    private const uint DxgiAdapterFlagSoftware = 0x2;
    private const uint DevPropTypeFileTime = 0x00000010;
    private const uint DevPropTypeString = 0x00000012;
    private const uint CmLocateDevNodeNormal = 0;
    private const uint MaxReasonableAdapterScanCount = 64;
    private static readonly TimeSpan GpuProbeTimeout = TimeSpan.FromSeconds(2);
    private const string SelectionFallbackWarningKey = "Settings.Performance.Gpu.Warning.SelectionFallback";
    private static readonly DevPropKey DevPKeyDeviceInstanceId = new(
        new Guid(0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57),
        256);
    private static readonly DevPropKey DevPKeyDeviceDriverDate = new(
        new Guid(0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0x0c, 0x75, 0xd6),
        2);
    private static readonly DevPropKey DevPKeyDeviceDriverVersion = new(
        new Guid(0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0x0c, 0x75, 0xd6),
        3);
    private readonly object _candidateProbeGate = new();
    private Task<IReadOnlyList<WindowsGpuCandidate>>? _candidateProbeTask;
    private GpuProbeProgress? _candidateProbeProgress;
    private IReadOnlyList<WindowsGpuCandidate>? _cachedCandidates;
    private Exception? _cachedProbeFailure;

    private static ReadOnlySpan<ushort> AmdBlacklist => MemoryMarshal.Cast<char, ushort>(
        /* CHIP_TAHITI   */ "\u6780\u6784\u6788\u678A\u6790\u6791\u6792\u6798\u6799\u679A\u679B\u679E\u679F" +
        /* CHIP_PITCAIRN */ "\u6800\u6801\u6802\u6806\u6808\u6809\u6810\u6811\u6816\u6817\u6818\u6819" +
        /* CHIP_OLAND    */ "\u6600\u6601\u6602\u6603\u6604\u6605\u6606\u6607\u6608" +
        "\u6610\u6611\u6613\u6617\u6620\u6621\u6623\u6631" +
        /* CHIP_VERDE    */ "\u6820\u6821\u6822\u6823\u6824\u6825\u6826\u6827\u6828\u6829\u682A\u682B\u682C" +
        "\u682D\u682F\u6830\u6831\u6835\u6837\u6838\u6839\u683B\u683D\u683F" +
        /* CHIP_HAINAN   */ "\u6660\u6663\u6664\u6665\u6667\u666F" +
        /* CHIP_KAVERI   */ "\u1304\u1305\u1306\u1307\u1309\u130A\u130B\u130C\u130D\u130E\u130F" +
        "\u1310\u1311\u1312\u1313\u1315\u1316\u1317\u1318\u131B\u131C\u131D" +
        /* CHIP_BONAIRE  */ "\u6640\u6641\u6646\u6647\u6649\u6650\u6651\u6658\u665c\u665d\u665f" +
        /* CHIP_HAWAII   */ "\u67A0\u67A1\u67A2\u67A8\u67A9\u67AA\u67B0\u67B1\u67B8\u67B9\u67BA\u67BE" +
        /* CHIP_KABINI   */ "\u9830\u9831\u9832\u9833\u9834\u9835\u9836\u9837" +
        "\u9838\u9839\u983a\u983b\u983c\u983d\u983e\u983f" +
        /* CHIP_MULLINS  */ "\u9850\u9851\u9852\u9853\u9854\u9855\u9856\u9857" +
        "\u9858\u9859\u985A\u985B\u985C\u985D\u985E\u985F" +
        /* CHIP_TOPAZ    */ "\u6900\u6901\u6902\u6903\u6907" +
        /* CHIP_TONGA    */ "\u6920\u6921\u6928\u6929\u692B\u692F\u6930\u6938\u6939" +
        /* CHIP_FIJI     */ "\u7300\u730F" +
        /* CHIP_CARRIZO  */ "\u9870\u9874\u9875\u9876\u9877" +
        /* CHIP_STONEY   */ "\u98E4");

    private static ReadOnlySpan<ushort> IntelBlacklist => MemoryMarshal.Cast<char, ushort>(
        // Keep only pre-Xe Intel generations in the default deprecated list.
        // Modern Xe / Xe-LPG devices should remain visible without requiring the deprecated toggle.
        /* Gen11 */ "\u8A70\u8A71\u8A56\u8A58\u8A5B\u8A5D\u8A54\u8A5A\u8A5C\u8A57\u8A59\u8A50\u8A51\u8A52\u8A53" +
        /* Gen9  */ "\u3EA5\u3EA8\u3EA6\u3EA7\u3EA2\u3E90\u3E93\u3E99\u3E9C\u3EA1\u9BA5\u9BA8\u3EA4\u9B21\u9BA0" +
        "\u9BA2\u9BA4\u9BAA\u9BAB\u9BAC\u87CA\u3EA3\u9B41\u9BC0\u9BC2\u9BC4\u9BCA\u9BCB\u9BCC\u3E91\u3E92\u3E98" +
        "\u3E9B\u9BC5\u9BC8\u3E96\u3E9A\u3E94\u9BC6\u9BE6\u9BF6\u3EA9\u3EA0\u593B\u5923\u5926\u5927\u5917\u5912" +
        "\u591B\u5916\u5921\u591A\u591D\u591E\u591C\u87C0\u5913\u5915\u5902\u5906\u590B\u590A\u5908\u590E\u3185" +
        "\u3184\u1A85\u5A85\u0A84\u1A84\u5A84\u192A\u1932\u193B\u193A\u193D\u1923\u1926\u1927\u192B\u192D\u1912" +
        "\u191B\u1913\u1915\u1917\u191A\u1916\u1921\u191D\u191E\u1902\u1906\u190B\u190A\u190E");

    private static ReadOnlySpan<ushort> NvidiaBlacklist => MemoryMarshal.Cast<char, ushort>(
        /* Kepler */ "\u0FC6\u0FC8\u0FC9\u0FCD\u0FCE\u0FD1\u0FD2\u0FD3\u0FD4\u0FD5\u0FD8\u0FD9\u0FDF\u0FE0\u0FE1" +
        "\u0FE2\u0FE3\u0FE4\u0FE9\u0FEA\u0FEC\u0FED\u0FEE\u0FF6\u0FF8\u0FF9\u0FFA\u0FFB\u0FFC\u0FFD\u0FFE\u0FFF" +
        "\u1001\u1004\u1005\u1007\u1008\u100A\u100C\u1021\u1022\u1023\u1024\u1026\u1027\u1028\u1029\u102A\u102D" +
        "\u103A\u103C\u1180\u1183\u1184\u1185\u1185\u1187\u1188\u1189\u1189\u118A\u118E\u118F\u1193\u1194\u1195" +
        "\u1198\u1199\u1199\u119A\u119D\u119E\u119F\u11A0\u11A1\u11A2\u11A3\u11A7\u11B4\u11B6\u11B7\u11B8\u11BA" +
        "\u11BC\u11BD\u11BE\u11C0\u11C2\u11C3\u11C4\u11C5\u11C6\u11C8\u11CB\u11E0\u11E1\u11E2\u11E3\u11E3\u11FA" +
        "\u11FC\u1280\u1281\u1282\u1284\u1286\u1287\u1288\u1289\u128B\u1290\u1290\u1291\u1292\u1292\u1293\u1295" +
        "\u1295\u1296\u1298\u1299\u1299\u129A\u12B9\u12BA" +
        /* Maxwell */ "\u1340\u1341\u1344\u1346\u1347\u1348\u1349\u134B\u134D\u134E\u134F\u137A\u137B\u1380" +
        "\u1381\u1382\u1390\u1391\u1392\u1393\u1398\u1399\u139A\u139B\u139C\u139D\u13B0\u13B1\u13B2\u13B3\u13B4" +
        "\u13B6\u13B9\u13BA\u13BB\u13BC\u13C0\u13C2\u13D7\u13D8\u13D9\u13DA\u13F0\u13F1\u13F2\u13F3\u13F8\u13F9" +
        "\u13FA\u13FB\u1401\u1402\u1406\u1407\u1427\u1430\u1431\u1436\u1617\u1618\u1619\u161A\u1667\u174D\u174E" +
        "\u179C\u17C2\u17C8\u17F0\u17F1\u17FD");

    public GpuSelectionResolution Resolve(GpuPreference preference)
    {
        var options = GetGpuOptions(preference.AllowDeprecatedGpu);
        var selected = ResolveSelection(preference, options, out var selectionChanged);

        return new GpuSelectionResolution(
            Snapshot: new GpuCapabilitySnapshot(
                SupportMode: GpuPlatformSupportMode.WindowsSupported,
                IsEditable: true,
                AppliesToCore: true,
                SupportsDeprecatedToggle: true,
                Options: options,
                StatusTextKey: "Settings.Performance.Gpu.Status.WindowsReady",
                Provider: "windows-dxgi"),
            SelectedOption: selected,
            SelectionChanged: selectionChanged,
            SelectionWarningTextKey: selectionChanged ? SelectionFallbackWarningKey : null);
    }

    private IReadOnlyList<GpuOptionDescriptor> GetGpuOptions(bool allowDeprecatedGpu)
    {
        if (!OperatingSystem.IsWindows())
        {
            return [GpuOptionDescriptor.Disabled];
        }

        var candidates = GetGpuCandidates();
        return BuildOptionList(allowDeprecatedGpu, candidates);
    }

    private IReadOnlyList<WindowsGpuCandidate> GetGpuCandidates()
    {
        if (!OperatingSystem.IsWindows())
        {
            return [];
        }

        Task<IReadOnlyList<WindowsGpuCandidate>> probeTask;
        GpuProbeProgress probeProgress;
        lock (_candidateProbeGate)
        {
            if (_cachedCandidates is not null)
            {
                return _cachedCandidates;
            }

            if (_cachedProbeFailure is not null)
            {
                throw new InvalidOperationException(
                    "Windows GPU capability probe previously failed earlier in this session.",
                    _cachedProbeFailure);
            }

            _candidateProbeProgress ??= new GpuProbeProgress();
            _candidateProbeTask ??= Task.Run<IReadOnlyList<WindowsGpuCandidate>>(() => EnumerateCandidates(_candidateProbeProgress));
            probeTask = _candidateProbeTask;
            probeProgress = _candidateProbeProgress;
        }

        if (!probeTask.Wait(GpuProbeTimeout))
        {
            var timeout = new TimeoutException(
                $"Timed out after {GpuProbeTimeout.TotalSeconds:0.##} seconds while probing Windows GPU capabilities. " +
                "The probe is waiting on DXGI adapter enumeration / D3D12 feature checks / DisplayConfig adapter mapping. " +
                $"TaskStatus={probeTask.Status}; OS={RuntimeInformation.OSDescription}; ProcessArch={RuntimeInformation.ProcessArchitecture}; " +
                $"BaseDir={AppContext.BaseDirectory}; Progress={probeProgress.BuildSummary()}.");
            WriteProbeDiagnosticsLog(probeProgress, outcome: "timeout", exception: timeout);
            lock (_candidateProbeGate)
            {
                _candidateProbeTask = null;
                _candidateProbeProgress = null;
                _cachedProbeFailure = timeout;
            }

            throw timeout;
        }

        try
        {
            var candidates = probeTask.GetAwaiter().GetResult();
            WriteProbeDiagnosticsLog(probeProgress, outcome: "success", candidates: candidates);
            lock (_candidateProbeGate)
            {
                _cachedCandidates = candidates;
                _cachedProbeFailure = null;
            }

            return candidates;
        }
        catch (Exception ex)
        {
            WriteProbeDiagnosticsLog(probeProgress, outcome: "failed", exception: ex);
            lock (_candidateProbeGate)
            {
                _candidateProbeTask = null;
                _candidateProbeProgress = null;
                _cachedProbeFailure = ex;
            }

            throw new InvalidOperationException(
                "Windows GPU capability probe failed. " +
                $"OS={RuntimeInformation.OSDescription}; ProcessArch={RuntimeInformation.ProcessArchitecture}; BaseDir={AppContext.BaseDirectory}. " +
                $"The failure happened after the probe task completed. Progress={probeProgress.BuildSummary()}",
                ex);
        }
    }

    [SupportedOSPlatform("windows")]
    private static IReadOnlyList<WindowsGpuCandidate> EnumerateCandidates(GpuProbeProgress progress)
    {
        progress.Update("CreateDXGIFactory2.Begin");
        IDXGIFactory1? factory = null;
        try
        {
            var factoryGuid = typeof(IDXGIFactory1).GUID;
            var hr = CreateDXGIFactory2(0, ref factoryGuid, out factory);
            if (hr < 0 || factory is null)
            {
                throw new InvalidOperationException(
                    $"CreateDXGIFactory2 failed. HRESULT=0x{hr:X8}; factoryIsNull={factory is null}.");
            }

            progress.Update("CreateDXGIFactory2.End");
            Exception? primaryFailure = null;
            try
            {
                progress.Update("EnumerationStrategy.Begin", detail: "EnumAdapters1");
                var primaryCandidates = EnumerateCandidatesWithEnumAdapters1(factory, progress);
                if (primaryCandidates.Count > 0)
                {
                    progress.Update("EnumerationStrategy.Success", detail: $"EnumAdapters1;candidates={primaryCandidates.Count}");
                    return primaryCandidates;
                }

                progress.Update("EnumerationStrategy.Empty", detail: "EnumAdapters1 returned zero candidates.");
            }
            catch (Exception ex)
            {
                primaryFailure = ex;
                progress.Update(
                    "EnumerationStrategy.Fail",
                    detail: $"EnumAdapters1; {ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
            }

            try
            {
                progress.Update("EnumerationStrategy.Begin", detail: "EnumAdapters");
                var fallbackCandidates = EnumerateCandidatesWithEnumAdapters(factory, progress);
                progress.Update("EnumerationStrategy.Success", detail: $"EnumAdapters;candidates={fallbackCandidates.Count}");
                return fallbackCandidates;
            }
            catch (Exception ex)
            {
                progress.Update(
                    "EnumerationStrategy.Fail",
                    detail: $"EnumAdapters; {ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
                throw new InvalidOperationException(
                    "Failed while enumerating Windows GPU candidates with both DXGI strategies. Primary=EnumAdapters1; Secondary=EnumAdapters.",
                    primaryFailure is null ? ex : new AggregateException(primaryFailure, ex));
            }
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException(
                "Failed while enumerating Windows GPU candidates. " +
                "This includes DXGI adapter enumeration, D3D12 support checks, and DisplayConfig adapter mapping.",
                ex);
        }
        finally
        {
            ReleaseComObjectQuietly(factory);
        }
    }

    [SupportedOSPlatform("windows")]
    private static IReadOnlyList<WindowsGpuCandidate> EnumerateCandidatesWithEnumAdapters1(
        IDXGIFactory1 factory,
        GpuProbeProgress progress)
    {
        var candidates = new List<WindowsGpuCandidate>();
        var adapterFailures = new List<Exception>();
        var seenCandidateIdentities = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var reachedEndOfList = false;

        for (uint index = 0; index < MaxReasonableAdapterScanCount; index++)
        {
            progress.Update("EnumAdapters1.Begin", index);
            IDXGIAdapter1? adapter = null;
            int hr;
            try
            {
                hr = factory.EnumAdapters1(index, out adapter);
            }
            catch (Exception ex) when (IsRecoverableAdapterProbeException(ex))
            {
                progress.Update(
                    "EnumAdapters1.QueryException",
                    index,
                    detail: $"{ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
                if (index == 0 && candidates.Count == 0)
                {
                    throw new InvalidOperationException(
                        "IDXGIFactory1.EnumAdapters1 threw while querying the first adapter.",
                        ex);
                }

                reachedEndOfList = true;
                break;
            }

            try
            {
                if (hr == DxgiErrorNotFound)
                {
                    progress.Update("EnumAdapters1.EndOfList", index);
                    reachedEndOfList = true;
                    break;
                }

                if (hr < 0)
                {
                    progress.Update("EnumAdapters1.Error", index, detail: $"HRESULT=0x{hr:X8}; adapterNull={adapter is null}; acceptedCandidates={candidates.Count}");
                    if (index == 0 && candidates.Count == 0)
                    {
                        throw new InvalidOperationException(
                            $"IDXGIFactory1.EnumAdapters1 failed on the first adapter query. HRESULT=0x{hr:X8}; adapterNull={adapter is null}.");
                    }

                    reachedEndOfList = true;
                    break;
                }

                if (adapter is null)
                {
                    progress.Update("EnumAdapters1.NullAdapter", index, detail: $"acceptedCandidates={candidates.Count}");
                    if (index == 0 && candidates.Count == 0)
                    {
                        throw new InvalidOperationException(
                            "IDXGIFactory1.EnumAdapters1 returned success but produced a null adapter on the first query.");
                    }

                    reachedEndOfList = true;
                    break;
                }

                progress.Update("TryBuildCandidate.Begin", index);
                try
                {
                    var candidate = TryBuildCandidate(index, adapter, progress);
                    RecordCandidate(candidates, seenCandidateIdentities, candidate, index, progress);
                }
                catch (Exception ex) when (IsRecoverableAdapterProbeException(ex))
                {
                    var failure = new InvalidOperationException(
                        $"Failed while processing DXGI adapter index {index} with EnumAdapters1.",
                        ex);
                    adapterFailures.Add(failure);
                    progress.Update(
                        "AdapterIteration.Exception",
                        index,
                        detail: $"{ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
                    continue;
                }
            }
            finally
            {
                ReleaseComObjectQuietly(adapter);
            }
        }

        if (!reachedEndOfList)
        {
            throw new InvalidOperationException(
                $"IDXGIFactory1.EnumAdapters1 exceeded the safety limit of {MaxReasonableAdapterScanCount} indices without reporting end-of-list. " +
                $"acceptedCandidates={candidates.Count}.");
        }

        if (candidates.Count == 0 && adapterFailures.Count > 0)
        {
            throw new InvalidOperationException(
                $"DXGI EnumAdapters1 did not yield any usable GPU candidates because {adapterFailures.Count} adapter probe(s) failed during capability inspection.",
                new AggregateException(adapterFailures));
        }

        progress.Update("Enumeration.Complete", detail: $"EnumAdapters1;candidates={candidates.Count}");
        return candidates;
    }

    [SupportedOSPlatform("windows")]
    private static IReadOnlyList<WindowsGpuCandidate> EnumerateCandidatesWithEnumAdapters(
        IDXGIFactory1 factory,
        GpuProbeProgress progress)
    {
        var candidates = new List<WindowsGpuCandidate>();
        var adapterFailures = new List<Exception>();
        var seenCandidateIdentities = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var reachedEndOfList = false;

        for (uint index = 0; index < MaxReasonableAdapterScanCount; index++)
        {
            progress.Update("EnumAdapters.Begin", index);
            IDXGIAdapter? adapter = null;
            int hr;
            try
            {
                hr = factory.EnumAdapters(index, out adapter);
            }
            catch (Exception ex) when (IsRecoverableAdapterProbeException(ex))
            {
                progress.Update(
                    "EnumAdapters.QueryException",
                    index,
                    detail: $"{ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
                if (index == 0 && candidates.Count == 0)
                {
                    throw new InvalidOperationException(
                        "IDXGIFactory1.EnumAdapters threw while querying the first adapter.",
                        ex);
                }

                reachedEndOfList = true;
                break;
            }

            try
            {
                if (hr == DxgiErrorNotFound)
                {
                    progress.Update("EnumAdapters.EndOfList", index);
                    reachedEndOfList = true;
                    break;
                }

                if (hr < 0)
                {
                    progress.Update("EnumAdapters.Error", index, detail: $"HRESULT=0x{hr:X8}; adapterNull={adapter is null}; acceptedCandidates={candidates.Count}");
                    if (index == 0 && candidates.Count == 0)
                    {
                        throw new InvalidOperationException(
                            $"IDXGIFactory1.EnumAdapters failed on the first adapter query. HRESULT=0x{hr:X8}; adapterNull={adapter is null}.");
                    }

                    reachedEndOfList = true;
                    break;
                }

                if (adapter is null)
                {
                    progress.Update("EnumAdapters.NullAdapter", index, detail: $"acceptedCandidates={candidates.Count}");
                    if (index == 0 && candidates.Count == 0)
                    {
                        throw new InvalidOperationException(
                            "IDXGIFactory1.EnumAdapters returned success but produced a null adapter on the first query.");
                    }

                    reachedEndOfList = true;
                    break;
                }

                progress.Update("TryBuildCandidate.Legacy.Begin", index);
                try
                {
                    var candidate = TryBuildCandidate(index, adapter, progress);
                    RecordCandidate(candidates, seenCandidateIdentities, candidate, index, progress);
                }
                catch (Exception ex) when (IsRecoverableAdapterProbeException(ex))
                {
                    var failure = new InvalidOperationException(
                        $"Failed while processing DXGI adapter index {index} with EnumAdapters.",
                        ex);
                    adapterFailures.Add(failure);
                    progress.Update(
                        "AdapterIteration.Legacy.Exception",
                        index,
                        detail: $"{ex.GetType().Name}: {TruncateForDiagnostics(ex.Message, 256)}");
                    continue;
                }
            }
            finally
            {
                ReleaseComObjectQuietly(adapter);
            }
        }

        if (!reachedEndOfList)
        {
            throw new InvalidOperationException(
                $"IDXGIFactory1.EnumAdapters exceeded the safety limit of {MaxReasonableAdapterScanCount} indices without reporting end-of-list. " +
                $"acceptedCandidates={candidates.Count}.");
        }

        if (candidates.Count == 0 && adapterFailures.Count > 0)
        {
            throw new InvalidOperationException(
                $"DXGI EnumAdapters did not yield any usable GPU candidates because {adapterFailures.Count} adapter probe(s) failed during capability inspection.",
                new AggregateException(adapterFailures));
        }

        progress.Update("Enumeration.Legacy.Complete", detail: $"EnumAdapters;candidates={candidates.Count}");
        return candidates;
    }

    private static void RecordCandidate(
        List<WindowsGpuCandidate> candidates,
        HashSet<string> seenCandidateIdentities,
        WindowsGpuCandidate? candidate,
        uint index,
        GpuProbeProgress progress)
    {
        if (candidate is null)
        {
            progress.Update("TryBuildCandidate.Rejected", index);
            return;
        }

        var identity = BuildPhysicalAdapterIdentity(candidate);
        if (!seenCandidateIdentities.Add(identity))
        {
            progress.Update("TryBuildCandidate.DuplicatePhysicalAdapter", index, candidate.Description, detail: identity);
            return;
        }

        candidates.Add(candidate);
        progress.Update("TryBuildCandidate.Accepted", index, candidate.Description, detail: identity);
    }

    internal static IReadOnlyList<GpuOptionDescriptor> BuildOptionList(
        bool allowDeprecatedGpu,
        IReadOnlyList<WindowsGpuCandidate> candidates)
    {
        var filteredCandidates = candidates
            .Where(candidate => allowDeprecatedGpu || !candidate.IsDeprecated)
            .ToArray();

        if (filteredCandidates.Length == 0)
        {
            return [GpuOptionDescriptor.Disabled];
        }

        var options = new List<GpuOptionDescriptor>
        {
            GpuOptionDescriptor.Disabled,
        };

        var defaultCandidate = filteredCandidates[0];
        options.Add(GpuOptionDescriptor.SystemDefault(
            displayName: defaultCandidate.Description,
            isDeprecated: defaultCandidate.IsDeprecated,
            driverDate: defaultCandidate.DriverDate,
            driverVersion: defaultCandidate.DriverVersion));

        options.AddRange(filteredCandidates.Select(candidate => new GpuOptionDescriptor(
            Id: string.IsNullOrWhiteSpace(candidate.InstancePath)
                ? BuildSyntheticOptionId(candidate.AdapterIndex, candidate.Description)
                : candidate.InstancePath,
            Kind: GpuOptionKind.SpecificGpu,
            DisplayName: candidate.Description,
            Description: candidate.Description,
            InstancePath: candidate.InstancePath,
            GpuIndex: candidate.AdapterIndex,
            IsDeprecated: candidate.IsDeprecated,
            DriverDate: candidate.DriverDate,
            DriverVersion: candidate.DriverVersion)));

        return DeduplicateSpecificGpuNames(options);
    }

    private static WindowsGpuCandidate? TryBuildCandidate(
        uint index,
        IDXGIAdapter1 adapter,
        GpuProbeProgress progress)
    {
        adapter.GetDesc1(out var desc);
        return TryBuildCandidateCore(
            index,
            adapter,
            desc.Description?.TrimEnd('\0').Trim() ?? string.Empty,
            desc.VendorId,
            desc.DeviceId,
            desc.AdapterLuid,
            isSoftwareAdapter: (desc.Flags & DxgiAdapterFlagSoftware) != 0,
            progress);
    }

    private static WindowsGpuCandidate? TryBuildCandidate(
        uint index,
        IDXGIAdapter adapter,
        GpuProbeProgress progress)
    {
        adapter.GetDesc(out var desc);
        return TryBuildCandidateCore(
            index,
            adapter,
            desc.Description?.TrimEnd('\0').Trim() ?? string.Empty,
            desc.VendorId,
            desc.DeviceId,
            desc.AdapterLuid,
            isSoftwareAdapter: false,
            progress);
    }

    private static WindowsGpuCandidate? TryBuildCandidateCore(
        uint index,
        object adapter,
        string description,
        uint vendorId,
        uint deviceId,
        Luid adapterLuid,
        bool isSoftwareAdapter,
        GpuProbeProgress progress)
    {
        progress.Update(
            "Adapter.Describe",
            index,
            description,
            $"vendor=0x{vendorId:X4}; device=0x{deviceId:X4}; luid={adapterLuid.HighPart}:{adapterLuid.LowPart}; software={isSoftwareAdapter}");
        if (string.IsNullOrWhiteSpace(description) || isSoftwareAdapter)
        {
            return null;
        }

        if (IsProbablyIndirectDisplayAdapter(description, string.Empty))
        {
            progress.Update("Adapter.RejectedIndirectDisplay", index, description, "source=description");
            return null;
        }

        var instancePath = GetAdapterInstancePath(adapterLuid)?.Trim() ?? string.Empty;
        if (IsProbablyIndirectDisplayAdapter(description, instancePath)
            || IsIndirectDisplayAdapter(instancePath))
        {
            progress.Update(
                "Adapter.RejectedIndirectDisplay",
                index,
                description,
                $"instancePath={(string.IsNullOrWhiteSpace(instancePath) ? "<none>" : instancePath)}");
            return null;
        }

        var driverInfo = GetGpuDriverInformation(description, instancePath);
        progress.Update(
            "Adapter.Metadata",
            index,
            description,
            $"instancePath={(string.IsNullOrWhiteSpace(instancePath) ? "<none>" : instancePath)}; driverVersion={(string.IsNullOrWhiteSpace(driverInfo.DriverVersion) ? "<none>" : driverInfo.DriverVersion)}; " +
            $"driverDate={(driverInfo.DriverDate.HasValue ? driverInfo.DriverDate.Value.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture) : "<none>")}");
        progress.Update("CheckD3D12Support.Level11_0.Begin", index, description);
        if (!CheckD3D12Support(adapter, D3DFeatureLevel.Level11_0))
        {
            progress.Update("CheckD3D12Support.Level11_0.Unsupported", index, description);
            return null;
        }

        progress.Update("CheckD3D12Support.Level12_0.Begin", index, description);
        var deprecated = IsGpuDeprecated(adapter, vendorId, deviceId, driverInfo);
        progress.Update("CheckD3D12Support.Level12_0.End", index, description, $"deprecated={deprecated}");
        return new WindowsGpuCandidate(
            AdapterIndex: index,
            Description: description,
            InstancePath: instancePath,
            IsDeprecated: deprecated,
            DriverDate: driverInfo.DriverDate,
            DriverVersion: driverInfo.DriverVersion,
            AdapterLuidLowPart: adapterLuid.LowPart,
            AdapterLuidHighPart: adapterLuid.HighPart);
    }

    private static string? GetAdapterInstancePath(Luid luid)
    {
        try
        {
            var request = new DisplayConfigAdapterName
            {
                Header = new DisplayConfigDeviceInfoHeader
                {
                    Type = DisplayConfigDeviceInfoType.GetAdapterName,
                    Size = (uint)Marshal.SizeOf<DisplayConfigAdapterName>(),
                    AdapterId = luid,
                    Id = 0,
                },
                AdapterDevicePath = string.Empty,
            };

            if (DisplayConfigGetDeviceInfo(ref request) != CrSuccess)
            {
                return null;
            }

            var interfacePath = request.AdapterDevicePath?.TrimEnd('\0').Trim();
            if (string.IsNullOrWhiteSpace(interfacePath))
            {
                return null;
            }

            var instanceIdKey = DevPKeyDeviceInstanceId;
            uint size = 0;
            var result = CM_Get_Device_Interface_Property(
                interfacePath,
                ref instanceIdKey,
                out var propertyType,
                null,
                ref size,
                0);
            if (result != CrBufferSmall || propertyType != DevPropTypeString || size < 2)
            {
                return null;
            }

            var buffer = new byte[size];
            result = CM_Get_Device_Interface_Property(
                interfacePath,
                ref instanceIdKey,
                out propertyType,
                buffer,
                ref size,
                0);
            if (result != CrSuccess || propertyType != DevPropTypeString || size < 2)
            {
                return null;
            }

            return Encoding.Unicode.GetString(buffer, 0, checked((int)size - 2)).Trim();
        }
        catch
        {
            return null;
        }
    }

    private static GpuDriverInformation GetGpuDriverInformation(string description, string instancePath)
    {
        if (string.IsNullOrWhiteSpace(instancePath))
        {
            return new GpuDriverInformation(description, null, null);
        }

        try
        {
            if (CM_Locate_DevNode(out var devInst, instancePath, CmLocateDevNodeNormal) != CrSuccess)
            {
                return new GpuDriverInformation(description, null, null);
            }

            return new GpuDriverInformation(
                description,
                ReadDevNodeStringProperty(devInst, DevPKeyDeviceDriverVersion),
                ReadDevNodeFileTimeProperty(devInst, DevPKeyDeviceDriverDate));
        }
        catch
        {
            return new GpuDriverInformation(description, null, null);
        }
    }

    private static string? ReadDevNodeStringProperty(uint devInst, DevPropKey propertyKey)
    {
        uint size = 0;
        var result = CM_Get_DevNode_Property(devInst, ref propertyKey, out var propertyType, null, ref size, 0);
        if (result != CrBufferSmall || propertyType != DevPropTypeString || size < 2)
        {
            return null;
        }

        var buffer = new byte[size];
        result = CM_Get_DevNode_Property(devInst, ref propertyKey, out propertyType, buffer, ref size, 0);
        if (result != CrSuccess || propertyType != DevPropTypeString || size < 2)
        {
            return null;
        }

        var value = Encoding.Unicode.GetString(buffer, 0, checked((int)size - 2)).Trim();
        return value.Length == 0 ? null : value;
    }

    private static DateTime? ReadDevNodeFileTimeProperty(uint devInst, DevPropKey propertyKey)
    {
        var buffer = new byte[sizeof(long)];
        var size = (uint)buffer.Length;
        var result = CM_Get_DevNode_Property(devInst, ref propertyKey, out var propertyType, buffer, ref size, 0);
        if (result != CrSuccess || propertyType != DevPropTypeFileTime || size < sizeof(long))
        {
            return null;
        }

        var fileTime = BitConverter.ToInt64(buffer, 0);
        if (fileTime <= 0)
        {
            return null;
        }

        try
        {
            return DateTime.FromFileTimeUtc(fileTime).Date;
        }
        catch
        {
            return null;
        }
    }

    private static bool IsIndirectDisplayAdapter(string instancePath)
    {
        if (!OperatingSystem.IsWindows() || string.IsNullOrWhiteSpace(instancePath))
        {
            return false;
        }

        try
        {
            using var registryKey = Registry.LocalMachine.OpenSubKey(
                @"SYSTEM\CurrentControlSet\Enum\" + instancePath,
                writable: false);
            var upperFilters = registryKey?.GetValue("UpperFilters");
            return upperFilters switch
            {
                string[] values => values.Any(value => value.Equals("IndirectKmd", StringComparison.OrdinalIgnoreCase)),
                string value => value.Contains("IndirectKmd", StringComparison.OrdinalIgnoreCase),
                _ => false,
            };
        }
        catch
        {
            return false;
        }
    }

    private static IReadOnlyList<GpuOptionDescriptor> DeduplicateSpecificGpuNames(
        IReadOnlyList<GpuOptionDescriptor> options)
    {
        if (options.Count <= 1)
        {
            return options;
        }

        var duplicateNames = options
            .Where(option => option.Kind == GpuOptionKind.SpecificGpu)
            .GroupBy(option => option.Description, StringComparer.OrdinalIgnoreCase)
            .Where(group => group.Count() > 1)
            .Select(group => group.Key)
            .ToHashSet(StringComparer.OrdinalIgnoreCase);

        return options
            .Select(option => option.Kind != GpuOptionKind.SpecificGpu || !duplicateNames.Contains(option.Description)
                ? option
                : option with { DisplayName = $"{option.Description} (GPU {option.GpuIndex})" })
            .ToArray();
    }

    internal static string BuildPhysicalAdapterIdentity(WindowsGpuCandidate candidate)
    {
        if (!string.IsNullOrWhiteSpace(candidate.InstancePath))
        {
            return $"PNP:{candidate.InstancePath.Trim()}";
        }

        if (candidate.AdapterLuidLowPart != 0 || candidate.AdapterLuidHighPart != 0)
        {
            return string.Create(
                CultureInfo.InvariantCulture,
                $"LUID:{candidate.AdapterLuidHighPart}:{candidate.AdapterLuidLowPart}");
        }

        return string.Create(
            CultureInfo.InvariantCulture,
            $"UNRESOLVED:{candidate.Description.Trim()}#{candidate.AdapterIndex}");
    }

    private static GpuOptionDescriptor ResolveSelection(
        GpuPreference preference,
        IReadOnlyList<GpuOptionDescriptor> options,
        out bool selectionChanged)
    {
        selectionChanged = false;

        var disabled = options.FirstOrDefault(option => option.Kind == GpuOptionKind.Disabled)
            ?? GpuOptionDescriptor.Disabled;

        if (!preference.UseGpu)
        {
            return disabled;
        }

        var fallback = options.FirstOrDefault(option => option.Kind == GpuOptionKind.SystemDefault)
            ?? disabled;

        if (preference.HasSpecificSelection)
        {
            var specificByPath = options.FirstOrDefault(
                option => option.Kind == GpuOptionKind.SpecificGpu
                    && string.Equals(option.InstancePath, preference.PreferredGpuInstancePath, StringComparison.Ordinal));
            if (specificByPath is not null)
            {
                return specificByPath;
            }

            var specificByName = options.FirstOrDefault(
                option => option.Kind == GpuOptionKind.SpecificGpu
                    && string.Equals(option.Description, preference.PreferredGpuDescription, StringComparison.OrdinalIgnoreCase));
            if (specificByName is not null)
            {
                return specificByName;
            }

            selectionChanged = true;
        }

        if (ReferenceEquals(fallback, disabled) && options.Count == 1)
        {
            selectionChanged = true;
        }

        return fallback;
    }

    private static bool CheckD3D12Support(object adapter, D3DFeatureLevel minimumFeatureLevel)
    {
        var deviceGuid = typeof(ID3D12Device).GUID;
        var hr = D3D12CreateDevice(adapter, minimumFeatureLevel, ref deviceGuid, IntPtr.Zero);
        return hr == SFalse;
    }

    private static bool IsRecoverableAdapterProbeException(Exception exception)
    {
        return exception is not (OutOfMemoryException or StackOverflowException or AccessViolationException);
    }

    private static bool IsGpuDeprecated(
        object adapter,
        uint vendorId,
        uint deviceId,
        GpuDriverInformation driverInfo)
    {
        if (!CheckD3D12Support(adapter, D3DFeatureLevel.Level12_0))
        {
            return true;
        }

        if (driverInfo.DriverDate is DateTime driverDate
            && driverDate < GpuCapabilityConstants.DirectMlDriverMinimumDate)
        {
            return true;
        }

        var blacklist = vendorId switch
        {
            0x8086 => IntelBlacklist,
            0x1002 => AmdBlacklist,
            0x10DE => NvidiaBlacklist,
            _ => default,
        };

        return vendorId == 0x8086
            ? IsIntelDeviceBlacklistedByDefault((ushort)deviceId)
            : blacklist.Contains((ushort)deviceId);
    }

    internal static bool IsIntelDeviceBlacklistedByDefault(ushort deviceId)
    {
        return IntelBlacklist.Contains(deviceId);
    }

    internal static bool IsProbablyIndirectDisplayAdapter(string description, string instancePath)
    {
        if (description.Contains("Remote", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Basic Render", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Virtual", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Indirect", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Idd", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return !string.IsNullOrWhiteSpace(instancePath)
            && (instancePath.Contains("ROOT\\", StringComparison.OrdinalIgnoreCase)
                || instancePath.Contains("INDIRECT", StringComparison.OrdinalIgnoreCase));
    }

    private static string BuildSyntheticOptionId(uint index, string description)
    {
        return $"DXGI#{index}#{description}";
    }

    private static string TruncateForDiagnostics(string value, int maxLength = 512)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return "<empty>";
        }

        var trimmed = value.Trim();
        return trimmed.Length <= maxLength
            ? trimmed
            : trimmed[..maxLength] + "...";
    }

    private static void WriteProbeDiagnosticsLog(
        GpuProbeProgress progress,
        string outcome,
        IReadOnlyList<WindowsGpuCandidate>? candidates = null,
        Exception? exception = null)
    {
        try
        {
            var debugDirectory = Path.Combine(AppContext.BaseDirectory, "debug");
            Directory.CreateDirectory(debugDirectory);

            var builder = new StringBuilder();
            builder.AppendLine($"TimestampUtc: {DateTimeOffset.UtcNow:O}");
            builder.AppendLine($"Outcome: {outcome}");
            builder.AppendLine($"OS: {RuntimeInformation.OSDescription}");
            builder.AppendLine($"ProcessArchitecture: {RuntimeInformation.ProcessArchitecture}");
            builder.AppendLine($"BaseDirectory: {AppContext.BaseDirectory}");
            builder.AppendLine($"Summary: {progress.BuildSummary()}");

            if (candidates is not null)
            {
                builder.AppendLine($"CandidateCount: {candidates.Count}");
                foreach (var candidate in candidates)
                {
                    builder.AppendLine(
                        string.Create(
                            CultureInfo.InvariantCulture,
                            $"Candidate: index={candidate.AdapterIndex}; description={candidate.Description}; instancePath={(string.IsNullOrWhiteSpace(candidate.InstancePath) ? "<none>" : candidate.InstancePath)}; " +
                            $"identity={BuildPhysicalAdapterIdentity(candidate)}; deprecated={candidate.IsDeprecated}; luid={candidate.AdapterLuidHighPart}:{candidate.AdapterLuidLowPart}; " +
                            $"driverVersion={(string.IsNullOrWhiteSpace(candidate.DriverVersion) ? "<none>" : candidate.DriverVersion)}; " +
                            $"driverDate={(candidate.DriverDate.HasValue ? candidate.DriverDate.Value.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture) : "<none>")}"));
                }
            }

            if (exception is not null)
            {
                builder.AppendLine("Exception:");
                builder.AppendLine(exception.ToString());
            }

            var entries = progress.SnapshotEntries();
            if (entries.Count > 0)
            {
                builder.AppendLine("ProgressEntries:");
                foreach (var entry in entries)
                {
                    builder.AppendLine(entry);
                }
            }

            File.WriteAllText(Path.Combine(debugDirectory, "windows-gpu-probe.log"), builder.ToString());
        }
        catch
        {
            // Probe diagnostics are best-effort only.
        }
    }

    private sealed class GpuProbeProgress
    {
        private readonly object _gate = new();
        private readonly Stopwatch _stopwatch = Stopwatch.StartNew();
        private readonly List<string> _entries = [];
        private string _stage = "NotStarted";
        private uint? _adapterIndex;
        private string _adapterDescription = string.Empty;
        private string _detail = string.Empty;

        public void Update(string stage, uint? adapterIndex = null, string? adapterDescription = null, string? detail = null)
        {
            lock (_gate)
            {
                _stage = stage;
                _adapterIndex = adapterIndex;
                if (adapterDescription is not null)
                {
                    _adapterDescription = adapterDescription;
                }

                if (detail is not null)
                {
                    _detail = detail;
                }

                _entries.Add(
                    string.Create(
                        CultureInfo.InvariantCulture,
                        $"{DateTimeOffset.UtcNow:O} | stage={_stage}; elapsedMs={_stopwatch.ElapsedMilliseconds}; adapterIndex={(_adapterIndex.HasValue ? _adapterIndex.Value.ToString(CultureInfo.InvariantCulture) : "<none>")}; adapterDescription={(_adapterDescription.Length == 0 ? "<none>" : _adapterDescription)}; detail={(_detail.Length == 0 ? "<none>" : _detail)}"));
                if (_entries.Count > 512)
                {
                    _entries.RemoveAt(0);
                }
            }
        }

        public string BuildSummary()
        {
            lock (_gate)
            {
                return string.Create(
                    CultureInfo.InvariantCulture,
                    $"stage={_stage}; elapsedMs={_stopwatch.ElapsedMilliseconds}; adapterIndex={(_adapterIndex.HasValue ? _adapterIndex.Value.ToString(CultureInfo.InvariantCulture) : "<none>")}; adapterDescription={(_adapterDescription.Length == 0 ? "<none>" : _adapterDescription)}; detail={(_detail.Length == 0 ? "<none>" : _detail)}");
            }
        }

        public IReadOnlyList<string> SnapshotEntries()
        {
            lock (_gate)
            {
                return _entries.ToArray();
            }
        }
    }

    [SupportedOSPlatform("windows")]
    private static void ReleaseComObjectQuietly(object? comObject)
    {
        if (!OperatingSystem.IsWindows() || comObject is null || !Marshal.IsComObject(comObject))
        {
            return;
        }

        try
        {
            _ = Marshal.ReleaseComObject(comObject);
        }
        catch
        {
            // Best-effort cleanup only. Enumeration results should survive release failures.
        }
    }

    [DllImport("dxgi.dll", ExactSpelling = true)]
    private static extern int CreateDXGIFactory2(
        uint flags,
        ref Guid riid,
        [MarshalAs(UnmanagedType.Interface)] out IDXGIFactory1 factory);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern int DisplayConfigGetDeviceInfo(ref DisplayConfigAdapterName requestPacket);

    [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "CM_Get_Device_Interface_PropertyW")]
    private static extern int CM_Get_Device_Interface_Property(
        [MarshalAs(UnmanagedType.LPWStr)] string deviceInterface,
        ref DevPropKey propertyKey,
        out uint propertyType,
        [MarshalAs(UnmanagedType.LPArray)] byte[]? propertyBuffer,
        ref uint propertyBufferSize,
        uint flags);

    [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "CM_Locate_DevNodeW")]
    private static extern int CM_Locate_DevNode(
        out uint devInst,
        [MarshalAs(UnmanagedType.LPWStr)] string deviceId,
        uint flags);

    [DllImport("cfgmgr32.dll", CharSet = CharSet.Unicode, ExactSpelling = true, EntryPoint = "CM_Get_DevNode_PropertyW")]
    private static extern int CM_Get_DevNode_Property(
        uint devInst,
        ref DevPropKey propertyKey,
        out uint propertyType,
        [MarshalAs(UnmanagedType.LPArray)] byte[]? propertyBuffer,
        ref uint propertyBufferSize,
        uint flags);

    [DllImport("d3d12.dll", ExactSpelling = true)]
    private static extern int D3D12CreateDevice(
        [MarshalAs(UnmanagedType.IUnknown)] object adapter,
        D3DFeatureLevel minimumFeatureLevel,
        ref Guid riid,
        IntPtr devicePointer);

    private enum DisplayConfigDeviceInfoType : uint
    {
        GetAdapterName = 4,
    }

    [ComImport]
    [Guid("770AAE78-F26F-4DBA-A829-253C83D1B387")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IDXGIFactory1
    {
        int SetPrivateData(ref Guid name, uint dataSize, IntPtr data);
        int SetPrivateDataInterface(ref Guid name, IntPtr unknown);
        int GetPrivateData(ref Guid name, ref uint dataSize, IntPtr data);
        int GetParent(ref Guid riid, out IntPtr parent);
        [PreserveSig]
        int EnumAdapters(uint adapter, [MarshalAs(UnmanagedType.Interface)] out IDXGIAdapter dxgiAdapter);
        int MakeWindowAssociation(IntPtr windowHandle, uint flags);
        int GetWindowAssociation(out IntPtr windowHandle);
        int CreateSwapChain(IntPtr device, IntPtr desc, out IntPtr swapChain);
        int CreateSoftwareAdapter(IntPtr moduleHandle, out IntPtr dxgiAdapter);
        [PreserveSig]
        int EnumAdapters1(uint adapter, [MarshalAs(UnmanagedType.Interface)] out IDXGIAdapter1 dxgiAdapter);
        [PreserveSig]
        bool IsCurrent();
    }

    [ComImport]
    [Guid("29038F61-3839-4626-91FD-086879011A05")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IDXGIAdapter1
    {
        int SetPrivateData(ref Guid name, uint dataSize, IntPtr data);
        int SetPrivateDataInterface(ref Guid name, IntPtr unknown);
        int GetPrivateData(ref Guid name, ref uint dataSize, IntPtr data);
        int GetParent(ref Guid riid, out IntPtr parent);
        int EnumOutputs(uint output, out IntPtr dxgiOutput);
        int GetDesc(out DXGIAdapterDesc desc);
        int CheckInterfaceSupport(ref Guid interfaceName, out long umdVersion);
        int GetDesc1(out DXGIAdapterDesc1 desc);
    }

    [ComImport]
    [Guid("2411E7E1-12AC-4CCF-BD14-9798E8534DC0")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IDXGIAdapter
    {
        int SetPrivateData(ref Guid name, uint dataSize, IntPtr data);
        int SetPrivateDataInterface(ref Guid name, IntPtr unknown);
        int GetPrivateData(ref Guid name, ref uint dataSize, IntPtr data);
        int GetParent(ref Guid riid, out IntPtr parent);
        int EnumOutputs(uint output, out IntPtr dxgiOutput);
        int GetDesc(out DXGIAdapterDesc desc);
        int CheckInterfaceSupport(ref Guid interfaceName, out long umdVersion);
    }

    [ComImport]
    [Guid("189819F1-1DB6-4B57-BE54-1821339B85F7")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface ID3D12Device
    {
    }

    private enum D3DFeatureLevel : uint
    {
        Level11_0 = 0xB000,
        Level12_0 = 0xC000,
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Luid
    {
        public uint LowPart;
        public int HighPart;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct DisplayConfigDeviceInfoHeader
    {
        public DisplayConfigDeviceInfoType Type;
        public uint Size;
        public Luid AdapterId;
        public uint Id;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DisplayConfigAdapterName
    {
        public DisplayConfigDeviceInfoHeader Header;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string AdapterDevicePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct DevPropKey
    {
        public DevPropKey(Guid fmtid, uint pid)
        {
            Fmtid = fmtid;
            Pid = pid;
        }

        public readonly Guid Fmtid;
        public readonly uint Pid;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DXGIAdapterDesc
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string Description;

        public uint VendorId;
        public uint DeviceId;
        public uint SubSysId;
        public uint Revision;
        public nuint DedicatedVideoMemory;
        public nuint DedicatedSystemMemory;
        public nuint SharedSystemMemory;
        public Luid AdapterLuid;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DXGIAdapterDesc1
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string Description;

        public uint VendorId;
        public uint DeviceId;
        public uint SubSysId;
        public uint Revision;
        public nuint DedicatedVideoMemory;
        public nuint DedicatedSystemMemory;
        public nuint SharedSystemMemory;
        public Luid AdapterLuid;
        public uint Flags;
    }

    private sealed record GpuDriverInformation(
        string Description,
        string? DriverVersion,
        DateTime? DriverDate);

    internal sealed record WindowsGpuCandidate(
        uint AdapterIndex,
        string Description,
        string InstancePath,
        bool IsDeprecated,
        DateTime? DriverDate,
        string? DriverVersion,
        uint AdapterLuidLowPart = 0,
        int AdapterLuidHighPart = 0);
}
