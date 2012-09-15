from scrapy.spider import BaseSpider
from scrapy.http import FormRequest

class PandoraSpider(BaseSpider):
    name = "pandora"
    allowed_domains = ["pandora.com"]
    start_urls = [
        "http://www.pandora.com/account/sign-in",
        ]
    
    def parse(self, response):
        return [FormRequest.from_response(response,
                                          formdata={'email': 'foo',
                                                    'password': 'bar'},
                                          dont_click=True,
                                          callback=self.after_login)]

    def after_login(self, response):
        # check login is successful 
        
        print "okay"
        return 
    
        if "I'm sorry, I don't recognize your email and password." in response.body:
            self.log("Login failed", level=log.ERROR)
        else:
            self.log("Login success", level=log.INFO)
            
        return 
