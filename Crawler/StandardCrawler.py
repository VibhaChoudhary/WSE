import threading
from googlesearch import search
from TaskQueue import *
from LinkExtractor import *

logging.basicConfig(level=logging.INFO, filename='standard_crawler.log', filemode='w',
                    format="%(message)s")
SEED_COUNT = 10
MAX_THREADS = 1000
MAX_CRAWL = 1000
MAX_CRAWL_PER_SITE = 100

class MyCrawler:
    def __init__(self, seed):
        self.lock1 = threading.RLock()
        self.lock2 = threading.RLock()
        self.validator = Validate()
        self.seed_url = seed
        self.crawl_count = 0
        self.fetch_count = 0
        self.frontier = TaskPriorityQueue()
        self.page_crawled = dict()
        self.site_crawled = dict()
        self.total_4xx = 0
        self.total_error = 0
        self.total_size = 0
        self.build_frontier()

    def build_frontier(self):
        # initialise priority queue with seed urls
        for url in self.seed_url:
            if self.validator.has_illegal_folder(url):
                continue
            if self.validator.has_illegal_extension(url):
                continue
            task = Task(url)
            # normalise site url
            key = hashlib.md5(task.host.encode("utf-8")).hexdigest()
            # initialise parse count and novelty score for site
            if key not in self.site_crawled:
                self.site_crawled[key] = 1
            self.frontier.put(task)


    def crawl(self):
        while self.crawl_count < MAX_CRAWL:
            task = self.frontier.get()
            # self.frontier.task_done()
            key = hashlib.md5(task.host.encode("utf-8")).hexdigest()
            task.crawl_time = datetime.datetime.now().strftime("%H:%M:%S.%f")
            with self.lock1:
                # update crawled sites
                self.page_crawled[task.taskid] = task.url
                if self.crawl_count < MAX_CRAWL:
                    self.crawl_count += 1
                    self.site_crawled[key] += 1
                else:
                    self.frontier.task_done()
                    break
            # dispatch the task to fetch method
            self.fetch(task)
            self.frontier.task_done()


    def fetch(self, task):
        # scrape all the links
        parser = LinksExtractor(task.url)
        task.response = parser.response
        if parser.size is not None:
            task.page_size = parser.size

        # output to file and console
        logging.info("%s", task)
        print(task)

        # update statistics
        with self.lock2:
            self.fetch_count += 1
            self.total_size += task.page_size
            if (task.response.__str__().endswith("error")):
                self.total_error += 1
            if(task.response.__str__().startswith("4") ):
                self.total_4xx += 1

        # push new links to queue
        for link in parser.get_links():
            if self.crawl_count >= MAX_CRAWL:
                break
            new_task = Task(link)
            new_task.depth = task.depth + 1
            key = hashlib.md5(new_task.host.encode("utf-8")).hexdigest()
            # check if the url is already crawled
            if new_task.taskid in self.page_crawled:
                continue
            # check if the url is already in the queue
            if new_task.taskid in self.frontier:
                continue
            # check if the url belongs to a crawled site
            if key not in self.site_crawled:
                with self.lock1:
                    self.site_crawled[key] = 1
            elif self.site_crawled[key] > MAX_CRAWL_PER_SITE:
                continue
            # push new url to queue
            self.frontier.put(new_task)

# entry point
def main():
    query = input("Enter your query ")
    # get seed urls from google search
    start_time = datetime.datetime.now()
    seed = search(query, tld="com", num=SEED_COUNT, stop=SEED_COUNT)
    # initialise crawler with seed urls
    mc = MyCrawler(seed)
    start_time = datetime.datetime.now()
    while mc.crawl_count < MAX_CRAWL:
        # call crawl
        for i in range(min((MAX_THREADS - threading.active_count()), mc.frontier.qsize())):
            threading.Thread(target = mc.crawl, args=()).start()
    while mc.fetch_count < MAX_CRAWL:
        continue
    diff = (datetime.datetime.now() - start_time).total_seconds()
    logging.info("Total crawl time: %ss Total crawled: %s, Total 4xx: %s, Total_errors: %s Total_size %s",
                 diff, mc.crawl_count, mc.total_4xx, mc.total_error, mc.total_size)
    print("Total crawl time:" + diff.__str__() + " Total crawled:" + mc.crawl_count.__str__() +
          " Total 4xx:" + mc.total_4xx.__str__() + " Total_errors:" + mc.total_error.__str__() +
          " Total_size:" + mc.total_size.__str__() + "KB\n")

if __name__ == '__main__':
    main()
