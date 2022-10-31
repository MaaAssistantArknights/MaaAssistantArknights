// <copyright file="Logger.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms.VisualStyles;

namespace MeoAsstGui
{
    public class Logger
    {
        public static void Info(string message)
        {
            Log(message);
        }

        public static void Error(string message, string function = "")
        {
            Log(function + " Error: " + message);
        }

        private static readonly string Filename = "gui.log";
        private static readonly object _lock = new object();

        private static void Log(string message)
        {
            lock (_lock)
            {
                File.AppendAllText(Filename, DateTime.Now.ToString() + ' ' + message + '\n');
            }
        }
    }
}
