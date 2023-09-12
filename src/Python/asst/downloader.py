import os
import shutil
import json
from concurrent.futures import ThreadPoolExecutor
from threading import Lock
import requests


# 获取文件大小
def length(url_list):
    def getlenhead(single_url):
        response = requests.head(single_url)
        file_size = response.headers.get('Content-Length')
        if file_size is not None:
            return int(file_size)
        else:
            return False

    for url in url_list:
        single_file_length = getlenhead(url)
        if single_file_length:
            return single_file_length


# 定义Download类在初始化时保存几个参数
class Downloader:
    # 初始化类
    def __init__(self, urlist, chunksize, max_conn, use_proxies=None):
        self.urlist = urlist  # 镜像url列表
        self.chunksize = chunksize  # 分片大小
        self.max_conn = max_conn  # 单个url最大连接数
        self.proxies = use_proxies  # 代理服务器
        self.lock = Lock()
        self.chunk_status = []  # 状态列表
        self.failed_requests = {url: {'success': 0, 'fail': 0} for url in urlist}  # 记录每个 URL 的失败次数和成功次数
        self.listhash = hex(hash(tuple(urlist)))  # 计算urlist的hash

    def download_chunk(self, url, chunk_id, total_size):
        start = chunk_id * self.chunksize
        end = min(start + self.chunksize - 1, total_size - 1)
        headers = {'Range': f'bytes={start}-{end}'}
        filename = f"temp/{self.listhash}/{chunk_id}"
        if self.chunk_status[chunk_id] != 2:
            try:
                response = requests.get(url, headers=headers, proxies=self.proxies, timeout=5)
                if response.status_code in (301, 302, 303, 307, 308):  # 处理HTTP 3xx 重定向问题，继续发送原来的header（range）
                    redirect_url = response.headers['Location']
                    response = requests.get(redirect_url, headers=headers, timeout=3, proxies=self.proxies)

                if response.status_code == 206:  # 206状态码
                    self.failed_requests[url]['success'] += 1
                    with open(filename, 'wb') as file:
                        file.write(response.content)
                    self.chunk_status[chunk_id] = 2
                elif response.status_code >= 400:
                    self.failed_requests[url]['fail'] += 1

                if self.failed_requests[url]['fail'] / (
                        self.failed_requests[url]['success'] + self.failed_requests[url]['fail']) > 0.5 and \
                        self.failed_requests[url]['fail'] > 10:
                    # 如果某 URL 的失败率高于 50%，则跳过该 URL
                    return
            except requests.RequestException as e:
                if self.chunk_status[chunk_id] == 1:
                    self.chunk_status[chunk_id] = 0

    def download_file(self, total_size, file_path):
        num_chunks = (total_size + self.chunksize - 1) // self.chunksize
        self.chunk_status = [0] * num_chunks
        try:
            shutil.rmtree(f'temp/{self.listhash}')
        except:
            pass
        os.makedirs(f'temp/{self.listhash}', exist_ok=True)
        with ThreadPoolExecutor(max_workers=self.max_conn * len(self.urlist)) as executor:
            for url in self.urlist:
                for chunk_id in range(num_chunks):
                    executor.submit(self.download_chunk, url, chunk_id, total_size)

        # 合并所有临时文件到一个文件
        with open(file_path, 'wb') as outfile:
            for chunk_id in range(num_chunks):
                filename = f"temp/{self.listhash}/{chunk_id}"
                with open(filename, 'rb') as infile:
                    shutil.copyfileobj(infile, outfile)

        # 删除临时目录
        shutil.rmtree(f'temp/{self.listhash}')

        # 验证下载文件
        if os.path.getsize(file_path) != total_size:
            print("文件大小不一致，下载可能出错。")


def file_download(download_url_list, download_path, request_proxies=None):
    chunksize = 1024 * 1024     # 分片大小1MB
    max_conn = 4                # 最大连接数
    # 创建对象
    downloader = Downloader(download_url_list, chunksize, max_conn, use_proxies=request_proxies)

    # 下载文件
    total_size = length(download_url_list)
    print("文件大小已获取，开始下载")
    return downloader.download_file(total_size, download_path)


if __name__ == '__main__':
    while True:
        json_string = input("请输入下载指令：")
        # 解析JSON字符串
        """
        Example:
        {
            "urlist":["https://mirror1.example.com/file.zip","https://mirror2.example.com/file.zip","https://mirror3.example.com/file.zip"],
            "filepath":"C:\\Download\\File.zip"
            "proxies":"http://127.0.0.1:7890"
        }
        """
        try:
            data = json.loads(json_string)
            if isinstance(data, dict):
                # 这两个参数是必须的
                urlist = data.get("urlist", None)
                filepath = data.get("filepath", None)
                # 这个参数是可选的
                proxies = data.get("proxies", None)

                # 对于变量的类型进行检查
                if not isinstance(urlist, list) or not all(isinstance(url, str) for url in urlist):
                    print("urlist类型错误")
                    continue
                if not isinstance(filepath, str):
                    print("filepath类型错误")
                    continue
                if proxies is not None and not isinstance(proxies, str):
                    print("proxies类型错误")
                    continue

                # 启动下载
                file_download(download_url_list=urlist, download_path=filepath, request_proxies=proxies)
            else:
                print("Json结构错误")
        except json.JSONDecodeError:
            print("无法解析JSON。")
