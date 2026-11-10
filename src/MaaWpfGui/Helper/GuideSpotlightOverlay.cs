// <copyright file="GuideSpotlightOverlay.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Media;

namespace MaaWpfGui.Helper;

/// <summary>
/// 设置指引的全局遮罩层：压暗内容区，在当前目标控件处挖孔，并在旁边显示文字提示。
/// </summary>
public sealed class GuideSpotlightOverlay : FrameworkElement
{
    // 遮罩压暗：深色主题使用纯净半透明黑（约 55% 暗），消除发白起雾感；浅色主题使用轻柔暗化（约 30% 黑）
    private static readonly Brush DimBrushDark = CreateBrush(0x8C, 0x00, 0x00, 0x00);
    private static readonly Brush DimBrushLight = CreateBrush(0x4D, 0x00, 0x00, 0x00);

    // 挖孔高亮边框：内圈亮橙 + 外圈柔和扩散光晕
    private static readonly Brush HoleBorder = CreateBrush(0xFF, 0xF3, 0x63, 0x1B);
    private static readonly Pen HolePen = CreatePen(HoleBorder, 1.5);
    private static readonly Pen HoleGlowPen = CreatePen(CreateBrush(0x30, 0xF3, 0x63, 0x1B), 5);

    // 气泡文字：深色模式纯净高对比白，浅色模式深炭灰，阅读极度舒适清晰
    private static readonly Brush CalloutTextDark = CreateBrush(0xFF, 0xFA, 0xFA, 0xFC);
    private static readonly Brush CalloutTextLight = CreateBrush(0xFF, 0x1C, 0x1C, 0x1E);

    // 气泡底板与阴影：深色为精致悬浮暗卡片（#1C2028），浅色为纯净白底，四周带 1px 优雅描边与轻量柔和阴影
    private static readonly Brush CalloutBackdropDark = CreateBrush(0xF8, 0x1C, 0x20, 0x28);
    private static readonly Brush CalloutBackdropLight = CreateBrush(0xFA, 0xFF, 0xFF, 0xFF);
    private static readonly Brush CalloutShadowDark = CreateBrush(0x55, 0x00, 0x00, 0x00);
    private static readonly Brush CalloutShadowLight = CreateBrush(0x20, 0x00, 0x00, 0x00);

    private static readonly Pen CalloutBorderDark = CreatePen(CreateBrush(0x60, 0xF3, 0x63, 0x1B), 1);
    private static readonly Pen CalloutBorderLight = CreatePen(CreateBrush(0x60, 0xF3, 0x63, 0x1B), 1);

    /// <summary>
    /// 提示文字字体：跟随系统 UI 字体，避免硬编码单一字体在日 / 韩界面下缺字形。
    /// </summary>
    private readonly Typeface _bubbleTypeface;

    /// <summary>
    /// 参与引导的高亮目标注册表（按注册顺序，后注册者优先）。
    /// 只存目标本身：文字 / 位置 / 呈现方式 / 是否激活全部在渲染时从 <see cref="GuideHighlight"/> 的附加属性实时读取，
    /// 遮罩不再缓存任何副本，避免遮罩与目标各自维护一套状态而互相脱节。
    /// </summary>
    private readonly List<FrameworkElement> _targets = [];

    private bool _invalidateQueued;

    /// <summary>
    /// Initializes a new instance of the <see cref="GuideSpotlightOverlay"/> class.
    /// </summary>
    public GuideSpotlightOverlay()
    {
        _bubbleTypeface = new Typeface(
            SystemFonts.MessageFontFamily,
            FontStyles.Normal,
            FontWeights.Medium,
            FontStretches.Normal);
        IsHitTestVisible = false;
        Focusable = false;
        ClipToBounds = false;
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;
        LayoutUpdated += OnLayoutUpdated;
    }

    /// <summary>
    /// Gets 当前加载的遮罩实例（同一时间只有一个引导界面）。
    /// </summary>
    public static GuideSpotlightOverlay? Current { get; private set; }

    /// <summary>
    /// 把一个声明了引导附加属性的目标纳入遮罩。
    /// </summary>
    /// <param name="target">目标控件。</param>
    internal void RegisterTarget(FrameworkElement target)
    {
        if (!_targets.Contains(target))
        {
            _targets.Add(target);
        }

        InvalidateVisual();
    }

    /// <summary>
    /// 把目标移出遮罩。
    /// </summary>
    /// <param name="target">目标控件。</param>
    internal void UnregisterTarget(FrameworkElement target)
    {
        if (_targets.Remove(target))
        {
            InvalidateVisual();
        }
    }

    /// <summary>
    /// 目标状态变化后重绘。文字、位置、呈现方式、显隐、布局任一改变都走这里，
    /// 遮罩按需重读附加属性，因此不需要在两侧同步副本。
    /// </summary>
    internal void Refresh()
    {
        InvalidateVisual();
    }

    /// <inheritdoc />
    protected override void OnRender(DrawingContext drawingContext)
    {
        base.OnRender(drawingContext);
        if (_targets.Count == 0 || ActualWidth <= 0 || ActualHeight <= 0)
        {
            return;
        }

        var bounds = new Rect(0, 0, ActualWidth, ActualHeight);
        if (!TryResolveTarget(bounds, out var target, out Rect hole))
        {
            return;
        }

        if (GuideHighlight.GetMode(target) == GuideHighlight.GuideHighlightMode.Frame)
        {
            drawingContext.DrawRoundedRectangle(null, HoleGlowPen, hole, 6, 6);
            drawingContext.DrawRoundedRectangle(null, HolePen, hole, 6, 6);
            return;
        }

        // 底部采用与外框严格同弧度的圆角（4px），顶部平角贴合分割线硬过渡
        var mask = new GeometryGroup { FillRule = FillRule.EvenOdd };
        mask.Children.Add(CreateContentBoundsGeometry(bounds, 4.0));
        mask.Children.Add(new RectangleGeometry(hole, 8, 8));
        mask.Freeze();

        bool isDark = ThemeHelper.IsDarkMode();
        Brush dimBrush = isDark ? DimBrushDark : DimBrushLight;

        // 硬过渡：采用纯色压暗画刷，配合顶部 1px 分割线形成干净利落的硬质分界，不再使用模糊渐变
        drawingContext.DrawGeometry(dimBrush, null, mask);
        drawingContext.DrawRoundedRectangle(null, HoleGlowPen, hole, 8, 8);
        drawingContext.DrawRoundedRectangle(null, HolePen, hole, 8, 8);

        string text = GuideHighlight.GetText(target);
        if (text.Length > 0)
        {
            DrawCallout(drawingContext, hole, bounds, text, GuideHighlight.GetPlacement(target));
        }
    }

    /// <summary>
    /// 选出当前应高亮的目标并算出挖孔：取最后一个仍激活、可见且能定位的目标。
    /// 例如 ｢启用周计划｣ 步骤中高级页未展开时，周计划复选框不可见，遮罩应落在可见的 ｢高级设置｣ 页签上。
    /// </summary>
    /// <param name="bounds">遮罩自身范围。</param>
    /// <param name="target">命中的目标控件。</param>
    /// <param name="hole">目标在遮罩坐标系中的挖孔矩形。</param>
    /// <returns>是否命中可渲染的目标。</returns>
    private bool TryResolveTarget(Rect bounds, out FrameworkElement target, out Rect hole)
    {
        // 只接受与遮罩同属一棵视觉树 / 同一个顶层窗口的目标。Popup 内容（如 ｢添加任务｣ 菜单项）
        // 在独立顶层窗口里，本遮罩既压不暗也挖不到孔，显式跳过；菜单项自身已有 ｢点击这里｣ 标签引导。
        PresentationSource? source = PresentationSource.FromVisual(this);

        for (int i = _targets.Count - 1; i >= 0; i--)
        {
            var candidate = _targets[i];
            if (!candidate.IsLoaded || !candidate.IsVisible || !GuideHighlight.GetIsActive(candidate))
            {
                continue;
            }

            if (source is null || PresentationSource.FromVisual(candidate) != source)
            {
                continue;
            }

            Rect rect;
            try
            {
                rect = GetTargetRect(candidate);
            }
            catch (InvalidOperationException)
            {
                continue;
            }

            rect.Inflate(4, 4);
            rect.Intersect(bounds);
            if (rect.IsEmpty || rect.Width <= 0 || rect.Height <= 0)
            {
                continue;
            }

            target = candidate;
            hole = rect;
            return true;
        }

        target = null!;
        hole = default;
        return false;
    }

    /// <summary>
    /// 提示文字的排版缓存。FormattedText 构造要走字体度量，滚动 / 拖动遮罩时每帧重建会持续分配，
    /// 因此只在文字、主题（深浅色）或 DPI 变化时重建。
    /// </summary>
    private string _calloutText = string.Empty;
    private bool _calloutDark;
    private double _calloutPixelsPerDip = -1;
    private FormattedText? _calloutTextFormat;

    private void DrawCallout(
        DrawingContext drawingContext,
        Rect hole,
        Rect bounds,
        string text,
        GuideHighlight.GuideHighlightPlacement placement)
    {
        bool isDark = ThemeHelper.IsDarkMode();
        var dpi = VisualTreeHelper.GetDpi(this);
        EnsureCalloutFormat(text, isDark, dpi.PixelsPerDip);

        FormattedText textFormat = _calloutTextFormat!;

        const double padding = 12;
        const double gap = 12;
        const double margin = 8;
        double width = textFormat.Width + (padding * 2);
        double height = textFormat.Height + (padding * 2);
        Rect area = GetBubbleRect(hole, bounds, width, height, placement, gap, margin);

        // FormattedText 的 TextAlignment.Center 是相对 MaxTextWidth 居中的，而气泡宽度取的是文字实际宽度。
        // 两者不等时（短文本 / 单行）文字会在气泡里整体偏移，这里把居中原点对齐到气泡内容区。
        textFormat.MaxTextWidth = width - (padding * 2) + 0.01;

        // 1. 柔和环境光阴影
        Rect shadowRect = new Rect(area.X, area.Y + 2, area.Width, area.Height);
        drawingContext.DrawRoundedRectangle(
            isDark ? CalloutShadowDark : CalloutShadowLight,
            null,
            shadowRect,
            8,
            8);

        // 2. 气泡卡片主体与小三角形指针
        Brush backdrop = isDark ? CalloutBackdropDark : CalloutBackdropLight;
        Pen borderPen = isDark ? CalloutBorderDark : CalloutBorderLight;

        drawingContext.DrawRoundedRectangle(
            backdrop,
            borderPen,
            area,
            8,
            8);

        DrawCaret(drawingContext, hole, area, backdrop, borderPen);

        // 3. 提示文字
        drawingContext.DrawText(textFormat, new Point(area.X + padding, area.Y + padding));
    }

    private static void DrawCaret(
        DrawingContext drawingContext,
        Rect hole,
        Rect area,
        Brush fill,
        Pen stroke)
    {
        Point p1, p2, p3;
        const double caretSize = 5.0;

        // 依据气泡相对于挖孔的实际几何方位决定指针方向与附着边，避免请求方位与实际回退方位脱节
        if (area.Left >= hole.Right - 5)
        {
            // 气泡在挖孔右侧：指针在气泡左边缘，朝左指向挖孔
            double y = Math.Clamp(hole.Top + (hole.Height / 2), area.Top + 8, area.Bottom - 8);
            p1 = new Point(area.Left, y - caretSize);
            p2 = new Point(area.Left - caretSize, y);
            p3 = new Point(area.Left, y + caretSize);
        }
        else if (area.Right <= hole.Left + 5)
        {
            // 气泡在挖孔左侧：指针在气泡右边缘，朝右指向挖孔
            double y = Math.Clamp(hole.Top + (hole.Height / 2), area.Top + 8, area.Bottom - 8);
            p1 = new Point(area.Right, y - caretSize);
            p2 = new Point(area.Right + caretSize, y);
            p3 = new Point(area.Right, y + caretSize);
        }
        else if (area.Top >= hole.Bottom - 5)
        {
            // 气泡在挖孔下方：指针在气泡上边缘，朝上指向挖孔
            double x = Math.Clamp(hole.Left + (hole.Width / 2), area.Left + 8, area.Right - 8);
            p1 = new Point(x - caretSize, area.Top);
            p2 = new Point(x, area.Top - caretSize);
            p3 = new Point(x + caretSize, area.Top);
        }
        else
        {
            // 气泡在挖孔上方：指针在气泡下边缘，朝下指向挖孔
            double x = Math.Clamp(hole.Left + (hole.Width / 2), area.Left + 8, area.Right - 8);
            p1 = new Point(x - caretSize, area.Bottom);
            p2 = new Point(x, area.Bottom + caretSize);
            p3 = new Point(x + caretSize, area.Bottom);
        }

        var caretGeom = new StreamGeometry();
        using (var ctx = caretGeom.Open())
        {
            ctx.BeginFigure(p1, true, false);
            ctx.LineTo(p2, true, true);
            ctx.LineTo(p3, true, true);
        }

        caretGeom.Freeze();
        drawingContext.DrawGeometry(fill, stroke, caretGeom);

        // 用底板色消除小三角形与气泡卡片接缝处的描边，使其融为一体
        var seamPen = new Pen(fill, stroke.Thickness + 0.5);
        seamPen.Freeze();
        drawingContext.DrawLine(seamPen, p1, p3);
    }

    /// <summary>
    /// 生成遮罩外边框几何体：顶部为平角（贴合分割线），底部采用与外层 GroupBox 框线严格一致的圆角，
    /// 确保遮罩下边缘与外框完美吻合，消除未遮罩缝隙与直角毛刺。
    /// </summary>
    private static Geometry CreateContentBoundsGeometry(Rect bounds, double bottomRadius)
    {
        if (bottomRadius <= 0)
        {
            return new RectangleGeometry(bounds);
        }

        double r = Math.Min(bottomRadius, Math.Min(bounds.Width / 2, bounds.Height / 2));
        var geom = new PathGeometry();
        var fig = new PathFigure
        {
            StartPoint = new Point(bounds.Left, bounds.Top),
            IsClosed = true,
            IsFilled = true,
        };

        fig.Segments.Add(new LineSegment(new Point(bounds.Right, bounds.Top), false));
        fig.Segments.Add(new LineSegment(new Point(bounds.Right, bounds.Bottom - r), false));
        fig.Segments.Add(new ArcSegment(new Point(bounds.Right - r, bounds.Bottom), new Size(r, r), 0, false, SweepDirection.Clockwise, false));
        fig.Segments.Add(new LineSegment(new Point(bounds.Left + r, bounds.Bottom), false));
        fig.Segments.Add(new ArcSegment(new Point(bounds.Left, bounds.Bottom - r), new Size(r, r), 0, false, SweepDirection.Clockwise, false));
        fig.Segments.Add(new LineSegment(new Point(bounds.Left, bounds.Top), false));

        geom.Figures.Add(fig);
        geom.Freeze();
        return geom;
    }

    /// <summary>
    /// 按需重建提示文字的排版对象。
    /// </summary>
    /// <param name="text">提示文字。</param>
    /// <param name="isDark">当前是否深色主题。</param>
    /// <param name="pixelsPerDip">当前 DPI 缩放。</param>
    private void EnsureCalloutFormat(string text, bool isDark, double pixelsPerDip)
    {
        if (_calloutTextFormat is not null
            && _calloutText == text
            && _calloutDark == isDark
            && Math.Abs(_calloutPixelsPerDip - pixelsPerDip) < 0.0001)
        {
            return;
        }

        _calloutTextFormat = CreateCalloutText(text, isDark ? CalloutTextDark : CalloutTextLight, pixelsPerDip);
        _calloutText = text;
        _calloutDark = isDark;
        _calloutPixelsPerDip = pixelsPerDip;
    }

    private FormattedText CreateCalloutText(string text, Brush brush, double pixelsPerDip) => new(
        text,
        CultureInfo.CurrentUICulture,
        FlowDirection.LeftToRight,
        _bubbleTypeface,
        13,
        brush,
        pixelsPerDip)
    {
        MaxTextWidth = 280,
        TextAlignment = TextAlignment.Center,
    };

    /// <summary>
    /// 选择气泡落点：先在期望方向上找完全落在遮罩内且不压住挖孔的候选；
    /// 都放不下时按 ｢溢出量 + 与挖孔的距离 + 是否重叠｣ 打分取最优，最后钳进遮罩范围。
    /// 只判断"完全放得下"会在目标贴近边界时把气泡钳到对角角落，看上去与目标脱节。
    /// </summary>
    /// <param name="hole">挖孔矩形。</param>
    /// <param name="bounds">遮罩自身范围。</param>
    /// <param name="width">气泡宽度。</param>
    /// <param name="height">气泡高度。</param>
    /// <param name="placement">期望的相对位置。</param>
    /// <param name="gap">气泡与挖孔的间距。</param>
    /// <param name="margin">气泡与遮罩边缘的最小距离。</param>
    /// <returns>气泡矩形。</returns>
    private static Rect GetBubbleRect(
        Rect hole,
        Rect bounds,
        double width,
        double height,
        GuideHighlight.GuideHighlightPlacement placement,
        double gap,
        double margin)
    {
        const double overlapPenalty = 1_000_000;

        Rect Candidate(GuideHighlight.GuideHighlightPlacement p) => p switch
        {
            GuideHighlight.GuideHighlightPlacement.Left => new Rect(
                hole.Left - gap - width,
                hole.Top + ((hole.Height - height) / 2),
                width,
                height),
            GuideHighlight.GuideHighlightPlacement.Right => new Rect(
                hole.Right + gap,
                hole.Top + ((hole.Height - height) / 2),
                width,
                height),
            GuideHighlight.GuideHighlightPlacement.Bottom => new Rect(
                hole.Left + ((hole.Width - width) / 2),
                hole.Bottom + gap,
                width,
                height),
            _ => new Rect(
                hole.Left + ((hole.Width - width) / 2),
                hole.Top - gap - height,
                width,
                height),
        };

        var opposite = placement switch
        {
            GuideHighlight.GuideHighlightPlacement.Left => GuideHighlight.GuideHighlightPlacement.Right,
            GuideHighlight.GuideHighlightPlacement.Right => GuideHighlight.GuideHighlightPlacement.Left,
            GuideHighlight.GuideHighlightPlacement.Bottom => GuideHighlight.GuideHighlightPlacement.Top,
            _ => GuideHighlight.GuideHighlightPlacement.Bottom,
        };

        List<GuideHighlight.GuideHighlightPlacement> order = [placement, opposite];
        foreach (var alternative in new[]
                 {
                     GuideHighlight.GuideHighlightPlacement.Top,
                     GuideHighlight.GuideHighlightPlacement.Bottom,
                     GuideHighlight.GuideHighlightPlacement.Left,
                     GuideHighlight.GuideHighlightPlacement.Right,
                 })
        {
            if (!order.Contains(alternative))
            {
                order.Add(alternative);
            }
        }

        Rect best = default;
        double bestScore = double.MaxValue;
        foreach (var p in order)
        {
            Rect rect = Candidate(p);
            double score = (rect.IntersectsWith(hole) ? overlapPenalty : 0)
                           + (OverflowAmount(rect, bounds, margin) * 4)
                           + DistanceToHole(rect, hole);
            if (score < bestScore)
            {
                bestScore = score;
                best = rect;
            }
        }

        return ClampInto(bounds, best, margin);
    }

    private static double OverflowAmount(Rect rect, Rect bounds, double margin) =>
        Math.Max(0, margin - rect.Left)
        + Math.Max(0, margin - rect.Top)
        + Math.Max(0, rect.Right - (bounds.Width - margin))
        + Math.Max(0, rect.Bottom - (bounds.Height - margin));

    private static double DistanceToHole(Rect rect, Rect hole)
    {
        double dx = Math.Max(0, Math.Max(hole.Left - rect.Right, rect.Left - hole.Right));
        double dy = Math.Max(0, Math.Max(hole.Top - rect.Bottom, rect.Top - hole.Bottom));
        return dx + dy;
    }

    private static Rect ClampInto(Rect bounds, Rect rect, double margin)
    {
        double maxX = Math.Max(margin, bounds.Width - rect.Width - margin);
        double maxY = Math.Max(margin, bounds.Height - rect.Height - margin);
        return new Rect(
            Math.Clamp(rect.X, margin, maxX),
            Math.Clamp(rect.Y, margin, maxY),
            rect.Width,
            rect.Height);
    }

    private Rect GetTargetRect(UIElement target)
    {
        if (VisualTreeHelper.GetParent(this) is not FrameworkElement parent)
        {
            return Rect.Empty;
        }

        Rect rect = target.TransformToAncestor(parent).TransformBounds(new Rect(target.RenderSize));
        Point overlayOrigin = TranslatePoint(new Point(0, 0), parent);
        rect.Offset(-overlayOrigin.X, -overlayOrigin.Y);
        return rect;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        Current = this;
        ThemeHelper.ThemeChanged -= OnThemeChanged;
        ThemeHelper.ThemeChanged += OnThemeChanged;
        GuideHighlight.NotifyOverlayReady(this);
        InvalidateVisual();
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        ThemeHelper.ThemeChanged -= OnThemeChanged;

        // 目标随指引界面卸载后不再参与绘制，显式释放引用。同时要断开目标上的 OverlayProperty：
        // 否则目标重新加载时 Apply 会命中这个已卸载的遮罩、只刷新它而不注册到新遮罩，高亮随之丢失
        foreach (FrameworkElement target in _targets)
        {
            GuideHighlight.Detach(target);
        }

        _targets.Clear();
        if (Current == this)
        {
            Current = null;
        }
    }

    private void OnThemeChanged(object? sender, EventArgs e)
    {
        Dispatcher.BeginInvoke(new Action(InvalidateVisual));
    }

    private void OnLayoutUpdated(object? sender, EventArgs e)
    {
        // 没有激活目标时不排重绘：LayoutUpdated 在每次布局变化后触发，无高亮时重绘只会白跑一遍渲染管线
        if (_invalidateQueued || _targets.Count == 0)
        {
            return;
        }

        _invalidateQueued = true;
        Dispatcher.BeginInvoke(
            System.Windows.Threading.DispatcherPriority.Render,
            new Action(() =>
            {
                _invalidateQueued = false;
                InvalidateVisual();
            }));
    }

    private static Brush CreateBrush(byte alpha, byte red, byte green, byte blue)
    {
        var brush = new SolidColorBrush(Color.FromArgb(alpha, red, green, blue));
        brush.Freeze();
        return brush;
    }

    private static Pen CreatePen(Brush brush, double thickness)
    {
        var pen = new Pen(brush, thickness);
        pen.Freeze();
        return pen;
    }
}
