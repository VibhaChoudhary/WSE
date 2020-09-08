from urllib.request import urlopen
from bs4 import BeautifulSoup
from Validator import *


class LinksExtractor:

    def __init__(self, url):
        self.response = 0
        self.size = 0
        self.url = url
        self.links = []  # create an empty list for storing hyperlinks
        self.validator = Validate()
        self.scrape()

    def scrape(self):
        try:
            html_page = urlopen(self.url, timeout=5)
            self.response = html_page.getcode()
            if not self.validator.has_valid_mime_type(html_page):
                return
            if not self.validator.has_crawl_permission(self.url):
                return
            content = html_page.read()
            self.size = content.__sizeof__()/1024
            soup = BeautifulSoup(content, 'html.parser', from_encoding="utf-8")
            links = []
            for link in soup.findAll('frame'):
                links.append(link.get('src'))
            for link in soup.findAll('a'):
                links.append(link.get('href'))
            for url in links:
                if url is not None:
                    if self.validator.has_illegal_start(url):
                        continue
                    if self.validator.has_illegal_folder(url):
                        continue
                    if self.validator.has_illegal_extension(url):
                        continue
                    if (url.startswith('http://') or url.startswith('https://')):
                        self.links.append(url)
                    else:
                        parsed_uri = urlparse(self.url)
                        base = '{uri.scheme}://{uri.netloc}/'.format(uri=parsed_uri)
                        url = urljoin(base, url)
                        self.links.append(url)

        except urllib.error.HTTPError as h:
            self.response = h.code
            return
        except urllib.error.URLError as u:
            self.response = "url open error"
            return
        except Exception as e:
            self.response = "time out error"
            return

    def get_links(self):  # return the list of extracted links
        return self.links


