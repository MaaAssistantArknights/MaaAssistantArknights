// <copyright file="NetworkHelper.cs" company="MaaAssistantArknights">
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

using System;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 网络相关助手类
/// </summary>
public static class NetworkHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(NetworkHelper));

    /// <summary>
    /// 获取当前 WLAN 网络接口的 IP 地址
    /// </summary>
    /// <returns>WLAN IP 地址，如果未找到则返回 null</returns>
    public static string? GetWlanIpAddress()
    {
        try
        {
            var networkInterfaces = NetworkInterface.GetAllNetworkInterfaces()
                .Where(ni => ni.OperationalStatus == OperationalStatus.Up
                            && ni.NetworkInterfaceType == NetworkInterfaceType.Wireless80211
                            && !ni.Description.ToLower().Contains("virtual")
                            && !ni.Description.ToLower().Contains("loopback"))
                .ToList();

            foreach (var networkInterface in networkInterfaces)
            {
                var ipProperties = networkInterface.GetIPProperties();
                var ipv4Address = ipProperties.UnicastAddresses
                    .FirstOrDefault(ua => ua.Address.AddressFamily == AddressFamily.InterNetwork
                                         && !IPAddress.IsLoopback(ua.Address)
                                         && ua.Address.ToString() != "169.254.0.0");

                if (ipv4Address != null)
                {
                    var ipAddress = ipv4Address.Address.ToString();
                    _logger.Information("Found WLAN IP address: {IpAddress} on interface: {InterfaceName}", 
                        ipAddress, networkInterface.Name);
                    return ipAddress;
                }
            }

            // 如果没有找到 WLAN 接口，尝试获取第一个活跃的非环回网络接口
            var fallbackInterfaces = NetworkInterface.GetAllNetworkInterfaces()
                .Where(ni => ni.OperationalStatus == OperationalStatus.Up
                            && ni.NetworkInterfaceType != NetworkInterfaceType.Loopback
                            && !ni.Description.ToLower().Contains("virtual"))
                .ToList();

            foreach (var networkInterface in fallbackInterfaces)
            {
                var ipProperties = networkInterface.GetIPProperties();
                var ipv4Address = ipProperties.UnicastAddresses
                    .FirstOrDefault(ua => ua.Address.AddressFamily == AddressFamily.InterNetwork
                                         && !IPAddress.IsLoopback(ua.Address)
                                         && !ua.Address.ToString().StartsWith("169.254"));

                if (ipv4Address != null)
                {
                    var ipAddress = ipv4Address.Address.ToString();
                    _logger.Information("Found fallback IP address: {IpAddress} on interface: {InterfaceName}", 
                        ipAddress, networkInterface.Name);
                    return ipAddress;
                }
            }

            _logger.Warning("No suitable network interface found for IP address detection");
            return null;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Error occurred while getting WLAN IP address");
            return null;
        }
    }

    /// <summary>
    /// 更新代理设置为当前 WLAN IP 地址
    /// </summary>
    /// <param name="port">代理端口，默认为 8080</param>
    /// <returns>是否成功更新代理设置</returns>
    public static bool UpdateProxyFromWlanIp(int port = 8080)
    {
        try
        {
            var ipAddress = GetWlanIpAddress();
            if (string.IsNullOrEmpty(ipAddress))
            {
                _logger.Warning("Cannot update proxy: No WLAN IP address found");
                return false;
            }

            var proxyAddress = $"{ipAddress}:{port}";
            
            // 更新代理设置
            var viewModel = ViewModels.UserControl.Settings.VersionUpdateSettingsUserControlModel.Instance;
            viewModel.Proxy = proxyAddress;
            
            _logger.Information("Successfully updated proxy to WLAN IP: {ProxyAddress}", proxyAddress);
            return true;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Error occurred while updating proxy from WLAN IP");
            return false;
        }
    }

    /// <summary>
    /// 检查指定的 IP 地址和端口是否可达
    /// </summary>
    /// <param name="ipAddress">IP 地址</param>
    /// <param name="port">端口</param>
    /// <param name="timeout">超时时间（毫秒），默认 3000ms</param>
    /// <returns>是否可达</returns>
    public static bool IsPortReachable(string ipAddress, int port, int timeout = 3000)
    {
        try
        {
            using var client = new TcpClient();
            var result = client.BeginConnect(ipAddress, port, null, null);
            var success = result.AsyncWaitHandle.WaitOne(timeout);
            
            if (success)
            {
                client.EndConnect(result);
                return true;
            }
            
            return false;
        }
        catch
        {
            return false;
        }
    }
}
