from .cli import main
from .git import get_latest_file_content
from .translate import ChatTranslator
from .xaml_load import XamlParser, judge_encoding, parse_lang_str

__all__ = [
    "ChatTranslator",
    "judge_encoding",
    "parse_lang_str",
    "XamlParser",
    "get_latest_file_content",
    "main",
]
