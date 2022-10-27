using System;
using System.Diagnostics;
using System.Windows;
using Application = System.Windows.Forms.Application;

namespace MeoAsstGui.Views
{
    /// <summary>
    ///     ErrorView.xaml 的交互逻辑
    /// </summary>
    public partial class ErrorView : Window
    {
        protected bool ShouldExit { get; set; }

        public ErrorView()
        {
            InitializeComponent();
        }

        public ErrorView(string error, string details, bool shouldExit)
        {
            InitializeComponent();
            Title = error;
            Error.Text = error;
            ErrorDetails.Text = details;
            ShouldExit = shouldExit;

            var isZhCn = ViewStatusStorage.Get("GUI.Localization", Localization.DefaultLanguage) == "zh-cn";
            ErrorQqGroup.Visibility = isZhCn ? Visibility.Visible : Visibility.Hidden;
            ErrorQqGroupLink.Visibility = isZhCn ? Visibility.Visible : Visibility.Hidden;
        }

        public void Pause()
        {
            while (ShouldExit)
            {
                Application.DoEvents();
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            if (ShouldExit)
            {
                Environment.Exit(0);
            }

            base.OnClosed(e);
        }

        private void LinkToCreateGitHubIssue_OnClick(object sender, RoutedEventArgs e)
        {
            Process.Start(LinkToJoinQqGroup.NavigateUri.ToString());
        }

        private void LinkToJoinQqGroup_OnClick(object sender, RoutedEventArgs e)
        {
            Process.Start(LinkToJoinQqGroup.NavigateUri.ToString());
        }
    }
}
