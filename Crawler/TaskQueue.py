import datetime
import hashlib
import heapq
import itertools
import queue as Q
import sys
from urllib.parse import urlparse

counter = itertools.count()  # unique sequence count
try:
    cmp
except NameError:
    cmp = lambda x, y: (x > y) - (x < y)


class Task:
    def __init__(self, url, depth=1, importance=1, novelty=sys.maxsize):
        self.taskid = hashlib.md5(url.encode("utf-8")).hexdigest()
        self.host = '{uri.netloc}/'.format(uri=urlparse(url))
        self.url = url
        self.depth = depth
        self.importance = importance
        self.novelty = novelty
        self.crawl_time = datetime.datetime.now().strftime("%H:%M:%S.%f")
        self.response = 0
        self.page_size = 0
        self.sequence = next(counter)

    def __cmp__(self, other):
        if self.novelty != other.novelty:
            diff = -cmp(self.novelty, other.novelty)
        elif self.importance != other.importance:
            diff = -cmp(self.importance, other.importance)
        else:
            diff = cmp(self.sequence, other.sequence)
        return diff

    def __lt__(self, other):
        return self.__cmp__(other) < 0

    def __str__(self):
        return "Url:" + self.url + " " + "Depth:" + str(self.depth) + " " + \
               "Page size:" + str(self.page_size) + " " \
               "Time:" + str(self.crawl_time) + " " + "Response:" + str(self.response) + " " + \
               "Importance:" + str(self.importance) + " " + "Novelty:" + str(self.novelty)

class TaskPriorityQueue(Q.Queue):
    def _init(self, maxsize):
        self.queue = []
        self.queue_dict = dict()

    def _qsize(self, len=len):
        return len(self.queue_dict)

    def _put(self, task, heappush=heapq.heappush):
        if task.taskid in self.queue_dict:
            current = self.queue_dict[task.taskid]
            current.importance = max(task.importance, current.importance)
            current.novelty = min(task.novelty, current.novelty)
            if task > current:
                heapq.heapify(self.queue)
        else:
            heappush(self.queue, task)
            self.queue_dict[task.taskid] = task

    def _get(self, heappop=heapq.heappop):
        while self.queue:
            current = heappop(self.queue)
            if current.taskid is None:
                continue
            self.queue_dict.pop(current.taskid, None)
            return current
        return None

    def __contains__(self, taskid):
        return taskid in self.queue_dict

    def __getitem__(self, taskid):
        return self.queue_dict[taskid]

    def __setitem__(self, taskid, item):
        assert item.taskid == taskid
        self.put(item)

    def __delitem__(self, taskid):
        self.queue_dict.pop(taskid).taskid = None
