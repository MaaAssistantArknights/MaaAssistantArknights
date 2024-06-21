using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using DirectN;
using MaaWpfGui.Constants;
using Microsoft.Win32;

namespace MaaWpfGui.Helper
{
    public abstract class GpuOption
    {
        public static GpuOption Disable => DisableOption.Instance;

        public static GpuOption SystemDefault => SystemDefaultOption.Instance;

        private static readonly List<GpuOption> _unavailableOptions = [Disable];

        private static IDXGIFactory4? GetDxgiFactory()
        {
            var hr = DXGIFunctions.CreateDXGIFactory2(0, typeof(IDXGIFactory4).GUID, out var comobj);
            if (!hr.IsSuccess)
            {
                return null;
            }

            return (IDXGIFactory4)comobj;
        }

        public static List<GpuOption> GetGpuOptions()
        {
            var factory = GetDxgiFactory();

            if (factory == null)
            {
                return _unavailableOptions;
            }

            uint i = 0;

            var options = new List<GpuOption>()
            {
                Disable,
                SystemDefault,
            };

            while (true)
            {
                IDXGIAdapter1 adapter;
                var hr = factory.EnumAdapters1(i, out adapter);
                if (hr == HRESULTS.DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }

                var index = i;
                i++;

                var desc = adapter.GetDesc1();

                if ((desc.Flags & (uint)DXGI_ADAPTER_FLAG.DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
                {
                    // skip software device
                    continue;
                }

                if (!CheckD3D12Support(adapter))
                {
                    // skip adapters without D3D12 support
                    continue;
                }

                var instance_path = GetAdapterInstancePath((ulong)desc.AdapterLuid.Value);

                if (instance_path != null && IsIndirectDisplayAdapter(instance_path))
                {
                    // skip virtual adapters (streaming/RDP)
                    continue;
                }

                options.Add(new SpecificGpuOption(index, desc, instance_path ?? string.Empty));
            }

            if (i == 0)
            {
                return _unavailableOptions;
            }

            // show index if multiple adapters with same name installed
            var counter = new Dictionary<string, int>();
            foreach (var option in options.OfType<SpecificGpuOption>())
            {
                counter[option.Description] = counter.GetValueOrDefault(option.Description, 0) + 1;
            }

            foreach (var option in options.OfType<SpecificGpuOption>())
            {
                if (counter.GetValueOrDefault(option.Description, 0) > 1)
                {
                    option.ShowIndex = true;
                }
            }

            return options;
        }

        private static bool CheckD3D12Support(IDXGIAdapter1 adapter)
        {
            // using the same feature level as onnxruntime does
            var hr = D3D12Functions.D3D12CheckDeviceCreate<ID3D12Device>(adapter, D3D_FEATURE_LEVEL.D3D_FEATURE_LEVEL_11_0);

            return hr == HRESULTS.S_FALSE;
        }

        private static string? GetAdapterInstancePath(ulong luid)
        {
            try
            {
                var adpname = Vanara.PInvoke.User32.DisplayConfigGetDeviceInfo<Vanara.PInvoke.Gdi32.DISPLAYCONFIG_ADAPTER_NAME>(luid, 0, Vanara.PInvoke.Gdi32.DISPLAYCONFIG_DEVICE_INFO_TYPE.DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME);

                var interface_path = adpname.adapterDevicePath;

                var interface_guid_pos = interface_path.LastIndexOf('#');
                if (interface_guid_pos == -1)
                {
                    return null;
                }

                return interface_path[4..interface_guid_pos].Replace('#', '\\');
            }
            catch
            {
                return null;
            }
        }

        private static bool IsIndirectDisplayAdapter(string instance_path)
        {
            try
            {
                using var regkey = Registry.LocalMachine.OpenSubKey("SYSTEM\\CurrentControlSet\\Enum\\" + instance_path, false);

                if (regkey == null)
                {
                    return false;
                }

                var upperfilters = regkey.GetValue("UpperFilters");

                if (upperfilters == null)
                {
                    return false;
                }

                if (upperfilters is string[] sa)
                {
                    return sa.Any(s => s.Equals("IndirectKmd", StringComparison.InvariantCultureIgnoreCase));
                }
            }
            catch
            {
                // ignore
            }

            return false;
        }

        public static GpuOption GetCurrent()
        {
            bool.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.PerformanceUseGpu, "false"), out var useGpu);
            var preferredGpuInstancePath = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty);
            var preferredGpuDescription = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty);

            GpuOption result;
            if (useGpu)
            {
                var options = GetGpuOptions();
                if (ReferenceEquals(options, _unavailableOptions))
                {
                    result = Disable;
                }

                result = SystemDefault;
                if (preferredGpuInstancePath != string.Empty)
                {
                    result = options.OfType<SpecificGpuOption>().FirstOrDefault(x => string.Equals(((SpecificGpuOption)x).InstancePath, preferredGpuInstancePath, StringComparison.Ordinal), SystemDefault);
                }

                if (result == SystemDefault && preferredGpuDescription != string.Empty)
                {
                    // instance path lookup failed, fallback to description (name) lookup
                    result = options.OfType<SpecificGpuOption>().FirstOrDefault(x => string.Equals(((SpecificGpuOption)x).Description, preferredGpuDescription, StringComparison.OrdinalIgnoreCase), SystemDefault);
                }
            }
            else
            {
                result = Disable;
            }

            return result;
        }

        public static void SetCurrent(GpuOption option)
        {
            switch (option)
            {
                case DisableOption _:
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformanceUseGpu, "false");
                    break;
                case SystemDefaultOption _:
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformanceUseGpu, "true");
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty);
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty);
                    break;
                case SpecificGpuOption x:
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformanceUseGpu, "true");
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformancePreferredGpuDescription, x.Description);
                    ConfigurationHelper.SetValue(ConfigurationKeys.PerformancePreferredGpuInstancePath, x.InstancePath);
                    break;
                default:
                    break;
            }
        }

        public class DisableOption : GpuOption
        {
            public static DisableOption Instance { get; } = new DisableOption();

            private DisableOption() { }

            public override bool Equals(object? obj)
            {
                return obj is DisableOption;
            }

            public override int GetHashCode() => typeof(DisableOption).GetHashCode();

            public override string ToString() => LocalizationHelper.GetString("GpuOptionDisable");
        }

        public abstract class EnableOption : GpuOption
        {
            public abstract uint Index { get; }
        }

        public class SystemDefaultOption : EnableOption
        {
            public static SystemDefaultOption Instance { get; } = new SystemDefaultOption();

            private SystemDefaultOption() { }

            public override uint Index => 0;

            public override bool Equals(object? obj)
            {
                return obj is SystemDefaultOption;
            }

            public override int GetHashCode() => typeof(SystemDefaultOption).GetHashCode();

            public override string ToString() => LocalizationHelper.GetString("GpuOptionSystemDefault");
        }

        public class SpecificGpuOption : EnableOption
        {
            private DXGI_ADAPTER_DESC1 _description;
            private uint _index;
            private string _instance_path;

            public bool ShowIndex { get; set; }

            public SpecificGpuOption(uint index, DXGI_ADAPTER_DESC1 description, string instance_path)
            {
                _index = index;
                _description = description;
                _instance_path = instance_path;
            }

            public override uint Index => _index;

            public string Description => _description.Description;

            public string InstancePath => _instance_path;

            public override bool Equals(object? obj)
            {
                if (obj is SpecificGpuOption x)
                {
                    return x._description.AdapterLuid.Value == _description.AdapterLuid.Value;
                }

                return false;
            }

            public override int GetHashCode() => (typeof(SpecificGpuOption), _description, _index, _instance_path).GetHashCode();

            public override string ToString() => ShowIndex ? _description + $" (GPU {_index})" : _description.Description;
        }
    }
}
