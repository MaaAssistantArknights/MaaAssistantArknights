using System;
using Serilog;

namespace MaaBuilder;

public partial class Build
{
    readonly Action<string> Information = msg =>
    {
        Log.Information("{Message}", msg);
    };

    readonly Action<string> Warning = msg =>
    {
        Log.Warning("{Message}", msg);
    };    
}
