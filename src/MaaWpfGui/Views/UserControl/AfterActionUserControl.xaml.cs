using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Views.UserControl;

/// <summary>
/// AfterActionUserControl.xaml 的交互逻辑
/// </summary>
public partial class AfterActionUserControl : System.Windows.Controls.UserControl
{
    private AfterActionSetting Model => (DataContext as SettingsViewModel)?.AfterActionSetting;

    public AfterActionUserControl()
    {
        InitializeComponent();
        InitializeCheckBox();
    }

    private void InitializeCheckBox()
    {
        Dispatcher.Invoke(async () =>
        {
            await Task.Delay(500);
            while (Model == null)
            {
                await Task.Delay(100);
            }

            InitCheckBox();
        });
        return;

        void InitCheckBox()
        {
            if (Model == null)
            {
                return;
            }

            if (Model.ExitEmulator)
            {
                CheckBoxExitArknights.IsChecked = false;
                CheckBoxBackToAndroidHome.IsChecked = false;
                CheckBoxExitArknights.Visibility = Visibility.Collapsed;
                CheckBoxBackToAndroidHome.Visibility = Visibility.Collapsed;
            }

            if (Model.Hibernate)
            {
                CheckBoxShutdown.IsChecked = false;
                CheckBoxShutdown.Visibility = Visibility.Collapsed;
            }

            if (Model.Shutdown)
            {
                CheckBoxExitArknights.IsChecked = false;
                CheckBoxBackToAndroidHome.IsChecked = false;
                CheckBoxExitEmulator.IsChecked = false;
                CheckBoxExitSelf.IsChecked = false;
                CheckBoxHibernate.IsChecked = false;

                CheckBoxHibernate.Visibility = Visibility.Collapsed;
                CheckBoxExitArknights.Visibility = Visibility.Collapsed;
                CheckBoxBackToAndroidHome.Visibility = Visibility.Collapsed;
                CheckBoxExitEmulator.Visibility = Visibility.Collapsed;
                CheckBoxExitSelf.Visibility = Visibility.Collapsed;
            }

            if (Model.Hibernate || Model.Shutdown)
            {
                CheckBoxIfNoOtherMaa.Visibility = Visibility.Visible;
            }
            else
            {
                CheckBoxIfNoOtherMaa.IsChecked = false;
                CheckBoxIfNoOtherMaa.Visibility = Visibility.Collapsed;
            }
        }
    }

    private void CheckBox_OnClick(object sender, RoutedEventArgs e)
    {
        if (Model == null)
        {
            return;
        }

        if (sender is not CheckBox checkBox)
        {
            return;
        }

        switch (checkBox.Name)
        {
            case "CheckBoxExitEmulator":
                if (Model.ExitEmulator)
                {
                    CheckBoxExitArknights.IsChecked = false;
                    CheckBoxBackToAndroidHome.IsChecked = false;
                    CheckBoxExitArknights.Visibility = Visibility.Collapsed;
                    CheckBoxBackToAndroidHome.Visibility = Visibility.Collapsed;
                }
                else
                {
                    CheckBoxExitArknights.Visibility = Visibility.Visible;
                    CheckBoxBackToAndroidHome.Visibility = Visibility.Visible;
                }

                break;
            case "CheckBoxHibernate":
                if (Model.Hibernate)
                {
                    CheckBoxShutdown.IsChecked = false;
                    CheckBoxShutdown.Visibility = Visibility.Collapsed;
                }
                else
                {
                    CheckBoxShutdown.Visibility = Visibility.Visible;
                }

                break;
            case "CheckBoxShutdown":
                if (Model.Shutdown)
                {
                    CheckBoxExitArknights.IsChecked = false;
                    CheckBoxBackToAndroidHome.IsChecked = false;
                    CheckBoxExitEmulator.IsChecked = false;
                    CheckBoxExitSelf.IsChecked = false;
                    CheckBoxHibernate.IsChecked = false;

                    CheckBoxHibernate.Visibility = Visibility.Collapsed;
                    CheckBoxExitArknights.Visibility = Visibility.Collapsed;
                    CheckBoxBackToAndroidHome.Visibility = Visibility.Collapsed;
                    CheckBoxExitEmulator.Visibility = Visibility.Collapsed;
                    CheckBoxExitSelf.Visibility = Visibility.Collapsed;
                }
                else
                {
                    CheckBoxHibernate.Visibility = Visibility.Visible;
                    CheckBoxExitArknights.Visibility = Visibility.Visible;
                    CheckBoxBackToAndroidHome.Visibility = Visibility.Visible;
                    CheckBoxExitEmulator.Visibility = Visibility.Visible;
                    CheckBoxExitSelf.Visibility = Visibility.Visible;
                }

                break;
        }

        if (Model.Hibernate || Model.Shutdown)
        {
            CheckBoxIfNoOtherMaa.Visibility = Visibility.Visible;
        }
        else
        {
            CheckBoxIfNoOtherMaa.IsChecked = false;
            CheckBoxIfNoOtherMaa.Visibility = Visibility.Collapsed;
        }
    }
}
