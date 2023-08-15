import json
from pathlib import Path
from collections import OrderedDict

path = str(Path(__file__).parents[2]) + '/'

servers = {
    'EN': 'YoStarEN',
    'JP': 'YoStarJP',
    'KR': 'YoStarKR',
    'TW': 'txwy'
}

with open(path+'resource/tasks.json', 'r', encoding='UTF-8') as file:
    order = list(json.load(file).keys())
    print('CN', len(order))

for server, name in servers.items():
    with open(path+f'resource/global/{name}/resource/tasks.json', 'r', encoding='UTF-8') as file:
        tasks = json.load(file)
    tasks = OrderedDict(sorted(tasks.items(), key=lambda item:order.index(item[0]) if item[0] in order else -1))
    with open(path+f'resource/global/{name}/resource/tasks.json', 'w', encoding='UTF-8') as file:
        json.dump(tasks, file, indent=4, ensure_ascii=False)
    print(server, len(tasks))