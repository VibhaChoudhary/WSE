import urllib.robotparser
from urllib.parse import urljoin
from urllib.parse import urlparse
import urllib.request
import socket
import logging
timeout = 10
socket.setdefaulttimeout(timeout)


class Validate:
    def __init__(self):
        self.robo = urllib.robotparser.RobotFileParser()
        self.allowed_mime_types = ["text/html", "text/plain", "text/enriched"]
        self.extensions_blacklist = [".cgi", ".gif", ".jpg", ".png", ".css", ".js", ".mp3", ".mp4", ".mkv", ".ppt", ".doc", ".pdf",
                                     ".pptx", ".docx", ".rar", ".zip", ".xls", ".xlsx" , ".tar.gz"".tar.bz2"]
        self.folders_blacklist = ["/cgi-bin/", "/images/", "/javascripts/", "/photos/", "/js/", "/css/", "/stylesheets/"]
        self.starts_with_blacklist = ["#", "mailto:", "javascript:", "whatsapp:", "tel:", "geo:", "news:", "ftp"]

    def has_crawl_permission(self, url):
        try:
            parsed_uri = urlparse(url)
            root = '{uri.scheme}://{uri.netloc}/'.format(uri=parsed_uri)
            self.robo.set_url(urljoin(root, 'robots.txt'))
            self.robo.read()
            return self.robo.can_fetch("*", url)
        except Exception as e:
            return False
        return True

    def has_illegal_folder(self, url):
        for folder in self.folders_blacklist:
            if folder in url:
               return True
        return False

    def has_illegal_extension(self, url):
        for extension in self.extensions_blacklist:
            if url.endswith(extension):
               return True
        return False

    def has_illegal_start(self, url):
        for start in self.starts_with_blacklist:
            if url.startswith(start):
                return True
        return False

    def has_valid_mime_type(self, response):
        info = response.info()
        if info.get_content_type() in self.allowed_mime_types:
            return True
        return False


