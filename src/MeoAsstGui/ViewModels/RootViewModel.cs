// MeoAssistanceGui - A part of the MeoAssistance-Arknight project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System.Threading.Tasks;
using Stylet;
using StyletIoC;

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
            // 有个界面一启动就崩溃的问题，暂时找不到原因，而且日志也没生成
            // 因为下一行就有日志生成了，所以可能是这里崩的
            // 反正先try起来看看有没有用_(:з」∠)_
            try
            {
                CheckAndUpdateNow();
            }
            catch (System.Exception e)
            {
                System.Console.WriteLine("CheckAndUpdateNow Exception caught: {0}", e);
            }
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
            var ivm = _container.Get<InfrastructureConstructionViewModel>();

            Items.Add(mfvm);
            Items.Add(rvm);
            Items.Add(ivm);
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
