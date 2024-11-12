using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Win32.Foundation;

namespace MaaWpfGui.Extensions
{
    internal static class Win32Extension
    {
        public static long AsLong(this LUID luid)
        {
            return ((long)luid.HighPart << 32) | luid.LowPart;
        }
    }
}
