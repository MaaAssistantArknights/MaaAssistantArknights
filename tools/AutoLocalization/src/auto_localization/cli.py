import argparse
import logging
import os
from shutil import copy

from dotenv import load_dotenv

from .git import get_latest_file_content
from .xaml_load import XamlParser

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logging.debug(os.path.abspath('.env'))
load_dotenv(dotenv_path='.env')
if os.path.exists('../.env'):
    load_dotenv(dotenv_path='../.env')
if os.path.exists('../../.env'):
    load_dotenv(dotenv_path='../../.env')
root_path = os.getenv("LOCALIZATION_PATH")
assert root_path, "LOCALIZATION_PATH is not set"
zh_cn_path = os.path.join(root_path, "zh-cn.xaml")
en_us_path = os.path.join(root_path, "en-us.xaml")
zh_tw_path = os.path.join(root_path, "zh-tw.xaml")
ja_jp_path = os.path.join(root_path, "ja-jp.xaml")
ko_kr_path = os.path.join(root_path, "ko-kr.xaml")


def initiate(args):
    logging.info("initiate project")
    copy('./copy.env', '.env')
    with open('./copy.env', 'r') as f:
        content = f.read()
    api_key = input("please input openai api key:")
    content = content.replace('OPENAI_API_KEY=YOUR_API_KEY\n', f'OPENAI_API_KEY={api_key}\n')
    with open(".env", "w") as f:
        f.write(content)


def create(args):
    logging.info(f"create project{args}")
    if args.language:
        generate_by_language(args.test, args.force, root_path, args.language)
        return
    if args.force:
        logging.info("force create")
        translate_force(args.test)
    else:
        logging.info("no force create")
        translate_compare(args.test)


def update(args):
    logging.info("update project")
    if args.language:
        update_by_language(args.test, root_path, args.language)
        return
    translate_update(args.test)


def translate_force(test):
    zh_cn_parser = XamlParser(zh_cn_path)
    en_us_parser = XamlParser(en_us_path)

    # 生成中文翻译
    zh_cn_parser.translate_force(zh_tw_path, skip_translate=test)
    zh_cn_parser.translate_compare(en_us_parser, skip_translate=test)
    en_us_parser = XamlParser(en_us_path)
    en_us_parser.translate_force(ja_jp_path, skip_translate=test)
    en_us_parser.translate_force(ko_kr_path, skip_translate=test)
    logging.info("generate force done")


def translate_compare(test):
    zh_cn_parser = XamlParser(zh_cn_path)
    en_us_parser = XamlParser(en_us_path)
    zh_tw_parser = XamlParser(zh_tw_path)
    ja_jp_parser = XamlParser(ja_jp_path)
    ko_kr_parser = XamlParser(ko_kr_path)
    zh_tw_parser.translate_compare(zh_cn_parser, skip_translate=test)
    en_us_parser.translate_compare(zh_cn_parser, skip_translate=test)
    en_us_parser = XamlParser(en_us_path)
    ja_jp_parser.translate_compare(en_us_parser, skip_translate=test)
    ko_kr_parser.translate_compare(en_us_parser, skip_translate=test)
    logging.info("generate compare done")


def generate_by_language(test, force, path, language):
    zh_cn_parser = XamlParser(zh_cn_path)
    if force:
        assert language in ["en-us", "zh-tw", "ja-jp", "ko-kr"], f"language {language} is not supported"
        language_path = os.path.join(path, f"{language}.xaml")
        zh_cn_parser.translate_force(language_path, skip_translate=test)
        return
    en_us_parser = XamlParser(en_us_path)
    match language:
        case s if s in ["en-us", "zh-tw"]:
            file_path = os.path.join(path, f"{language}.xaml")
            parser = XamlParser(file_path)
            parser.translate_compare(zh_cn_parser, skip_translate=test)
        case s if s in ["ja-jp", "ko-kr"]:
            file_path = os.path.join(path, f"{language}.xaml")
            parser = XamlParser(file_path)
            parser.translate_compare(en_us_parser, skip_translate=test)
        case _:
            raise ValueError(f"language {language} is not supported")


def translate_update(test):
    zh_cn_new_parser = XamlParser(zh_cn_path)
    zh_cn_old_content = get_latest_file_content(file_path=zh_cn_path, encoding=zh_cn_new_parser.encoding)
    zh_cn_old_parser = XamlParser(parse_type=1,
                                  xaml_string=zh_cn_old_content,
                                  language=zh_cn_new_parser.language,
                                  encoding=zh_cn_new_parser.encoding)
    en_us_new_parser = XamlParser(en_us_path)
    en_us_old_content = get_latest_file_content(file_path=en_us_path, encoding=en_us_new_parser.encoding)
    en_us_old_parser = XamlParser(parse_type=1,
                                  xaml_string=en_us_old_content,
                                  language=en_us_new_parser.language,
                                  encoding=en_us_new_parser.encoding)
    zh_tw_parser = XamlParser(zh_tw_path)
    ja_jp_parser = XamlParser(ja_jp_path)
    ko_kr_parser = XamlParser(ko_kr_path)
    zh_tw_parser.update_translate(zh_cn_old_parser, zh_cn_new_parser, skip_translate=test)
    # TODO: 英文更新翻译逻辑?
    en_us_new_parser.translate_compare(zh_cn_new_parser, skip_translate=test)
    en_us_new_parser = XamlParser(en_us_path)
    ja_jp_parser.update_translate(en_us_old_parser, en_us_new_parser, skip_translate=test)
    ko_kr_parser.update_translate(en_us_old_parser, en_us_new_parser, skip_translate=test)
    logging.info("update done")


def update_by_language(test, path, language):
    match language:
        case s if s in ["en-us", 'zh-tw']:
            compare_file_path = zh_cn_path
        case s if s in ["ja-jp", "ko-kr"]:
            compare_file_path = en_us_path
        case _:
            raise ValueError(f"language {language} is not supported")
    compare_new_parser = XamlParser(compare_file_path)
    compare_old_content = get_latest_file_content(file_path=compare_file_path,
                                                  encoding=compare_new_parser.encoding)
    compare_old_parser = XamlParser(parse_type=1,
                                    xaml_string=compare_old_content,
                                    language=compare_new_parser.language,
                                    encoding=compare_new_parser.encoding)
    file_path = os.path.join(path, f"{language}.xaml")
    parser = XamlParser(file_path)
    parser.update_translate(compare_old_parser, compare_new_parser, skip_translate=test)


def cli_ui(test=None):
    parser = argparse.ArgumentParser(description="一个用于自动翻译本地化目录下不同语言文档的命令行工具。")
    subparsers = parser.add_subparsers()
    parser_init = subparsers.add_parser('init', help='初始化工具')
    parser_init.set_defaults(func=initiate)

    parser_create = subparsers.add_parser('create', help='初始化其他语言的文档,可选参数：-f --force, -t --test')
    parser_create.set_defaults(func=create)
    parser_create.add_argument("-f", "--force", action="store_true", help="强制覆盖已有的部分")
    parser_create.add_argument("-t", "--test", action="store_true", help="测试更新情况（跳过chatgpt）")
    parser_create.add_argument("-l", "--language", help="指定语言，可选参数：en-us, zh-tw, ja-jp, ko-kr")

    parser_update = subparsers.add_parser('update', help='更新本地化翻译,可选参数：-t --test')
    parser_update.set_defaults(func=update)
    parser_update.add_argument("-t", "--test", action="store_true", help="测试更新情况（跳过chatgpt）")
    parser_update.add_argument("-l", "--language", help="指定语言，可选参数：en-us, zh-tw, ja-jp, ko-kr")
    if test:
        args = parser.parse_args(test)
        args.func(args)
    else:
        args = parser.parse_args()
        args.func(args)


def main():
    cli_ui()


if __name__ == '__main__':
    cli_ui(["init"])
    print()
