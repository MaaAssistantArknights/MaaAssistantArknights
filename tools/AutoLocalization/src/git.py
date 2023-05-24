"""
Author       : UniMars
Date         : 2023-05-24 03:00:39
LastEditors  : UniMars
LastEditTime : 2023-05-24 03:18:47
Description  : file head
"""
import os.path
import subprocess


def get_latest_file_content(file_path='./cli.py', encoding='utf-8'):
    dirname = os.path.dirname(file_path)
    basename = os.path.basename(file_path)
    # 获取最近一次修改该文件的commit的ID
    result = subprocess.run(['git', 'rev-list', '-n', '1', 'tag-autolocalization', '--', basename],
                            cwd=dirname,
                            capture_output=True,
                            text=True)

    commit_id = result.stdout.replace('"', '').replace('\'', '').strip()
    heading = 'HEAD' if not commit_id else commit_id
    result = subprocess.run(['git', 'show', f'{heading}:./{basename}'],
                            cwd=dirname,
                            capture_output=True,
                            text=True,
                            encoding=encoding)
    return result.stdout


if __name__ == '__main__':
    print(get_latest_file_content('./cli.py'))
