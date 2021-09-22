using Stylet;
using StyletIoC;
using System.Windows;
using System.Windows.Threading;

namespace MeoAsstGui
{
    public class Bootstrapper : Bootstrapper<RootViewModel>
    {
        // 初始化些啥自己加
        protected override void OnStart()
        {
            ViewStatusStorage.Load();
        }

        protected override void ConfigureIoC(IStyletIoCBuilder builder)
        {
            builder.Bind<MainFunctionViewModel>().ToSelf().InSingletonScope();
            builder.Bind<RecruitViewModel>().ToSelf().InSingletonScope();
            builder.Bind<AsstProxy>().ToSelf().InSingletonScope();
        }

        // 退出时执行啥自己加
        protected override void OnExit(ExitEventArgs e)
        {
            // MessageBox.Show("O(∩_∩)O 拜拜");
            ViewStatusStorage.Save();
        }

        protected override void OnUnhandledException(DispatcherUnhandledExceptionEventArgs e)
        {
            // 抛异常了，可以打些日志
        }
    }
}