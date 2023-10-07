using System.ComponentModel;

namespace MaaWpfGui.Configuration
{
    // https://github.com/dotnet/runtime/issues/27252
    public class PropertyChangedEventDetailArgs : PropertyChangedEventArgs
    {
        public PropertyChangedEventDetailArgs(string propertyName, object oldValue, object newValue)
            : base(propertyName)
        {
            OldValue = oldValue;
            NewValue = newValue;
        }

        // ReSharper disable once UnusedAutoPropertyAccessor.Global
        public object OldValue { get; private set; }

        public object NewValue { get; private set; }
    }
}
