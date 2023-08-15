from collections import OrderedDict
import json

# MODIFY dir TO IMPLEMENT GITHUB ACTIONS
# Everything else is scalable and should work as-is
# maybe \\ need to be changed to / if Linux

dir = "C:\\Users\\David\\Downloads"
path = dir + "\\MaaAssistantArknights\\resource\\global"

CNJson = open(
    dir + "\\MaaAssistantArknights\\resource\\tasks.json",
    encoding="utf-8",
)
ENJson = open(
    path + "\\YostarEN\\resource\\tasks.json",
    encoding="utf-8",
)
JPJson = open(
    path + "\\YostarJP\\resource\\tasks.json",
    encoding="utf-8",
)
KRJson = open(
    path + "\\YostarKR\\resource\\tasks.json",
    encoding="utf-8",
)
TWJson = open(
    path + "\\txwy\\resource\\tasks.json",
    encoding="utf-8",
)

CN = json.load(CNJson)
EN = json.load(ENJson)
JP = json.load(JPJson)
KR = json.load(KRJson)
TW = json.load(TWJson)

orderedEN = []
orderedJP = []
orderedKR = []
orderedTW = []

w = 0
x = 0
y = 0
z = 0

for i in CN:
    for e in EN:
        if i == e:
            orderedEN.insert(w, e)
            w += 1

    for j in JP:
        if i == j:
            orderedJP.insert(x, j)
            x += 1

    for k in KR:
        if i == k:
            orderedKR.insert(y, k)
            y += 1

    for t in TW:
        if i == t:
            orderedTW.insert(z, t)
            z += 1

output = OrderedDict(sorted(EN.items(), key=lambda item: orderedEN.index(item[0])))
with open(path + "\\YoStarEN\\resource\\tasks.json", "w", encoding="utf-8") as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(JP.items(), key=lambda item: orderedJP.index(item[0])))
with open(path + "\\YoStarJP\\resource\\tasks.json", "w", encoding="utf-8") as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(KR.items(), key=lambda item: orderedKR.index(item[0])))
with open(path + "\\YoStarKR\\resource\\tasks.json", "w", encoding="utf-8") as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(TW.items(), key=lambda item: orderedTW.index(item[0])))
with open(path + "\\txwy\\resource\\tasks.json", "w", encoding="utf-8") as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)
