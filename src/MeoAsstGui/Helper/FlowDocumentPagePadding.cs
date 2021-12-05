// MeoAssistanceGui - A part of the MeoAssistance-Arknight project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;

namespace MeoAsstGui
{
    public static class FlowDocumentPagePadding
    {
        public static Thickness GetPagePadding(DependencyObject obj)
        {
            return (Thickness)obj.GetValue(PagePaddingProperty);
        }
        public static void SetPagePadding(DependencyObject obj, Thickness value)
        {
            obj.SetValue(PagePaddingProperty, value);
        }
        public static readonly DependencyProperty PagePaddingProperty =
            DependencyProperty.RegisterAttached("PagePadding", typeof(Thickness), typeof(FlowDocumentPagePadding), new UIPropertyMetadata(new Thickness(double.NegativeInfinity), (o, args) =>
            {
                var fd = o as FlowDocument;
                if (fd == null) return;
                var dpd = DependencyPropertyDescriptor.FromProperty(FlowDocument.PagePaddingProperty, typeof(FlowDocument));
                dpd.RemoveValueChanged(fd, PaddingChanged);
                fd.PagePadding = (Thickness)args.NewValue;
                dpd.AddValueChanged(fd, PaddingChanged);
            }));
        public static void PaddingChanged(object s, EventArgs e)
        {
            ((FlowDocument)s).PagePadding = GetPagePadding((DependencyObject)s);
        }
    }
}
