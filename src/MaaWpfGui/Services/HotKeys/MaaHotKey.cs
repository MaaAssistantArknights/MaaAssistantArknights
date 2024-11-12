// <copyright file="MaaHotKey.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Text;
using System.Windows.Input;
using GlobalHotKey;

namespace MaaWpfGui.Services.HotKeys
{
    public class MaaHotKey : HotKey
    {
        public MaaHotKey()
        {
        }

        public MaaHotKey(Key key, ModifierKeys modifiers)
            : base(key, modifiers)
        {
        }

        public override string ToString()
        {
            var str = new StringBuilder();

            if (Modifiers.HasFlag(ModifierKeys.Control))
            {
                str.Append("Ctrl + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Shift))
            {
                str.Append("Shift + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Alt))
            {
                str.Append("Alt + ");
            }

            if (Modifiers.HasFlag(ModifierKeys.Windows))
            {
                str.Append("Win + ");
            }

            str.Append(Key);

            return str.ToString();
        }
    }
}
