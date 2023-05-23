import unittest

from xaml_load import XamlParser, judge_encoding, parse_lang_str


class TestXamlParser(unittest.TestCase):

    def setUp(self):
        self.zh_sample_path = '../../sample/backup/zh-cn.xaml'
        self.zh_new_sample_path = '../../sample/backup/zh-cn_new.xaml'
        self.en_sample_path = '../../sample/backup/en-us.xaml'
        self.en_force_sample_path = '../../sample/backup/en-us_force.xaml'
        self.en_compare_sample_path = '../../sample/backup/en-us_compare.xaml'
        self.en_update_sample_path = '../../sample/backup/en-us_update.xaml'
        with open(self.zh_sample_path, 'r', encoding='utf-8') as f:
            self.zh_sample_content = f.read()
        with open(self.zh_new_sample_path, 'r', encoding='utf-8') as f:
            self.zh_new_sample_content = f.read()
        with open(self.en_sample_path, 'r', encoding='utf-8') as f:
            self.en_sample_content = f.read()
        with open(self.en_force_sample_path, 'r', encoding='utf-8') as f:
            self.en_force_sample_content = f.read()
        with open(self.en_compare_sample_path, 'r', encoding='utf-8') as f:
            self.en_compare_sample_content = f.read()
        with open(self.en_update_sample_path, 'r', encoding='utf-8') as f:
            self.en_update_sample_content = f.read()
        self.zh_path = '../../sample/zh-cn.xaml'
        self.zh_new_path = '../../sample/zh-cn_new.xaml'
        self.en_force_path = '../../sample/en-us_force.xaml'
        self.en_compare_path = '../../sample/en-us_compare.xaml'
        self.en_update_path = '../../sample/en-us_update.xaml'
        self.zh_tw_path = '../../sample/zh-tw.xaml'
        self.ja_path = '../../sample/ja-jp.xaml'
        self.ko_path = '../../sample/ko-kr.xaml'
        with open(self.zh_path, 'w', encoding='utf-8') as f:
            f.write(self.zh_sample_content)
        with open(self.zh_new_path, 'w', encoding='utf-8') as f:
            f.write(self.zh_new_sample_content)
        with open(self.en_force_path, 'w', encoding='utf-8') as f:
            f.write(self.en_sample_content)
        with open(self.en_compare_path, 'w', encoding='utf-8') as f:
            f.write(self.en_sample_content)
        with open(self.en_update_path, 'w', encoding='utf-8') as f:
            f.write(self.en_sample_content)

    def test_judge_encoding(self):
        expected_encoding = 'UTF-8'
        result_encoding = judge_encoding(self.zh_path)
        self.assertEqual(expected_encoding, result_encoding)

    def test_parse_lang_str(self):
        expected_zh_cn_language = "Chinese (Simplified)"
        expected_zh_tw_language = "Chinese (Traditional)"
        expected_en_language = "English"
        expected_ja_language = "Japanese"
        expected_ko_language = "Korean"
        self.assertEqual(expected_zh_cn_language, parse_lang_str(self.zh_path))
        self.assertEqual(expected_zh_tw_language, parse_lang_str(self.zh_tw_path))
        self.assertEqual(expected_en_language, parse_lang_str(self.en_force_path))
        self.assertEqual(expected_en_language, parse_lang_str(self.en_compare_path))
        self.assertEqual(expected_en_language, parse_lang_str(self.en_update_path))
        self.assertEqual(expected_ja_language, parse_lang_str(self.ja_path))
        self.assertEqual(expected_ko_language, parse_lang_str(self.ko_path))
        with self.assertRaises(ValueError):
            parse_lang_str("./test/bla")

    def test_init(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_path)
        with self.assertRaises(AssertionError):
            XamlParser(parse_type=1, xaml_string=self.zh_sample_content)
        zh_string_parser = XamlParser(parse_type=1, language='Chinese', xaml_string=self.zh_sample_content)
        self.assertIsInstance(zh_parser, XamlParser)
        self.assertIsInstance(zh_string_parser, XamlParser)
        self.assertFalse(zh_string_parser.write_xaml())
        with self.assertRaises(ValueError):
            XamlParser(parse_type=2)
        with self.assertRaises(AssertionError):
            XamlParser(parse_type=0, file="./test/blah.xaml")
        s = zh_parser.tostring
        self.assertTrue(s.count('&#x0a;') == 0)

    def test_xpath(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_sample_path)
        # 检验空结果
        with self.assertRaises(AssertionError):
            next(zh_parser.xpath('.//test'))
        self.assertIsNone(next(zh_parser.xpath('.//test', accept_empty=True)))

        # 检验多结果
        with self.assertRaises(AssertionError):
            next(zh_parser.xpath('.//s:String[@x:Key]'))
        self.assertLess(1, len(list(zh_parser.xpath('.//s:String[@x:Key]', only_one=False))))

        # 检验单结果
        res = list(zh_parser.xpath('.//s:String[@x:Key="Settings"]'))
        self.assertEqual(1, len(res))
        self.assertEqual("设置", res[0].text)

    def test_translate_compare_structure(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_sample_path)
        en_parser = XamlParser(parse_type=0, file=self.en_sample_path)
        en_compare_parser = XamlParser(parse_type=0, file=self.en_compare_sample_path)
        self.assertFalse(zh_parser.compare_structure(en_parser))
        self.assertTrue(zh_parser.compare_structure(en_compare_parser))

    def test_translate_force(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_path)
        zh_parser.translate_force(self.en_force_path, skip_translate=True)
        with open(self.en_force_path, 'r', encoding='utf-8') as f:
            translated_content = f.read()
            compare_parser = XamlParser(parse_type=0, file=self.en_force_path)
            self.assertEqual(translated_content, self.en_force_sample_content)
            self.assertTrue(zh_parser.compare_structure(compare_parser))

    def test_translate_compare(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_path)
        en_parser = XamlParser(parse_type=0, file=self.en_compare_path)

        en_parser.translate_compare(zh_parser, skip_translate=True)

        with open(self.en_compare_path, 'r', encoding='utf-8') as f:
            translated_content = f.read()
            compare_parser = XamlParser(parse_type=0, file=self.en_compare_path)
            self.assertEqual(translated_content, self.en_compare_sample_content)
            self.assertTrue(zh_parser.compare_structure(compare_parser))

    def test_update_translate(self):
        zh_parser = XamlParser(parse_type=0, file=self.zh_path)
        zh_new_parser = XamlParser(parse_type=0, file=self.zh_new_path)
        en_parser = XamlParser(parse_type=0, file=self.en_update_path)
        en_new_parser = XamlParser(parse_type=0, file=self.en_compare_path)

        en_parser.update_translate(zh_parser, zh_new_parser, skip_translate=True)
        en_new_parser.update_translate(zh_parser, zh_new_parser, skip_translate=True)
        with open(self.en_update_path, 'r', encoding='utf-8') as f:
            translated_content = f.read()
            compare_parser = XamlParser(parse_type=0, file=self.en_update_path)
            self.assertEqual(translated_content, self.en_update_sample_content)
            self.assertTrue(zh_new_parser.compare_structure(compare_parser))


if __name__ == '__main__':
    unittest.main()
