using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace MaaWpfGui
{
    /// <summary>
    /// CreateNewProfileDialog.xaml 的交互逻辑
    /// </summary>
    public partial class CreateNewProfileDialog : Window
    {
        public CreateNewProfileDialog(List<CombData> configList)
        {
            InitializeComponent();
            ConfigList.ItemsSource = configList;
            ConfigList.SelectedIndex = 0;
            ProfileName.Text = Localization.GetString("DefaultProfileName");
        }

        private void btnDialogOk_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void Window_ContentRendered(object sender, EventArgs e)
        {
            ProfileName.SelectAll();
            ProfileName.Focus();
        }

        public string NewProfileName
        {
            get => ProfileName.Text;
        }

        public string InherbitProfileName
        {
            get => ConfigList.SelectedValue.ToString();
        }
    }
}
