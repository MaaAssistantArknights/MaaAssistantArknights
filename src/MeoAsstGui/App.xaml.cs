using Microsoft.Toolkit.Uwp.Notifications;
using System.Windows;
using Windows.ApplicationModel.Activation;
using Windows.UI.Notifications;

namespace MeoAsstGui
{
    /// <summary>
    /// App.xaml 的交互逻辑
    /// </summary>
    public partial class App : Application
    {
        //protected override async void OnBackgroundActivated(BackgroundActivatedEventArgs args)
        //{
        //    var deferral = args.TaskInstance.GetDeferral();

        //    switch (args.TaskInstance.Task.Name)
        //    {
        //        case "ToastBackgroundTask":
        //            var details = args.TaskInstance.TriggerDetails as ToastNotificationActionTriggerDetail;
        //            if (details != null)
        //            {
        //                ToastArguments arguments = ToastArguments.Parse(details.Argument);
        //                var userInput = details.UserInput;

        //                // Perform tasks
        //            }
        //            break;
        //    }

        //    deferral.Complete();
        //}
    }
}