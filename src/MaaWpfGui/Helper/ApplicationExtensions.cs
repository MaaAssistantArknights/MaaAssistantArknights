using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace MaaWpfGui.Helper
{
    internal static class ApplicationExtensions
    {
        public static bool IsShuttingDown(this Application application)
        {
            try
            {
                application.ShutdownMode = application.ShutdownMode;
                return false;
            }
            catch
            {
                return true;
            }
        }
    }
}
