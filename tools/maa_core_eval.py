# -*- coding: utf-8 -*-
"""通过 MaaCore.dll 对本地图片跑与线上一致的识别与匹配，用于离线验证 pipeline 任务。

典型场景：拿手动截图或用户日志压缩包里的截图，验证某个任务在 core 眼里是否命中、
得分多少、OCR 认出了什么文本 —— 识别走的是 core 本体（模板、前处理、ocrReplace、
各服 OCR 模型均与运行时一致），避免用外部脚本近似复现导致结论偏差。

仅 Debug 构建的 MaaCore.dll 可用（Debug 任务只在 ASST_DEBUG 下注册）。
连着模拟器在线验证任务流请用 Custom 任务，本模块只做离线图片评估。

用法一（作为模块，供脚本组合调用）::

    import sys; sys.path.insert(0, "tools")
    from maa_core_eval import CoreEval

    ev = CoreEval()                                   # 默认 build/bin/Debug + 仓库 resource
    ev = CoreEval(global_client="YoStarJP")           # 国际服资源与 OCR 模型
    ev.report(images=["1.png"], tasks=["xxx@Roguelike@StageEnter"])
    ev.pipeline(images=["1.png"], tasks=["A", "B"])   # 首命中 + 命中任务的 next 列表
    ev.ocr(images=["1.png"], roi=[100, 200, 300, 50]) # OCR 原始识别文本
    ev.replay(images=["1.png", "2.png"], tasks=["A"]) # 按 next 链逐图推进

用法二（命令行）::

    python tools/maa_core_eval.py --mode report --tasks "A,B" 1.png 2.png
    python tools/maa_core_eval.py --mode ocr --roi 100,200,300,50 1.png
    python tools/maa_core_eval.py --mode replay --tasks "A" 1.png 2.png 3.png
"""

import argparse
import ctypes
import json
import os
import pathlib
import sys
import tempfile
import threading

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
MSG_ALL_TASKS_COMPLETED = 3


class CoreEval:
    """MaaCore 离线图片评估的 python 封装。

    :param dll_dir:   MaaCore.dll 所在目录（须为 Debug 构建），默认 build/bin/Debug
    :param resource:  资源根目录（AsstLoadResource 语义：其下应有 resource/ 子目录），默认仓库根
    :param global_client: 国际服名（YoStarJP / YoStarEN / YoStarKR / txwy），
                          加载主资源后再叠加对应分服资源与 OCR 模型
    :param user_dir:  core 日志（debug/asst.log）写入目录，默认临时目录
    :param verbose:   额外打印 core 回调的其余消息
    """

    def __init__(
        self,
        dll_dir=REPO_ROOT / "build" / "bin" / "Debug",
        resource=REPO_ROOT,
        global_client=None,
        user_dir=None,
        verbose=False,
    ):
        self._verbose = verbose
        self._resource = pathlib.Path(resource)
        self._user_dir = pathlib.Path(user_dir) if user_dir else pathlib.Path(tempfile.mkdtemp(prefix="maa-eval-"))
        self._user_dir.mkdir(parents=True, exist_ok=True)

        dll_dir = pathlib.Path(dll_dir)
        os.environ["PATH"] = str(dll_dir) + os.pathsep + os.environ.get("PATH", "")
        self._lib = ctypes.WinDLL(str(dll_dir / "MaaCore.dll"))
        self._declare_functions()

        def on_callback(msg, details, arg):
            self._on_callback(msg, details)

        # 持有回调引用，实例存活期间不被 GC
        self._callback = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)(on_callback)
        self._completed = threading.Event()
        self._results = []

        self._lib.AsstSetUserDir(str(self._user_dir).encode("utf-8"))
        if not self._lib.AsstLoadResource(str(self._resource).encode("utf-8")):
            raise RuntimeError(f"load resource failed: {resource}")
        if global_client:
            global_path = self._resource / "resource" / "global" / global_client
            if not self._lib.AsstLoadResource(str(global_path).encode("utf-8")):
                raise RuntimeError(f"load global resource failed: {global_path}")

        self._handle = self._lib.AsstCreateEx(self._callback, None)
        if not self._handle:
            raise RuntimeError("AsstCreateEx failed")

    def _declare_functions(self):
        lib = self._lib
        c = ctypes.c_char_p
        lib.AsstSetUserDir.argtypes, lib.AsstSetUserDir.restype = [c], ctypes.c_bool
        lib.AsstLoadResource.argtypes, lib.AsstLoadResource.restype = [c], ctypes.c_bool
        lib.AsstCreateEx.restype = ctypes.c_void_p
        lib.AsstDestroy.argtypes = [ctypes.c_void_p]
        lib.AsstDestroy.restype = None
        lib.AsstAppendTask.argtypes = [ctypes.c_void_p, c, c]
        lib.AsstAppendTask.restype = ctypes.c_int64
        lib.AsstStart.argtypes, lib.AsstStart.restype = [ctypes.c_void_p], ctypes.c_bool
        lib.AsstRunning.argtypes, lib.AsstRunning.restype = [ctypes.c_void_p], ctypes.c_bool

    def _on_callback(self, msg, details):
        try:
            detail = json.loads(details.decode("utf-8")) if details else {}
        except (UnicodeDecodeError, json.JSONDecodeError):
            return
        if detail.get("what") == "DebugImageTest":
            self._results.append(detail.get("details", {}))
        elif msg == MSG_ALL_TASKS_COMPLETED:
            self._completed.set()
        elif self._verbose:
            print(f"[core] msg={msg} {json.dumps(detail, ensure_ascii=False)}")

    def _run_task(self, params, timeout=600):
        """append 一个 Debug 任务并同步等待完成，返回所有 DebugImageTest 结果。"""
        self._results.clear()
        self._completed.clear()
        task_id = self._lib.AsstAppendTask(
            self._handle, b"Debug", json.dumps(params, ensure_ascii=False).encode("utf-8")
        )
        if not task_id:
            raise RuntimeError(f'append Debug task failed: {json.dumps(params, ensure_ascii=False)}')
        if not self._lib.AsstStart(self._handle):
            raise RuntimeError("AsstStart failed")
        if not self._completed.wait(timeout):
            raise TimeoutError(f"Debug task not completed in {timeout}s")
        # 等工作线程回到 idle，否则下一次 append+start 会被拒绝
        for _ in range(100):
            if not self._lib.AsstRunning(self._handle):
                break
            threading.Event().wait(0.02)
        return list(self._results)

    @staticmethod
    def _abs_images(images):
        return [str(pathlib.Path(p).resolve()) for p in images]

    def report(self, images, tasks):
        """每张图 × 每个任务独立评估：[{image, results: [{task, hit, score, rect, ...}]}]。

        命中结果按任务算法携带 score+templ（模板）、score+text（OCR）、count（特征匹配）。
        """
        return self._run_task(
            {"mode": "report", "images": self._abs_images(images), "tasks": list(tasks)}
        )

    def pipeline(self, images, tasks):
        """每张图按任务列表跑一次线上同款首命中匹配：[{image, hit, task, next, ...}]。

        返回的 next 是命中任务的 next 任务列表，可用于自行驱动任务链回放。
        """
        return self._run_task(
            {"mode": "pipeline", "images": self._abs_images(images), "tasks": list(tasks)}
        )

    def ocr(self, images, roi=None):
        """OCR 引擎对该图（或指定 roi）的原始识别结果：[{image, results: [{text, score, rect}]}]。

        不套任务的 ocrReplace 与 expected 过滤；要看任务配置过滤后的效果用 report。
        """
        params = {"mode": "ocr", "images": self._abs_images(images)}
        if roi:
            params["roi"] = list(roi)
        return self._run_task(params)

    def templ(self, images, templates, threshold=0.7, roi=None, resize=None, task=None):
        """每张图 × 裸模板文件匹配（物品图标等非任务模板）。

        :param templates: 模板名列表，与 core 的 get_templ 一致：物品 ID（如 "2001"）、
                          相对 resource/template 的路径（如 "items/2001.png"）、
                          或 resource/template 下的目录（自动展开其中全部模板）
        :param threshold: hit 判定阈值；匹配恒报最佳得分，排查 ｢为什么没认出｣ 时可调低
        :param resize:    评估前把图 INTER_AREA 缩放到 [w, h]（core 侧执行），
                          复刻线上各识别器的尺度预处理时使用
        :param task:      Matcher 配置来源任务名（maskRange / colorScales / method 等取自
                          该任务，threshold 缺省时也取任务阈值），复刻线上自定义识别器
                          的 Matcher 用法时使用
        :return: [{image, results: [{template, hit, score, rect, error?}]}]
        """
        names = []
        templ_root = self._resource / "resource" / "template"
        for name in templates:
            path = templ_root / name
            if path.is_dir():
                names.extend(sorted(str(p.relative_to(templ_root)).replace("\\", "/") for p in path.rglob("*.png")))
            else:
                names.append(name)
        params = {"mode": "templ", "images": self._abs_images(images), "templates": names}
        if threshold is not None:
            params["threshold"] = threshold
        if roi:
            params["roi"] = list(roi)
        if resize:
            params["resize"] = list(resize)
        if task:
            params["task"] = task
        return self._run_task(params)

    def depot_items(self, images, item_ids=None, threshold=None):
        """复刻线上 DepotImageAnalyzer 的仓库物品匹配（python 侧做自定义预处理）。

        线上的自定义处理在此复刻：模板右下 80x50 涂黑（数量角标）、图 INTER_AREA 缩放
        到 DepotMatchData 的 roi 尺寸、maskRange/阈值等 Matcher 配置取自 DepotMatchData
        任务 —— 尺寸、mask、阈值直接读 tasks.json，与 core 运行时同源；缩放和匹配仍由
        core 执行。数量识别等其余链路不在此复刻，只做模板匹配评估。

        :param item_ids: 物品 ID / 模板路径 / 目录列表，默认 "items" 全部材料模板
        :return: 同 templ
        """
        from PIL import Image

        task = self._read_task_def("DepotMatchData")
        roi = task.get("roi", [0, 0, 1066, 599])

        templ_root = self._resource / "resource" / "template"
        names = []
        tmpdir = pathlib.Path(tempfile.mkdtemp(prefix="maa-eval-templ-"))
        try:
            for name in (item_ids or ["items"]):
                path = templ_root / name
                # 纯物品 ID（如 "2001"）落到 items/<id>.png，与 core get_templ 的查找一致
                if not path.exists() and not pathlib.Path(name).suffix:
                    candidate = templ_root / "items" / f"{name}.png"
                    if candidate.exists():
                        path = candidate
                files = sorted(path.rglob("*.png")) if path.is_dir() else [path]
                for src in files:
                    templ = Image.open(src).convert("RGB")
                    w, h = templ.size
                    if w > 80 and h > 50:
                        templ.paste((0, 0, 0), (w - 80, h - 50, w, h))
                    dst = tmpdir / src.name
                    templ.save(dst)
                    names.append(str(dst))
            return self.templ(
                images,
                names,
                threshold=threshold,
                resize=[roi[2], roi[3]],
                task="DepotMatchData",
            )
        finally:
            import shutil

            shutil.rmtree(tmpdir, ignore_errors=True)

    def _read_task_def(self, task_name):
        tasks_file = self._resource / "resource" / "tasks" / "tasks.json"
        with open(tasks_file, encoding="utf-8") as f:
            return json.load(f).get(task_name, {})

    def replay(self, images, tasks):
        """图片序列按 next 链逐图推进（纯识别，不执行点击等 action）。

        返回 [{step, image, hit, task, next, ...}]；链在未命中、next 为空或图耗尽处停止。
        """
        steps = []
        current = list(tasks)
        for i, image in enumerate(self._abs_images(images)):
            result = self.pipeline([image], current)[0]
            result["step"] = i
            steps.append(result)
            if not result.get("hit"):
                break
            current = result.get("next", [])
            if not current:
                break
        return steps

    def close(self):
        if getattr(self, "_handle", None):
            self._lib.AsstDestroy(self._handle)
            self._handle = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass


def _print_report(result):
    for entry in result:
        print(f"{pathlib.Path(entry['image']).name}:")
        for r in entry["results"]:
            fields = " ".join(f"{k}={r[k]}" for k in ("score", "count", "templ", "text", "rect") if k in r)
            print(f"  {'hit ' if r['hit'] else 'MISS'} {r['task']} {fields}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("images", nargs="+", help="图片路径（可多张）")
    parser.add_argument("--mode", choices=["report", "pipeline", "ocr", "templ", "depot", "replay"], default="report")
    parser.add_argument("--tasks", help="逗号分隔的任务名列表")
    parser.add_argument("--templates", help="逗号分隔的模板名（物品 ID / items/xxx.png / 目录），templ 模式用")
    parser.add_argument("--threshold", type=float, default=None, help="templ/depot 模式的 hit 判定阈值，默认取任务阈值或 0.7")
    parser.add_argument("--roi", help="ocr/templ 模式的识别区域 x,y,w,h")
    parser.add_argument("--dll-dir", default=str(REPO_ROOT / "build" / "bin" / "Debug"))
    parser.add_argument("--resource", default=str(REPO_ROOT), help="资源根目录（其下应有 resource/ 子目录）")
    parser.add_argument("--global", dest="global_client", help="国际服名，如 YoStarJP")
    parser.add_argument("--user-dir", help="core 日志写入目录")
    parser.add_argument("--verbose", action="store_true", help="打印 core 其余回调消息")
    args = parser.parse_args()

    if args.mode in ("report", "pipeline", "replay") and not args.tasks:
        parser.error(f"--mode {args.mode} 需要 --tasks")
    if args.mode == "templ" and not args.templates:
        parser.error("--mode templ 需要 --templates")
    ev = CoreEval(
        dll_dir=args.dll_dir,
        resource=args.resource,
        global_client=args.global_client,
        user_dir=args.user_dir,
        verbose=args.verbose,
    )
    try:
        if args.mode == "report":
            tasks = [t.strip() for t in args.tasks.split(",") if t.strip()]
            _print_report(ev.report(args.images, tasks))
        elif args.mode == "pipeline":
            tasks = [t.strip() for t in args.tasks.split(",") if t.strip()]
            for r in ev.pipeline(args.images, tasks):
                hit = "hit " if r["hit"] else "MISS"
                extras = " ".join(f"{k}={r[k]}" for k in ("score", "count", "text", "rect") if k in r)
                print(f"{pathlib.Path(r['image']).name}: {hit} {r.get('task', '')} {extras} next={r['next']}")
        elif args.mode == "ocr":
            roi = [int(v) for v in args.roi.split(",")] if args.roi else None
            for entry in ev.ocr(args.images, roi):
                print(f"{pathlib.Path(entry['image']).name}:")
                for r in entry["results"]:
                    print(f"  {r['score']:.4f} {r['text']} {r['rect']}")
        elif args.mode == "templ":
            roi = [int(v) for v in args.roi.split(",")] if args.roi else None
            templates = [t.strip() for t in args.templates.split(",") if t.strip()]
            for entry in ev.templ(args.images, templates, threshold=args.threshold, roi=roi):
                print(f"{pathlib.Path(entry['image']).name}:")
                for r in entry["results"]:
                    if "error" in r:
                        print(f"  ERR  {r['template']} {r['error']}")
                    else:
                        mark = "hit " if r["hit"] else "MISS"
                        print(f"  {mark} {r['template']} score={r.get('score', 0):.4f} {r.get('rect', '')}")
        elif args.mode == "depot":
            item_ids = [t.strip() for t in args.templates.split(",")] if args.templates else None
            for entry in ev.depot_items(args.images, item_ids=item_ids, threshold=args.threshold):
                print(f"{pathlib.Path(entry['image']).name}:")
                for r in sorted(entry["results"], key=lambda x: -x.get("score", 0)):
                    name = pathlib.Path(r["template"]).stem
                    if "error" in r:
                        print(f"  ERR  {name} {r['error']}")
                    elif r["hit"]:
                        print(f"  hit  {name} score={r.get('score', 0):.4f} {r.get('rect', '')}")
        else:
            tasks = [t.strip() for t in args.tasks.split(",") if t.strip()]
            for s in ev.replay(args.images, tasks):
                hit = "hit " if s["hit"] else "MISS"
                extras = " ".join(f"{k}={s[k]}" for k in ("score", "count", "text", "rect") if k in s)
                print(f"step{s['step']} {pathlib.Path(s['image']).name}: {hit} {s.get('task', '')} {extras} next={s['next']}")
    finally:
        ev.close()


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    main()
