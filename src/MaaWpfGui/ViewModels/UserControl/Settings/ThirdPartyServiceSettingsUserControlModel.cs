// <copyright file="ThirdPartyServiceSettingsUserControlModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
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
using System.Threading.Tasks;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class ThirdPartyServiceSettingsUserControlModel : PropertyChangedBase
{
    static ThirdPartyServiceSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public static ThirdPartyServiceSettingsUserControlModel Instance { get; }

    #region 企鹅和一图流上报

    /// <summary>
    /// Gets or sets the id of PenguinStats.
    /// </summary>
    public string PenguinId
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.PenguinId = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.PenguinId;

    /// <summary>
    /// Gets or sets a value indicating whether to enable penguin upload.
    /// </summary>
    public bool EnablePenguin
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToPenguin = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToPenguin;

    /// <summary>
    /// Gets or sets a value indicating whether to enable yituliu upload.
    /// </summary>
    public bool EnableYituliu
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToYituliu = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.ReportToYituliu;

    #endregion 企鹅和一图流上报

    #region 一图流 OpenAPI

    /// <summary>
    /// Gets or sets 一图流第三方 OpenAPI Token，存储层加密，界面层明文。
    /// </summary>
    public string YituliuOpenApiToken
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                YituliuTokenValidationText = string.Empty;
                ConfigFactory.CurrentConfig.Gui.ThirdParty.YituliuOpenApiToken = SimpleEncryptionHelper.Encrypt(value);
            }
        }
    } = SimpleEncryptionHelper.Decrypt(ConfigFactory.CurrentConfig.Gui.ThirdParty.YituliuOpenApiToken);

    /// <summary>
    /// Gets or sets a value indicating whether 干员识别改为从一图流 OpenAPI 获取。
    /// </summary>
    public bool EnableOperBoxYituliuApi
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.ThirdParty.OperBoxUseYituliuApi = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.ThirdParty.OperBoxUseYituliuApi;

    public bool IsVerifyingYituliuToken
    {
        get; set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(CanVerifyYituliuToken));
            }
        }
    }

    public bool CanVerifyYituliuToken => !IsVerifyingYituliuToken;

    private (YituliuApiService.TokenValidationResult Result, int OperatorCount)? _lastYituliuTokenValidation;

    public string YituliuTokenValidationText
    {
        get; set => SetAndNotify(ref field, value);
    } = string.Empty;

    /// <summary>
    /// 验证一图流 OpenAPI Token 是否具备干员数据读取权限。
    /// UI 绑定的方法
    /// </summary>
    [UsedImplicitly]
    public async Task VerifyYituliuToken()
    {
        var token = YituliuOpenApiToken.Trim();
        if (string.IsNullOrEmpty(token))
        {
            YituliuTokenValidationText = LocalizationHelper.GetString("YituliuTokenEmpty");
            return;
        }

        IsVerifyingYituliuToken = true;
        try
        {
            var validation = await YituliuApiService.ValidateTokenAsync(token);
            _lastYituliuTokenValidation = validation;
            YituliuTokenValidationText = FormatYituliuTokenValidationText(validation);
        }
        finally
        {
            IsVerifyingYituliuToken = false;
        }
    }

    private static string FormatYituliuTokenValidationText((YituliuApiService.TokenValidationResult Result, int OperatorCount) validation)
    {
        return validation.Result switch {
            YituliuApiService.TokenValidationResult.Valid => LocalizationHelper.GetStringFormat("YituliuTokenValid", validation.OperatorCount),
            YituliuApiService.TokenValidationResult.WriteOnly => LocalizationHelper.GetString("YituliuTokenWriteOnly"),
            YituliuApiService.TokenValidationResult.Invalid => LocalizationHelper.GetString("YituliuTokenInvalid"),
            _ => LocalizationHelper.GetString("YituliuTokenNetworkError"),
        };
    }

    /// <summary>
    /// 刷新验证结果文本的本地化显示。
    /// </summary>
    private void RefreshLocalization()
    {
        if (_lastYituliuTokenValidation is { } validation)
        {
            YituliuTokenValidationText = FormatYituliuTokenValidationText(validation);
        }
    }

    #endregion 一图流 OpenAPI
}
