import ctypes
import unittest
from types import SimpleNamespace
from unittest.mock import patch

from asst.asst import Asst


class GetImageTest(unittest.TestCase):
    def setUp(self):
        self.payload = b"\x89PNG\x00\xff"
        self.result_size = len(self.payload)
        callback_type = ctypes.CFUNCTYPE(
            ctypes.c_uint64, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64
        )
        self.callback = callback_type(self.get_image)
        library = SimpleNamespace(
            AsstCreate=lambda: 1,
            AsstDestroy=lambda handle: None,
            AsstGetImage=self.callback,
        )
        self.library_patch = patch.object(Asst, "_Asst__lib", library, create=True)
        self.library_patch.start()
        self.instance = Asst()

    def tearDown(self):
        del self.instance
        self.library_patch.stop()

    def get_image(self, handle, buffer, size):
        if size < len(self.payload):
            return ctypes.c_uint64(-1).value
        ctypes.memmove(buffer, self.payload, len(self.payload))
        return self.result_size

    def test_insufficient_buffer_returns_none(self):
        self.assertIsNone(self.instance.get_image(len(self.payload) - 1))

    def test_larger_buffer_returns_only_image_bytes(self):
        self.assertEqual(self.instance.get_image(len(self.payload) + 4), self.payload)

    def test_exact_buffer_returns_image_bytes(self):
        self.assertEqual(self.instance.get_image(len(self.payload)), self.payload)

    def test_empty_image_returns_none(self):
        self.result_size = 0
        self.assertIsNone(self.instance.get_image(len(self.payload)))


if __name__ == "__main__":
    unittest.main()
