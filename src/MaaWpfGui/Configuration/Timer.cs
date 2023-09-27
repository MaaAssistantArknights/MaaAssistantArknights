using System.Text.Json.Serialization;

namespace MaaWpfGui.Configuration
{
    public class Timer
    {
        [JsonConstructor]
        public Timer(bool enable, string config, int hour, int minute)
        {
            Enable = enable;
            Config = config;
            Hour = hour;
            Minute = minute;
        }

        [JsonInclude]
        public bool Enable { get; }

        [JsonInclude]
        public string Config { get; }

        [JsonInclude]
        public int Hour { get; }

        [JsonInclude]
        public int Minute { get; }
    }
}
