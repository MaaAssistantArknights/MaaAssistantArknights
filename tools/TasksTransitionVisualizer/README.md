# 说明

一个简单脚本，读取tasks.json, 根据指定的前缀词筛选task，将获得的task按照next关系绘制成有向图。

Generate a transition graph for nodes whose name begin with a specific keyword

```
optional arguments:
  -h, --help            show this help message and exit
  --keyword KEYWORD     the keyword to generate the graph for, case sensitive
  --nodesize [400-800]  the size of nodes in the graph
  --arrowsize [20-50]   the size of arrows in the graph
  --fontsize [5-10]     the size of font in the graph
  --list                if list all the names of nodes matched
```
