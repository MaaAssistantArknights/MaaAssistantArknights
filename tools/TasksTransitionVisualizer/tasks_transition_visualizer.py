import os
import sys
import json
import logging
import warnings
import argparse
import matplotlib.pyplot as plt
import networkx as nx
import webbrowser

# nx.nx_pydot.graphviz_layout reports DeprecationWarning
warnings.filterwarnings('ignore', category=DeprecationWarning)

logging.basicConfig(format='%(message)s', level=logging.INFO)

parser = argparse.ArgumentParser(
    description='Generate a transition graph for nodes whose name begin with a specific keyword')
parser.add_argument('--keyword', type=str, default='Mizuki@Roguelike',
                    help='the keyword to generate the graph for, case sensitive')
parser.add_argument('--nodesize', type=int, default=500, choices=range(400, 801), metavar='[400-800]',
                    help='the size of nodes in the graph')
parser.add_argument('--arrowsize', type=int, default=30, choices=range(20, 51), metavar='[20-50]',
                    help='the size of arrows in the graph')
parser.add_argument('--fontsize', type=int, default=8, choices=range(5, 11), metavar='[5-10]',
                    help='the size of font in the graph')
parser.add_argument('--list', action='store_true',
                    help='if list all the names of nodes matched')
args = parser.parse_args()

project_root_path = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                 os.pardir, os.pardir))
task_json_path = os.path.join(project_root_path, 'resource', 'tasks.json')
try:
    with open(task_json_path, 'r', encoding='utf-8') as f:
        json_dict = json.load(f)
except Exception as e:
    logging.error(
        "Failed to read task json at {}: {}".format(task_json_path), e)
    exit(-1)

keyword = args.keyword
if keyword == "Mizuki@Roguelike":
    logging.info("Default to generate graph for {}".format(keyword))
else:
    logging.info("Generating graph for %s", keyword)


def simplify_node(node_name):
    # shorten node name for display in the graph
    return node_name.replace(keyword+'@', '')


def recover_node(node_name):
    # recover the node names, used when build graph
    return keyword+'@'+node_name

# recursively find the nodes
def search_node(node, s, visited, nodes):
    # nodes without next field are not considered
    if not (node in json_dict.keys() and 'next' in json_dict[node]):
        return
    if node in visited:
        return
    visited.add(node)
    if node.startswith(s):
        nodes.append(simplify_node(node))
    else:
        return
    for next_node in json_dict[node]['next']:
        search_node(next_node, s, visited, nodes)


visited = set()
nodes = []
for node in json_dict:
    search_node(node, keyword, visited, nodes)

if len(nodes) == 0:
    logging.warning("Found 0 tasks starting with {}".format(keyword))
    exit(0)
else:
    logging.info('Found {} tasks starting with {}'.format(len(nodes), keyword))
    if args.list:
        logging.info('\n'.join(nodes)) 
nodes.sort()

# generate networkx.MultiDiGraph
G = {}
for node in nodes:
    G[node] = json_dict[recover_node(node)]['next']
    for i in range(len(G[node])):
        G[node][i] = simplify_node(G[node][i])
G = nx.MultiDiGraph(G)
logging.info("Built graph model completed.")

# export pdf
plt.figure(figsize=(60, 60))
pos = nx.nx_pydot.graphviz_layout(G, prog='dot')
nx.draw_networkx_nodes(G, pos, node_size=args.nodesize,
                       alpha=0.8, node_color='lightblue')
nx.draw_networkx_edges(G, pos, alpha=1, arrows=True, arrowsize=args.arrowsize)
nx.draw_networkx_labels(G, pos, font_size=args.fontsize,
                        font_family='sans-serif')
plt.axis('off')
fig = plt.gcf()
relative_path = os.path.join(
    "tools", "TasksTransitionVisualizer", "graph_{}.pdf".format(keyword))
file_path = os.path.join(project_root_path, relative_path)

try:
    fig.savefig(file_path)
    logging.info("Transition graph file for keyword {} saved to {}".format(
        keyword, relative_path))
except PermissionError:
    logging.error(
        "File {} is in use. Close and re-run the task.".format(file_path))
except Exception as e:
    logging.error(
        "Cannot save graph file at {}. Error: {}".format(relative_path, e))


webbrowser.open(file_path, new=2)
