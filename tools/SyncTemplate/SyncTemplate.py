#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
同步 template 目录结构
参考 resource/template（已重新组织为文件夹结构），更新 resource/global 下各语言的 resource/template

使用方法:
    python SyncTemplate.py

功能:
    1. 在 global 中创建参考目录的文件夹结构
    2. 根据文件名匹配参考目录中的路径，将 global 中散开的文件移动到对应文件夹
    3. 列出无法匹配的文件（参考目录中没有对应路径的文件）
"""

import os
import shutil
from pathlib import Path
from collections import defaultdict

# 基础路径（相对于脚本所在目录）
# 脚本位于 tools/SyncTemplate，需要向上两级到达项目根目录
SCRIPT_DIR = Path(__file__).parent.absolute()
BASE_DIR = SCRIPT_DIR.parent.parent
REF_TEMPLATE = BASE_DIR / "resource" / "template"
GLOBAL_DIR = BASE_DIR / "resource" / "global"

# 语言目录
LANGUAGES = ["txwy", "YoStarEN", "YoStarJP", "YoStarKR"]


def get_ref_file_mapping(directory):
    """获取参考目录的文件映射和统计信息"""
    # file_name_map: 文件名 -> 相对路径（只包含唯一出现的文件名）
    # file_name_count: 文件名 -> 出现次数
    # dirs: 所有目录的集合
    file_name_map = {}
    file_name_count = defaultdict(int)
    dirs = set()
    
    if not directory.exists():
        return file_name_map, file_name_count, dirs
    
    # 第一遍：统计每个文件名出现的次数
    for root, dirnames, filenames in os.walk(directory):
        root_path = Path(root)
        rel_root = root_path.relative_to(directory)
        
        # 添加目录
        if rel_root != Path('.'):
            dirs.add(str(rel_root))
        
        # 统计文件名出现次数
        for filename in filenames:
            file_name_count[filename] += 1
    
    # 第二遍：只记录唯一出现的文件名（出现次数为1）
    for root, dirnames, filenames in os.walk(directory):
        root_path = Path(root)
        rel_root = root_path.relative_to(directory)
        
        # 添加文件（只处理唯一出现的文件）
        for filename in filenames:
            if file_name_count[filename] == 1:
                rel_path = rel_root / filename
                rel_path_str = str(rel_path)
                file_name_map[filename] = rel_path_str
    
    return file_name_map, file_name_count, dirs


def find_all_files_by_name(directory, filename):
    """在目录中查找所有指定文件名的文件（包括子目录）"""
    found_files = []
    
    if not directory.exists():
        return found_files
    
    for file_path in directory.rglob(filename):
        if file_path.is_file():
            found_files.append(file_path)
    
    return found_files




def ensure_directory_exists(file_path):
    """确保文件所在的目录存在"""
    file_path.parent.mkdir(parents=True, exist_ok=True)


def sync_templates():
    """同步 template 目录结构"""
    print("=" * 80)
    print("同步 Template 目录结构")
    print("=" * 80)
    print()
    
    # 检查参考目录是否存在
    if not REF_TEMPLATE.exists():
        print(f"错误: 参考目录不存在: {REF_TEMPLATE}")
        return
    
    # 获取参考目录的文件映射和目录结构
    ref_file_map, ref_file_count, ref_dirs = get_ref_file_mapping(REF_TEMPLATE)
    
    # 统计唯一文件和重复文件
    unique_files = sum(1 for count in ref_file_count.values() if count == 1)
    duplicate_files = sum(1 for count in ref_file_count.values() if count > 1)
    
    print(f"参考目录: {REF_TEMPLATE}")
    print(f"  - 唯一文件（可移动）: {unique_files} 个")
    print(f"  - 重复文件（不移动）: {duplicate_files} 个")
    print(f"  - 目录: {len(ref_dirs)} 个")
    print()
    
    # 统计信息
    stats = {
        'moved_files': defaultdict(int),
        'created_dirs': defaultdict(int),
        'skipped_duplicate': defaultdict(list),
        'skipped_already_in_place': defaultdict(int),
        'not_found': defaultdict(list)
    }
    
    # 处理每个语言目录
    for lang in LANGUAGES:
        lang_template = GLOBAL_DIR / lang / "resource" / "template"
        
        if not lang_template.exists():
            print(f"警告: {lang_template} 不存在，跳过")
            continue
        
        print(f"处理 {lang}...")
        
        # 获取 global 目录中已有的目录结构
        lang_dirs = set()
        if lang_template.exists():
            for item in lang_template.rglob('*'):
                if item.is_dir():
                    rel_path = item.relative_to(lang_template)
                    if rel_path != Path('.'):
                        lang_dirs.add(str(rel_path))
        
        # 创建缺失的目录结构
        missing_dirs = ref_dirs - lang_dirs
        created_dirs = 0
        for dir_rel in missing_dirs:
            target_dir = lang_template / dir_rel
            if not target_dir.exists():
                target_dir.mkdir(parents=True, exist_ok=True)
                created_dirs += 1
        
        if created_dirs > 0:
            stats['created_dirs'][lang] = created_dirs
            print(f"  已创建 {created_dirs} 个目录")
        
        # 处理每个唯一文件（在参考目录中只出现一次的文件）
        moved_count = 0
        skipped_duplicate = []
        skipped_in_place = 0
        not_found = []
        
        for filename, target_rel_path in ref_file_map.items():
            # 跳过重复文件（在参考目录中出现多次）
            if ref_file_count[filename] > 1:
                skipped_duplicate.append(filename)
                continue
            
            # 在 global 目录中查找这个文件（可能在根目录，也可能在子目录）
            found_files = find_all_files_by_name(lang_template, filename)
            
            if not found_files:
                # 文件不存在，记录但不处理（会自动从根目录 resource 中寻找）
                not_found.append(filename)
                continue
            
            # 确定目标路径
            target_path = lang_template / target_rel_path
            
            # 如果找到多个同名文件，跳过不处理
            if len(found_files) > 1:
                skipped_duplicate.append(filename)
                continue
            
            # 如果文件已经在目标位置，跳过
            if target_path in found_files:
                skipped_in_place += 1
                continue
            
            # 如果目标位置已经有文件，跳过（不覆盖）
            if target_path.exists():
                skipped_in_place += 1
                continue
            
            # 只有一个文件，移动到目标位置
            file_to_move = found_files[0]
            
            # 确保目标目录存在
            ensure_directory_exists(target_path)
            
            # 移动文件
            shutil.move(str(file_to_move), str(target_path))
            moved_count += 1
        
        if moved_count > 0:
            stats['moved_files'][lang] = moved_count
            print(f"  已移动 {moved_count} 个文件到对应文件夹")
        
        if skipped_in_place > 0:
            stats['skipped_already_in_place'][lang] = skipped_in_place
            print(f"  跳过 {skipped_in_place} 个已在正确位置的文件")
        
        if skipped_duplicate:
            stats['skipped_duplicate'][lang] = skipped_duplicate
            print(f"  跳过 {len(skipped_duplicate)} 个重复文件（参考目录中有多个同名文件）")
            for f in sorted(skipped_duplicate)[:5]:
                print(f"    - {f}")
            if len(skipped_duplicate) > 5:
                print(f"    ... 还有 {len(skipped_duplicate) - 5} 个")
        
        if not_found:
            stats['not_found'][lang] = not_found
            print(f"  未找到 {len(not_found)} 个文件")
        
        if not created_dirs and not moved_count:
            print("  ✓ 无需更新")
        
        print()
    
    # 输出汇总报告
    print("=" * 80)
    print("汇总报告")
    print("=" * 80)
    
    total_moved = sum(stats['moved_files'].values())
    total_created = sum(stats['created_dirs'].values())
    total_skipped_duplicate = sum(len(files) for files in stats['skipped_duplicate'].values())
    total_skipped_in_place = sum(stats['skipped_already_in_place'].values())
    
    print(f"\n总计:")
    print(f"  - 创建目录: {total_created} 个")
    print(f"  - 移动文件: {total_moved} 个")
    print(f"  - 跳过重复文件: {total_skipped_duplicate} 个")
    print(f"  - 跳过已在正确位置: {total_skipped_in_place} 个")
    
    print("\n同步完成!")


if __name__ == "__main__":
    try:
        sync_templates()
    except KeyboardInterrupt:
        print("\n\n操作已取消")
    except Exception as e:
        print(f"\n错误: {e}")
        import traceback
        traceback.print_exc()
