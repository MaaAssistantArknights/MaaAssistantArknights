// <copyright file="UpdateCredentialModel.cs" company="MaaAssistantArknights">
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
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using JetBrains.Annotations;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.Models;

public sealed class UpdateCredentialModel : PropertyChangedBase
{
    private readonly UpdateCredentialMessages _messages;
    private readonly Func<bool> _isSelected;
    private readonly Func<string, bool> _isComplete;
    private readonly Action<string> _saveValue;
    private readonly Action<DateTimeOffset?, UpdateCredentialStatus> _saveExpiration;
    private readonly Action<int> _onCompleteInput;
    private CancellationTokenSource _requestCancellation = new();
    private string _value;
    private string? _lastNotifiedErrorKey;
    private UpdateCredentialStatus _statusBeforeCheck;
    private string? _errorKeyBeforeCheck;

    public UpdateCredentialModel(
        string value,
        DateTimeOffset? expiresAt,
        UpdateCredentialStatus status,
        UpdateCredentialMessages messages,
        Func<bool> isSelected,
        Func<string, bool> isComplete,
        Action<string> saveValue,
        Action<DateTimeOffset?, UpdateCredentialStatus> saveExpiration,
        Action<int> onCompleteInput)
    {
        _messages = messages;
        _isSelected = isSelected;
        _isComplete = isComplete;
        _saveValue = saveValue;
        _saveExpiration = saveExpiration;
        _onCompleteInput = onCompleteInput;
        _value = value.Trim();
        ExpiresAt = string.IsNullOrEmpty(_value) ? null : expiresAt;
        Status = string.IsNullOrEmpty(_value) ? UpdateCredentialStatus.None : status;
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    public enum UpdateCredentialStatus
    {
        None,
        Checking,
        Valid,
        Invalid,
        Expired,
        Failed,
    }

    public readonly record struct UpdateCredentialResult(
        UpdateCredentialStatus Status,
        DateTimeOffset? ExpiresAt = null,
        string? ErrorKey = null);

    public readonly record struct UpdateCredentialRequest(string Value, int Revision, CancellationToken CancellationToken);

    public sealed record UpdateCredentialMessages(
        string Checking,
        string Invalid,
        string Expired,
        string Failed,
        string RemainingDays,
        string? NeverExpires = null,
        string? UnknownExpiration = null);

    public string Value
    {
        get => _value;
        set {
            value = value.Trim();
            if (!SetAndNotify(ref _value, value))
            {
                return;
            }

            _saveValue(value);
            if (string.IsNullOrEmpty(value))
            {
                ExpiresAt = null;
                _saveExpiration(null, UpdateCredentialStatus.None);
            }

            Reset();
            if (_isSelected() && _isComplete(value))
            {
                _onCompleteInput(Revision);
            }
        }
    }

    public int Revision { get; private set; }

    public UpdateCredentialStatus Status { get; private set => SetAndNotify(ref field, value); }

    public DateTimeOffset? ExpiresAt { get; private set => SetAndNotify(ref field, value); }

    private string? ErrorKey { get; set => SetAndNotify(ref field, value); }

    [PropertyDependsOn(nameof(Status), nameof(ExpiresAt))]
    public bool IsExpired => Status == UpdateCredentialStatus.Expired || ExpiresAt <= DateTimeOffset.UtcNow;

    [PropertyDependsOn(nameof(Status), nameof(IsExpired))]
    public bool HasError => Status is UpdateCredentialStatus.Invalid or UpdateCredentialStatus.Expired or UpdateCredentialStatus.Failed || IsExpired;

    [PropertyDependsOn(nameof(ExpiresAt))]
    public DateTime? ExpiresLocalTime => ExpiresAt?.LocalDateTime;

    [PropertyDependsOn(nameof(Status), nameof(ExpiresAt), nameof(ErrorKey))]
    public string StatusText
    {
        get {
            string? key = GetStatusKey();
            if (key is not null)
            {
                return LocalizationHelper.GetString(key);
            }

            if (ExpiresAt is { } expiresAt)
            {
                return LocalizationHelper.GetStringFormat(_messages.RemainingDays,
                    (expiresAt - DateTimeOffset.UtcNow).TotalDays.ToString("F1"));
            }

            return string.Empty;
        }
    }

    [PropertyDependsOn(nameof(Status), nameof(ExpiresAt))]
    public string StatusBrush => Status switch {
        UpdateCredentialStatus.Invalid or UpdateCredentialStatus.Expired or UpdateCredentialStatus.Failed => UiLogColor.Error,
        UpdateCredentialStatus.Checking => UiLogColor.Info,
        _ => IsExpired
            ? UiLogColor.Error
            : ExpiresAt is { } expiresAt
                ? (expiresAt - DateTimeOffset.UtcNow).TotalDays <= 7 ? UiLogColor.Warning : UiLogColor.Success
                : Status == UpdateCredentialStatus.Valid && _messages.NeverExpires is not null ? UiLogColor.Success : UiLogColor.Info,
    };

    public void Reset()
    {
        CancelRequests();
        Status = UpdateCredentialStatus.None;
        ErrorKey = null;
        _statusBeforeCheck = UpdateCredentialStatus.None;
        _errorKeyBeforeCheck = null;
        _lastNotifiedErrorKey = null;
    }

    public void CancelCheck()
    {
        CancelRequests();
        if (Status == UpdateCredentialStatus.Checking)
        {
            Status = _statusBeforeCheck;
            ErrorKey = _errorKeyBeforeCheck;
        }
    }

    public void RefreshDisplay()
    {
        Execute.OnUIThread(() => {
            if (Status == UpdateCredentialStatus.Valid && IsExpired)
            {
                Status = UpdateCredentialStatus.Expired;
                ErrorKey = null;
            }

            OnPropertyChanged(nameof(IsExpired));
            OnPropertyChanged(nameof(HasError));
            OnPropertyChanged(nameof(StatusText));
            OnPropertyChanged(nameof(StatusBrush));
            OnPropertyChanged(nameof(ExpiresLocalTime));
        });
    }

    public async Task<UpdateCredentialRequest?> BeginCheckAsync()
    {
        UpdateCredentialRequest? request = null;
        await Execute.OnUIThreadAsync(() => {
            if (!_isSelected() || string.IsNullOrEmpty(Value))
            {
                return;
            }

            CancelPendingRequest();
            request = new UpdateCredentialRequest(Value, Revision, _requestCancellation.Token);
            if (Status != UpdateCredentialStatus.Checking)
            {
                _statusBeforeCheck = Status;
                _errorKeyBeforeCheck = ErrorKey;
            }

            Status = UpdateCredentialStatus.Checking;
            ErrorKey = null;
        }).ConfigureAwait(false);
        return request;
    }

    public bool IsCurrent(UpdateCredentialRequest request) =>
        _isSelected() && !request.CancellationToken.IsCancellationRequested &&
        request.Revision == Revision && request.Value == Value;

    public async Task<bool> ApplyAsync(UpdateCredentialRequest request, UpdateCredentialResult result)
    {
        bool applied = false;
        await Execute.OnUIThreadAsync(() => {
            if (!IsCurrent(request))
            {
                return;
            }

            var status = result.Status;
            if (status == UpdateCredentialStatus.Valid)
            {
                ExpiresAt = result.ExpiresAt;
                if (ExpiresAt <= DateTimeOffset.UtcNow)
                {
                    status = UpdateCredentialStatus.Expired;
                }

                _saveExpiration(ExpiresAt, status);
            }
            else if (status == UpdateCredentialStatus.Expired)
            {
                if (result.ExpiresAt is { } expiresAt && expiresAt <= DateTimeOffset.UtcNow)
                {
                    ExpiresAt = expiresAt;
                }
                else if (ExpiresAt > DateTimeOffset.UtcNow)
                {
                    ExpiresAt = null;
                }

                _saveExpiration(ExpiresAt, status);
            }

            Status = status;
            ErrorKey = result.ErrorKey;
            if (status == UpdateCredentialStatus.Valid)
            {
                _lastNotifiedErrorKey = null;
            }

            NotifyError();
            applied = true;
        }).ConfigureAwait(false);
        return applied;
    }

    public async Task InvalidateAsync(string value, CancellationToken cancellationToken = default)
    {
        await Execute.OnUIThreadAsync(() => {
            if (cancellationToken.IsCancellationRequested || !_isSelected() || string.IsNullOrEmpty(value) || value != Value)
            {
                return;
            }

            CancelRequests();
            Status = UpdateCredentialStatus.Invalid;
            ErrorKey = null;
            NotifyError();
        }).ConfigureAwait(false);
    }

    [UsedImplicitly]
    public void Copy()
    {
        Clipboard.Clear();
        Clipboard.SetDataObject(Value);
    }

    private void CancelRequests()
    {
        Revision++;
        CancelPendingRequest();
    }

    private void CancelPendingRequest()
    {
        _requestCancellation.Cancel();
        _requestCancellation.Dispose();
        _requestCancellation = new CancellationTokenSource();
    }

    private string? GetStatusKey() => Status switch {
        UpdateCredentialStatus.Checking => _messages.Checking,
        UpdateCredentialStatus.Invalid => ErrorKey ?? _messages.Invalid,
        UpdateCredentialStatus.Expired => ErrorKey ?? _messages.Expired,
        UpdateCredentialStatus.Failed => ErrorKey ?? _messages.Failed,
        _ => IsExpired ? _messages.Expired :
            Status == UpdateCredentialStatus.Valid && ExpiresAt is null ? _messages.NeverExpires ?? _messages.UnknownExpiration : null,
    };

    private void NotifyError()
    {
        if (Status is not (UpdateCredentialStatus.Invalid or UpdateCredentialStatus.Expired or UpdateCredentialStatus.Failed))
        {
            return;
        }

        string? key = GetStatusKey();
        if (key is null || key == _lastNotifiedErrorKey)
        {
            return;
        }

        _lastNotifiedErrorKey = key;
        ToastNotification.ShowDirect(LocalizationHelper.GetString(key));
    }
}
