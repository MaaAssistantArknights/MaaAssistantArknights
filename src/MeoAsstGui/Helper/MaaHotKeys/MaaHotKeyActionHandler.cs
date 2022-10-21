using System;
using System.Windows;
using StyletIoC;

namespace MeoAsstGui.MaaHotKeys
{
    public class MaaHotKeyActionHandler : IMaaHotKeyActionHandler
    {
        private readonly IContainer _container;

        public MaaHotKeyActionHandler(IContainer container)
        {
            _container = container;
        }

        public void HandleKeyPressed(MaaHotKeyAction action)
        {
            switch (action)
            {
                case MaaHotKeyAction.ShowGui:
                    HandleShowGui();
                    break;
                case MaaHotKeyAction.LinkStart:
                    HandleLinkStart();
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(action), action, null);
            }
        }

        protected virtual void HandleShowGui()
        {
            if (Application.Current.MainWindow is null)
            {
                return;
            }

            if (Application.Current.MainWindow.WindowState == WindowState.Minimized)
            {
                Application.Current.MainWindow.WindowState = WindowState.Normal;
            }
            else
            {
                Application.Current.MainWindow.WindowState = WindowState.Minimized;
            }
        }

        protected virtual void HandleLinkStart()
        {
            var mainModel = _container.Get<TaskQueueViewModel>();

            if (mainModel.Stopping)
            {
                return;
            }

            if (mainModel.Idle)
            {
                mainModel.LinkStart();

                if (Application.Current.MainWindow == null ||
                    Application.Current.MainWindow.WindowState != WindowState.Minimized)
                {
                    return;
                }

                using (var toast = new ToastNotification(Localization.GetString("BackgroundLinkStarted")))
                {
                    toast.Show();
                }
            }
            else
            {
                mainModel.Stop();

                if (Application.Current.MainWindow == null ||
                    Application.Current.MainWindow.WindowState != WindowState.Minimized)
                {
                    return;
                }

                using (var toast = new ToastNotification(Localization.GetString("BackgroundLinkStopped")))
                {
                    toast.Show();
                }
            }
        }
    }
}
