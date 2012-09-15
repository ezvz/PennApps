# Scrapy settings for pandora_scraper project
#
# For simplicity, this file contains only the most important settings by
# default. All the other settings are documented here:
#
#     http://doc.scrapy.org/topics/settings.html
#

BOT_NAME = 'pandora_scraper'
BOT_VERSION = '1.0'

SPIDER_MODULES = ['pandora_scraper.spiders']
NEWSPIDER_MODULE = 'pandora_scraper.spiders'
DEFAULT_ITEM_CLASS = 'pandora_scraper.items.PandoraScraperItem'
USER_AGENT = '%s/%s' % (BOT_NAME, BOT_VERSION)

