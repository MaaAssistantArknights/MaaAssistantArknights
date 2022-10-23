using System.Windows;

namespace MeoAsstGui
{
    /// <summary>
    /// Manager of the MAA main window
    /// </summary>
    public interface IMainWindowManager
    {
        /// <summary>
        /// Show the main window
        /// </summary>
        void Show();

        /// <summary>
        /// Collapse the main window
        /// </summary>
        void Collapse();

        /// <summary>
        /// Show the main window if it collapsed and vice versa.
        /// </summary>
        void SwitchWindowState();

        /// <summary>
        /// Get the current window state of the main window
        /// </summary>
        /// <returns>WindowState</returns>
        WindowState GetWindowState();

        /// <summary>
        /// Sets whether to minimize to taskbar.
        /// </summary>
        /// <param name="shouldMinimizeToTaskbar">Whether to minimize to taskbar.</param>
        void SetMinimizeToTaskbar(bool shouldMinimizeToTaskbar);
    }
}
