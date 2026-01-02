#nullable enable
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels
{
    /// <summary>
    /// Provides support for PropertyDependsOnAttribute: when a source property changes,
    /// dependent properties are automatically notified.
    /// </summary>
    public abstract class PropertyDependsOnViewModel : PropertyChangedBase
    {
        private readonly Dictionary<string, List<string>> _propertyDependencies = []; // key: source property name, value: dependent property names

        /// <summary>
        /// Scan for PropertyDependsOnAttribute on this instance's properties and wire notifications.
        /// Call this from the derived type's constructor.
        /// </summary>
        protected void InitializePropertyDependencies()
        {
            var type = GetType();
            var properties = type.GetProperties(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);

            foreach (var property in properties)
            {
                var dependsOnAttributes = property.GetCustomAttributes<PropertyDependsOnAttribute>(true);
                foreach (var attribute in dependsOnAttributes.Where(i => i.PropertyNames.Length > 0))
                {
                    foreach (var key in attribute.PropertyNames)
                    {
                        if (!_propertyDependencies.TryGetValue(key, out var value))
                        {
                            value = [];
                            _propertyDependencies[key] = value;
                        }

                        if (_propertyDependencies.TryGetValue(property.Name, out var values) && values.Contains(key))
                        {
                            throw new ArgumentException($"属性 {key} 依赖于属性 {property.Name}, 但它们之间存在循环依赖关系");
                        }

                        value.Add(property.Name);
                    }
                }
            }

            PropertyChanged += OnPropertyChanged;
        }

        private void OnPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (string.IsNullOrEmpty(e.PropertyName) || !_propertyDependencies.TryGetValue(e.PropertyName, out var value))
            {
                return;
            }

            foreach (var dependentProperty in value)
            {
                NotifyOfPropertyChange(dependentProperty);
            }
        }
    }
}
