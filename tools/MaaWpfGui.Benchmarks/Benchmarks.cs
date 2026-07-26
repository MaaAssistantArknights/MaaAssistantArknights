// -----------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// -----------------------------------------------------------------------

using BenchmarkDotNet.Attributes;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MaaWpfGui.Benchmarks;

// For more information on the VS BenchmarkDotNet Diagnosers see https://learn.microsoft.com/visualstudio/profiling/profiling-with-benchmark-dotnet
/*
[CPUUsageDiagnoser]
public class Benchmarks
{
    private SHA256 sha256 = SHA256.Create();
    private byte[] data;

    [GlobalSetup]
    public void Setup()
    {
        data = new byte[10000];
        new Random(42).NextBytes(data);
    }

    [Benchmark]
    public byte[] Sha256()
    {
        return sha256.ComputeHash(data);
    }
}*/
// 重点看：Mean / Allocated / Gen0
[MemoryDiagnoser]
public class Benchmarks
{
    public sealed class LogItem
    {
        public string Time { get; init; } = "07:45:34";
        public string Color { get; init; } = "Info";
        public string Content { get; init; } = "Sample log content";
    }

    [Params(50, 200, 1000)]
    public int N { get; set; }

    private List<LogItem> _items = null!;

    [GlobalSetup]
    public void Setup()
    {
        _items = Enumerable.Range(1, N)
            .Select(i => new LogItem { Content = $"Line {i}" })
            .ToList();
    }

    // 你当前风格：Aggregate + 字符串相加
    [Benchmark(Baseline = true)]
    public string AggregatePlus()
        => _items.Aggregate(
            string.Empty,
            (current, logItem) => current + $"[{logItem.Time}][{logItem.Color}]{logItem.Content}\n");

    // 候选写法：Join + Select
    [Benchmark]
    public string JoinSelect()
        => string.Join("\n", _items.Select(logItem => $"[{logItem.Time}][{logItem.Color}]{logItem.Content}"));

    // 备用：StringBuilder
    [Benchmark]
    public string StringBuilderForeach()
    {
        var sb = new StringBuilder();
        foreach (var logItem in _items) {
            sb.Append('[').Append(logItem.Time).Append(']')
              .Append('[').Append(logItem.Color).Append(']')
              .Append(logItem.Content).Append('\n');
        }

        return sb.ToString();
    }
}
