using MaaWpfGui.Helper;
using Stylet;

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
            Name = name;
            FilePath = filePath;
            _isChecked = isChecked;
        }

        /// <summary>
        /// Gets the original_name.
        /// </summary>
        public string FilePath { get; }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string Name { get; }

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

        private int _index;

        public int Index
        {
            get => _index;
            set => SetAndNotify(ref _index, value);
        }
    }
}
