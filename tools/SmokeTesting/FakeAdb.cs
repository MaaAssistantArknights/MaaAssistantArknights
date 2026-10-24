// <copyright file="FakeAdb.cs" company="MaaAssistantArknights">
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

using System;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Drawing;
using System.Drawing.Imaging;

class FakeEmulatorAdb
{
    static string Root = Environment.GetEnvironmentVariable("MAA_FAKE_ADB_ROOT") ?? AppDomain.CurrentDomain.BaseDirectory;
    static void Record(string kind, string command)
    {
        using (var guard = new Mutex(false, "Local\\MaaFakeEmulatorLog"))
        {
            guard.WaitOne();
            try { File.AppendAllText(Path.Combine(Root, "fake-adb.log"), DateTime.UtcNow.ToString("O") + "\t" + Process.GetCurrentProcess().Id + "\t" + kind + "\t" + command + Environment.NewLine); }
            finally { guard.ReleaseMutex(); }
        }
    }
    static void Main(string[] args)
    {
        string command = string.Join(" ", args);
        Record("CALL", command);
        string mode = File.ReadAllText(Path.Combine(Root, "fake-mode.txt")).Trim();
        if ((mode == "connect_hang" && command == "devices") ||
            (mode == "work_hang" && command.Contains("screencap") && File.Exists(Path.Combine(Root, "fake-task-started"))))
        {
            Record("BLOCKED", command);
            Thread.Sleep(Timeout.Infinite);
            return;
        }
        if (command == "devices") Console.Write("List of devices attached\n127.0.0.1:65530\tdevice\n");
        else if (command.StartsWith("connect ")) Console.Write("connected to 127.0.0.1:65530\n");
        else if (command.Contains("settings get secure android_id")) Console.Write("0123456789abcdef\n");
        else if (command.Contains("ro.build.version.release")) Console.Write("12\n");
        else if (command.Contains("wm size")) Console.Write("1280\n720\n");
        else if (command.Contains("ro.product.cpu.abilist")) Console.Write("x86_64\n");
        else if (command.Contains("SurfaceOrientation")) Console.Write("0\n");
        else if (command.Contains("screencap"))
        {
            using (var bitmap = new Bitmap(1280, 720))
            using (var graphics = Graphics.FromImage(bitmap))
            using (var memory = new MemoryStream())
            {
                graphics.Clear(Color.FromArgb(30, 40, 50));
                graphics.DrawString("MAA FAKE DEVICE - NO GAME", SystemFonts.DefaultFont, Brushes.White, 30, 30);
                bitmap.Save(memory, ImageFormat.Png);
                byte[] data = memory.ToArray();
                Console.OpenStandardOutput().Write(data, 0, data.Length);
            }
        }
        else if (command.Contains("am start"))
        {
            File.WriteAllText(Path.Combine(Root, "fake-task-started"), DateTime.UtcNow.ToString("O"));
            Console.Write("Starting: Intent { fake device }\n" + new string(' ', 200) + "\n");
        }
        else if (command.Contains("SurfaceFlinger")) Console.Write("16666666\n");
        Record("RETURN", command);
    }
}
