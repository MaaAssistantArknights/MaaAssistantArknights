using System.ComponentModel;
using System.Text.Json.Serialization;
using ObservableCollections;

namespace MaaWpfGui.Configuration
{
    public class Root : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        [JsonInclude]
        public ObservableDictionary<string, SpecificConfig> Configurations { get; private set; } = new ObservableDictionary<string, SpecificConfig>();

        [JsonInclude]
        public ObservableDictionary<int, Timer> Timers { get; private set; } = new ObservableDictionary<int, Timer>();

        public string Current { get; set; } = "Default";

        [JsonIgnore]
        public SpecificConfig CurrentConfig
        {
            get
            {
                Configurations.TryGetValue(Current, out var result);
                return result;
            }

            set => Configurations[Current] = value;
        }

        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }
}
