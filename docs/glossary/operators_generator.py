# -*- coding: utf-8 -*-
import re
import requests

OPERATOR_URL = 'https://api.prts.plus/arknights/operator?server='
SERVERS: list[str] = ['cn', 'en', 'ja', 'ko']
OUTPUT_FILE = 'operators.md'

operators: dict[int, dict[int, dict[str, str]]] = {}  # star, id, server, name

for server in SERVERS:
    r = requests.get(OPERATOR_URL + server)
    server_data = r.json()
    if server_data['status_code'] != 200:
        continue

    operator_data: list[dict[str, str]] = server_data['data']
    for operator in operator_data:
        m = re.match(r'^char_(\d+).*$', operator['id'])  # char_999_something
        if not m:
            continue
        id = int(m.group(1))

        star: int = operator['star']
        operators[star] = operators.get(star, {})
        operators[star][id] = operators[star].get(id, {})
        operators[star][id][server] = operator['name']

# Output
with open(OUTPUT_FILE, 'w', encoding='utf-8') as fp:
    for star in [1, 2, 3, 4, 5, 6]:
        print('中文|English|日本語|한국어', file=fp)
        print('---|---|---|---', file=fp)

        id_list = list(operators[star].keys())
        id_list.sort()
        for id in id_list:
            operator: dict[str, str] = operators[star][id]
            print('|'.join([operator.get(server, '')
                  for server in SERVERS]), file=fp)

        print(file=fp)
