using Stylet;
using StyletIoC;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace MeoAsstGui
{
    public class InfrastructureConstructionViewModel : Screen
    {
        private IWindowManager _windowManager;
        private IContainer _container;

        public ObservableCollection<ItemViewModel> ItemViewModels { get; set; }

        public string MyName { get; set; }

        public InfrastructureConstructionViewModel(IContainer container, IWindowManager windowManager)
        {
            _container = container;
            _windowManager = windowManager;
            DisplayName = "基建换班";
            ItemViewModels = new ObservableCollection<ItemViewModel>();
            InitializeItems();
        }

        public void InitializeItems()
        {
            ItemViewModels.Add(new ItemViewModel(1, "宿舍"));
            ItemViewModels.Add(new ItemViewModel(2, "制造站"));
            ItemViewModels.Add(new ItemViewModel(3, "贸易站"));
            ItemViewModels.Add(new ItemViewModel(4, "发电站"));
            ItemViewModels.Add(new ItemViewModel(5, "办公室"));
            ItemViewModels.Add(new ItemViewModel(6, "宿舍"));
            //ItemViewModels.Add(new ItemViewModel(6, "会客室"));
            //ItemViewModels.Add(new ItemViewModel(7, "控制中枢"));
        }

        public async void ChangeShift()
        {
            //TODO 直接遍历ItemViewModels里面的内容，是排序后的
            var asstProxy = _container.Get<AsstProxy>();
            var task = Task.Run(() =>
            {
                return asstProxy.AsstCatchDefault();
            });
            bool catchd = await task;
            if (!catchd)
            {
                return;
            }
            asstProxy.AsstStartInfrastShift();
        }
    }

    public class ItemViewModel : PropertyChangedBase
    {
        public ItemViewModel(int id, string name)
        {
            this.ID = id;
            this.Name = name;
        }

        private string _name;

        public string Name
        {
            get { return _name; }
            set
            {
                SetAndNotify(ref _name, value);
            }
        }

        private int _id;

        public int ID
        {
            get { return _id; }
            set
            {
                SetAndNotify(ref _id, value);
            }
        }

        // 换成图标的话要这个，暂时没用
        private string _iconPath;

        public string IconPath
        {
            get { return _name; }
            set
            {
                SetAndNotify(ref _iconPath, value);
            }
        }

        private string _token;

        public string Token
        {
            get { return _token; }
            set
            {
                SetAndNotify(ref _token, value);
            }
        }

        private string _runStatus;

        public string RunStatus
        {
            get { return _runStatus; }
            set
            {
                SetAndNotify(ref _runStatus, value);
            }
        }
    }
}