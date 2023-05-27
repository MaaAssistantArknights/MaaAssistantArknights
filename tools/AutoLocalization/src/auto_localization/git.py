import os.path
import subprocess


def get_latest_file_content(file_path='./cli.py', encoding='utf-8', tag_name=""):
    dirname = os.path.dirname(file_path)
    basename = os.path.basename(file_path)
    # 获取最近一次修改该文件的commit的ID
    heading = 'HEAD'
    if tag_name:
        result = subprocess.run(['git', 'rev-list', '-n', '1', 'tag-autolocalization', '--', basename],
                                cwd=dirname,
                                capture_output=True,
                                text=True)
        commit_id = result.stdout.replace('"', '').replace('\'', '').strip()
        heading = commit_id if commit_id else heading
    result = subprocess.run(['git', 'show', f'{heading}:./{basename}'],
                            cwd=dirname,
                            capture_output=True,
                            text=True,
                            encoding=encoding)
    return result.stdout


if __name__ == '__main__':
    print(get_latest_file_content('cli.py'))
