using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MaaWpfGui.Configuration
{
    public class GUIConfig : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public DarkModeType DarkMode { get; set; } = DarkModeType.SyncWithOS;

        public bool UseNotify { get; set; } = true;

        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }

    /// <summary>
    /// 表示深色模式的类型。
    /// </summary>
    public enum DarkModeType
    {
        /// <summary>
        /// 明亮的主题。
        /// </summary>
        Light,

        /// <summary>
        /// 暗黑的主题。
        /// </summary>
        Dark,

        /// <summary>
        /// 与操作系统的深色模式同步。
        /// </summary>
        SyncWithOS
    }
}
