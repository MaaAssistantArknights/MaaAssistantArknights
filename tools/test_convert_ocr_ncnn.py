import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from convert_ocr_ncnn import convert_models


class ConvertOcrNcnnTests(unittest.TestCase):
    def test_converts_all_ocr_models_and_preserves_sources(self):
        with tempfile.TemporaryDirectory() as directory:
            resource = Path(directory)
            paths = [
                "PaddleOCR/det",
                "PaddleOCR/rec",
                "PaddleCharOCR/det",
                "PaddleCharOCR/rec",
                "global/YoStarEN/resource/PaddleOCR/rec",
            ]
            for path in paths:
                folder = resource / path
                folder.mkdir(parents=True)
                (folder / "inference.onnx").write_bytes(path.encode())
                (folder / "keys.txt").write_text("labels", encoding="utf-8")

            def run(command, *, cwd, check):
                kind = Path(command[1]).stem
                shape = "[1,3,640,640]" if kind == "det" else "[1,3,48,320]"
                self.assertEqual(command[2:], [f"inputshape={shape}", "fp16=0"])
                self.assertTrue(check)
                data = (cwd / command[1]).read_bytes()
                for suffix in ("param", "bin"):
                    (cwd / f"{kind}.ncnn.{suffix}").write_bytes(data)
                (cwd / "intermediate.pnnx.param").touch()

            with patch("convert_ocr_ncnn.subprocess.run", side_effect=run) as converter:
                convert_models(resource)
            self.assertEqual(converter.call_count, len(paths))
            for path in paths:
                folder = resource / path
                kind = folder.name
                self.assertEqual(
                    (folder / "inference.onnx").read_bytes(), path.encode()
                )
                self.assertEqual((folder / "keys.txt").read_text(), "labels")
                for suffix in ("param", "bin"):
                    self.assertEqual(
                        (folder / f"{kind}.ncnn.{suffix}").read_bytes(), path.encode()
                    )
                self.assertFalse((folder / "intermediate.pnnx.param").exists())

    def test_rejects_empty_resource_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                convert_models(Path(directory))

    def test_failed_conversion_does_not_install_outputs(self):
        with tempfile.TemporaryDirectory() as directory:
            resource = Path(directory)
            model_dir = resource / "det"
            model_dir.mkdir()
            (model_dir / "inference.onnx").write_bytes(b"model")
            error = subprocess.CalledProcessError(1, "pnnx")
            with patch("convert_ocr_ncnn.subprocess.run", side_effect=error):
                with self.assertRaises(subprocess.CalledProcessError):
                    convert_models(resource)
            self.assertEqual(list(model_dir.glob("*.ncnn.*")), [])

    def test_rejects_missing_or_empty_converter_outputs(self):
        for empty in (False, True):
            with self.subTest(empty=empty), tempfile.TemporaryDirectory() as directory:
                resource = Path(directory)
                model_dir = resource / "rec"
                model_dir.mkdir()
                (model_dir / "inference.onnx").write_bytes(b"model")

                def run(command, *, cwd, check):
                    (cwd / "rec.ncnn.param").write_text("param")
                    if empty:
                        (cwd / "rec.ncnn.bin").touch()

                with patch("convert_ocr_ncnn.subprocess.run", side_effect=run):
                    with self.assertRaises(RuntimeError):
                        convert_models(resource)
                self.assertEqual(list(model_dir.glob("*.ncnn.*")), [])


if __name__ == "__main__":
    unittest.main()
