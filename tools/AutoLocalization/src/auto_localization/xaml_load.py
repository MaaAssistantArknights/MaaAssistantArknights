import logging
import os
import re
from copy import deepcopy

from cchardet import detect
from lxml import etree
from xmldiff import main
from xmldiff.actions import UpdateTextIn, InsertComment, UpdateTextAfter, DeleteNode

from .translate import ChatTranslator

SPACE = '{http://www.w3.org/XML/1998/namespace}space'

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')


def judge_encoding(file):
    with open(file, 'rb') as _:
        content = _.read()
        result = detect(content)
        return result['encoding']


def parse_lang_str(doc_path):
    lang_str = os.path.basename(doc_path)
    match lang_str:
        case s if 'zh-cn' in s:
            return "Chinese (Simplified)"
        case s if 'zh-tw' in s:
            return "Chinese (Traditional)"
        case s if 'en-us' in s:
            return "English"
        case s if 'ja-jp' in s:
            return "Japanese"
        case s if 'ko-kr' in s:
            return "Korean"
        case _:
            raise ValueError('wrong language input')


class XamlParser:
    """
    XamlParser类，用于解析xaml文件

    """

    def __init__(self, file=None, parse_type=0, xaml_string=None, language=None, encoding='utf-8'):
        """
        初始化XamlParser类
        :param file: 从文件解析时，传入文件路径
        :param parse_type: 0表示从文件解析，1表示从字符串解析
        :param xaml_string: 从字符串解析时，传入xaml字符串
        :param language: 从字符串解析时，传入语言
        :param encoding: 从字符串解析时，传入编码
        """
        self.__test = False
        self.__counter = 0
        if parse_type == 0:
            self._file_path, self.__encoding, self.__language, xaml_string = self.__from_file(file)
        elif parse_type == 1:
            assert language is not None, 'language must be specified when parse_type is 1'
            self._file_path, self.__encoding, self.__language = file, encoding, language
        else:
            raise ValueError('parse_type must be 0 or 1')
        xaml_string = xaml_string.replace('&#x0a;', '\\n')
        self.__root = etree.fromstring(xaml_string)
        # 获取命名空间
        self.__nsmap = self.__root.nsmap
        self.__default_namespace = self.__nsmap.get(None)
        self.__x_ns = self.__nsmap.get('x')
        self.__x_uid_ns, self.__x_key_ns = f'{{{self.__x_ns}}}Uid', f'{{{self.__x_ns}}}Key'
        self.__merged_node = self.__root.find('./ResourceDictionary.MergedDictionaries', namespaces=self.__nsmap)
        self.__cp_root = self.copy_node(self.__root, cp_text=True)
        if self.__merged_node is not None:
            self.__cp_merged_node = self.copy_node(self.__merged_node, cp_text=True)
            self.__cp_root.append(self.__cp_merged_node)
        else:
            self.__cp_merged_node = None
        self.__gen_cp_tree_by_traverse(element=self.__root, current_cp_node=self.__cp_root)

    @staticmethod
    def __from_file(file):
        assert os.path.exists(file), f'{file}file not exists'
        encoding = judge_encoding(file)
        language = parse_lang_str(file)
        with open(file, 'r', encoding=encoding) as f:
            content = f.read()
        return file, encoding, language, content

    def __gen_cp_tree_by_traverse(self, element, current_cp_node):
        for child in element:
            if child.tag == etree.Comment:
                # 注释节点
                parent = child.getparent()
                if parent is None:
                    continue
                children = list(parent)
                index = children.index(child)
                if index == 0:
                    if self.__merged_node is not None and parent.tag == f'{{{self.__default_namespace}}}ResourceDictionary':
                        continue
                    # 注释节点为第一个节点
                    cp_comment = self.copy_node(child, cp_text=False)
                    current_cp_node.append(cp_comment)
                    continue
                cp_comment = self.copy_node(child, cp_text=False)
                prev_node = children[index - 1]
                prev_cp_node = current_cp_node[index - 1]
                assert prev_node.tag == prev_cp_node.tag and prev_node.attrib == prev_cp_node.attrib, "前节点类型不一致"
                current_cp_node.append(cp_comment)
                continue

            cp_node = self.copy_node(child)
            if child.tag == f'{{{self.__default_namespace}}}ResourceDictionary.MergedDictionaries':
                # 次根 MergedDictionaries: 遍历后退出循环
                current_cp_node = self.__cp_merged_node
                self.__gen_cp_tree_by_traverse(child, current_cp_node=current_cp_node)
                break
            elif child.tag == f'{{{self.__default_namespace}}}ResourceDictionary':
                # resource dictionary uid组
                cp_node.text = child.text
                current_cp_node.append(cp_node)

                # 添加说明注释
                # chat = ChatTranslator(language=self.language, base_language="english")
                uid = child.get(self.__x_uid_ns)
                new_comment = etree.Comment(f"${uid}:")
                # new_comment.text += self.language + ' '
                # new_comment.text += chat.translate(uid)
                new_comment.tail = child.text
                if child[0].tag != etree.Comment:
                    child.insert(0, new_comment)
                else:
                    child[0].text = new_comment.text
                cp_comment = self.copy_node(new_comment, cp_text=True)
                cp_node.insert(0, cp_comment)
                self.__gen_cp_tree_by_traverse(child, current_cp_node=cp_node)
            elif child.get(self.__x_key_ns) is not None:
                # 正常节点
                current_cp_node.append(cp_node)
            else:
                current_cp_node.append(cp_node)
                self.__gen_cp_tree_by_traverse(child, current_cp_node=cp_node)
        return current_cp_node

    @staticmethod
    def copy_node(node, cp_text=False):
        if node.tag == etree.Comment:
            cp_node = etree.Comment(node.text) if cp_text else etree.Comment("")
            cp_node.tail = node.tail
        else:
            cp_node = etree.Element(node.tag, attrib=node.attrib, nsmap=node.nsmap)
            cp_node.text = node.text if cp_text else ""
            cp_node.sourceline = node.sourceline
            cp_node.tail = node.tail
        return cp_node

    def write_xaml(self, tree=None, file_path=None):
        if tree is None:
            tree = self.tree
        if file_path is None:
            file_path = self._file_path
        if file_path is None:
            return False
        xaml_data = etree.tostring(tree, pretty_print=True, encoding=self.__encoding)
        # xaml_data = html.unescape(xaml_data.decode(self.__encoding))
        xaml_data = xaml_data.decode(self.__encoding)
        with open(file_path, 'w', encoding=self.__encoding) as _:
            _.write(xaml_data)
        return True

    def compare_structure(self, compare_parser):
        """
        比较两个xaml结构，比较时忽略类似<ResourceDictionary x:Uid="xxx">下的首个注释节点
        :param compare_parser:
        :return: 如果结构一致返回True，否则返回False
        """
        this_tree = self.cp_tree
        compare_tree = compare_parser.cp_tree
        assert self.x_key_ns == compare_parser.x_key_ns, f"{self.language} 和 {compare_parser.language}x:Key命名空间不一致"
        assert self.x_uid_ns == compare_parser.x_uid_ns, f"{self.language} 和 {compare_parser.language}x:Uid命名空间不一致"
        pt = re.compile(r'/\*/\*/\*\[\d*]/comment\(\)\[1]')
        uniqueattrs = [self.x_key_ns, self.x_uid_ns]
        ignored_attrs = [SPACE]
        res = main.diff_trees(this_tree, compare_tree, diff_options={
            'F': 0.1,
            'ratio_mode': 'accurate',
            'uniqueattrs': uniqueattrs,
            'ignored_attrs': ignored_attrs, })
        for i in res:
            if type(i).__name__ == 'UpdateTextIn' and pt.search(i.node):
                continue
            return False
        return True

    def xpath(self, xpath_str, only_one=True, accept_empty=False):
        ns = self.nsmap
        if None in ns:
            del ns[None]
        if "" in ns:
            del ns[""]
        search_result = self.tree.xpath(xpath_str, namespaces=ns)
        if not len(search_result):
            assert accept_empty, f"xpath: {xpath_str} 搜索结果为空"
            yield None
        if only_one:
            assert len(search_result) == 1, f"xpath: {search_result} 搜索结果数量不为1"
            yield search_result[0]
        else:
            yield from search_result

    def getpath(self, node):
        tree = etree.ElementTree(self.__root)
        return tree.getpath(node)

    def counter(self, start=False, test=False, messages=""):
        if test:
            self.__test = True
        if self.__test:
            return
        if start:
            self.__counter = 0
            logging.info(messages)
        else:
            self.__counter += 1
        logging.info(f"{messages}: {self.__counter}")

    def translate_force(self, target_path, skip_translate=False):
        """
        强制输出本文件到目标文件的翻译，完成后本文件也会更新
        :param target_path: 输出的目标文件路径
        :param skip_translate: 是否跳过chatgpt翻译
        :return:
        """
        target_language = parse_lang_str(target_path)
        output_tree = deepcopy(self.cp_tree)
        chat = None if skip_translate else ChatTranslator(language=target_language, base_language=self.language)
        self.counter(start=True,
                     test=skip_translate,
                     messages=f"start force translate {self.language} -> {target_language}")
        t = self.__merged_node if self.__merged_node is not None else self.__root
        for i in t.findall('.//s:String[@x:Key]', namespaces=self.__nsmap):
            key = i.get(self.__x_key_ns)
            node = output_tree.find(f'.//s:String[@x:Key="{key}"]', namespaces=self.__nsmap)
            node.text = i.text if skip_translate else chat.translate(i.text)
            node.text = i.text if node.text is None else node.text
            self.counter(messages=f"translate {i.text[0:20]}")
        self.write_xaml(output_tree, target_path)
        self.write_xaml()

    def translate_compare(self, compare_parser, skip_translate=False):
        """
        输出本文件到目标文件的翻译，若目标文件存在与本文件相同的节点，则跳过该节点，完成后本文件也会更新
        :param compare_parser: 目标文件的XamlParser
        :param skip_translate: 是否跳过chatgpt翻译
        :return: 返回翻译后的Xaml lxml树
        """
        target_cp_tree = self.cp_tree
        base_cp_tree = compare_parser.cp_tree
        assert self.nsmap == compare_parser.nsmap, f"{self.language} 和 {compare_parser.language}命名空间不一致"
        assert self.x_key_ns == compare_parser.x_key_ns, f"{self.language}和{compare_parser.language}x:Key命名空间不一致"
        assert self.x_uid_ns == compare_parser.x_uid_ns, f"{self.language}和{compare_parser.language}x:Uid命名空间不一致"
        uniqueattrs = [self.x_key_ns, self.x_uid_ns]
        ignored_attrs = [SPACE]
        chat = None if skip_translate else ChatTranslator(language=self.language, base_language=compare_parser.language)
        self.counter(start=True,
                     test=skip_translate,
                     messages=f"start compare translate {compare_parser.language} -> {self.language}")
        res = main.diff_trees(target_cp_tree, base_cp_tree, diff_options={
            'F': 0.1,
            'ratio_mode': 'accurate',
            'uniqueattrs': uniqueattrs,
            'ignored_attrs': ignored_attrs, })
        new_action = []
        logging.info(f"all movements contains {len([i for i in res if type(i).__name__ == 'UpdateTextIn'])} steps")
        for i in res:
            if type(i).__name__ == 'MoveNode' and 'comment()' in i.node:
                try:
                    str_node = next(compare_parser.xpath(i.target))[i.position]
                    parts = i.node.split('/')
                    # 提取父节点的路径
                    parent_path = '/'.join(parts[:-1])
                    node_position = "{}/comment()[{}]".format(i.target,
                                                              len(str_node.xpath('preceding-sibling::comment()')) + 1)
                    if parent_path == compare_parser.getpath(str_node.getparent()):
                        # 如果父节点路径相同，说明
                        new_action.append(i)
                        new_action.append(UpdateTextIn(node_position, str_node.text))
                        continue
                    new_action.append(InsertComment(i.target, i.position, str_node.text))
                    new_action.append(UpdateTextAfter(node_position, str_node.tail))
                    new_action.append(DeleteNode(i.node))
                except Exception as _:
                    logging.warning(f"{type(_)}: {_}")
                    new_action.append(i)
            elif type(i).__name__ == 'UpdateTextIn':
                if 'comment()' in i.node:
                    continue
                str_node = next(compare_parser.xpath(i.node))
                if str_node.text.strip():
                    text = str_node.text if skip_translate else chat.translate(str_node.text)
                    text = str_node.text if text is None else text
                else:
                    text = str_node.text
                self.counter(messages=f"translate {str_node.text[0:20]}")
                new_action.append(UpdateTextIn(i.node, text))
            elif type(i).__name__ == 'InsertComment':

                str_node = next(compare_parser.xpath(i.target))[i.position]
                if str_node.tag == etree.Comment:
                    node_position = "{}/comment()[{}]".format(i.target,
                                                              len(str_node.xpath('preceding-sibling::comment()')) + 1)
                    new_action.append(InsertComment(i.target, i.position, str_node.text))
                    new_action.append(UpdateTextAfter(node_position, str_node.tail))
                else:
                    new_action.append(i)
            else:
                new_action.append(i)
        final_tree = main.patch_tree(new_action, self.tree)
        compare_parser.write_xaml()
        self.write_xaml(final_tree, self._file_path)
        return final_tree

    def update_translate(self, compare_old_parser, compare_new_parser, skip_translate=False):
        """
        将比较新旧两个文件的更新，并将更新同步到本文件，若本文件与旧文件结构不同，则会与旧文件调用translate_compare
        :param compare_old_parser: 旧文件的XamlParser
        :param compare_new_parser: 新文件的XamlParser
        :param skip_translate: 是否跳过chatgpt翻译
        :return: 返回翻译后的Xaml lxml树
        """
        compare_old_tree = compare_old_parser.merged_root_tree
        if not self.compare_structure(compare_old_parser):
            target_tree = self.translate_compare(compare_old_parser, skip_translate=skip_translate)
        else:
            target_tree = self.cp_tree
        compare_new_tree = compare_new_parser.merged_root_tree
        assert compare_old_parser.nsmap == compare_new_parser.nsmap, \
            f"old {compare_old_parser.language}和 new {compare_new_parser.language}命名空间不一致"
        assert compare_old_parser.x_key_ns == compare_new_parser.x_key_ns, \
            f"old {compare_old_parser.language}和 new {compare_new_parser.language}x:Key命名空间不一致"
        assert compare_old_parser.x_uid_ns == compare_new_parser.x_uid_ns, \
            f"old {compare_old_parser.language}和 new {compare_new_parser.language}x:Uid命名空间不一致"

        chat = None if skip_translate else ChatTranslator(language=self.language,
                                                          base_language=compare_new_parser.language)
        self.counter(start=True,
                     test=skip_translate,
                     messages=f"start compare translate {compare_new_parser.language} -> {self.language}")
        uniqueattrs = [self.x_key_ns, self.x_uid_ns]
        ignored_attrs = [SPACE]
        res = main.diff_trees(compare_old_tree, compare_new_tree, diff_options={
            'F': 0.1,
            'ratio_mode': 'accurate',
            'uniqueattrs': uniqueattrs,
            'ignored_attrs': ignored_attrs
        })
        new_actions = []
        logging.info(f"all movements contains {len([i for i in res if type(i).__name__ == 'UpdateTextIn'])} steps")
        for i in res:
            if type(i).__name__ == 'UpdateTextIn':
                if i.node.endswith('comment()[1]') and i.text[0] == '$':
                    continue
                elif 's:String' in i.node:
                    text = i.text if skip_translate else chat.translate(i.text)
                    text = i.text if text is None else text
                    self.counter(messages=f"translate {i.text[0:20]}")
                    new_actions.append(UpdateTextIn(i.node, text))
                else:
                    new_actions.append(i)
            else:
                new_actions.append(i)

        final_tree = main.patch_tree(new_actions, target_tree)
        compare_new_parser.write_xaml()
        self.write_xaml(final_tree)
        return final_tree

    @property
    def encoding(self):
        return self.__encoding

    @property
    def cp_tree(self):
        return self.__cp_root

    @property
    def tree(self):
        return self.__root

    @property
    def nsmap(self):
        return deepcopy(self.__nsmap)

    @property
    def x_uid_ns(self):
        return self.__x_uid_ns

    @property
    def x_key_ns(self):
        return self.__x_key_ns

    @property
    def merged_root_tree(self):
        root = self.copy_node(self.__root, cp_text=True)
        if self.__merged_node is not None:
            root.append(deepcopy(self.__merged_node))
        else:
            root = deepcopy(self.__root)
        return root

    @property
    def language(self):
        return self.__language

    @property
    def tostring(self):
        return etree.tostring(self.tree, encoding=self.__encoding, pretty_print=True).decode(self.__encoding)


if __name__ == '__main__':
    zh_parser = XamlParser(parse_type=0, file='../../example/zh-cn.xaml')
    zh_new_parser = XamlParser(parse_type=0, file='../../example/zh-cn_new.xaml')
    print()
