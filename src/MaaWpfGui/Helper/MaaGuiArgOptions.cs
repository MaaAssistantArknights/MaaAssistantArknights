using CommandLine;

namespace MaaWpfGui
{
    public class MaaGuiArgOptions
    {
        [Option('f', "config-file", Required = false, HelpText = "Set config file to override gui.json.")]
        public string ConfigFilePath { get; set; }
    }
}
