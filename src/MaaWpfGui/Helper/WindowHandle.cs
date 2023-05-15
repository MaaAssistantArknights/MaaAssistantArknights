using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media.Media3D;

namespace MaaWpfGui.Helper
{
    public struct WindowHandle
    {
        private IntPtr _handle;

        public IntPtr Handle => _handle;

        public static WindowHandle None => new WindowHandle() { _handle = IntPtr.Zero };

        public static implicit operator WindowHandle(IntPtr h) => new WindowHandle() { _handle = h };

        public static implicit operator WindowHandle(Window w)
        {
            if (w == null)
            {
                return None;
            }

            var interop = new WindowInteropHelper(w);
            return new WindowHandle { _handle = interop.Handle };
        }
    }
}
