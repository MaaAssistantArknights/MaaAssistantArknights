import os
import shutil
import tempfile
from concurrent.futures import ThreadPoolExecutor
from threading import Lock
import requests

# 获取文件大小
def length(urlist):
    def getlenhead(url):
        response = requests.head(url)
        file_size = response.headers.get('Content-Length')
        if file_size is not None:
            return int(file_size)
        else:
            return False
    for url in urlist:
        length = getlenhead(url)
        if(length):
            return length

# 定义Download类在初始化时保存几个参数
class Downloader:
    # 初始化类
    def __init__(self, urlist, chunksize, max_conn, proxies=None):
        self.urlist = urlist        # 镜像url列表
        self.chunksize = chunksize  # 分片大小
        self.max_conn = max_conn    # 单个url最大连接数
        self.proxies = proxies      # 代理服务器
        self.lock = Lock()
        self.chunk_status = []      # 状态列表
        self.failed_requests = {url: {'success': 0, 'fail': 0} for url in urlist} # 记录每个 URL 的失败次数和成功次数

    def download_chunk(self, url, chunk_id, total_size):
        start = chunk_id * self.chunksize
        end = min(start + self.chunksize - 1, total_size - 1)
        headers = {'Range': f'bytes={start}-{end}'}
        filename = f"temp/{chunk_id}"
        if self.chunk_status[chunk_id] != 2:
            try:
                response = requests.get(url, headers=headers, proxies=self.proxies)
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

                if self.failed_requests[url]['fail'] / (self.failed_requests[url]['success'] + self.failed_requests[url]['fail']) > 0.5 and self.failed_requests[url]['fail'] > 10:
                    # 如果某 URL 的失败率高于 50%，则跳过该 URL
                    return
            except requests.RequestException as e:
                if self.chunk_status[chunk_id] == 1:
                    self.chunk_status[chunk_id] = 0

    def download_file(self, total_size, filepath):
        num_chunks = (total_size + self.chunksize - 1) // self.chunksize
        self.chunk_status = [0] * num_chunks
        os.makedirs('temp', exist_ok=True)
        with ThreadPoolExecutor(max_workers=self.max_conn * len(self.urlist)) as executor:
            for url in self.urlist:
                for chunk_id in range(num_chunks):
                    executor.submit(self.download_chunk, url, chunk_id, total_size)

        # 合并所有临时文件到一个文件
        with open(filepath, 'wb') as outfile:
            for chunk_id in range(num_chunks):
                filename = f"temp/{chunk_id}"
                with open(filename, 'rb') as infile:
                    shutil.copyfileobj(infile, outfile)

        # 删除临时目录
        shutil.rmtree('temp')

        # 验证下载文件
        if os.path.getsize(filepath) != total_size:
            print("文件大小不一致，下载可能出错。")

def file_download(urlist, filepath, proxies=None):
    chunksize = 1024 * 1024  # 分片大小1MB
    max_conn = 4
    # 创建对象
    downloader = Downloader(urlist, chunksize, max_conn, proxies=proxies)

    # 下载文件
    total_size = length(urlist)
    print("文件大小已获取，开始下载")
    return downloader.download_file(total_size, filepath)

if __name__ == '__main__':
    # 测试。。。
    urlist = [r"https://mirror.eh.cx/KataGo/katago/pack/2023-06-15/2023-06-15-Macosx(amd64)+Linux(amd64)(无引擎).zip",r"https://ota.eh.cx/KataGo/katago/pack/2023-06-15/2023-06-15-Macosx(amd64)+Linux(amd64)(无引擎).zip",r"https://fake.eh.cx/a"]
    # 其中前两个是有效的url，第三个是无效的url
    path = r"D:\Downloads\2023-06-15-Macosx(amd64)+Linux(amd64)(无引擎).zip"
    file_download(urlist,path)
