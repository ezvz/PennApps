require 'rubygems'
require 'watir-webdriver'
require 'nokogiri'
require 'headless'
require 'mongo'
require 'net/http'
require 'uri'

def add_to_mongo(hash)
	puts "Adding to database..."
	@conn = Mongo::Connection.new
	@db   = @conn['prod']
	@coll = @db['rubysongs']
	index = @coll.count + 1

	hash.each_pair do |key, value|
		@coll.insert({"info" => key + "~" + value[0] + "~" + value[1] + "~" + value[2]})
		index = index + 1
	end
end

def login_to_pandora(username, password)
	browser = Watir::Browser.start "http://www.pandora.com/account/sign-in"
	puts "Login page reached. Logging in..."
	browser.text_field(:name => 'email').when_present.set(username)
	browser.text_field(:name => 'password').when_present.set(password)
	browser.button(:value, 'Sign in').click
	puts "Login successful. Navigating to likes page..."
	return browser
end

def goto_likes(browser)
	browser.goto 'http://www.pandora.com/profile/likes/pennapps'
	browser.wait_until {browser.div(:id, "track_like_pages").exists? }
	puts "Likes page reached."
	return browser
end

def expand_likes(browser)
	puts "Processing list..."
	counter = 0
	while(!browser.div(:class, "no_more tracklike").exists?)
		show_more = browser.div(:class => "show_more", :index => counter)
		if show_more.visible?
			show_more.fire_event("onclick")
		end
		counter = counter + 1
		sleep 2
	end
	return browser
end

def xpath_to_array(html, xp)
	out = []
	html.xpath(xp).each do |obj|
		out.push(obj.text)
	end
	return out
end

def parse_likes(browser, username)
	puts "Scraping songs..."
	page_html = Nokogiri::HTML.parse(browser.html)

	name_xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "first", " " ))]'
	station_xpath = '//*[contains(concat( " ", @class, " " ), concat( " ", "like_context_stationname", " " ))]';
	artist_xpath = '//p//a[(((count(preceding-sibling::*) + 1) = 1) and parent::*)]'

	songs = {}
	names = xpath_to_array(page_html, name_xpath)
	stations = xpath_to_array(page_html, station_xpath)
	artists = xpath_to_array(page_html, artist_xpath)
	names.each_with_index {|k,i| songs[k] = [artists[i], stations[i], username]}
	
	puts songs.keys.size.to_s + " songs found: "
	puts songs
	return songs
end

def scrape_pandora(username, password)
	headless = Headless.new
	headless.start

	browser = login_to_pandora(username, password)
	browser = goto_likes(browser)
	browser = expand_likes(browser)
	songs = parse_likes(browser, username)
	add_to_mongo(songs)

	headless.destroy
	puts "Done!"
	return songs
end

output = scrape_pandora(ARGV[0], ARGV[1])
