// <copyright file="YituliuApiService.cs" company="MaaAssistantArknights">
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
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.Services.Web;

/// <summary>
/// 一图流第三方 OpenAPI，用于读取干员练度数据。
/// 鉴权方式为请求头 Authorization 直接携带 token，业务结果看响应体 code 而非 HTTP 状态码。
/// </summary>
public static class YituliuApiService
{
    private const int CodeSuccess = 200;
    private const int CodeInsufficientPermissions = 20010;
    private const int CodeInvalidCredentials = 20027;

    public enum TokenValidationResult
    {
        /// <summary>token 有效且具备读取权限</summary>
        Valid,

        /// <summary>token 有效但只有写入权限，无法读取</summary>
        WriteOnly,

        /// <summary>token 无效或已失效</summary>
        Invalid,

        /// <summary>网络请求失败</summary>
        NetworkError,
    }

    public class OperatorInfoResponse
    {
        [JsonProperty("code")]
        public int Code { get; set; }

        [JsonProperty("msg")]
        public string Msg { get; set; } = string.Empty;

        [JsonProperty("data")]
        public List<OperatorInfo>? Data { get; set; }
    }

    public class OperatorInfo
    {
        [JsonProperty("id")]
        public string Id { get; set; } = null!; // char_002_amiya

        [JsonProperty("level")]
        public int Level { get; set; }

        [JsonProperty("evolvePhase")]
        public int EvolvePhase { get; set; }

        [JsonProperty("mainSkillLevel")]
        public int MainSkillLevel { get; set; }

        [JsonProperty("potentialRank")]
        public int PotentialRank { get; set; }

        [JsonProperty("skills")]
        public List<OperBoxData.SkillData>? Skills { get; set; }

        [JsonProperty("equips")]
        public List<OperBoxData.EquipData>? Equips { get; set; }
    }

    /// <summary>
    /// 验证 token 是否具备干员数据读取权限。
    /// </summary>
    /// <param name="token">第三方 OpenAPI token</param>
    /// <returns>验证结果与拥有干员数量（仅 Valid 时有意义）</returns>
    public static async Task<(TokenValidationResult Result, int OperatorCount)> ValidateTokenAsync(string token)
    {
        var (response, body) = await RequestOperatorInfoAsync(token).ConfigureAwait(false);
        if (response == null)
        {
            return (TokenValidationResult.NetworkError, 0);
        }

        return ParseValidationResult(body) switch {
            TokenValidationResult.Valid => (TokenValidationResult.Valid, body!.Data?.Count ?? 0),
            var result => (result, 0),
        };
    }

    /// <summary>
    /// 拉取干员练度数据。
    /// </summary>
    /// <param name="token">第三方 OpenAPI token</param>
    /// <returns>成功时返回干员列表，失败时返回验证结果供提示</returns>
    public static async Task<(TokenValidationResult Result, List<OperatorInfo>? Data)> GetOperatorInfoAsync(string token)
    {
        var (response, body) = await RequestOperatorInfoAsync(token).ConfigureAwait(false);
        if (response == null)
        {
            return (TokenValidationResult.NetworkError, null);
        }

        return ParseValidationResult(body) switch {
            TokenValidationResult.Valid => (TokenValidationResult.Valid, body!.Data),
            var result => (result, null),
        };
    }

    private static async Task<(HttpResponseMessage? Response, OperatorInfoResponse? Body)> RequestOperatorInfoAsync(string token)
    {
        try
        {
            var response = await Instances.HttpService.GetAsync(
                new Uri(MaaUrls.YituliuOpenApiOperatorInfo),
                extraHeader: new Dictionary<string, string> { ["Authorization"] = token }).ConfigureAwait(false);
            response.EnsureSuccessStatusCode();
            var json = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            return (response, JsonConvert.DeserializeObject<OperatorInfoResponse>(json));
        }
        catch (Exception e)
        {
            Log.Error("Failed to request yituliu open-api operator info: {Message}", e.Message);
            return (null, null);
        }
    }

    private static TokenValidationResult ParseValidationResult(OperatorInfoResponse? body)
    {
        return body?.Code switch {
            CodeSuccess => TokenValidationResult.Valid,
            CodeInsufficientPermissions => TokenValidationResult.WriteOnly,
            CodeInvalidCredentials => TokenValidationResult.Invalid,
            _ => TokenValidationResult.Invalid,
        };
    }
}
