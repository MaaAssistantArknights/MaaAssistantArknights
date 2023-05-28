import unittest
from unittest.mock import patch

from src.auto_localization import ChatTranslator


class TestChatTranslator(unittest.TestCase):
    def setUp(self):
        self.translator = ChatTranslator()

    @patch('openai.ChatCompletion.create')
    def test_translate_default_sentence(self, mock_create):
        mock_create.return_value = {
            'choices': [
                {
                    'message': {
                        'content': """{"message":200,"content":"Tips: \\n \\n \n  1. Please use this function on the interface with the 'Start Action' button;  \\n \\n \n  2. Using a friend's assistance can turn off 'Auto Formation' and manually select operators before starting;  \\n \\n \n  3. To conduct a simulation of the paradox, turn off 'Auto Formation', select the skills, and then start on the 'Start Simulation' button interface;  \\n \\n \n  4. For 'Annihilation', multiple tasks are built-in under the 'resource/copilot' folder. Please manually form a squad and start on the 'Start Deployment' interface (can be used with 'number of loops');  \\n \\n \n  5. Video recognition is now supported. Please drag the strategy video file and then start. The video resolution should be 16:9, without black borders, simulator borders, or other interfering elements."}"""
                    }
                }
            ]
        }

        result = self.translator.translate()
        result_split_list = result.split("\n")
        length = len(result_split_list)
        assert_result = """Tips: \n \n \n  1. Please use this function on the interface with the 'Start Action' button;  \n \n \n  2. Using a friend's assistance can turn off 'Auto Formation' and manually select operators before starting;  \n \n \n  3. To conduct a simulation of the paradox, turn off 'Auto Formation', select the skills, and then start on the 'Start Simulation' button interface;  \n \n \n  4. For 'Annihilation', multiple tasks are built-in under the 'resource/copilot' folder. Please manually form a squad and start on the 'Start Deployment' interface (can be used with 'number of loops');  \n \n \n  5. Video recognition is now supported. Please drag the strategy video file and then start. The video resolution should be 16:9, without black borders, simulator borders, or other interfering elements."""
        self.assertEqual(length, 16)
        self.assertEqual(result, assert_result)

    @patch('openai.ChatCompletion.create')
    def test_translate_custom_sentence(self, mock_create):
        mock_create.return_value = {
            'choices': [
                {
                    'message': {
                        'content': '{"message":200,"content":"Custom translated text"}'
                    }
                }
            ]
        }

        result = self.translator.translate(sentence="This is a custom sentence")
        self.assertEqual(result, "Custom translated text")

    @patch('openai.ChatCompletion.create')
    def test_translate_unknown_language(self, mock_create):
        mock_create.return_value = {
            'choices': [
                {
                    'message': {
                        'content': '{"message":404,"content":"未知语言"}'
                    }
                }
            ]
        }

        result = self.translator.translate(target_language="未知")
        self.assertIsNone(result)

    @patch('openai.ChatCompletion.create')
    def test_translate_error(self, mock_create):
        mock_create.return_value = {
            'choices': [
                {
                    'message': {
                        'content': '{"message":500,"content":"内部服务器错误"}'
                    }
                }
            ]
        }

        result = self.translator.translate()
        self.assertIsNone(result)

    @patch('openai.ChatCompletion.create')
    def test_wrong_input_error(self, mock_create):
        mock_create.return_value = {
            'choices': [
                {
                    'message': {
                        'content': '我不明白你在说什么'
                    }
                }
            ]
        }

        result = self.translator.translate()
        self.assertIsNone(result)


if __name__ == '__main__':
    unittest.main()
