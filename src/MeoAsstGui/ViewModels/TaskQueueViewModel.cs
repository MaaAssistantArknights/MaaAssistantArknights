using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;
using Stylet;
using StyletIoC;

namespace MeoAsstGui
{
    public class TaskQueueViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public ObservableCollection<ItemViewModel> ItemViewModels { get; set; }

        public TaskQueueViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "一键长草";
            ItemViewModels = new ObservableCollection<ItemViewModel>();
            InitializeItems();
        }

        public void InitializeItems()
        {
            string stroageKey = "TaskQueue.";
            ItemViewModels.Add(new ItemViewModel("刷理智", stroageKey));
            ItemViewModels.Add(new ItemViewModel("基建换班", stroageKey));
            ItemViewModels.Add(new ItemViewModel("访问好友", stroageKey));
            ItemViewModels.Add(new ItemViewModel("领取日常奖励", stroageKey));
        }

        private string _statusPrompt = "Tips：上方任务可拖动调整顺序\nTips2：任务会按每个页面中的设置进行\nTips3：目前刷理智只会刷上次的图，所以建议放在最前面";

        public string StatusPrompt
        {
            get { return _statusPrompt; }
            set
            {
                SetAndNotify(ref _statusPrompt, value);
            }
        }

        public async void LinkStart()
        {
            StatusPrompt = "正在捕获模拟器窗口……";

            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatchDefault();
            });
            bool catchd = await task;
            if (!catchd)
            {
                StatusPrompt = "捕获模拟器窗口失败，若是第一次运行，请尝试使用管理员权限";
                return;
            }
            // 直接遍历ItemViewModels里面的内容，是排序后的
            bool ret = true;
            foreach (var item in ItemViewModels)
            {
                if (item.IsChecked == false)
                {
                    continue;
                }
                if (item.Name == "基建换班")
                {
                    var ifvm = _container.Get<InfrastructureConstructionViewModel>();
                    ret &= ifvm.Start();
                } 
                else if (item.Name == "刷理智")
                {
                    var mfvm = _container.Get<MainFunctionViewModel>();
                    ret &= mfvm.StartSanity();
                }
                else if (item.Name == "访问好友")
                {
                    var mfvm = _container.Get<MainFunctionViewModel>();
                    ret &= mfvm.StartVisit();
                }
                else if (item.Name == "领取日常奖励")
                {
                    var mfvm = _container.Get<MainFunctionViewModel>();
                    ret &= mfvm.StartAward();
                }
            }

            ret &= asstProxy.AsstStart();

            if (!ret)
            {
                StatusPrompt = "出现未知错误";
            }
        }

        public void Stop()
        {
            var asstProxy = _container.Get<AsstProxy>();
            asstProxy.AsstStop();
            StatusPrompt = "已停止";
        }
    }
}
