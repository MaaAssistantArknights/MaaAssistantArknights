# WHAT DOES THIS TOOL DO?
#
# In order for MAA ZH-server features to be compatible with global servers,
# crops/screenshots/templates of ZH servers with Chinese characters etc. must be
# replaced with those of corresponding global servers, which needs your help.
# This tool helps you find which templates in ZH server (usually recently added)
# have not yet had a global server version, and copy them to a separate folder
# so that you can easily .
#
# There are some ZH templates without any Chinese characters etc. which can thus
# be used across different servers. You may add them to the ignore_list.txt
# file.
#
# USAGE
#
# 1. Install Python
# 2. Run "$python diff_resource.py xxx" in the terminal where xxx is the desired
# server name i.e. YostarJP/YostarEN/YostarKR/txwy
# 3. Results can be found in the diff_image folder
#


import os
import sys
import shutil

# Put the file names of text-less images (so that global servers use the same
# image as the ZH (ZH_CN) server does) in this file.
# Note you have to manually update the content of this file.
ignore_list_file_name = "ignore_list.txt"

server_list = [
    "YostarJP",
    "YostarEN",
    "YostarKR",
    "txwy"
]

# Get the server name from argument
try:
    # server_name = YostarJP
    server_name = sys.argv[1]
    if server_name not in server_list:
        raise
except:
    server_name = None
    while (not server_name):
        print("Enter one and only one server name:",
              ", ".join(server_list))
        t = input()
        server_name = t if t in server_list else None


# Get current directory so script is run at the correct dir
cur_dir = os.path.dirname(os.path.abspath(__file__))

zh_dir = os.path.join(cur_dir, "../../resource/template/")
gl_dir = os.path.join(cur_dir, "../../resource/global/",
                      server_name, "resource/template/")

zh_files = [f for f in os.listdir(
    zh_dir) if os.path.isfile(os.path.join(zh_dir, f))]
gl_files = [f for f in os.listdir(
    gl_dir) if os.path.isfile(os.path.join(gl_dir, f))]

with open(os.path.join(cur_dir, ignore_list_file_name)) as f:
    ignore_files = [line.rstrip('\n') for line in f]

# ZH server pics not found in global server nor ignored will be filtered here
diff_files = [i for i in zh_files if

              # NOTE: You May Customize Here
              # not i.startswith("Mizuki@Roguelike") and
              "Rogue" not in i and

              i not in gl_files and
              i not in ignore_files
              ]

# These pictures will be copied to the diff_image folder for reference.
# Contributors of global servers may screenshot the corresponding files.
# Note this folder is in .gitignore so it will not be uploaded.
out_dir = os.path.join(cur_dir, "diff_image/", server_name)
if os.path.exists(out_dir):
    shutil.rmtree(out_dir)
os.makedirs(out_dir)

for i in diff_files:
    # print(i)
    shutil.copyfile(os.path.join(zh_dir, i), os.path.join(out_dir, i))

print("Pictures not included in", server_name,
      "server resources is copied to diff_image/" + server_name)
