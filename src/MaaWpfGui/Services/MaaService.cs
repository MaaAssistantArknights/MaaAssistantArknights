using System;
using System.Runtime.InteropServices;
using MaaWpfGui.Main;
using Newtonsoft.Json.Linq;
using AsstHandle = System.IntPtr;
using AsstInstanceOptionKey = System.Int32;
using AsstTaskId = System.Int32;

namespace MaaWpfGui.Services
{
    public class MaaService
    {
        public delegate void CallbackDelegate(int msg, IntPtr jsonBuffer, IntPtr customArg);

        public delegate void ProcCallbackMsg(AsstMsg msg, JObject details);

        [DllImport("MaaCore.dll")]
        public static extern AsstHandle AsstCreateEx(CallbackDelegate callback, IntPtr customArg);

        [DllImport("MaaCore.dll")]
        public static extern void AsstDestroy(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        public static extern unsafe bool AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, byte* value);

        [DllImport("MaaCore.dll")]
        public static extern unsafe bool AsstLoadResource(byte* dirname);

        [DllImport("MaaCore.dll")]
        public static extern void AsstClearAvatarCache();

        [DllImport("MaaCore.dll")]
        public static extern unsafe bool AsstConnect(AsstHandle handle, byte* adbPath, byte* address, byte* config);

        [DllImport("MaaCore.dll")]
        public static extern unsafe AsstTaskId AsstAppendTask(AsstHandle handle, byte* type, byte* taskParams);

        [DllImport("MaaCore.dll")]
        public static extern unsafe bool AsstSetTaskParams(AsstHandle handle, AsstTaskId id, byte* taskParams);

        [DllImport("MaaCore.dll")]
        public static extern bool AsstStart(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        public static extern bool AsstRunning(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        public static extern bool AsstStop(AsstHandle handle);

        [DllImport("MaaCore.dll")]
        public static extern unsafe ulong AsstGetImage(AsstHandle handle, byte* buff, ulong buffSize);

        [DllImport("MaaCore.dll")]
        public static extern ulong AsstGetNullSize();

        [DllImport("MaaCore.dll")]
        public static extern IntPtr AsstGetVersion();
    }
}
