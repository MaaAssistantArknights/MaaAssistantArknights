using System;
using System.Windows;

namespace MeoAsstGui
{
    /// <inheritdoc/>
    public class MainWindowManager : IMainWindowManager
    {
        /// <summary>
        /// Gets the main window object
        /// </summary>
        protected static Window MainWindow => Application.Current.MainWindow;

        /// <summary>
        /// Gets or sets a value indicating whether whether minimize to tray.
        /// </summary>
        protected bool ShouldMinimizeToTaskbar { get; set; }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainWindowManager"/> class.
        /// </summary>
        public MainWindowManager()
        {
            MainWindow.StateChanged += MainWindow_StateChanged;
        }

        /// <inheritdoc/>
        public virtual void Show()
        {
            MainWindow.Show();
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Normal;
            MainWindow.Activate();
        }

        /// <inheritdoc/>
        public virtual void Collapse()
        {
            MainWindow.WindowState = MainWindow.WindowState = WindowState.Minimized;
        }

        /// <inheritdoc/>
        public virtual void SwitchWindowState()
        {
            if (MainWindow.WindowState == WindowState.Minimized)
            {
                Show();
            }
            else
            {
                Collapse();
            }
        }

        /// <inheritdoc/>
        public virtual WindowState GetWindowState() => MainWindow.WindowState;

        /// <inheritdoc/>
        public virtual void SetMinimizeToTaskbar(bool shouldMinimizeToTaskbar)
        {
            ShouldMinimizeToTaskbar = shouldMinimizeToTaskbar;
        }

        /// <summary>
        /// Handle the main window's state changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected virtual void MainWindow_StateChanged(object sender, EventArgs e)
        {
            if (ShouldMinimizeToTaskbar)
            {
                ChangeVisibility(MainWindow.WindowState != WindowState.Minimized);
            }
        }

        /// <summary>
        /// Change visibility of the main window
        /// </summary>
        /// <param name="visible"></param>
        protected virtual void ChangeVisibility(bool visible)
        {
            if (visible)
            {
                MainWindow.ShowInTaskbar = true;
                MainWindow.Visibility = Visibility.Visible;
            }
            else
            {
                MainWindow.ShowInTaskbar = false;
                MainWindow.Visibility = Visibility.Hidden;
            }
        }
    }
}
