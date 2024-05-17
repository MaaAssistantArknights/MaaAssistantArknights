// <copyright file="MaaWineBridge.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MAA project
// Copyright (C) 2021 MistEO and Contributors
// </copyright>

using System.Runtime.InteropServices;

namespace MaaWpfGui.WineCompat;

internal static class MaaWineBridge
{
    // MaaWineBridge
    [DllImport("MaaCore.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern int dl_has_maacore();

    public static WineBridgeAvailability Availability { get; }

    static MaaWineBridge()
    {
        if (!WineRuntimeInformation.IsRunningUnderWine)
        {
            Availability = WineBridgeAvailability.NotAvailable;
            return;
        }

        try
        {
            if (dl_has_maacore() != 0)
            {
                Availability = WineBridgeAvailability.Operational;
            }
            else
            {
                Availability = WineBridgeAvailability.Faulted;
            }
        }
        catch
        {
            Availability = WineBridgeAvailability.NotAvailable;
        }
    }
}
