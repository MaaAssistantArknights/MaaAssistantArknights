# WHAT DOES THIS TOOL DO?
#
# This tool sorts the global json files so that they are in the same order as
# the ZH ones. This way, it is easier to find the attributes, see what is
# missing, etc.
#
# USAGE
#
# 1. Install Python
# 2. Run "$python match_json.py xxx" in the terminal where xxx is the desired
# server name i.e. YoStarJP/YoStarEN/YoStarKR/txwy
# 3. The json file will be updated in-place
#

import json
import os
import sys
from collections import OrderedDict

server_list = {
    "YoStarJP": [
        "yostarjp",
        "jp",
        "日",
        "日服"
    ],
    "YoStarEN": [
        "yostaren",
        "en",
        "美",
        "美服",
        "國際",
        "國際服"
    ],
    "YoStarKR": [
        "yostarkr",
        "kr",
        "韓",
        "韓服"
    ],
    "txwy": [
        "txwy"
        "繁中",
        "繁中服"
    ]
}

# Get the server name from argument
try:
    # server_name = YoStarJP
    server_name = None
    for listed_server in server_list:
        server_name = listed_server if (not server_name) and sys.argv[1].casefold() in server_list[listed_server] else server_name
    if server_name not in server_list:
        raise
except Exception:
    server_name = None
    while (not server_name):
        print("Enter one and only one server name:",
              ", ".join(server_list))
        t = input()
        for listed_server in server_list:
            server_name = listed_server if (not server_name) and t.casefold() in server_list[listed_server] else server_name

cur_dir = os.path.dirname(os.path.abspath(__file__))
proj_dir = os.path.join(cur_dir, "../../")

# File name of the json
# NOTE: You may change this to e.g. recruitment.json
# This is hardcoded because tasks.json is the most likely modified one
try:
    json_name = sys.argv[2]
except IndexError:
    json_name = "tasks.json"

zh_json_file = os.path.join(proj_dir, "resource", json_name)
gl_json_file = os.path.join(proj_dir, "resource", "global",
                            server_name, "resource", json_name)

# For test purpose
# gl_json_file = os.path.join(cur_dir, "test.json")

with open(zh_json_file, 'r', encoding='utf-8') as zh_fh:
    zh_json = json.load(zh_fh)

with open(gl_json_file, 'r', encoding='utf-8') as gl_fh:
    gl_json = json.load(gl_fh)

# Sort the global json object by the order of the ZH one
# Keys not found in ZH json will be put at the end (should not happen though)
LAST_POSITIONS = 1e10
key_order = {k: v for v, k in enumerate(zh_json)}
gl_json_new = OrderedDict(
    sorted(gl_json.items(), key=lambda i: key_order.get(i[0], LAST_POSITIONS)))

with open(gl_json_file, 'wb') as gl_fh:
    t = json.dumps(gl_json_new, indent=4, ensure_ascii=False)
    gl_fh.write(bytes(t, "utf-8"))
    print("successfully sorted",
          os.path.join("resource", "global", server_name, "resource", json_name),
          "by the order in ZH json file")
