using System.Security.RightsManagement;
using MaaWpfGui.Helper;
using Stylet;
using Vanara.PInvoke;

namespace MaaWpfGui.ViewModels
{
    public class CopilotItemViewModel : PropertyChangedBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CopilotItemViewModel"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="filePath">The original Name of file</param>
        /// <param name="isChecked">isChecked</param>
        public CopilotItemViewModel(string name, string filePath, bool isChecked = true)
        {
            _name = name;
            _filePath = filePath;
            _isChecked = isChecked;
        }

        private string _filePath;

        /// <summary>
        /// Gets or sets the original_name.
        /// </summary>
        public string FilePath
        {
            get => _filePath;
            set => SetAndNotify(ref _filePath, value);
        }

        private string _name;

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name
        {
            get => _name;
            set => SetAndNotify(ref _name, value);
        }

        private bool _isChecked;

        /// <summary>
        /// Gets or sets a value indicating whether gets or sets whether the key is checked.
        /// </summary>
        public bool IsChecked
        {
            get => _isChecked;
            set
            {
                SetAndNotify(ref _isChecked, value);
                Instances.CopilotViewModel.SaveCopilotTask();
            }
        }

        // 换成图标的话要这个，暂时没用
        private string _iconPath;

        /// <summary>
        /// Gets or sets the icon path.
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public string IconPath
        {
            get => _iconPath;
            set => SetAndNotify(ref _iconPath, value);
        }

        private string _token;

        /// <summary>
        /// Gets or sets the token.
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public string Token
        {
            get => _token;
            set => SetAndNotify(ref _token, value);
        }

        private string _runStatus;

        /// <summary>
        /// Gets or sets the running status.
        /// </summary>
        // ReSharper disable once UnusedMember.Global
        public string RunStatus
        {
            get => _runStatus;
            set => SetAndNotify(ref _runStatus, value);
        }

        private int _index = 0;

        public int Index
        {
            get => _index;
            set => SetAndNotify(ref _index, value);
        }

        public int GetIndex() => Index;
    }
}
