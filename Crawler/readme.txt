All the files:

1. PrioritisedCrawler.py => Main file to run Priority Crawler.
2. StandardCrawler.py => Main file to run Standard Crawler
3. TaskQueue.py => Priority queue implementation used by the crawler
4. LinkExtractor.py => Responsible for scraping the page and returning all href links
5. Validator.py => Provides utility methods for validating page for eg, checking for crawl permission, checking valid
   mime types, etc

How to Run the Crawler:
1. To run priority crawler
    python  PrioritisedCrawler.py

2. To run standard crawler
    python  StandardCrawler.py

The crawler program prompts the user to enter query in console. Other settings of the crawler can be modified by changing
following constants in PrioritisedCrawler.py or StandardCrawler.py:

# SEED_COUNT = 10             Number of google search results to be provided as seed url
# NUM_THREADS = 20            Number of threads
# MAX_CRAWL = 1000            Maximum pages allowed to be crawled
# MAX_CRAWL_PER_SITE = 100    Maximum pages allowed to be crawled per site

Each crawler run creates a log in the same directory.
=> PrioritisedCrawler.py creates prioritisedcrawler.log
=> StandardCrawler.py creates standardcrawler.log

