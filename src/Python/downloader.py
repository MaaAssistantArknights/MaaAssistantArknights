import requests, os
from concurrent.futures import ThreadPoolExecutor, as_completed
from threading import Lock

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
    def __init__(self, urlist, chunksize, max_conn):
        self.urlist = urlist        # 镜像url列表
        self.chunksize = chunksize  # 分片大小
        self.max_conn = max_conn    # 单个url最大连接数
        self.lock = Lock()
        self.chunk_status = []      # 状态列表

    def download_chunk(self, url, chunk_id, filepath):
        start = chunk_id * self.chunksize
        end = start + self.chunksize - 1
        headers = {'Range': f'bytes={start}-{end}'}     # 定义header，只定义Range
        try:
            with self.lock:
                if self.chunk_status[chunk_id] != 2:    # 检查如果还未下载完（0或1）
                    self.chunk_status[chunk_id] = 1     # 将当前区块标注为正在下载
                else:
                    return
            response = requests.get(url, headers=headers)
            if response.status_code in (301, 302, 303, 307, 308):  # 处理HTTP 3xx 重定向问题，继续发送原来的header（range）
                redirect_url = response.headers['Location']
                response = requests.get(redirect_url, headers=headers)
            if response.status_code == 206:  # 206状态码
                with self.lock:
                    if self.chunk_status[chunk_id] != 2:            # 检查是否已经下载完成，如果还没下载完成则写入
                        self.write(filepath, response.content, start)
                        self.chunk_status[chunk_id] = 2             # 将状态标注为已完成
        except requests.RequestException as e:                      # 处理下载出错的情况
            with self.lock:
                if self.chunk_status[chunk_id] == 1:                # 如果当前区块状态为正在下载中
                    self.chunk_status[chunk_id] = 0                 # 重置状态为0，使其它线程优先接手该区块

    def write(self, filepath, data, position):
        with open(filepath, 'rb+') as file:
            file.seek(position)
            file.write(data)

    def download_file(self, total_size, filepath):
        num_chunks = (total_size + self.chunksize - 1) // self.chunksize
        self.chunk_status = [0] * num_chunks
        with ThreadPoolExecutor(max_workers=self.max_conn * len(self.urlist)) as executor:
            futures = {executor.submit(self.download_chunk, url, chunk_id, filepath) 
                       for chunk_id in range(num_chunks) for url in self.urlist}
            for future in as_completed(futures):
                future.result()

        # 验证下载文件大小
        if os.path.getsize(filepath) != total_size:
            print("文件大小不符合，下载可能出错。")

def file_download(urlist):
    chunksize = 1024 * 512  # 分片大小512KB
    max_conn = 2
    # 创建对象
    downloader = Downloader(urlist, chunksize, max_conn)

    # 下载文件
    total_size = length(urlist)
    filepath = 'path/to/your/file'
    return downloader.download_file(total_size, filepath)

if __name__ == '__main__':
    urlist = []
    statu = file_download(urlist)
    if statu:
        print("下载完成")