// <copyright file="FaultTolerantRootConverter.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using Serilog;

namespace MaaWpfGui.Configuration.Converter;

/// <summary>
/// 根级别容错转换器：反序列化 <see cref="Root"/> 时，
/// 若遇到可恢复的 JSON 类型转换错误，则删除对应属性后重试。
/// </summary>
/// <remarks>
/// 叶子节点上的 <see cref="TolerantEnumConverter{TEnum}"/> 会把未知枚举值转换为带路径信息的
/// <see cref="JsonException"/>；此转换器据此移除对应属性，使对象回退到声明时的默认值。
/// </remarks>
internal sealed class FaultTolerantRootConverter : JsonConverter<Root>
{
    private readonly ILogger _logger = Log.ForContext<FaultTolerantRootConverter>();

    public override Root? Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
    {
        if (reader.TokenType != JsonTokenType.StartObject)
        {
            throw new JsonException("Expected StartObject token");
        }

        using var jsonDoc = JsonDocument.ParseValue(ref reader);
        string json = jsonDoc.RootElement.GetRawText();

        JsonNode? node = JsonNode.Parse(json);

        if (node is not JsonObject rootObj)
        {
            throw new JsonException("Root configuration JSON must be a JSON object.");
        }

        const int maxRetryCount = 20;

        for (int retry = 0; retry <= maxRetryCount; retry++)
        {
            try
            {
                var obj = JsonSerializer.Deserialize<Root>(
                    rootObj.ToJsonString(),
                    GetOptionsWithoutThisConverter(options));

                if (retry > 0)
                {
                    _logger.Information(
                        "Deserialization succeeded after {RetryCount} recovery attempt(s)",
                        retry);
                }

                return obj;
            }
            catch (JsonException ex) when (IsRecoverableTypeConversionFailure(ex))
            {
                if (retry == maxRetryCount)
                {
                    _logger.Error(
                        ex,
                        "Exceeded maximum retry count while removing properties with invalid values");

                    throw;
                }

                if (!TryRemovePropertyByMetadataPath(rootObj, ex.Path))
                {
                    throw;
                }

                _logger.Warning(
                    ex,
                    "Invalid value in configuration JSON at path {Path}; removed property and retrying ({Retry}/{MaxRetry})",
                    ex.Path,
                    retry + 1,
                    maxRetryCount);
            }
        }

        throw new JsonException("Unexpected deserialization failure.");
    }

    public override void Write(Utf8JsonWriter writer, Root value, JsonSerializerOptions options)
    {
        JsonSerializer.Serialize(writer, value, GetOptionsWithoutThisConverter(options)); // 使用默认序列化
    }

    private static bool IsRecoverableTypeConversionFailure(JsonException ex)
    {
        if (string.IsNullOrWhiteSpace(ex.Path))
        {
            return false;
        }

        var message = ex.Message;
        return message.Contains("could not be converted", StringComparison.OrdinalIgnoreCase);
    }

    private static bool TryRemovePropertyByMetadataPath(JsonObject root, string? path)
    {
        if (string.IsNullOrEmpty(path))
        {
            return false;
        }

        var segments = new List<string>();
        if (!TryParseJsonExceptionPath(path, segments) || segments.Count == 0)
        {
            return false;
        }

        JsonNode? parent = root;
        for (int i = 0; i < segments.Count - 1; i++)
        {
            parent = NavigateJsonPathSegment(parent, segments[i]);
            if (parent is null)
            {
                return false;
            }
        }

        var leaf = segments[^1];
        return parent is JsonObject leafObject && leafObject.Remove(leaf);
    }

    private static JsonNode? NavigateJsonPathSegment(JsonNode? current, string segment)
    {
        return current switch {
            JsonObject obj => obj[segment],
            JsonArray arr when int.TryParse(segment, out var idx) && idx >= 0 && idx < arr.Count => arr[idx],
            _ => null,
        };
    }

    /// <summary>
    /// 解析 <see cref="JsonException.Path"/> 风格路径（如 <c>$.Configurations.default.TaskQueue[0].Stage</c>）。
    /// </summary>
    private static bool TryParseJsonExceptionPath(string path, List<string> segments)
    {
        segments.Clear();
        if (path.Length == 0 || path[0] != '$')
        {
            return false;
        }

        var i = 1;
        if (i < path.Length && path[i] == '.')
        {
            i++;
        }

        while (i < path.Length)
        {
            if (path[i] == '.')
            {
                i++;
                continue;
            }

            if (path[i] == '[')
            {
                i++;
                if (i >= path.Length)
                {
                    return false;
                }

                if (path[i] == '\'' || path[i] == '"')
                {
                    var q = path[i++];
                    var start = i;
                    while (i < path.Length && path[i] != q)
                    {
                        i++;
                    }

                    if (i >= path.Length)
                    {
                        return false;
                    }

                    segments.Add(path.Substring(start, i - start));
                    i++;
                    if (i < path.Length && path[i] == ']')
                    {
                        i++;
                    }
                }
                else
                {
                    var start = i;
                    while (i < path.Length && path[i] != ']')
                    {
                        i++;
                    }

                    if (i >= path.Length)
                    {
                        return false;
                    }

                    segments.Add(path.Substring(start, i - start).Trim());
                }

                if (i < path.Length && path[i] == ']')
                {
                    i++;
                }
            }
            else
            {
                var start = i;
                while (i < path.Length && path[i] != '.' && path[i] != '[')
                {
                    i++;
                }

                var name = path.Substring(start, i - start);
                if (name.Length > 0)
                {
                    segments.Add(name);
                }
            }
        }

        return segments.Count > 0;
    }

    /// <summary>
    /// 获取不包含当前 Converter 的 JsonSerializerOptions，避免无限递归
    /// </summary>
    private static JsonSerializerOptions GetOptionsWithoutThisConverter(JsonSerializerOptions options)
    {
        var newOptions = new JsonSerializerOptions(options);
        for (int i = newOptions.Converters.Count - 1; i >= 0; i--)
        {
            if (newOptions.Converters[i] is FaultTolerantRootConverter)
            {
                newOptions.Converters.RemoveAt(i);
            }
        }
        return newOptions;
    }
}
