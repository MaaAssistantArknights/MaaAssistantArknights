using Stylet;
using StyletIoC;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
            InitProxy();
            CheckUpdate();
            InitViewModels();
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

        private async void CheckUpdate()
        {
            var vuvm = _container.Get<VersionUpdateViewModel>();

            var task = Task.Run(() =>
            {
                return vuvm.CheckUpdate();
            });

            var needUpdate = await task;
            if (needUpdate)
            {
                _windowManager.ShowWindow(vuvm);
            }
        }
    }
}
