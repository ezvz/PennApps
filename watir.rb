require 'rubygems'
require 'watir-webdriver'
require 'nokogiri'
require 'headless'
require 'mongo'

def add_to_mongo(hash)
	@conn = Mongo::Connection.new
	@db   = @conn['pandorify']
	@coll = @db['Song']
	index = @coll.count + 1

	hash.each_pair do |key, value|
		@coll.insert({index => key + "~" + value[0] + "~" + value[1]})
		index = index + 1
	end
end

def scrape_pandora(username, password)

	headless = Headless.new
	headless.start

	browser = Watir::Browser.start "http://www.pandora.com/account/sign-in"


	browser.text_field(:name => 'email').when_present.set(username)

	browser.text_field(:name => 'password').when_present.set(password)

	browser.button(:value, 'Sign in').click

	browser.goto 'http://www.pandora.com/profile/likes/pennapps'

	browser.wait_until {browser.div(:id, "track_like_pages").exists? }

	page_html = Nokogiri::HTML.parse(browser.html)

	xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "first", " " ))]'
	puts page_html.xpath(xpath).inner_text

	#browser.div(:class, "show_more").fireEvent("onmousedown")

	counter = 0
	while(!browser.div(:class, "no_more tracklike").exists?)
		d = browser.div(:class => "show_more", :index => counter)
		if d.visible?
			d.fire_event("onclick")
		end
		counter = counter + 1
		sleep 4
	end

	page_html = Nokogiri::HTML.parse(browser.html)

	songs = {}

	name_xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "first", " " ))]'
	station_xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "like_context_stationname", " " ))]';
	artist_xpath = '//p//a[(((count(preceding-sibling::*) + 1) = 1) and parent::*)]'

	names = []
	stations = []
	artists = []

	page_html.xpath(name_xpath).each do |name|
		names.push(name.text)
	end
	page_html.xpath(station_xpath).each do |station|
		stations.push(station.text)
	end
	page_html.xpath(artist_xpath).each do |artist|
		artists.push(artist.text)
	end
	names.each_with_index {|k,i| songs[k] = [artists[i], stations[i]]}

	puts songs

	add_to_mongo(songs)

	headless.destroy
end

scrape_pandora("pennapps@team.com", "password")
#basta@mozilla.com

	#browser.text_field(:id => 'login_password').when_present.set("password")