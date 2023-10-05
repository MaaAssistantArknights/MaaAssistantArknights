using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Text.Json.Serialization;
using ObservableCollections;

namespace MaaWpfGui.Configuration
{
    public class Root : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public ObservableDictionary<string, SpecificConfig> Configurations { get; set; } = new ObservableDictionary<string, SpecificConfig>();

        public string Current { get; set; } = "Default";

        [JsonIgnore]
        public SpecificConfig CurrentConfig
        {
            get
            {
                SpecificConfig result = null;
                Configurations.TryGetValue(Current, out result);
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
