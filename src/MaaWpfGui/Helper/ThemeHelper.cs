using System;
using System.Windows;
using HandyControl.Data;
using HandyControl.Themes;
using HandyControl.Tools;

namespace MaaWpfGui.Helper
{
    /// <summary>
    /// The theme class.
    /// </summary>
    public class ThemeHelper : Theme
    {
        public override ResourceDictionary GetSkin(SkinType skinType) =>
            ResourceHelper.GetSkin(typeof(App).Assembly, "Res/Themes", skinType);

        public override ResourceDictionary GetTheme() => new ResourceDictionary()
        {
            Source = new Uri("pack://application:,,,/HandyControl;component/Themes/Theme.xaml"),
        };
    }
}
