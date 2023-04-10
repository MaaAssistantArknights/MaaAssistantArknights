// <copyright file="MessageBoxHelper.cs" company="MaaAssistantArknights">
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
#pragma warning disable 0618
#pragma warning disable SA1307
#pragma warning disable SA1401
#pragma warning disable IDE0051
using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;
using System.Windows;
using System.Windows.Interop;
using HandyControl.Data;
using Point = System.Windows.Point;

[assembly: SecurityPermission(SecurityAction.RequestMinimum, UnmanagedCode = true)]

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// 消息框弹窗格式管理
    /// </summary>
    public static class MessageBoxHelper
    {
        private delegate IntPtr HookProc(int nCode, IntPtr wParam, IntPtr lParam);

        private delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

        private const int WHCALLWNDPROCRET = 12;
        private const int WMDESTROY = 0x0002;
        private const int WMINITDIALOG = 0x0110;
        private const int WMTIMER = 0x0113;
        private const int WMUSER = 0x400;
        private const int DMGETDEFID = WMUSER + 0;
        private const int MBOK = 1;
        private const int MBCancel = 2;
        private const int MBAbort = 3;
        private const int MBRetry = 4;
        private const int MBIgnore = 5;
        private const int MBYes = 6;
        private const int MBNo = 7;

        [DllImport("user32.dll")]
        private static extern bool GetWindowRect(IntPtr hWnd, ref Rectangle lpRect);

        [DllImport("user32.dll")]
        private static extern int MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, bool bRepaint);

        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern IntPtr SetWindowsHookEx(int idHook, HookProc lpfn, IntPtr hInstance, int threadId);

        [DllImport("user32.dll")]
        private static extern int UnhookWindowsHookEx(IntPtr idHook);

        [DllImport("user32.dll")]
        private static extern IntPtr CallNextHookEx(IntPtr idHook, int nCode, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", EntryPoint = "GetWindowTextLengthW", CharSet = CharSet.Unicode)]
        private static extern int GetWindowTextLength(IntPtr hWnd);

        [DllImport("user32.dll", EntryPoint = "GetWindowTextW", CharSet = CharSet.Unicode)]
        private static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxLength);

        [DllImport("user32.dll")]
        private static extern int EndDialog(IntPtr hDlg, IntPtr nResult);

        [DllImport("user32.dll")]
        private static extern bool EnumChildWindows(IntPtr hWndParent, EnumChildProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll", EntryPoint = "GetClassNameW", CharSet = CharSet.Unicode)]
        private static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport("user32.dll")]
        private static extern int GetDlgCtrlID(IntPtr hwndCtl);

        [DllImport("user32.dll")]
        private static extern IntPtr GetDlgItem(IntPtr hDlg, int nIDDlgItem);

        [DllImport("user32.dll", EntryPoint = "SetWindowTextW", CharSet = CharSet.Unicode)]
        private static extern bool SetWindowText(IntPtr hWnd, string lpString);

        [StructLayout(LayoutKind.Sequential)]

        public struct CWPRETSTRUCT
        {
            public IntPtr lResult;
            public IntPtr lParam;
            public IntPtr wParam;
            public uint message;
            public IntPtr hwnd;
        }

        private static readonly HookProc hookProc;
        private static readonly EnumChildProc enumProc;
        [ThreadStatic]
        private static IntPtr hHook;
        [ThreadStatic]
        private static int nButton;

        /// <summary>
        /// OK text
        /// </summary>
        public static string OK = "OK";

        /// <summary>
        /// Cancel text
        /// </summary>
        public static string Cancel = "Cancel";

        /// <summary>
        /// Abort text
        /// </summary>
        public static string Abort = "Abort";

        /// <summary>
        /// Retry text
        /// </summary>
        public static string Retry = "Retry";

        /// <summary>
        /// Ignore text
        /// </summary>
        public static string Ignore = "Ignore";

        /// <summary>
        /// Yes text
        /// </summary>
        public static string Yes = "Yes";

        /// <summary>
        /// No text
        /// </summary>
        public static string No = "No";

        static MessageBoxHelper()
        {
            hookProc = new HookProc(MessageBoxHookProc);
            enumProc = new EnumChildProc(MessageBoxEnumProc);
            hHook = IntPtr.Zero;
        }

        /// <summary>
        /// Enables MessageBoxManager functionality
        /// </summary>
        /// <remarks>
        /// MessageBoxManager functionality is enabled on current thread only.
        /// Each thread that needs MessageBoxManager functionality has to call this method.
        /// </remarks>
        public static void Register()
        {
            if (hHook != IntPtr.Zero)
            {
                throw new NotSupportedException("One hook per thread allowed.");
            }

            hHook = SetWindowsHookEx(WHCALLWNDPROCRET, hookProc, IntPtr.Zero, AppDomain.GetCurrentThreadId());
        }

        /// <summary>
        /// Disables MessageBoxManager functionality
        /// </summary>
        /// <remarks>
        /// Disables MessageBoxManager functionality on current thread only.
        /// </remarks>
        public static void Unregister()
        {
            if (hHook != IntPtr.Zero)
            {
                UnhookWindowsHookEx(hHook);
                hHook = IntPtr.Zero;
            }
        }

        private static IntPtr MessageBoxHookProc(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode < 0)
            {
                return CallNextHookEx(hHook, nCode, wParam, lParam);
            }

            CWPRETSTRUCT msg = (CWPRETSTRUCT)Marshal.PtrToStructure(lParam, typeof(CWPRETSTRUCT));
            IntPtr hook = hHook;
            if (msg.message == WMINITDIALOG)
            {
                int nLength = GetWindowTextLength(msg.hwnd);
                StringBuilder className = new StringBuilder(10);
                GetClassName(msg.hwnd, className, className.Capacity);
                if (className.ToString() == "#32770")
                {
                    nButton = 0;
                    EnumChildWindows(msg.hwnd, enumProc, IntPtr.Zero);
                    if (nButton == 1)
                    {
                        IntPtr hButton = GetDlgItem(msg.hwnd, MBCancel);
                        if (hButton != IntPtr.Zero)
                        {
                            _ = SetWindowText(hButton, OK);
                        }
                    }
                }

                Window mainWindow = Application.Current.MainWindow;
                source = CreateHwndSource(mainWindow);
                if (source != null && mainWindow.WindowState == WindowState.Normal)
                {
                    CenterWindow(msg.hwnd);
                }
            }

            return CallNextHookEx(hook, nCode, wParam, lParam);
        }

        private static HwndSource source = null;

        private static HwndSource CreateHwndSource(Window owner)
        {
            return (HwndSource)PresentationSource.FromVisual(owner);
        }

        private static void CenterWindow(IntPtr hChildWnd)
        {
            Rectangle recChild = new Rectangle(0, 0, 0, 0);
            bool success = GetWindowRect(hChildWnd, ref recChild);

            int width = recChild.Width - recChild.X;
            int height = recChild.Height - recChild.Y;

            Rectangle recParent = new Rectangle(0, 0, 0, 0);
            success = GetWindowRect(source.Handle, ref recParent);

            Point ptCenter = new Point(0, 0);
            ptCenter.X = recParent.X + ((recParent.Width - recParent.X) / 2);
            ptCenter.Y = recParent.Y + ((recParent.Height - recParent.Y) / 2);

            Point ptStart = new Point(0, 0);
            ptStart.X = ptCenter.X - (width / 2);
            ptStart.Y = ptCenter.Y - (height / 2);

            int result = MoveWindow(hChildWnd, Convert.ToInt32(ptStart.X), Convert.ToInt32(ptStart.Y),
                                    width, height, false);
        }

        private static bool MessageBoxEnumProc(IntPtr hWnd, IntPtr lParam)
        {
            StringBuilder className = new StringBuilder(10);
            GetClassName(hWnd, className, className.Capacity);
            if (className.ToString() == "Button")
            {
                int ctlId = GetDlgCtrlID(hWnd);
                switch (ctlId)
                {
                    case MBOK:
                        SetWindowText(hWnd, OK);
                        break;
                    case MBCancel:
                        SetWindowText(hWnd, Cancel);
                        break;
                    case MBAbort:
                        SetWindowText(hWnd, Abort);
                        break;
                    case MBRetry:
                        SetWindowText(hWnd, Retry);
                        break;
                    case MBIgnore:
                        SetWindowText(hWnd, Ignore);
                        break;
                    case MBYes:
                        SetWindowText(hWnd, Yes);
                        break;
                    case MBNo:
                        SetWindowText(hWnd, No);
                        break;
                }

                nButton++;
            }

            return true;
        }

        private static void SetImage(MessageBoxImage messageBoxImage, ref string iconKey, ref string iconBrushKey)
        {
            var key = string.Empty;
            var brushKey = string.Empty;

            switch (messageBoxImage)
            {
                case MessageBoxImage.Question:
                    key = ResourceToken.AskGeometry;
                    brushKey = ResourceToken.AccentBrush;
                    break;
                case MessageBoxImage.Error:
                    key = ResourceToken.ErrorGeometry;
                    brushKey = ResourceToken.DangerBrush;
                    break;
                case MessageBoxImage.Warning:
                    key = ResourceToken.WarningGeometry;
                    brushKey = ResourceToken.WarningBrush;
                    break;
                case MessageBoxImage.Information:
                    key = ResourceToken.InfoGeometry;
                    brushKey = ResourceToken.InfoBrush;
                    break;
            }

            iconKey = string.IsNullOrEmpty(iconKey) ? key : iconKey;
            iconBrushKey = string.IsNullOrEmpty(iconBrushKey) ? brushKey : iconBrushKey;
        }

        public static MessageBoxResult Show(MessageBoxInfo info) => HandyControl.Controls.MessageBox.Show(info);

        public static MessageBoxResult Show(
            string messageBoxText,
            string caption = "",
            MessageBoxButton buttons = MessageBoxButton.OK,
            MessageBoxImage icon = MessageBoxImage.None,
            string iconKey = "",
            string iconBrushKey = "",
            string ok = "",
            string cancel = "",
            string yes = "",
            string no = "",
            bool useNativeMethod = false)
        {
            caption = string.IsNullOrEmpty(caption) ? LocalizationHelper.GetString("Tip") : caption;
            ok = string.IsNullOrEmpty(ok) ? LocalizationHelper.GetString("Ok") : ok;
            cancel = string.IsNullOrEmpty(cancel) ? LocalizationHelper.GetString("ManualRestart") : cancel;
            yes = string.IsNullOrEmpty(yes) ? LocalizationHelper.GetString("Yes") : yes;
            no = string.IsNullOrEmpty(no) ? LocalizationHelper.GetString("No") : no;

            if (useNativeMethod)
            {
                Unregister();
                OK = ok;
                Cancel = cancel;
                Yes = yes;
                No = no;
                Register();
                var result = MessageBox.Show(messageBoxText, caption, buttons, icon);
                Unregister();
                return result;
            }
            else
            {
                SetImage(icon, ref iconKey, ref iconBrushKey);
                var info = new MessageBoxInfo
                {
                    Message = messageBoxText,
                    Caption = caption,
                    Button = buttons,
                    IconKey = iconKey,
                    IconBrushKey = iconBrushKey,
                    ConfirmContent = ok,
                    CancelContent = cancel,
                    YesContent = yes,
                    NoContent = no,
                };
                return HandyControl.Controls.MessageBox.Show(info);
            }
        }
    }
}
