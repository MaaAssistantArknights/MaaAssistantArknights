using System;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Text;
using Serilog;

namespace MaaWpfGui.Helper;

public static class SimpleEncryptionHelper
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "SimpleEncryptionHelper");

    public static string Encrypt(string plainText, DataProtectionScope dataProtectionScope = DataProtectionScope.CurrentUser, [CallerMemberName] string caller = "")
    {
        if (string.IsNullOrEmpty(plainText))
        {
            return string.Empty;
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

    public static string Decrypt(string encryptedText, DataProtectionScope dataProtectionScope = DataProtectionScope.CurrentUser, [CallerMemberName] string caller = "")
    {
        if (string.IsNullOrEmpty(encryptedText))
        {
            return string.Empty;
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
