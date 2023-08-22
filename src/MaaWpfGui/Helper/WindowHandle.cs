// <copyright file="WindowHandle.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Windows;
using System.Windows.Interop;

namespace MaaWpfGui.Helper
{
    public struct WindowHandle
    {
        private IntPtr _handle;

        public IntPtr Handle => _handle;

        public static WindowHandle None => new WindowHandle() { _handle = IntPtr.Zero };

        public static implicit operator WindowHandle(IntPtr h) => new WindowHandle() { _handle = h };

        public static implicit operator WindowHandle(Window w)
        {
            if (w == null)
            {
                return None;
            }

            var interop = new WindowInteropHelper(w);
            return new WindowHandle { _handle = interop.Handle };
        }
    }
}
