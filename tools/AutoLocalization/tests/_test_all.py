import os.path
import unittest

from src.auto_localization import XamlParser, ChatTranslator, parse_lang_str


class TestAll(unittest.TestCase):
    def setUp(self) -> None:
        self.zh_sample_path = '../example/zh-cn.xaml'
        self.en_sample_path = '../example/en-us.xaml'
        print(os.path.abspath(self.zh_sample_path))
        with open(self.zh_sample_path, 'r', encoding='utf-8') as f:
            self.zh_sample_content = f.read()
        with open(self.en_sample_path, 'r', encoding='utf-8') as f:
            self.en_sample_content = f.read()
        self.zh_path = 'data/zh-cn.xaml'
        self.en_force_path = 'data/en-us_force.xaml'
        with open(self.zh_path, 'w', encoding='utf-8') as f:
            f.write(self.zh_sample_content)
        with open(self.en_force_path, 'w', encoding='utf-8') as f:
            f.write(self.en_sample_content)
        target_language = parse_lang_str(self.en_force_path)
        base_language = parse_lang_str(self.zh_path)
        self.chat = ChatTranslator(language=target_language, base_language=base_language)

    def test_translate_force(self):
        count_success = 0
        count_fail = 0
        zh_parser = XamlParser(parse_type=0, file=self.zh_path)
        for i in zh_parser.tree.findall('.//s:String[@x:Key]', namespaces=zh_parser.nsmap):
            text = self.chat.translate(i.text)
            if text is None:
                count_fail += 1
            else:
                count_success += 1
        print(f'count_success: {count_success}, count_fail: {count_fail}')
        self.assertGreater(0, count_fail)


if __name__ == '__main__':
    unittest.main()
