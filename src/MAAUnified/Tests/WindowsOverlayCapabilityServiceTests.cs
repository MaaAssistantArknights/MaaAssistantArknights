using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class WindowsOverlayCapabilityServiceTests
{
    [Fact]
    public async Task SetVisibleAsync_WithSelectedTarget_ShouldAttachImmediatelyAndEmitShowNative()
    {
        var api = new FakeNativeApi();
        api.AddWindow(0x10, "Overlay Host", rect: new WindowsOverlayCapabilityService.OverlayRect(0, 0, 100, 100));
        api.AddWindow(0x20, "Arknights", rect: new WindowsOverlayCapabilityService.OverlayRect(10, 20, 210, 320));
        var service = new WindowsOverlayCapabilityService(api);
        var events = new List<OverlayStateChangedEvent>();
        service.OverlayStateChanged += (_, e) => events.Add(e);

        var bind = await service.BindHostWindowAsync((nint)0x10, clickThrough: true, opacity: 0.85);
        var select = await service.SelectTargetAsync("hwnd:20");
        var visible = await service.SetVisibleAsync(true);

        Assert.True(bind.Success);
        Assert.True(select.Success);
        Assert.True(visible.Success);
        var showNative = Assert.Single(events, e => string.Equals(e.Action, "show-native", StringComparison.Ordinal));
        Assert.Equal(OverlayRuntimeMode.Native, showNative.Mode);
        Assert.True(showNative.Visible);
        Assert.Equal("hwnd:20", showNative.TargetId);
        Assert.True(api.SetWindowPosCalls > 0);
        Assert.True(api.ShowWindowCalls > 0);
    }

    [Fact]
    public async Task SelectTargetAsync_WhileVisible_ShouldAttachImmediatelyAndEmitTargetChange()
    {
        var api = new FakeNativeApi();
        api.AddWindow(0x10, "Overlay Host", rect: new WindowsOverlayCapabilityService.OverlayRect(0, 0, 100, 100));
        api.AddWindow(0x20, "Arknights A", rect: new WindowsOverlayCapabilityService.OverlayRect(10, 20, 210, 320));
        api.AddWindow(0x30, "Arknights B", rect: new WindowsOverlayCapabilityService.OverlayRect(40, 50, 240, 360));
        var service = new WindowsOverlayCapabilityService(api);
        var events = new List<OverlayStateChangedEvent>();
        service.OverlayStateChanged += (_, e) => events.Add(e);

        await service.BindHostWindowAsync((nint)0x10, clickThrough: true, opacity: 0.85);
        await service.SelectTargetAsync("hwnd:20");
        await service.SetVisibleAsync(true);
        events.Clear();

        var result = await service.SelectTargetAsync("hwnd:30");

        Assert.True(result.Success);
        var targetChange = Assert.Single(events, e => string.Equals(e.Action, "target-change", StringComparison.Ordinal));
        Assert.Equal(OverlayRuntimeMode.Native, targetChange.Mode);
        Assert.True(targetChange.Visible);
        Assert.Equal("hwnd:30", targetChange.TargetId);
    }

    [Fact]
    public async Task SelectTargetAsync_WhileVisibleAndAttachFails_ShouldReturnOverlayAttachFailedAndEmitPreviewFallback()
    {
        var api = new FakeNativeApi();
        api.AddWindow(0x10, "Overlay Host", rect: new WindowsOverlayCapabilityService.OverlayRect(0, 0, 100, 100));
        api.AddWindow(0x20, "Arknights A", rect: new WindowsOverlayCapabilityService.OverlayRect(10, 20, 210, 320));
        api.AddWindow(0x30, "Arknights B", rect: new WindowsOverlayCapabilityService.OverlayRect(40, 50, 240, 360));
        var service = new WindowsOverlayCapabilityService(api);
        var events = new List<OverlayStateChangedEvent>();
        service.OverlayStateChanged += (_, e) => events.Add(e);

        await service.BindHostWindowAsync((nint)0x10, clickThrough: true, opacity: 0.85);
        await service.SelectTargetAsync("hwnd:20");
        await service.SetVisibleAsync(true);
        events.Clear();
        api.FailSetWindowPos = true;

        var result = await service.SelectTargetAsync("hwnd:30");

        Assert.False(result.Success);
        Assert.Equal(PlatformErrorCodes.OverlayAttachFailed, result.ErrorCode);
        var fallback = Assert.Single(events, e => string.Equals(e.Action, "fallback-enter", StringComparison.Ordinal));
        Assert.Equal(OverlayRuntimeMode.Preview, fallback.Mode);
        Assert.True(fallback.Visible);
        Assert.Equal("preview", fallback.TargetId);
    }

    [Fact]
    public async Task SyncNowForTesting_WhenTargetDisappears_ShouldEmitTargetLostAndSwitchToPreview()
    {
        var api = new FakeNativeApi();
        api.AddWindow(0x10, "Overlay Host", rect: new WindowsOverlayCapabilityService.OverlayRect(0, 0, 100, 100));
        api.AddWindow(0x20, "Arknights", rect: new WindowsOverlayCapabilityService.OverlayRect(10, 20, 210, 320));
        var service = new WindowsOverlayCapabilityService(api);
        var events = new List<OverlayStateChangedEvent>();
        service.OverlayStateChanged += (_, e) => events.Add(e);

        await service.BindHostWindowAsync((nint)0x10, clickThrough: true, opacity: 0.85);
        await service.SelectTargetAsync("hwnd:20");
        await service.SetVisibleAsync(true);
        events.Clear();
        api.RemoveWindow(0x20);

        service.SyncNowForTesting();

        var targetLost = Assert.Single(events, e => string.Equals(e.Action, "target-lost", StringComparison.Ordinal));
        Assert.Equal(OverlayRuntimeMode.Preview, targetLost.Mode);
        Assert.True(targetLost.Visible);
        Assert.Equal("preview", targetLost.TargetId);
    }

    private sealed class FakeNativeApi : WindowsOverlayCapabilityService.INativeApi
    {
        private readonly Dictionary<nint, WindowsOverlayCapabilityService.OverlayRect> _rects = [];
        private readonly Dictionary<nint, string> _titles = [];
        private readonly HashSet<nint> _visibleWindows = [];

        public int SetWindowPosCalls { get; private set; }

        public int ShowWindowCalls { get; private set; }

        public bool FailSetWindowPos { get; set; }

        public void AddWindow(nint handle, string title, WindowsOverlayCapabilityService.OverlayRect rect)
        {
            _rects[handle] = rect;
            _titles[handle] = title;
            _visibleWindows.Add(handle);
        }

        public void RemoveWindow(nint handle)
        {
            _rects.Remove(handle);
            _titles.Remove(handle);
            _visibleWindows.Remove(handle);
        }

        public IEnumerable<nint> EnumerateWindows()
            => _rects.Keys;

        public bool IsWindow(nint hWnd)
            => _rects.ContainsKey(hWnd);

        public bool IsWindowVisible(nint hWnd)
            => _visibleWindows.Contains(hWnd);

        public int GetWindowTextLength(nint hWnd)
            => _titles.TryGetValue(hWnd, out var title) ? title.Length : 0;

        public string GetWindowText(nint hWnd)
            => _titles.TryGetValue(hWnd, out var title) ? title : string.Empty;

        public uint GetWindowThreadProcessId(nint hWnd)
            => 4242;

        public bool TryGetWindowRect(nint hWnd, out WindowsOverlayCapabilityService.OverlayRect rect)
            => _rects.TryGetValue(hWnd, out rect);

        public bool SetWindowPos(nint hWnd, nint hWndInsertAfter, int x, int y, int cx, int cy, uint uFlags)
        {
            SetWindowPosCalls++;
            return !FailSetWindowPos;
        }

        public bool ShowWindow(nint hWnd, int nCmdShow)
        {
            ShowWindowCalls++;
            return true;
        }

        public nint GetWindowLongPtr(nint hWnd, int nIndex)
            => (nint)1;

        public int GetLastWin32Error()
            => 0;

        public nint SetWindowLongPtr(nint hWnd, int nIndex, nint dwNewLong)
            => dwNewLong;

        public bool SetLayeredWindowAttributes(nint hwnd, uint crKey, byte bAlpha, uint dwFlags)
            => true;
    }
}
