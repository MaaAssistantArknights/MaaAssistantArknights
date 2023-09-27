using System.Text.Json.Serialization;
using ObservableCollections;

namespace MaaWpfGui.Configuration
{
    public class SpecificConfig
    {
        [JsonInclude]
        public GUI GUI { get; private set; } = new GUI();

        [JsonInclude]
        public ObservableDictionary<string, int> InfrastOrder { get; private set; } = new ObservableDictionary<string, int>();

        [JsonInclude]
        public ObservableDictionary<string, int> TaskQueueOrder { get; private set; } = new ObservableDictionary<string, int>();

        [JsonInclude]
        public ObservableDictionary<string, bool> DragItemIsChecked { get; private set; } = new ObservableDictionary<string, bool>();
    }
}
