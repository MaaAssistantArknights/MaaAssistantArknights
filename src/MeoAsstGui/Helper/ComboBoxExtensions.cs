using System.Windows.Controls;
using System.Windows.Input;

namespace MeoAsstGui.Helper
{
    /// <summary>
    /// <seealso cref="ComboBox"/> Extensions
    /// </summary>
    public static class ComboBoxExtensions
    {
        private const string InputTag = "TextInput";
        private const string SelectionTag = "Selection";

        /// <summary>
        /// Make <seealso cref="ComboBox"/> searchable
        /// </summary>
        /// <param name="targetComboBox">Target <seealso cref="ComboBox"/></param>
        public static void MakeComboBoxSearchable(this ComboBox targetComboBox)
        {
            var targetTextBox = targetComboBox?.Template.FindName("PART_EditableTextBox", targetComboBox) as TextBox;
            if (targetTextBox == null)
            {
                return;
            }

            targetComboBox.Items.IsLiveFiltering = true;
            targetComboBox.StaysOpenOnEdit = true;
            targetComboBox.IsEditable = true;
            targetComboBox.IsTextSearchEnabled = false;

            // enter input mode by default
            targetComboBox.Tag = InputTag;

            targetTextBox.PreviewKeyDown += (se, ev) =>
            {
                if (ev.Key == Key.Enter || ev.Key == Key.Return || ev.Key == Key.Tab)
                {
                    return;
                }

                // switch to input mode
                targetComboBox.Tag = InputTag;

                // reset selection
                if (targetComboBox.SelectedItem != null)
                {
                    targetComboBox.SelectedItem = null;
                    targetComboBox.Text = string.Empty;
                }
            };

            targetTextBox.TextChanged += (o, args) =>
            {
                if (targetComboBox.Tag is SelectionTag)
                {
                    // text changed in selection mode, switch to input mode
                    targetComboBox.Tag = InputTag;
                }
                else
                {
                    // input mode
                    var searchTerm = targetTextBox.Text;

                    // reset selection
                    if (targetComboBox.SelectionBoxItem != null)
                    {
                        targetComboBox.SelectedItem = null;
                        targetTextBox.Text = searchTerm;
                        targetTextBox.SelectionStart = targetTextBox.Text.Length;
                    }

                    // filter items
                    if (string.IsNullOrEmpty(searchTerm))
                    {
                        targetComboBox.Items.Filter = item => true;
                        targetComboBox.SelectedItem = default;
                    }
                    else
                    {
                        targetComboBox.Items.Filter = item => item.ToString().Contains(searchTerm);
                    }

                    targetTextBox.SelectionStart = targetTextBox.Text.Length;
                }

                targetComboBox.IsDropDownOpen = true;
            };

            targetComboBox.SelectionChanged += (o, args) =>
            {
                // selection changed, switch to selection mode
                if (targetComboBox.SelectedItem != null)
                {
                    targetComboBox.Tag = SelectionTag;
                }
            };

            targetComboBox.LostFocus += (o, args) =>
            {
                // reset text to the corresponding display text of selected item
                var text = targetComboBox.SelectedItem?.ToString();
                if (targetComboBox.Text != text)
                {
                    targetComboBox.Tag = SelectionTag;
                    targetComboBox.Text = text;
                    targetComboBox.Items.Filter = item => true;
                }
            };
        }
    }
}
