using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace MAAUnified.Platform;

public sealed class WindowsGpuCapabilityService : IGpuCapabilityService
{
    private const int DxgiErrorNotFound = unchecked((int)0x887A0002);
    private const int SFalse = 1;
    private const uint DxgiAdapterFlagSoftware = 0x2;
    private const string SelectionFallbackWarningKey = "Settings.Performance.Gpu.Warning.SelectionFallback";

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
        /* Xe (Tiger Lake, Elkhart Lake, Jasper Lake, Rocket Lake, Alder Lake, Raptor Lake) */ "\uA780\uA781" +
        "\uA788\uA789\uA78A\uA782\uA78B\uA783\uA7A0\uA7A1\uA7A8\uA7AA\uA7AB\uA7AC\uA7AD\uA7A9\uA721\u4905\u4907" +
        "\u4908\u4909\u4680\u4690\u4688\u468A\u468B\u4682\u4692\u4693\u46D3\u46D4\u46D0\u46D1\u46D2\u4626\u4628" +
        "\u462A\u46A2\u46B3\u46C2\u46A3\u46B2\u46C3\u46A0\u46B0\u46C0\u46A6\u46AA\u46A8\u46A1\u46B1\u46C1\u4C8A" +
        "\u4C8B\u4C90\u4C9A\u4E71\u4E61\u4E57\u4E55\u4E51\u4557\u4555\u4571\u4551\u4541\u9A59\u9A78\u9A60\u9A70" +
        "\u9A68\u9A40\u9A49" +
        /* Xe-LPG (Meteor Lake) */ "\u7D40\u7D45\u7D55\u7DD5" +
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

    private static IReadOnlyList<GpuOptionDescriptor> GetGpuOptions(bool allowDeprecatedGpu)
    {
        if (!OperatingSystem.IsWindows())
        {
            return [GpuOptionDescriptor.Disabled];
        }

        IDXGIFactory1? factory = null;
        try
        {
            var factoryGuid = typeof(IDXGIFactory1).GUID;
            var hr = CreateDXGIFactory2(0, ref factoryGuid, out factory);
            if (hr < 0 || factory is null)
            {
                return [GpuOptionDescriptor.Disabled];
            }

            var options = new List<GpuOptionDescriptor> { GpuOptionDescriptor.Disabled };
            var controllers = QueryDisplayControllers();

            for (uint index = 0; ; index++)
            {
                IDXGIAdapter1? adapter = null;
                try
                {
                    hr = factory.EnumAdapters1(index, out adapter);
                    if (hr == DxgiErrorNotFound)
                    {
                        break;
                    }

                    if (hr < 0 || adapter is null)
                    {
                        continue;
                    }

                    adapter.GetDesc1(out var desc);
                    var description = desc.Description?.TrimEnd('\0').Trim() ?? string.Empty;
                    if (string.IsNullOrWhiteSpace(description))
                    {
                        continue;
                    }

                    if ((desc.Flags & DxgiAdapterFlagSoftware) != 0)
                    {
                        continue;
                    }

                    if (!CheckD3D12Support(adapter, D3DFeatureLevel.Level11_0))
                    {
                        continue;
                    }

                    var controller = MatchController(controllers, description, desc.VendorId, desc.DeviceId);
                    var instancePath = controller?.PnpDeviceId ?? BuildSyntheticInstancePath(index, description);
                    if (IsProbablyIndirectDisplayAdapter(description, instancePath))
                    {
                        continue;
                    }

                    var deprecated = IsGpuDeprecated(adapter, desc, controller);
                    if (deprecated && !allowDeprecatedGpu)
                    {
                        continue;
                    }

                    var option = new GpuOptionDescriptor(
                        Id: instancePath,
                        Kind: GpuOptionKind.SpecificGpu,
                        DisplayName: description,
                        Description: description,
                        InstancePath: instancePath,
                        GpuIndex: index,
                        IsDeprecated: deprecated,
                        DriverDate: controller?.DriverDate,
                        DriverVersion: controller?.DriverVersion);

                    if (index == 0)
                    {
                        options.Add(GpuOptionDescriptor.SystemDefault(
                            displayName: description,
                            isDeprecated: deprecated,
                            driverDate: controller?.DriverDate,
                            driverVersion: controller?.DriverVersion));
                    }

                    options.Add(option);
                }
                finally
                {
                    if (adapter is not null)
                    {
                        Marshal.ReleaseComObject(adapter);
                    }
                }
            }

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
        catch
        {
            return [GpuOptionDescriptor.Disabled];
        }
        finally
        {
            if (factory is not null)
            {
                Marshal.ReleaseComObject(factory);
            }
        }
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

    private static bool CheckD3D12Support(IDXGIAdapter1 adapter, D3DFeatureLevel minimumFeatureLevel)
    {
        var deviceGuid = typeof(ID3D12Device).GUID;
        var hr = D3D12CreateDevice(adapter, minimumFeatureLevel, ref deviceGuid, IntPtr.Zero);
        return hr == SFalse;
    }

    private static bool IsGpuDeprecated(IDXGIAdapter1 adapter, DXGIAdapterDesc1 desc, DisplayControllerInfo? controller)
    {
        if (!CheckD3D12Support(adapter, D3DFeatureLevel.Level12_0))
        {
            return true;
        }

        if (controller?.DriverDate is DateTime driverDate
            && driverDate < GpuCapabilityConstants.DirectMlDriverMinimumDate)
        {
            return true;
        }

        var blacklist = desc.VendorId switch
        {
            0x8086 => IntelBlacklist,
            0x1002 => AmdBlacklist,
            0x10DE => NvidiaBlacklist,
            _ => default,
        };

        return blacklist.Contains((ushort)desc.DeviceId);
    }

    private static bool IsProbablyIndirectDisplayAdapter(string description, string instancePath)
    {
        if (description.Contains("Remote", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Basic Render", StringComparison.OrdinalIgnoreCase)
            || description.Contains("Virtual", StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return instancePath.Contains("ROOT\\", StringComparison.OrdinalIgnoreCase)
            || instancePath.Contains("INDIRECT", StringComparison.OrdinalIgnoreCase);
    }

    private static string BuildSyntheticInstancePath(uint index, string description)
    {
        return $"DXGI#{index}#{description}";
    }

    private static DisplayControllerInfo? MatchController(
        IList<DisplayControllerInfo> controllers,
        string description,
        uint vendorId,
        uint deviceId)
    {
        for (var i = 0; i < controllers.Count; i++)
        {
            var controller = controllers[i];
            if (string.Equals(controller.Name, description, StringComparison.OrdinalIgnoreCase))
            {
                controllers.RemoveAt(i);
                return controller;
            }
        }

        var vendorNeedle = $"VEN_{vendorId:X4}";
        var deviceNeedle = $"DEV_{deviceId:X4}";
        for (var i = 0; i < controllers.Count; i++)
        {
            var controller = controllers[i];
            if (controller.PnpDeviceId.Contains(vendorNeedle, StringComparison.OrdinalIgnoreCase)
                && controller.PnpDeviceId.Contains(deviceNeedle, StringComparison.OrdinalIgnoreCase))
            {
                controllers.RemoveAt(i);
                return controller;
            }
        }

        return null;
    }

    private static List<DisplayControllerInfo> QueryDisplayControllers()
    {
        if (!OperatingSystem.IsWindows())
        {
            return [];
        }

        try
        {
            var startInfo = new ProcessStartInfo
            {
                FileName = "powershell.exe",
                Arguments = "-NoProfile -Command \"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; Get-CimInstance Win32_VideoController | Select-Object Name,PNPDeviceID,DriverVersion,DriverDate | ConvertTo-Json -Compress\"",
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = Process.Start(startInfo);
            if (process is null)
            {
                return [];
            }

            if (!process.WaitForExit(5000))
            {
                try
                {
                    process.Kill(entireProcessTree: true);
                }
                catch
                {
                    // best effort
                }

                return [];
            }

            var json = process.StandardOutput.ReadToEnd().Trim();
            if (string.IsNullOrWhiteSpace(json))
            {
                return [];
            }

            using var document = JsonDocument.Parse(json);
            var controllers = new List<DisplayControllerInfo>();
            if (document.RootElement.ValueKind == JsonValueKind.Array)
            {
                foreach (var element in document.RootElement.EnumerateArray())
                {
                    if (TryParseController(element, out var controller))
                    {
                        controllers.Add(controller);
                    }
                }

                return controllers;
            }

            if (TryParseController(document.RootElement, out var singleController))
            {
                controllers.Add(singleController);
            }

            return controllers;
        }
        catch
        {
            return [];
        }
    }

    private static bool TryParseController(JsonElement element, out DisplayControllerInfo controller)
    {
        controller = default!;

        var name = ReadJsonString(element, "Name");
        if (string.IsNullOrWhiteSpace(name))
        {
            return false;
        }

        controller = new DisplayControllerInfo(
            Name: name.Trim(),
            PnpDeviceId: ReadJsonString(element, "PNPDeviceID").Trim(),
            DriverVersion: ReadJsonString(element, "DriverVersion").Trim(),
            DriverDate: ParseWmiDate(ReadJsonString(element, "DriverDate")));
        return true;
    }

    private static string ReadJsonString(JsonElement element, string propertyName)
    {
        if (!element.TryGetProperty(propertyName, out var value) || value.ValueKind != JsonValueKind.String)
        {
            return string.Empty;
        }

        return value.GetString() ?? string.Empty;
    }

    private static DateTime? ParseWmiDate(string value)
    {
        if (string.IsNullOrWhiteSpace(value) || value.Length < 8)
        {
            return null;
        }

        return DateTime.TryParseExact(
            value[..8],
            "yyyyMMdd",
            null,
            System.Globalization.DateTimeStyles.AssumeLocal,
            out var parsed)
            ? parsed.Date
            : null;
    }

    [DllImport("dxgi.dll", ExactSpelling = true)]
    private static extern int CreateDXGIFactory2(
        uint flags,
        ref Guid riid,
        [MarshalAs(UnmanagedType.Interface)] out IDXGIFactory1 factory);

    [DllImport("d3d12.dll", ExactSpelling = true)]
    private static extern int D3D12CreateDevice(
        [MarshalAs(UnmanagedType.IUnknown)] object adapter,
        D3DFeatureLevel minimumFeatureLevel,
        ref Guid riid,
        IntPtr devicePointer);

    [ComImport]
    [Guid("770AAE78-F26F-4DBA-A829-253C83D1B387")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IDXGIFactory1
    {
        int SetPrivateData(ref Guid name, uint dataSize, IntPtr data);
        int SetPrivateDataInterface(ref Guid name, IntPtr unknown);
        int GetPrivateData(ref Guid name, ref uint dataSize, IntPtr data);
        int GetParent(ref Guid riid, out IntPtr parent);
        int EnumAdapters(uint adapter, out IntPtr dxgiAdapter);
        int MakeWindowAssociation(IntPtr windowHandle, uint flags);
        int GetWindowAssociation(out IntPtr windowHandle);
        int CreateSoftwareAdapter(IntPtr moduleHandle, out IntPtr dxgiAdapter);
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

    private sealed record DisplayControllerInfo(
        string Name,
        string PnpDeviceId,
        string DriverVersion,
        DateTime? DriverDate);
}
