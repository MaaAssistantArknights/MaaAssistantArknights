from collections import OrderedDict
from pathlib import Path
import json

# MODIFY dir TO IMPLEMENT GITHUB ACTIONS
# Everything else is scalable and should work as-is
# maybe \\ need to be changed to / if Linux

path = str(Path(__file__).parents[2]) + "\\resource"

CN = json.load(
    open(path + "\\tasks.json", encoding="utf-8")
)
EN = json.load(
    open(path + "\\global\\YostarEN\\resource\\tasks.json", encoding="utf-8")
)
JP = json.load(
    open(path + "\\global\\YostarJP\\resource\\tasks.json", encoding="utf-8")
)
KR = json.load(
    open(path + "\\global\\YostarKR\\resource\\tasks.json", encoding="utf-8")
)
TW = json.load(open(path + "\\global\\txwy\\resource\\tasks.json", encoding="utf-8"))

orderedEN = []
orderedJP = []
orderedKR = []
orderedTW = []

# v = 0
# w = 0
# x = 0
# y = 0
# z = 0

# for i in CN:
#     for e in EN:
#         if i == e:
#             orderedEN.insert(w, e)
#             w += 1

#     for j in JP:
#         if i == j:
#             orderedJP.insert(x, j)
#             x += 1

#     for k in KR:
#         if i == k:
#             orderedKR.insert(y, k)
#             y += 1

#     for t in TW:
#         if i == t:
#             orderedTW.insert(z, t)
#             z += 1

#     v += 1

for i in CN:
    for e in EN:
        if i == e:
            orderedEN.append(e)

    for j in JP:
        if i == j:
            orderedJP.append(j)

    for k in KR:
        if i == k:
            orderedKR.append(k)

    for t in TW:
        if i == t:
            orderedTW.append(t)

output = OrderedDict(sorted(EN.items(), key=lambda item: orderedEN.index(item[0])))
with open(
    path + "\\global\\YoStarEN\\resource\\tasks.json", "w", encoding="utf-8"
) as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(JP.items(), key=lambda item: orderedJP.index(item[0])))
with open(
    path + "\\global\\YoStarJP\\resource\\tasks.json", "w", encoding="utf-8"
) as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(KR.items(), key=lambda item: orderedKR.index(item[0])))
with open(
    path + "\\global\\YoStarKR\\resource\\tasks.json", "w", encoding="utf-8"
) as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

output = OrderedDict(sorted(TW.items(), key=lambda item: orderedTW.index(item[0])))
with open(
    path + "\\global\\txwy\\resource\\tasks.json", "w", encoding="utf-8"
) as outfile:
    json.dump(output, outfile, indent=4, ensure_ascii=False)

# print("Official: " + str(v).rjust(4, " ") + " tasks")
# print("YostarEN: " + str(w).rjust(4, " ") + " tasks")
# print("YostarJP: " + str(x).rjust(4, " ") + " tasks")
# print("YostarKR: " + str(y).rjust(4, " ") + " tasks")
# print("txwy:     " + str(z).rjust(4, " ") + " tasks")