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

using System;
using System.ComponentModel;
using System.Drawing;
using System.Media;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Microsoft.Win32;
using Notification.Wpf;
using Notification.Wpf.Base;
using Notification.Wpf.Constants;
using Notification.Wpf.Controls;

namespace MeoAsstGui
{
    public class ToastNotification : IDisposable
    {
        private NotificationManager _notificationManager = new NotificationManager();

        /// <summary>
        /// 通知标题
        /// </summary>
        private string _notificationTitle = string.Empty;

        /// <summary>
        /// 通知文本内容
        /// </summary>
        private StringBuilder _contentCollection = new StringBuilder();

        /// <summary>
        /// 应用图标资源
        /// </summary>
        private ImageSource GetAppIcon()
        {
            try
            {
                var icon = Icon.ExtractAssociatedIcon(System.Windows.Forms.Application.ExecutablePath);
                var imageSource = Imaging.CreateBitmapSourceFromHIcon(
                    icon.Handle,
                    Int32Rect.Empty,
                    BitmapSizeOptions.FromEmptyOptions());

                return imageSource;
            }
            catch (Exception)
            {
                return new BitmapImage();
            }
        }

        /// <summary>
        /// Toast通知，基于 Notification.Wpf 实现，方便管理通知样式
        /// <para>建议使用 using 调用，如果不使用 using 调用，建议手动调用 Dispose() 方法释放</para>
        /// </summary>
        /// <param name="title">通知标题</param>
        public ToastNotification(string title = null)
        {
            // 初始化通知标题
            _notificationTitle = title ?? _notificationTitle;

            #region 初始化 Notification.Wpf 默认静态配置

            // 同时显示最大数量
            NotificationConstants.NotificationsOverlayWindowMaxCount = 5;

            // 默认显示位置
            NotificationConstants.MessagePosition = NotificationPosition.BottomRight;

            // 最小显示宽度
            NotificationConstants.MinWidth = 350d;

            // 最大显示宽度
            NotificationConstants.MaxWidth = 380d;

            #endregion
        }

        /// <summary>
        /// 添加一行内容文本
        /// </summary>
        /// <param name="text">文本内容</param>
        /// <returns>返回类本身，可继续调用其它方法</returns>
        public ToastNotification AddContentText(string text = null)
        {
            _contentCollection.AppendLine(text ?? string.Empty);
            return this;
        }

        #region 通知提示音

        #region 通知提示音列表枚举

        public enum NotificationSounds
        {
            [Description("默认响声")]
            Beep,

            [Description("感叹号")]
            Exclamation,

            [Description("星号")]
            Asterisk,

            [Description("问题")]
            Question,

            [Description("关键性停止")]
            Hand,

            [Description("通知 (Windows 10 及以上特有，低版本系统直接用也可以)")]
            Notification,

            [Description("无声")]
            None
        }

        #endregion

        /// <summary>
        /// 播放通知提示音
        /// 如果要播放音频文件，参考微软文档 SoundPlayer 类
        /// </summary>
        /// <param name="sounds">提示音类型</param>
        protected async Task PlayNotificationSoundAsync(NotificationSounds sounds = NotificationSounds.None)
        {
            await Task.Run(() =>
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
                                    PlayNotificationSoundAsync(NotificationSounds.Asterisk);
                                }
                            }
                            break;

                        case NotificationSounds.None:
                        default: break;
                    }
                }
                catch (Exception) { }
            }).ConfigureAwait(false);
        }

        #endregion

        #region 通知按钮设置

        #region 通知按钮变量
        // 左边按钮
        protected string _buttonLeftText = null;
        protected Action _buttonLeftAction = null;

        // 右边按钮
        protected string _buttonRightText = null;
        protected Action _buttonRightAction = null;
        #endregion

        /// <summary>
        /// 给通知添加一个在左边的按钮，比如确定按钮，多次设置只会按最后一次设置生效
        /// </summary>
        /// <param name="text">按钮标题</param>
        /// <param name="action">按钮执行方法</param>
        /// <returns>返回类本身，可继续调用其它方法</returns>
        public ToastNotification AddButtonLeft(string text, Action action)
        {
            _buttonLeftText = text;
            _buttonLeftAction = action;
            return this;
        }

        /// <summary>
        /// 给通知添加一个在右边的按钮，比如取消按钮，多次设置只会按最后一次设置生效
        /// </summary>
        /// <param name="text">按钮标题</param>
        /// <param name="action">按钮执行方法</param>
        /// <returns>返回类本身，可继续调用其它方法</returns>
        public ToastNotification AddButtonRight(string text, Action action)
        {
            _buttonRightText = text;
            _buttonRightAction = action;
            return this;
        }

        #endregion

        #region 通知显示

        #region 通知基本字体样式和内容模板

        /// <summary>
        /// 创建一个基本文本字体样式
        /// </summary>
        public TextContentSettings BaseTextSettings => new TextContentSettings()
        {
            FontStyle = FontStyles.Normal,
            FontFamily = new System.Windows.Media.FontFamily("Microsoft Yahei"),
            FontSize = 14,
            FontWeight = FontWeights.Normal,
            TextAlignment = TextAlignment.Left,
            HorizontalAlignment = HorizontalAlignment.Stretch,
            VerticalTextAlignment = VerticalAlignment.Stretch,
            Opacity = 1d
        };

        /// <summary>
        /// 创建一个基本通知内容模板
        /// </summary>
        public NotificationContent BaseContent()
        {
            var content = new NotificationContent()
            {
                Title = _notificationTitle,
                Message = _contentCollection.ToString(),

                Type = NotificationType.None,
                Icon = GetAppIcon(),
                CloseOnClick = true,

                RowsCount = 1,
                TrimType = NotificationTextTrimType.AttachIfMoreRows,

                Background = (SolidColorBrush)new BrushConverter().ConvertFrom("#FF332A3A"),

                LeftButtonContent = _buttonLeftText ?? NotificationConstants.DefaultLeftButtonContent,
                LeftButtonAction = _buttonLeftAction ?? null,

                RightButtonContent = _buttonRightText ?? NotificationConstants.DefaultRightButtonContent,
                RightButtonAction = _buttonRightAction ?? null,
            };

            // 默认的标题文本样式
            var titleTextSettings = BaseTextSettings;
            titleTextSettings.FontSize = 18d;
            content.TitleTextSettings = titleTextSettings;

            // 默认的内容文本样式
            var messageTextSettings = BaseTextSettings;
            content.MessageTextSettings = messageTextSettings;

            return content;
        }

        #endregion

        #region 显示通知方法

        /// <summary>
        /// 显示通知
        /// </summary>
        /// <param name="lifeTime">通知显示时间 (s)</param>
        /// <param name="row">内容显示行数，如果内容太多建议使用 ShowMore()</param>
        /// <param name="sound">播放提示音</param>
        /// <param name="notificationContent">通知内容</param>
        public void Show(double lifeTime = 10d, uint row = 1,
            NotificationSounds sound = NotificationSounds.Notification,
            NotificationContent notificationContent = null)
        {
            notificationContent = notificationContent ?? BaseContent();

            notificationContent.RowsCount = row;

            // 调整显示时间，如果存在按钮的情况下显示时间将强制设为最大时间
            lifeTime = lifeTime < 3d ? 3d : lifeTime;

            var timeSpan = _buttonLeftAction == null && _buttonRightAction == null
                ? TimeSpan.FromSeconds(lifeTime)
                : TimeSpan.MaxValue;

            // 显示通知
            _notificationManager.Show(
                notificationContent,
                expirationTime: timeSpan,
                ShowXbtn: false);

            // 播放通知提示音
            PlayNotificationSoundAsync(sound);
        }

        /// <summary>
        /// 显示内容更多的通知
        /// </summary>
        /// <param name="lifeTime">通知显示时间 (s)</param>
        /// <param name="row">内容显示行数，只用于预览一部分通知，多出内容会放在附加按钮的窗口中</param>
        /// <param name="sound">播放提示音，不设置就没有声音</param>
        /// <param name="notificationContent">通知内容</param>
        public void ShowMore(double lifeTime = 12d, uint row = 2,
            NotificationSounds sound = NotificationSounds.None,
            NotificationContent notificationContent = null)
        {
            notificationContent = notificationContent ?? BaseContent();
            notificationContent.TrimType = NotificationTextTrimType.Attach;

            Show(lifeTime: lifeTime,
                 row: row,
                 sound: sound,
                 notificationContent: notificationContent);
        }

        /// <summary>
        /// 显示公招特殊标签通知
        /// </summary>
        /// <param name="row">内容显示行数，比如第 2 行用来放星星</param>
        public void ShowRecruit(uint row = 1)
        {
            var content = BaseContent();

            // 给通知染上资深标签相似的颜色
            content.Background = (SolidColorBrush)new BrushConverter().ConvertFrom("#FF401804");
            content.Foreground = (SolidColorBrush)new BrushConverter().ConvertFrom("#FFFFC800");

            Show(row: row,
                 sound: NotificationSounds.Notification,
                 notificationContent: content);
        }

        #endregion

        #endregion

        /// <summary>
        /// 通知使用完后释放已使用的数据
        /// </summary>
        public void Dispose()
        {
            _contentCollection.Clear();

            _notificationManager = null;
            _notificationTitle = null;
            _contentCollection = null;
            _buttonLeftText = _buttonRightText = null;
            _buttonLeftAction = _buttonRightAction = null;
        }

    }
}
