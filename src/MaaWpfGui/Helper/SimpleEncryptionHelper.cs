// <copyright file="SimpleEncryptionHelper.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Text;
using Serilog;

namespace MaaWpfGui.Helper;

public static class SimpleEncryptionHelper
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "SimpleEncryptionHelper");

    /// <summary>
    /// 使用数据保护API对明文进行加密。
    /// </summary>
    /// <param name="plainText">需要加密的文本。如果为null或空字符串，则返回空字符串。</param>
    /// <param name="defaultText">用于比较的默认文本。如果plainText与defaultText相同，则直接返回plainText而不加密。</param>
    /// <param name="dataProtectionScope">数据保护的范围，默认为CurrentUser（当前用户）。</param>
    /// <param name="caller">调用方法的名称，由编译器自动填充。</param>
    /// <returns>返回加密后的Base64编码字符串。如果加密失败，则返回原始plainText。</returns>
    public static string Encrypt(string plainText, string defaultText = "", DataProtectionScope dataProtectionScope = DataProtectionScope.CurrentUser, [CallerMemberName] string caller = "")
    {
        if (string.IsNullOrEmpty(plainText))
        {
            return string.Empty;
        }

        if (plainText == defaultText)
        {
            return plainText;
        }

        try
        {
            var data = Encoding.UTF8.GetBytes(plainText);
            var encryptedData = ProtectedData.Protect(data, null, dataProtectionScope);
            return Convert.ToBase64String(encryptedData);
        }
        catch (Exception e)
        {
            ToastNotification.ShowDirect(caller + LocalizationHelper.GetString("EncryptionError"));
            _logger.Error("Failed to encrypt text: " + e.Message);
            return plainText;
        }
    }

    /// <summary>
    /// 使用数据保护API对加密文本进行解密。
    /// </summary>
    /// <param name="encryptedText">需要解密的Base64编码加密文本。如果为null或空字符串，则返回空字符串。</param>
    /// <param name="defaultText">用于比较的默认文本。如果encryptedText与defaultText相同，则直接返回defaultText而不解密。</param>
    /// <param name="dataProtectionScope">数据保护的范围，默认为CurrentUser（当前用户）。</param>
    /// <param name="caller">调用方法的名称，由编译器自动填充。</param>
    /// <returns>返回解密后的文本。如果解密失败，则返回原始encryptedText。</returns>
    public static string Decrypt(string encryptedText, string defaultText = "", DataProtectionScope dataProtectionScope = DataProtectionScope.CurrentUser, [CallerMemberName] string caller = "")
    {
        if (string.IsNullOrEmpty(encryptedText))
        {
            return string.Empty;
        }

        if (encryptedText == defaultText)
        {
            return defaultText;
        }

        try
        {
            var data = Convert.FromBase64String(encryptedText);
            var decryptedData = ProtectedData.Unprotect(data, null, dataProtectionScope);
            return Encoding.UTF8.GetString(decryptedData);
        }
        catch (Exception e)
        {
            ToastNotification.ShowDirect(caller + LocalizationHelper.GetString("EncryptionError"));
            _logger.Error("Failed to decrypt text: " + e.Message);
            return encryptedText;
        }
    }
}
