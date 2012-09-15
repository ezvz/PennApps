require 'rubygems'
require 'watir-webdriver'
require 'nokogiri'
require 'headless'

headless = Headless.new
headless.start

browser = Watir::Browser.start "http://www.pandora.com/account/sign-in"

#form["login_username"] = "pennapps@team.com"
#form["login_password"] = "password"

browser.text_field(:name => 'email').when_present.set("pennapps@team.com")

browser.text_field(:name => 'password').when_present.set("password")

browser.button(:value, 'Sign in').click



browser.goto 'http://www.pandora.com/profile/likes/pennapps'

browser.wait_until {browser.div(:id, "track_like_pages").exists? }

page_html = Nokogiri::HTML.parse(browser.html)

xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "first", " " ))]'
puts page_html.xpath(xpath).inner_text

#browser.div(:class, "show_more").fireEvent("onmousedown")

browser.div(:class, "show_more").fire_event("onclick")

page_html = Nokogiri::HTML.parse(browser.html)

xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "first", " " ))]'
puts page_html.xpath(xpath).inner_text
headless.destroy

#basta@mozilla.com

	#browser.text_field(:id => 'login_password').when_present.set("password")