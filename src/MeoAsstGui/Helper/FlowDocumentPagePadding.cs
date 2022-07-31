// <copyright file="FlowDocumentPagePadding.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Documents;

namespace MeoAsstGui
{
    /// <summary>
    /// The flow document page padding property.
    /// </summary>
    public static class FlowDocumentPagePadding
    {
        /// <summary>
        /// Gets flow document page padding property.
        /// </summary>
        /// <param name="obj">The <see cref="DependencyObject"/> instance.</param>
        /// <returns>The property value.</returns>
        public static Thickness GetPagePadding(DependencyObject obj)
        {
            return (Thickness)obj.GetValue(PagePaddingProperty);
        }

        /// <summary>
        /// Sets flow document page padding property.
        /// </summary>
        /// <param name="obj">The <see cref="DependencyObject"/> instance.</param>
        /// <param name="value">The new property value.</param>
        public static void SetPagePadding(DependencyObject obj, Thickness value)
        {
            obj.SetValue(PagePaddingProperty, value);
        }

        /// <summary>
        /// The flow document page padding property.
        /// </summary>
        public static readonly DependencyProperty PagePaddingProperty =
            DependencyProperty.RegisterAttached("PagePadding", typeof(Thickness), typeof(FlowDocumentPagePadding), new UIPropertyMetadata(new Thickness(double.NegativeInfinity), (o, args) =>
            {
                var fd = o as FlowDocument;
                if (fd == null)
                {
                    return;
                }

                var dpd = DependencyPropertyDescriptor.FromProperty(FlowDocument.PagePaddingProperty, typeof(FlowDocument));
                dpd.RemoveValueChanged(fd, PaddingChanged);
                fd.PagePadding = (Thickness)args.NewValue;
                dpd.AddValueChanged(fd, PaddingChanged);
            }));

        /// <summary>
        /// The event handler of the event when the padding is changed.
        /// </summary>
        /// <param name="s">The sender.</param>
        /// <param name="e">The event arguments.</param>
        public static void PaddingChanged(object s, EventArgs e)
        {
            ((FlowDocument)s).PagePadding = GetPagePadding((DependencyObject)s);
        }
    }
}
