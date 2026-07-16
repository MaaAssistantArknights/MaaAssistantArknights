#!/usr/bin/env python3
"""统计 MaaAssistantArknights / MaaRelease 两个仓库的 Release 下载量，生成 HTML 报告。

用法:
    python release_download_stats.py
    python release_download_stats.py --output report.html
    python release_download_stats.py --cache data.json --output report.html

依赖: gh CLI（需已 gh auth login）
"""

import argparse
import json
import subprocess
import sys
import shutil
import time
import re
from collections import defaultdict
from datetime import datetime
from pathlib import Path
from html import escape


def _find_gh() -> str:
    """查找 gh CLI 可执行文件路径。"""
    gh = shutil.which("gh")
    if gh:
        return gh
    # Windows 常见安装路径
    for candidate in [
        r"C:\Program Files\GitHub CLI\gh.exe",
        r"C:\Program Files (x86)\GitHub CLI\gh.exe",
        str(Path.home() / "AppData" / "Local" / "Programs" / "GitHub CLI" / "gh.exe"),
    ]:
        if Path(candidate).exists():
            return candidate
    return "gh"  # 回退，让系统报错


GH_EXECUTABLE = _find_gh()

# ============================================================
# 配置
# ============================================================

REPOS = [
    "MaaAssistantArknights/MaaAssistantArknights",
    "MaaAssistantArknights/MaaRelease",
]

PER_PAGE = 100
MAX_RETRIES = 3

# ============================================================
# 数据获取层
# ============================================================


def gh_api_paginate(repo: str, path: str = "releases") -> list:
    """通过 gh CLI 分页获取仓库的所有 Release 数据。"""
    all_items = []
    page = 1
    while True:
        url = f"repos/{repo}/{path}?per_page={PER_PAGE}&page={page}"
        data = _gh_api_with_retry(url)
        if not data:
            break
        all_items.extend(data)
        print(f"  [{repo}] 已获取 {len(all_items)} 条 (page {page})")
        if len(data) < PER_PAGE:
            break
        page += 1
    return all_items


def _gh_api_with_retry(url: str) -> list:
    """调用 gh api，失败自动重试 3 次，指数退避。"""
    for attempt in range(MAX_RETRIES):
        try:
            result = subprocess.run(
                [GH_EXECUTABLE, "api", url],
                capture_output=True,
                timeout=120,
                encoding="utf-8",
                errors="replace",
            )
            if result.returncode == 0 and result.stdout and result.stdout.strip():
                return json.loads(result.stdout)
            else:
                stderr = (result.stderr or "").strip()
                print(
                    f"  [警告] gh api 调用失败 (尝试 {attempt + 1}/{MAX_RETRIES}): {stderr[:200]}"
                )
        except subprocess.TimeoutExpired:
            print(f"  [警告] gh api 超时 (尝试 {attempt + 1}/{MAX_RETRIES})")
        except Exception as e:
            print(f"  [警告] gh api 异常 (尝试 {attempt + 1}/{MAX_RETRIES}): {e}")

        if attempt < MAX_RETRIES - 1:
            wait = 2**attempt  # 1s, 2s, 4s
            print(f"  等待 {wait}s 后重试...")
            time.sleep(wait)

    print(f"  [错误] gh api {url} 重试 {MAX_RETRIES} 次后仍失败，跳过")
    return []


def fetch_all_releases(use_cache: str | None = None) -> dict:
    """获取两个仓库的所有 Release 数据。支持缓存。"""
    if use_cache:
        cache_path = Path(use_cache)
        if cache_path.exists():
            print(f"从缓存读取数据: {cache_path}")
            with open(cache_path, "r", encoding="utf-8") as f:
                return json.load(f)

    result = {}
    for repo in REPOS:
        print(f"\n正在获取 {repo} 的 Release 数据...")
        releases = gh_api_paginate(repo)
        result[repo] = releases
        print(f"  共获取 {len(releases)} 个 Release")

    if use_cache:
        cache_path = Path(use_cache)
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        with open(cache_path, "w", encoding="utf-8") as f:
            json.dump(result, f, ensure_ascii=False, indent=2)
        print(f"数据已缓存到: {cache_path}")

    return result


# ============================================================
# Asset 分类层
# ============================================================

# 需要排除的 asset
EXCLUDE_PATTERNS = [
    re.compile(r"DebugSymbol", re.IGNORECASE),
    re.compile(r"^appcast\.xml$", re.IGNORECASE),
]

# OTA 包命名: MAAComponent-OTA-{from}_{to}-win-x64.zip
OTA_PATTERN = re.compile(r"^MAAComponent-OTA-(.+?)_(.+?)-win.*$")

# 完整包命名: MAA-{tag}-{platform}.{ext}
FULL_PKG_PATTERN = re.compile(
    r"^MAA-(.+?)-(win-x64|win-arm64|macos-universal|macos-runtime-universal|linux-x86_64|linux-aarch64)\.(zip|dmg|tar\.gz|AppImage)$"
)

# macOS delta 增量更新
DELTA_PATTERN = re.compile(r"^MAA\d+-\d+\.delta$")


def classify_asset(name: str) -> tuple[str, dict]:
    """分类 asset，返回 (类型, 额外信息)。被排除的返回 ('excluded', {})。"""
    for pat in EXCLUDE_PATTERNS:
        if pat.search(name):
            return ("excluded", {})

    m = OTA_PATTERN.match(name)
    if m:
        return ("ota", {"from_version": m.group(1), "to_version": m.group(2)})

    if DELTA_PATTERN.match(name):
        return ("delta", {})

    m = FULL_PKG_PATTERN.match(name)
    if m:
        platform = m.group(2)
        platform_map = {
            "win-x64": "Windows-x64",
            "win-arm64": "Windows-arm64",
            "macos-universal": "macOS",
            "macos-runtime-universal": "macOS",
            "linux-x86_64": "Linux-x86_64",
            "linux-aarch64": "Linux-aarch64",
        }
        return ("full", {"platform": platform_map.get(platform, platform)})

    return ("other", {})


# ============================================================
# 数据处理层
# ============================================================


def process_releases(raw_data: dict) -> dict:
    """处理原始 Release 数据，生成结构化的统计数据。"""
    # 合并后的 Release 列表: {tag -> release_info}
    merged_releases = {}

    for repo, releases in raw_data.items():
        for rel in releases:
            tag = rel.get("tag_name", "")
            if not tag:
                continue

            if tag not in merged_releases:
                merged_releases[tag] = {
                    "tag": tag,
                    "name": rel.get("name") or tag,
                    "published_at": rel.get("published_at")
                    or rel.get("created_at", ""),
                    "repo_sources": [],
                    "assets": {},
                    "total_downloads": 0,
                    "ota_sources": defaultdict(int),
                    "full_downloads": defaultdict(int),
                    "ota_downloads": 0,
                    "delta_downloads": 0,
                    "other_downloads": 0,
                }

            entry = merged_releases[tag]
            entry["repo_sources"].append(repo)

            for asset in rel.get("assets", []):
                aname = asset.get("name", "")
                count = asset.get("download_count", 0)

                atype, info = classify_asset(aname)
                if atype == "excluded":
                    continue

                # 同名 asset 合并下载量（主仓 + Release 仓）
                if aname in entry["assets"]:
                    entry["assets"][aname]["count"] += count
                else:
                    entry["assets"][aname] = {
                        "name": aname,
                        "count": count,
                        "type": atype,
                        "info": info,
                    }

    # 计算每个 Release 的汇总
    for tag, entry in merged_releases.items():
        total = 0
        for aname, a in entry["assets"].items():
            total += a["count"]
            if a["type"] == "ota":
                entry["ota_downloads"] += a["count"]
                from_v = a["info"].get("from_version", "unknown")
                entry["ota_sources"][from_v] += a["count"]
            elif a["type"] == "full":
                entry["full_downloads"][a["info"].get("platform", "unknown")] += a[
                    "count"
                ]
            elif a["type"] == "delta":
                entry["delta_downloads"] += a["count"]
            else:
                entry["other_downloads"] += a["count"]
        entry["total_downloads"] = total
        # defaultdict 转 dict 以便序列化
        entry["ota_sources"] = dict(entry["ota_sources"])
        entry["full_downloads"] = dict(entry["full_downloads"])

    # 按发布日期排序（旧 → 新）
    ordered_releases = sorted(
        merged_releases.values(),
        key=lambda r: r["published_at"] or "",
    )

    # 全局汇总
    total_all = sum(r["total_downloads"] for r in ordered_releases)
    total_full_sum = sum(sum(r["full_downloads"].values()) for r in ordered_releases)
    total_ota_sum = sum(r["ota_downloads"] for r in ordered_releases)
    total_delta_sum = sum(r["delta_downloads"] for r in ordered_releases)
    total_other_sum = sum(r["other_downloads"] for r in ordered_releases)

    # Top-20 Release（按总下载量排序）
    top_releases = sorted(
        [(r["tag"], r["total_downloads"]) for r in ordered_releases],
        key=lambda x: -x[1],
    )[:20]

    summary = {
        "total_releases": len(ordered_releases),
        "total_downloads": total_all,
        "full_downloads": total_full_sum,
        "ota_downloads": total_ota_sum,
        "delta_downloads": total_delta_sum,
        "other_downloads": total_other_sum,
        "per_platform": _aggregate_platforms(ordered_releases),
        "top_releases": top_releases,
    }

    return {
        "releases": ordered_releases,
        "summary": summary,
    }


def _aggregate_platforms(releases: list) -> dict:
    """聚合所有 Release 中按平台的完整包下载量。"""
    platform_totals = defaultdict(int)
    for r in releases:
        for platform, count in r["full_downloads"].items():
            platform_totals[platform] += count
    return dict(sorted(platform_totals.items(), key=lambda x: -x[1]))


# ============================================================
# HTML 报告生成层
# ============================================================


def generate_html(processed: dict, output_path: Path):
    """生成 HTML 报告。"""
    releases = processed["releases"]
    summary = processed["summary"]
    gen_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # --- 准备图表数据 ---
    # 包类型饼图
    pie_labels = ["完整包", "OTA 更新包", "macOS Delta", "其他"]
    pie_data = [
        summary["full_downloads"],
        summary["ota_downloads"],
        summary["delta_downloads"],
        summary["other_downloads"],
    ]

    # 平台分布柱状图
    platform_labels = list(summary["per_platform"].keys())
    platform_data = list(summary["per_platform"].values())

    # 累计下载趋势曲线（按发布日期正序：旧 → 新）
    # 只保留正式版和 beta 版，过滤 alpha / dev 内测版
    # 兼容多种历史版本号格式：
    #   v6.14.0, v6.14.0-beta.2, v2.9.0-beta, v1.3-beta.2  (v 前缀)
    #   release.stable.1.0, release.stable.1.1-beta.2       (早期 stable)
    #   release.beta.08.06, release.beta.01.2               (早期 beta)
    def is_release_or_beta(tag: str) -> bool:
        tag = tag.strip()
        # 排除 alpha / dev 内测版
        if "alpha" in tag.lower() or ".d0" in tag:
            return False
        # v 前缀格式: vX.Y.Z 或 vX.Y.Z-beta.N 或 vX.Y-beta 或 vX.Y-beta.N
        if re.match(r"^v\d+\.\d+[\.\d]*(-beta(\.\d+)?)?$", tag):
            return True
        # 早期 stable 格式
        if tag.startswith("release.stable."):
            return True
        # 早期 beta 格式
        if tag.startswith("release.beta."):
            return True
        return False

    trend_releases = [r for r in releases if is_release_or_beta(r["tag"])]
    trend_labels = [r["tag"] for r in trend_releases]
    trend_cumulative = []
    cum = 0
    for r in trend_releases:
        cum += r["total_downloads"]
        trend_cumulative.append(cum)

    # 正式版各版本下载量（非累计，按发布日期正序）
    def is_stable_only(tag: str) -> bool:
        tag = tag.strip()
        tag_lower = tag.lower()
        if "alpha" in tag_lower or "beta" in tag_lower or ".d0" in tag:
            return False
        if re.match(r"^v\d+\.\d+[\.\d]*$", tag):
            return True
        if tag.startswith("release.stable."):
            return True
        return False

    stable_releases = [r for r in releases if is_stable_only(r["tag"])]
    stable_labels = [r["tag"] for r in stable_releases]
    stable_counts = [r["total_downloads"] for r in stable_releases]

    # --- 构建 Release 明细表 ---
    rows_html = []
    for i, r in enumerate(reversed(releases)):  # 表格最新在最上面
        # OTA Top-5 来源
        ota_sources_sorted = sorted(r["ota_sources"].items(), key=lambda x: -x[1])
        ota_top5 = ota_sources_sorted[:5]
        ota_total = r["ota_downloads"]

        ota_bars = ""
        if ota_total > 0 and ota_top5:
            ota_bars = "<div class='ota-sources'>"
            for src, count in ota_top5:
                pct = (count / ota_total * 100) if ota_total > 0 else 0
                ota_bars += (
                    f"<div class='ota-bar-row'>"
                    f"<span class='ota-bar-label'>{escape(src)}</span>"
                    f"<div class='ota-bar-track'>"
                    f"<div class='ota-bar-fill' style='width:{pct:.1f}%'></div>"
                    f"</div>"
                    f"<span class='ota-bar-count'>{count:,} ({pct:.1f}%)</span>"
                    f"</div>"
                )
            ota_bars += "</div>"

        # asset 明细（可展开）
        assets_sorted = sorted(r["assets"].values(), key=lambda x: -x["count"])
        assets_html = ""
        if assets_sorted:
            assets_html = "<table class='asset-detail-table'><thead><tr><th>Asset</th><th>类型</th><th>下载量</th></tr></thead><tbody>"
            type_names = {
                "full": "完整包",
                "ota": "OTA",
                "delta": "Delta",
                "other": "其他",
            }
            for a in assets_sorted:
                assets_html += (
                    f"<tr><td>{escape(a['name'])}</td>"
                    f"<td>{type_names.get(a['type'], a['type'])}</td>"
                    f"<td class='num'>{a['count']:,}</td></tr>"
                )
            assets_html += "</tbody></table>"

        # 平台下载分布
        platform_cells = ""
        if r["full_downloads"]:
            platform_cells = " / ".join(
                f"{escape(p)}: {c:,}"
                for p, c in sorted(r["full_downloads"].items(), key=lambda x: -x[1])
            )

        pub_date = r["published_at"][:10] if r["published_at"] else "N/A"

        full_sum = sum(r["full_downloads"].values())
        # 判断版本渠道（带 .d0 的 dev 推进版一律为内测版）
        tag_lower = r["tag"].lower()
        is_dev = ".d0" in r["tag"]
        if is_dev or "alpha" in tag_lower:
            channel = "alpha"
        elif "beta" in tag_lower:
            channel = "beta"
        else:
            channel = "stable"
        rows_html.append(f"""
        <tr class="release-row" onclick="toggleDetail('detail-{i}')"
            data-version="{escape(r["tag"])}"
            data-date="{pub_date}"
            data-full="{full_sum}"
            data-ota="{r["ota_downloads"]}"
            data-total="{r["total_downloads"]}"
            data-channel="{channel}">
            <td class="col-version">{escape(r["tag"])}</td>
            <td class="col-date">{pub_date}</td>
            <td class="col-full num">{full_sum:,}</td>
            <td class="col-ota num">{r["ota_downloads"]:,}</td>
            <td class="col-total num">{r["total_downloads"]:,}</td>
            <td class="col-platform">{platform_cells}</td>
        </tr>
        <tr class="release-detail" id="detail-{i}" data-channel="{channel}" style="display:none">
            <td colspan="6">
                {ota_bars}
                {assets_html}
            </td>
        </tr>
        """)

    # Top-20 Release 柱状图数据（从高到低）
    top_rel_tags = [escape(tag) for tag, _ in summary["top_releases"]]
    top_rel_counts = [count for _, count in summary["top_releases"]]

    # 将图表数据嵌入 JSON
    chart_data = json.dumps(
        {
            "pie_labels": pie_labels,
            "pie_data": pie_data,
            "platform_labels": platform_labels,
            "platform_data": platform_data,
            "trend_labels": trend_labels,
            "trend_cumulative": trend_cumulative,
            "stable_labels": stable_labels,
            "stable_counts": stable_counts,
            "top_rel_tags": top_rel_tags,
            "top_rel_counts": top_rel_counts,
        },
        ensure_ascii=False,
    )

    html = f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MAA GitHub Release 下载量统计</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
    <style>
        :root {{
            --bg: #0d1117;
            --card-bg: #161b22;
            --border: #30363d;
            --text: #e6edf3;
            --text-muted: #8b949e;
            --accent: #58a6ff;
            --accent-green: #3fb950;
            --accent-orange: #d29922;
            --accent-purple: #bc8cff;
            --accent-red: #f85149;
        }}
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif;
            background: var(--bg);
            color: var(--text);
            line-height: 1.6;
            padding: 20px;
        }}
        h1 {{ text-align: center; margin: 20px 0 5px; font-size: 1.8em; }}
        .subtitle {{ text-align: center; color: var(--text-muted); margin-bottom: 30px; font-size: 0.9em; }}
        .summary-cards {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 30px;
        }}
        .card {{
            background: var(--card-bg);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 20px;
            text-align: center;
        }}
        .card .value {{ font-size: 2em; font-weight: 700; color: var(--accent); }}
        .card .label {{ color: var(--text-muted); font-size: 0.85em; margin-top: 4px; }}
        .card.green .value {{ color: var(--accent-green); }}
        .card.orange .value {{ color: var(--accent-orange); }}
        .card.purple .value {{ color: var(--accent-purple); }}
        .chart-section {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 30px;
        }}
        .chart-card {{
            background: var(--card-bg);
            border: 1px solid var(--border);
            border-radius: 12px;
            padding: 20px;
        }}
        .chart-card h3 {{ margin-bottom: 15px; font-size: 1.1em; }}
        .chart-card canvas {{ max-height: 300px; }}
        .full-chart {{ grid-column: 1 / -1; }}
        .section-title {{
            font-size: 1.3em;
            margin: 30px 0 15px;
            padding-bottom: 8px;
            border-bottom: 1px solid var(--border);
        }}
        table {{ width: 100%; border-collapse: collapse; }}
        th, td {{ padding: 8px 12px; text-align: left; }}
        thead th {{
            background: #21262d;
            color: var(--text-muted);
            font-size: 0.85em;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            position: sticky;
            top: 0;
        }}
        .release-table-container {{
            overflow-x: auto;
            max-height: 600px;
            overflow-y: auto;
            border: 1px solid var(--border);
            border-radius: 8px;
            scrollbar-width: thin;
            scrollbar-color: var(--border) transparent;
        }}
        .release-table-container::-webkit-scrollbar {{
            width: 8px;
            height: 8px;
        }}
        .release-table-container::-webkit-scrollbar-track {{
            background: transparent;
        }}
        .release-table-container::-webkit-scrollbar-thumb {{
            background-color: #30363d;
            border-radius: 4px;
            border: 2px solid transparent;
            background-clip: content-box;
        }}
        .release-table-container::-webkit-scrollbar-thumb:hover {{
            background-color: #484f58;
        }}
        .release-row {{ cursor: pointer; border-bottom: 1px solid var(--border); transition: background 0.15s; }}
        .release-row:hover {{ background: #1f2937; }}
        /* 展开详情时数据行钉在表头下方 */
        .release-row.sticky-active {{
            position: sticky;
            top: 40px;
            background: #161b22;
            z-index: 2;
            box-shadow: 0 1px 0 var(--border);
        }}
        .release-detail {{ background: #0d1117; }}
        .release-detail > td {{ padding: 15px 20px; }}
        .col-version {{ font-weight: 600; color: var(--accent); }}
        .num {{ text-align: right; font-variant-numeric: tabular-nums; }}
        .ota-sources {{ margin-bottom: 12px; }}
        .ota-sources::before {{ content: "OTA 升级来源 Top-5"; display: block; font-weight: 600; margin-bottom: 8px; color: var(--text-muted); font-size: 0.9em; }}
        .ota-bar-row {{ display: flex; align-items: center; gap: 8px; margin: 4px 0; font-size: 0.85em; }}
        .ota-bar-label {{ min-width: 200px; text-align: right; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }}
        .ota-bar-track {{ flex: 1; background: #21262d; border-radius: 4px; height: 18px; overflow: hidden; }}
        .ota-bar-fill {{ background: linear-gradient(90deg, var(--accent), var(--accent-purple)); height: 100%; border-radius: 4px; transition: width 0.3s; }}
        .ota-bar-count {{ min-width: 120px; color: var(--text-muted); }}
        .asset-detail-table {{ width: 100%; margin-top: 12px; font-size: 0.85em; }}
        .asset-detail-table th {{ background: transparent; }}
        .top-asset-table {{ width: 100%; margin-top: 12px; }}
        .top-asset-table th {{ background: #21262d; }}
        .sortable {{ cursor: pointer; user-select: none; }}
        .sortable:hover {{ color: var(--accent); }}
        .sort-arrow {{ font-size: 0.8em; opacity: 0.6; }}
        .sort-arrow.asc::after {{ content: ' ▲'; }}
        .sort-arrow.desc::after {{ content: ' ▼'; }}
        .filter-bar {{ display: flex; gap: 8px; margin-bottom: 12px; flex-wrap: wrap; }}
        .filter-btn {{
            background: var(--card-bg);
            border: 1px solid var(--border);
            color: var(--text-muted);
            padding: 6px 16px;
            border-radius: 20px;
            cursor: pointer;
            font-size: 0.85em;
            transition: all 0.15s;
        }}
        .filter-btn:hover {{ border-color: var(--accent); color: var(--text); }}
        .filter-btn.active {{ background: var(--accent); border-color: var(--accent); color: #fff; font-weight: 600; }}
        .close-detail-btn {{
            background: var(--card-bg);
            border: 1px solid var(--border);
            color: var(--text-muted);
            padding: 4px 12px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.8em;
        }}
        .close-detail-btn:hover {{ border-color: var(--accent); color: var(--accent); }}
        .close-detail-btn {{
            background: var(--card-bg);
            border: 1px solid var(--border);
            color: var(--text-muted);
            padding: 4px 12px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 0.8em;
        }}
        .close-detail-btn:hover {{ border-color: var(--accent); color: var(--accent); }}
        @media (max-width: 768px) {{
            .chart-section {{ grid-template-columns: 1fr; }}
        }}
    </style>
</head>
<body>
    <h1>MAA GitHub Release 下载量统计</h1>
    <p class="subtitle">
        仅统计 GitHub Release 下载量，不包含其他分发渠道（如镜像站、包管理器等）&nbsp;|&nbsp;
        数据来源: MaaAssistantArknights/MaaAssistantArknights + MaaAssistantArknights/MaaRelease &nbsp;|&nbsp;
        生成时间: {gen_time}
    </p>

    <!-- 汇总卡片 -->
    <div class="summary-cards">
        <div class="card">
            <div class="value">{summary["total_downloads"]:,}</div>
            <div class="label">总下载量</div>
        </div>
        <div class="card green">
            <div class="value">{summary["full_downloads"]:,}</div>
            <div class="label">完整包下载量</div>
        </div>
        <div class="card orange">
            <div class="value">{summary["ota_downloads"]:,}</div>
            <div class="label">OTA 更新包下载量</div>
        </div>
        <div class="card purple">
            <div class="value">{summary["total_releases"]:,}</div>
            <div class="label">Release 总数</div>
        </div>
    </div>

    <!-- 图表区域 -->
    <div class="chart-section">
        <div class="chart-card">
            <h3>📦 包类型占比</h3>
            <canvas id="pieChart"></canvas>
        </div>
        <div class="chart-card">
            <h3>🖥️ 完整包平台分布</h3>
            <canvas id="platformChart"></canvas>
        </div>
        <div class="chart-card full-chart">
            <h3>📈 累计下载量趋势</h3>
            <canvas id="trendChart"></canvas>
        </div>
        <div class="chart-card full-chart">
            <h3>📊 正式版各版本下载量</h3>
            <canvas id="stableChart"></canvas>
        </div>
    </div>

    <!-- Release 明细表 -->
    <h2 class="section-title">📋 Release 明细</h2>
    <p class="subtitle" style="text-align:left; margin-bottom:10px;">点击行展开查看 OTA 升级来源 Top-5 和 Asset 详情；点击表头排序</p>
    <div class="filter-bar">
        <button class="filter-btn" data-filter="stable">正式版</button>
        <button class="filter-btn" data-filter="beta">公测版</button>
        <button class="filter-btn" data-filter="alpha">内测版</button>
    </div>
    <div class="release-table-container">
        <table id="releaseTable">
            <thead>
                <tr>
                    <th>版本</th>
                    <th class="sortable" data-sort-key="date">发布日期 <span class="sort-arrow"></span></th>
                    <th class="sortable num" data-sort-key="full">完整包 <span class="sort-arrow"></span></th>
                    <th class="sortable num" data-sort-key="ota">OTA <span class="sort-arrow"></span></th>
                    <th class="sortable num" data-sort-key="total">合计 <span class="sort-arrow"></span></th>
                    <th>平台分布</th>
                </tr>
            </thead>
            <tbody id="releaseTableBody">
                {"".join(rows_html)}
            </tbody>
        </table>
    </div>

    <!-- Top-20 Release 下载量柱状图 -->
    <h2 class="section-title">🏆 下载量 Top-20 Release</h2>
    <div class="chart-card full-chart">
        <canvas id="topRelChart"></canvas>
    </div>

    <p class="subtitle" style="margin-top:30px;">
        本报告由 release_download_stats.py 自动生成
    </p>

    <script>
        const chartData = {chart_data};

        // 通用样式
        Chart.defaults.color = '#8b949e';
        Chart.defaults.borderColor = '#30363d';
        Chart.defaults.font.family = '-apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif';

        // 包类型饼图
        new Chart(document.getElementById('pieChart'), {{
            type: 'doughnut',
            data: {{
                labels: chartData.pie_labels,
                datasets: [{{
                    data: chartData.pie_data,
                    backgroundColor: ['#3fb950', '#d29922', '#bc8cff', '#f85149'],
                    borderWidth: 2,
                    borderColor: '#161b22',
                }}]
            }},
            options: {{
                responsive: true,
                plugins: {{
                    legend: {{ position: 'bottom' }},
                    tooltip: {{
                        callbacks: {{
                            label: ctx => ` ${{ctx.label}}: ${{ctx.raw.toLocaleString()}} (${{(ctx.raw/chartData.pie_data.reduce((a,b)=>a+b,0)*100).toFixed(1)}}%)`
                        }}
                    }}
                }}
            }}
        }});

        // 平台分布饼图
        const platformColors = ['#58a6ff', '#3fb950', '#bc8cff', '#d29922', '#f85149', '#ff7b72'];
        new Chart(document.getElementById('platformChart'), {{
            type: 'doughnut',
            data: {{
                labels: chartData.platform_labels,
                datasets: [{{
                    data: chartData.platform_data,
                    backgroundColor: platformColors.slice(0, chartData.platform_labels.length),
                    borderWidth: 2,
                    borderColor: '#161b22',
                }}]
            }},
            options: {{
                responsive: true,
                plugins: {{
                    legend: {{ position: 'bottom' }},
                    tooltip: {{
                        callbacks: {{
                            label: ctx => ` ${{ctx.label}}: ${{ctx.raw.toLocaleString()}} (${{(ctx.raw/chartData.platform_data.reduce((a,b)=>a+b,0)*100).toFixed(1)}}%)`
                        }}
                    }}
                }}
            }}
        }});

        // 累计趋势曲线
        new Chart(document.getElementById('trendChart'), {{
            type: 'line',
            data: {{
                labels: chartData.trend_labels,
                datasets: [{{
                    data: chartData.trend_cumulative,
                    borderColor: '#58a6ff',
                    backgroundColor: 'rgba(88,166,255,0.1)',
                    fill: true,
                    pointRadius: 0,
                    borderWidth: 2,
                }}]
            }},
            options: {{
                responsive: true,
                interaction: {{ mode: 'index', intersect: false }},
                plugins: {{ legend: {{ display: false }} }},
                scales: {{
                    x: {{
                        grid: {{ display: false }},
                        ticks: {{ maxTicksLimit: 15, maxRotation: 45 }}
                    }},
                    y: {{
                        grid: {{ color: '#21262d' }},
                        ticks: {{ callback: v => v.toLocaleString() }}
                    }}
                }}
            }}
        }});

        // 正式版各版本下载量折线图
        new Chart(document.getElementById('stableChart'), {{
            type: 'line',
            data: {{
                labels: chartData.stable_labels,
                datasets: [{{
                    data: chartData.stable_counts,
                    borderColor: '#3fb950',
                    backgroundColor: 'rgba(63,185,80,0.1)',
                    fill: true,
                    pointRadius: 2,
                    pointHoverRadius: 5,
                    borderWidth: 2,
                }}]
            }},
            options: {{
                responsive: true,
                interaction: {{ mode: 'index', intersect: false }},
                plugins: {{ legend: {{ display: false }} }},
                scales: {{
                    x: {{
                        grid: {{ display: false }},
                        ticks: {{ maxTicksLimit: 20, maxRotation: 45 }}
                    }},
                    y: {{
                        grid: {{ color: '#21262d' }},
                        ticks: {{ callback: v => v.toLocaleString() }}
                    }}
                }}
            }}
        }});

        // Top-20 Release 水平柱状图
        const topRelMax = Math.max(...chartData.top_rel_counts);
        new Chart(document.getElementById('topRelChart'), {{
            type: 'bar',
            data: {{
                labels: chartData.top_rel_tags,
                datasets: [{{
                    data: chartData.top_rel_counts,
                    backgroundColor: chartData.top_rel_tags.map((_, i) => `hsl(${{210 + i * 8}}, 70%, ${{60 - i * 1.2}}%)`),
                    borderRadius: 4,
                    barPercentage: 0.85,
                    categoryPercentage: 0.95,
                }}]
            }},
            options: {{
                responsive: true,
                indexAxis: 'y',
                layout: {{ padding: 0 }},
                plugins: {{
                    legend: {{ display: false }},
                    tooltip: {{
                        callbacks: {{
                            label: ctx => ' ' + ctx.raw.toLocaleString() + ' 次下载'
                        }}
                    }}
                }},
                scales: {{
                    x: {{
                        max: topRelMax,
                        grid: {{ color: '#21262d' }},
                        ticks: {{ callback: v => v.toLocaleString() }},
                        border: {{ display: false }},
                    }},
                    y: {{
                        grid: {{ display: false }},
                        border: {{ display: false }},
                        ticks: {{
                            autoSkip: false,
                            padding: 0,
                        }},
                        afterFit: scale => {{ scale.width = 80; }},
                    }}
                }}
            }}
        }});

        // 展开折叠
        // 滚动时自动管理 sticky：只有当详情内容超出可视区域时才保持冻结
        const tableContainer = document.querySelector('.release-table-container');
        let stickyDetailId = null;
        tableContainer.addEventListener('scroll', () => {{
            if (!stickyDetailId) return;
            const dataRow = document.querySelector(`tr[onclick="toggleDetail('${{stickyDetailId}}')"]`);
            if (!dataRow) return;
            const detail = document.getElementById(stickyDetailId);
            if (!detail || detail.style.display === 'none') return;

            // 用 getBoundingClientRect 相对于容器判断
            const containerRect = tableContainer.getBoundingClientRect();
            const detailRect = detail.getBoundingClientRect();
            // 详情底部是否超出了容器可视底部（留 10px 余量）
            const detailBottomBeyondView = detailRect.bottom > containerRect.bottom + 10;

            if (detailBottomBeyondView) {{
                dataRow.classList.add('sticky-active');
            }} else {{
                dataRow.classList.remove('sticky-active');
            }}
        }});

        function toggleDetail(id) {{
            const el = document.getElementById(id);
            const dataRow = el.previousElementSibling;
            const willShow = el.style.display === 'none';
            el.style.display = willShow ? '' : 'none';
            if (willShow) {{
                stickyDetailId = id;
                dataRow.classList.add('sticky-active');
            }} else {{
                stickyDetailId = null;
                dataRow.classList.remove('sticky-active');
            }}
        }}

        // 版本渠道筛选（多选）— 用 CSS 规则控制显隐，不逐行操作 DOM
        const activeFilters = new Set();
        // 动态创建一条 style 规则用于筛选
        const filterStyle = document.createElement('style');
        document.head.appendChild(filterStyle);
        function applyFilter() {{
            if (activeFilters.size === 0) {{
                filterStyle.textContent = '';
                return;
            }}
            // 构建选择器：隐藏未被选中的渠道
            const allChannels = ['stable', 'beta', 'alpha'];
            const hidden = allChannels.filter(c => !activeFilters.has(c));
            if (hidden.length === 0) {{
                filterStyle.textContent = '';
            }} else {{
                const selectors = hidden.map(c => `#releaseTableBody > tr[data-channel="${{c}}"]`);
                filterStyle.textContent = selectors.join(',') + ' {{ display: none !important; }}';
            }}
        }}
        document.querySelectorAll('.filter-btn').forEach(btn => {{
            btn.addEventListener('click', () => {{
                const filter = btn.dataset.filter;
                if (activeFilters.has(filter)) {{
                    activeFilters.delete(filter);
                    btn.classList.remove('active');
                }} else {{
                    activeFilters.add(filter);
                    btn.classList.add('active');
                }}
                applyFilter();
            }});
        }});

        // 表头点击排序
        const numericKeys = new Set(['full', 'ota', 'total']);
        let sortState = {{ key: null, desc: false }};

        // 预缓存行对（仅收集一次）
        const tbody = document.getElementById('releaseTableBody');
        const allPairs = [];
        const _allRows = tbody.querySelectorAll(':scope > tr');
        for (let i = 0; i < _allRows.length; i += 2) {{
            if (_allRows[i].classList.contains('release-row') && _allRows[i + 1]) {{
                allPairs.push([_allRows[i], _allRows[i + 1]]);
            }}
        }}

        document.querySelectorAll('.sortable').forEach(th => {{
            th.addEventListener('click', () => {{
                const key = th.dataset.sortKey;
                // 切换排序方向
                if (sortState.key === key) {{
                    sortState.desc = !sortState.desc;
                }} else {{
                    sortState.key = key;
                    sortState.desc = numericKeys.has(key);
                }}

                // 更新箭头
                document.querySelectorAll('.sort-arrow').forEach(a => a.className = 'sort-arrow');
                const arrow = th.querySelector('.sort-arrow');
                arrow.className = sortState.desc ? 'sort-arrow desc' : 'sort-arrow asc';

                // 排序缓存的行对
                const pairs = allPairs.slice();
                pairs.sort((a, b) => {{
                    let va = a[0].dataset[key];
                    let vb = b[0].dataset[key];
                    let cmp;
                    if (numericKeys.has(key)) {{
                        cmp = parseFloat(va) - parseFloat(vb);
                    }} else {{
                        cmp = va.localeCompare(vb);
                    }}
                    return sortState.desc ? -cmp : cmp;
                }});

                // 一次性重建 tbody 内容
                tbody.textContent = '';
                const frag = document.createDocumentFragment();
                for (const [dataRow, detailRow] of pairs) {{
                    frag.appendChild(dataRow);
                    frag.appendChild(detailRow);
                }}
                tbody.appendChild(frag);
            }});
        }});
    </script>
</body>
</html>"""

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(html)
    print(f"\n报告已生成: {output_path}")


# ============================================================
# 主入口
# ============================================================


def main():
    parser = argparse.ArgumentParser(description="MAA Release 下载量统计 → HTML 报告")
    parser.add_argument(
        "--cache",
        type=str,
        default=None,
        help="缓存文件路径。存在则直接读取，不存在则拉取数据后写入缓存。",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=str,
        default=None,
        help="输出 HTML 文件路径（默认: report.html）",
    )
    args = parser.parse_args()

    cur_dir = Path(__file__).parent
    cache_path = args.cache if args.cache else None
    output_path = Path(args.output) if args.output else cur_dir / "report.html"

    # 获取数据
    raw_data = fetch_all_releases(use_cache=cache_path)

    # 处理数据
    print("\n正在处理数据...")
    processed = process_releases(raw_data)

    # 生成报告
    print("\n正在生成 HTML 报告...")
    generate_html(processed, output_path)

    # 打印摘要
    s = processed["summary"]
    print(f"\n{'=' * 50}")
    print(f"  Release 总数: {s['total_releases']}")
    print(f"  总下载量:     {s['total_downloads']:,}")
    print(f"  完整包:       {s['full_downloads']:,}")
    print(f"  OTA 更新包:   {s['ota_downloads']:,}")
    print(f"  macOS Delta:  {s['delta_downloads']:,}")
    print(f"  其他:         {s['other_downloads']:,}")
    print(f"{'=' * 50}")


if __name__ == "__main__":
    main()
