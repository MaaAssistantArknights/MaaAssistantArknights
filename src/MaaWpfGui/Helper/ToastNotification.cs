// <copyright file="ToastNotification.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Media;
using System.Net;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Interop;
using HandyControl.Controls;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper.Notification;
using MaaWpfGui.WineCompat;
using Microsoft.Win32;
using Notification.Wpf.Constants;
using Serilog;
using Stylet;
using Windows.Win32;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// The toast notification.
    /// </summary>
    public class ToastNotification : IDisposable
    {
        private static readonly INotificationPoster _notificationPoster;

        private static readonly string _openUrlPrefix;

        static ToastNotification()
        {
            _notificationPoster = GetNotificationPoster();
            _notificationPoster.ActionActivated += OnActionActivated;
            _openUrlPrefix = $"OpenUrl:{_notificationPoster.GetHashCode()}:";
        }

        private static readonly ILogger _logger = Log.ForContext<ToastNotification>();

        /// <summary>
        /// Checks toast system.
        /// </summary>
        /// <returns>The toast system is initialized.</returns>
        private static INotificationPoster GetNotificationPoster()
        {
            if (WineRuntimeInformation.IsRunningUnderWine)
            {
                if (NotificationImplLibNotify.IsSupported)
                {
                    return new NotificationImplLibNotify();
                }
            }
            else if (OperatingSystem.IsWindowsVersionAtLeast(10, 0, 10240))
            {
                return new NotificationImplWinRT();
            }

            return new NotificationImplWpf();
        }

        /// <summary>
        /// 按钮激活后的事件，参数为按钮标签
        /// </summary>
        public static event EventHandler<string> ActionActivated;

        /// <summary>
        /// 通知标题
        /// </summary>
        private readonly string _notificationTitle = string.Empty;

        /// <summary>
        /// 通知文本内容
        /// </summary>
        private readonly StringBuilder _contentCollection = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="ToastNotification"/> class.
        /// </summary>
        /// <param name="title">通知标题</param>
        /// <remarks>
        /// <para>Toast通知，基于 <see cref="NotificationConstants"/> 实现，方便管理通知样式。</para>
        /// <para>建议使用 <see langword="using"/> 调用，如果不使用 <see langword="using"/> 调用，建议手动调用 <see cref="Dispose()"/> 方法释放。</para>
        /// <para>在线程中必须使用 <c>Dispatcher.Invoke</c> 相关方法调用。</para>
        /// </remarks>
        public ToastNotification(string title = null)
        {
            // 初始化通知标题
            _notificationTitle = title ?? _notificationTitle;
        }

        private static void OnActionActivated(object sender, string tag)
        {
            string decodedTag = WebUtility.UrlDecode(tag);

            if (decodedTag.StartsWith(_openUrlPrefix))
            {
                var url = decodedTag[_openUrlPrefix.Length..];
                Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
            }
            else
            {
                ActionActivated?.Invoke(sender, decodedTag);
            }
        }

        /// <summary>
        /// 添加一行文本内容
        /// </summary>
        /// <param name="text">文本内容</param>
        /// <returns>返回本类，可继续调用其它方法</returns>
        public ToastNotification AppendContentText(string text = null)
        {
            _contentCollection.AppendLine(text ?? string.Empty);
            return this;
        }

        #region 通知提示音

        #region 通知提示音列表枚举

        /// <summary>
        /// 通知提示音列表枚举，
        /// </summary>
        public enum NotificationSounds
        {
            /// <summary>
            /// 默认响声。
            /// </summary>
            [Description("默认响声")]
            Beep,

            /// <summary>
            /// 感叹号。
            /// </summary>
            [Description("感叹号")]
            Exclamation,

            /// <summary>
            /// 星号。
            /// </summary>
            [Description("星号")]
            Asterisk,

            /// <summary>
            /// 问题。
            /// </summary>
            [Description("问题")]
            Question,

            /// <summary>
            /// 关键性停止。
            /// </summary>
            [Description("关键性停止")]
            Hand,

            /// <summary>
            /// 通知 (Windows 10 及以上特有，低版本系统直接用也可以)。
            /// </summary>
            [Description("通知 (Windows 10 及以上特有，低版本系统直接用也可以)")]
            Notification,

            /// <summary>
            /// 无声。
            /// </summary>
            [Description("无声")]
            None,
        }

        #endregion 通知提示音列表枚举

        /// <summary>
        /// 播放通知提示音
        /// 如果要播放音频文件，参考微软文档 <see cref="SoundPlayer"/> 类
        /// </summary>
        /// <param name="sounds">提示音类型</param>
        protected void PlayNotificationSound(NotificationSounds sounds = NotificationSounds.None)
        {
            try
            {
                switch (sounds)
                {
                    case NotificationSounds.Exclamation: SystemSounds.Exclamation.Play(); break;
                    case NotificationSounds.Asterisk: SystemSounds.Asterisk.Play(); break;
                    case NotificationSounds.Question: SystemSounds.Question.Play(); break;
                    case NotificationSounds.Beep: SystemSounds.Beep.Play(); break;
                    case NotificationSounds.Hand: SystemSounds.Hand.Play(); break;

                    case NotificationSounds.Notification:
                        using (var key = Registry.CurrentUser.OpenSubKey(@"AppEvents\Schemes\Apps\.Default\Notification.Default\.Current"))
                        {
                            if (key != null)
                            {
                                // 获取 (Default) 项
                                var o = key.GetValue(null);
                                if (o != null)
                                {
                                    var theSound = new SoundPlayer((string)o);
                                    theSound.Play();
                                }
                            }
                            else
                            {
                                // 如果不支持就播放其它提示音
                                PlayNotificationSound(NotificationSounds.Asterisk);
                            }
                        }

                        break;

                    case NotificationSounds.None:
                    default: break;
                }
            }
            catch (Exception)
            {
                // Ignore
            }
        }

        #endregion 通知提示音

        #region 通知按钮设置

        #region 通知按钮变量

        private readonly List<NotificationAction> _actions = [];

        #endregion 通知按钮变量

        /// <summary>
        /// 给通知添加一个按钮
        /// </summary>
        /// <param name="label">按钮标题</param>
        /// <param name="tag">事件标签</param>
        /// <returns>返回类本身，可继续调用其它方法</returns>
        /// <remarks>按钮按下时，触发 <see cref="ActionActivated"/> 事件</remarks>
        public ToastNotification AddButton(string label, string tag)
        {
            _actions.Add(new NotificationAction(label, tag));
            return this;
        }

        public static string GetActionTagForOpenWeb(string url)
        {
            if (url.StartsWith("http://") || url.StartsWith("https://"))
            {
                return _openUrlPrefix + url;
            }

            throw new ArgumentException("URL must start with http:// or https://");
        }

        #endregion 通知按钮设置

        #region 通知显示
        #region 显示通知方法

        /// <summary>
        /// 显示通知
        /// </summary>
        /// <param name="lifeTime">通知显示时间 (s)</param>
        /// <param name="row">内容显示行数，如果内容太多建议使用 <see cref="ShowMore(double,uint,NotificationSounds,NotificationHint[])"/></param>
        /// <param name="sound">播放提示音</param>
        /// <param name="hints">通知额外元数据，可能影响通知展示方式</param>
        ///
        public void Show(double lifeTime = 10d, uint row = 1,
            NotificationSounds sound = NotificationSounds.Notification, params NotificationHint[] hints)
        {
            Execute.OnUIThread(() =>
            {
                // TODO: 整理过时代码
                if (!ConfigFactory.Root.GUI.UseNotify)
                {
                    Growl.Info(_notificationTitle + _contentCollection);
                    return;
                }

                var content = new NotificationContent { Summary = _notificationTitle, Body = _contentCollection.ToString(), };

                foreach (var action in _actions)
                {
                    content.Actions.Add(action);
                }

                content.Hints.Add(NotificationHint.RowCount((int)row));

                // 调整显示时间，如果存在按钮的情况下显示时间将强制设为最大时间
                lifeTime = lifeTime < 3d ? 3d : lifeTime;

                var timeSpan = _actions.Count != 0
                    ? TimeSpan.FromSeconds(lifeTime)
                    : TimeSpan.MaxValue;

                content.Hints.Add(NotificationHint.ExpirationTime(timeSpan));

                foreach (var hint in hints)
                {
                    content.Hints.Add(hint);
                }

                // 显示通知
                _notificationPoster.ShowNotification(content);

                if (_notificationPoster is not NotificationImplWinRT)
                {
                    PlayNotificationSound(sound); // 播放通知提示音
                }

                // 任务栏闪烁
                FlashWindowEx();
            });
        }

        /// <summary>
        /// 显示内容更多的通知
        /// </summary>
        /// <param name="lifeTime">通知显示时间 (s)</param>
        /// <param name="row">内容显示行数，只用于预览一部分通知，多出内容会放在附加按钮的窗口中</param>
        /// <param name="sound">播放提示音，不设置就没有声音</param>
        /// <param name="hints">通知提示</param>
        public void ShowMore(double lifeTime = 12d, uint row = 2,
            NotificationSounds sound = NotificationSounds.None, params NotificationHint[] hints)
        {
            var moreHints = new NotificationHint[hints.Length + 1];
            hints.CopyTo(moreHints, 0);
            moreHints[hints.Length] = NotificationHint.Expandable;
            Show(lifeTime: lifeTime,
                row: row,
                sound: sound,
                hints: moreHints);
        }

        /// <summary>
        /// 显示公招特殊标签通知
        /// </summary>
        /// <param name="row">内容显示行数，比如第 2 行用来放星星</param>
        public void ShowRecruit(uint row = 1)
        {
            Show(row: row,
                 sound: NotificationSounds.Notification,
                 hints: NotificationHint.RecruitHighRarity);
        }

        /// <summary>
        /// 显示公招小车标签通知
        /// </summary>
        /// <param name="row">内容显示行数，比如第 2 行用来放星星</param>
        public void ShowRecruitRobot(uint row = 1)
        {
            Show(row: row,
                sound: NotificationSounds.Notification,
                hints: NotificationHint.RecruitRobot);
        }

        /// <summary>
        /// 显示更新版本的通知
        /// </summary>
        /// <param name="row">内容行数</param>
        public void ShowUpdateVersion(uint row = 3)
        {
            ShowMore(row: row,
                    sound: NotificationSounds.Notification,
                    hints: NotificationHint.NewVersion);
        }

        /// <summary>
        /// 直接显示通知内容
        /// </summary>
        /// <param name="message">显示内容</param>
        public static void ShowDirect(string message)
        {
            using var toast = new ToastNotification(message);
            toast.Show();
        }

        #endregion 显示通知方法

        #endregion 通知显示

        #region 任务栏闪烁

        /// <summary>
        /// 闪烁窗口任务栏
        /// </summary>
        /// <param name="hWnd">窗口句柄</param>
        /// <param name="type">闪烁类型</param>
        /// <param name="count">闪烁次数</param>
        /// <returns>是否成功</returns>
        internal static bool FlashWindowEx(IntPtr hWnd = 0, FLASHWINFO_FLAGS type = FLASHWINFO_FLAGS.FLASHW_TRAY | FLASHWINFO_FLAGS.FLASHW_TIMERNOFG, uint count = 5)
        {
            try
            {
                var fInfo = default(FLASHWINFO);
                fInfo.cbSize = Convert.ToUInt32(Marshal.SizeOf(fInfo));
                fInfo.hwnd = new(hWnd != 0 ? hWnd : new WindowInteropHelper(System.Windows.Application.Current.MainWindow!).Handle);
                fInfo.dwFlags = type;
                fInfo.uCount = count;
                fInfo.dwTimeout = 0;
                return PInvoke.FlashWindowEx(in fInfo);
            }
            catch
            {
                _logger.Warning("FlashWindowEx error");
                return false;
            }
        }

        #endregion 任务栏闪烁

        /// <summary>
        /// 通知使用完后释放已使用的数据
        /// </summary>
        public void Dispose()
        {
            _contentCollection.Clear();
        }

        public static void Cleanup()
        {
            try
            {
                (_notificationPoster as IDisposable)?.Dispose();
            }
            catch (Exception e)
            {
                Log.ForContext<ToastNotification>().Error(e, "Cleanup error");
            }
        }
    }
}
