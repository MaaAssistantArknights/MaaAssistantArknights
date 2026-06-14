#pragma once

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>

#include <string>

// ---------------------------------------------------------------------------
// Progress UI structures
// ---------------------------------------------------------------------------

struct UpdateProgressUi
{
    bool enabled = false;
    HWND window = nullptr;
    HWND statusLabel = nullptr;
    HWND detailLabel = nullptr;
    HWND countLabel = nullptr;
    HWND progressBar = nullptr;
    int processedFileCount = 0;
    int totalFileCount = 0;
};

struct ProgressUiTheme
{
    bool isDarkMode = false;
    COLORREF backgroundColor = RGB(255, 255, 255);
    COLORREF primaryTextColor = RGB(32, 32, 32);
    COLORREF secondaryTextColor = RGB(96, 96, 96);
    COLORREF progressTrackColor = RGB(232, 234, 237);
    COLORREF progressBarColor = RGB(0, 120, 212);
    HBRUSH backgroundBrush = nullptr;
};

// ---------------------------------------------------------------------------
// Progress UI globals (defined in UpdaterUI.cpp)
// ---------------------------------------------------------------------------

extern UpdateProgressUi g_progressUi;
extern ProgressUiTheme g_progressUiTheme;

// ---------------------------------------------------------------------------
// Progress UI functions
// ---------------------------------------------------------------------------

bool IsSystemDarkModeEnabled();
void RefreshProgressUiTheme();
bool ShouldShowProgressUi();
bool InitializeProgressUi();
void DestroyProgressUi();
void PumpProgressUiMessages();
std::wstring BuildProgressCountText(int processedFileCount, int totalFileCount);
void RefreshProgressUiCountText();
void SetProgressUiTotalFileCount(int totalFileCount);
void SetProgressUiStatus(const std::wstring& status, const std::wstring& detail);
void AdvanceProgressUi(const std::wstring& status, const std::wstring& detail);
void CompleteProgressUi(const std::wstring& status, const std::wstring& detail);
void ShowProgressUiFailure(const std::wstring& failureReason);
