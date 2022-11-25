import os

all_headers = {}

for root, dirs, files in os.walk("."):
    if root.startswith('.'):
        root = root[1:]
    root = root.replace('\\', '/')
    if root.startswith('/'):
        root = root[1:]
    if root and not root.endswith('/'):
        root += '/'
    for f in files:
        if f.endswith(('.h', '.hpp')):
            all_headers[f] = root

print(all_headers)

for root, dirs, files in os.walk("."):
    if root.startswith('.'):
        root = root[1:]
    root = root.replace('\\', '/')
    if root.startswith('/'):
        root = root[1:]
    if root and not root.endswith('/'):
        root += '/'
    for f in files:
        if not f.endswith(('.h', '.hpp', '.c', '.cpp', '.cc')):
            continue
        temp = ''
        with open(root + f, encoding='utf-8') as fp:
            for line in fp.readlines():
                if line.startswith('#include') and '<' not in line:
                    if '/' in line:
                        header = line[line.rindex('/') + 1: len(line) - 2]
                    else:
                        header = line[line.index('"') + 1: len(line) - 2]
                    if header in all_headers:
                        new_header = all_headers[header].replace(root, '')
                        new_line = f"#include \"{new_header}{header}\"\n"
                        print(new_line)
                        temp += new_line
                    else:
                        temp += line
                else:
                    temp += line

        with open(root + f, mode='w', encoding='utf-8') as fp:
            fp.write(temp)
