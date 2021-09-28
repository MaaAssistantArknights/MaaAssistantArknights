using Stylet;
using StyletIoC;
using System.Threading.Tasks;

namespace MeoAsstGui
{
    public class RootViewModel : Conductor<Screen>.Collection.OneActive
    {
        private IContainer _container;
        private IWindowManager _windowManager;

        public RootViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
        }

        protected override void OnViewLoaded()
        {
            CheckAndUpdateNow();
            InitProxy();
            InitViewModels();
            ShowUpdateOrDownload();
        }

        private void InitProxy()
        {
            var p = _container.Get<AsstProxy>();
            p.Init();
        }

        private void InitViewModels()
        {
            var mfvm = _container.Get<MainFunctionViewModel>();
            var rvm = _container.Get<RecruitViewModel>();

            Items.Add(mfvm);
            Items.Add(rvm);
            ActiveItem = mfvm;
        }
        private bool CheckAndUpdateNow()
        {
            var vuvm = _container.Get<VersionUpdateViewModel>();
            return vuvm.CheckAndUpdateNow();
        }
        private async void ShowUpdateOrDownload()
        {
            var vuvm = _container.Get<VersionUpdateViewModel>();

            if (vuvm.IsFirstBootAfterUpdate)
            {
                _windowManager.ShowWindow(vuvm);
            }
            else
            {
                var task = Task.Run(() =>
                {
                    return vuvm.CheckAndDownloadUpdate();
                });

                await task;
            }
        }
    }
}