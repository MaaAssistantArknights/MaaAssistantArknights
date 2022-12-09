using CommandLine;

namespace MaaWpfGui
{
    public class MaaGuiArgOptions
    {
        [Option('f', "profile", Required = false, HelpText = "Set config file to override gui.json.")]
        public string ProfilePath { get; set; }
    }
}
