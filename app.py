import os
import subprocess
import sendgrid
import Queue
from getUri import *
from flask import Flask, render_template, request
from pymongo import Connection
from subprocess import call
import logging

logging.basicConfig(filename='logs/app.log',level=logging.DEBUG)

app = Flask(__name__)
app.config['DEBUG'] = True
#app.config['MONGOALCHEMY_DATABASE'] = 'prod'
#db = MongoAlchemy(app)

connection = Connection('localhost', 27017)
db = connection['prod']

# Run Ruby Process here on next
#sendEmail(next.user_email, next.p_uname)

import Queue, threading
queue = Queue.Queue()

def refresh_tables():
	db.songs.drop()

def convert(person):
    songs = db.songs
    filename = "stations_songs_"+person+".txt"
    fp = file(filename, 'w')
    delimiter = "---"

    stations = songs.distinct("station")
    for station in stations:
        print >>fp, delimiter
        print >>fp, station
        for song in songs.find({"station": station, "has_uri": 1}):
            print >>fp, song['uri']
    fp.close()
    return filename

def run_spotify_tool(person, filename):
	spotify_cmd = "./pandorify "+person['s_uname']+" "+ person['s_pword']+" "+filename
	process = None
	def target():
		logging.info("Spotify process started with cmd: "+spotify_cmd)
		process = subprocess.Popen(spotify_cmd, shell=True, stderr=subprocess.PIPE)
		(stdout, stderr) = process.communicate()
		logging.info("Spotify process finished")
		logging.debug(stderr)

	thread = threading.Thread(target=target)
	thread.start()
	
	thread.join(20000)
	if thread.is_alive():
		logging.info("Spotify hung, restarting")
		process.terminate()
		thread.join()
		return False
	return True

class ThreadCrawler(threading.Thread):
	def __init__(self, queue):
		threading.Thread.__init__(self)
		self.queue = queue
		
	def run(self):
		logging.debug("Started thread")
		while True:
			person = self.queue.get()
			pandora_user = person['p_uname']
			logging.info("Processing new person %s", pandora_user)
			email = person['user_email']
			logging.debug("Calling Ruby script")
			try: 
				subprocess.check_call(["ruby", "watir.rb", person['p_uname'], person['p_pword']])
			except subprocess.CalledProcessError: 
				logging.error("Watir failed to scrape")
				refresh_tables()
				self.queue.task_done()
				return
			songs = get_ruby_songs(pandora_user)
			format_songs(songs, email)
			logging.debug("Calling spotify tools")
			filename = convert(pandora_user)
			if person['s_uname'] and person['s_pword']:
				while not run_spotify_tool(person, filename):
					pass
			logging.info("Finished processing person")
			sendEmail(email, person['user_email'])
			#refresh_tables()
			self.queue.task_done()
			
t = ThreadCrawler(queue)
t.setDaemon(True)
t.start()
	
@app.route('/', methods=['POST', 'GET'])
def route_root():
	saved = ''
	songs = ''
	error = ''
	if request.method == 'POST':
		#if request.form['songs']:
		#	return request.form['songs']
		logging.info("POST recieved")

		pandora_user = request.form['pandoraUsername']
		pandora_pass = request.form['pandoraPassword']
		spotify_user = request.form['spotifyUsername']
		spotify_pass = request.form['spotifyPassword']
		email = request.form['email']

		person = {	"s_uname": spotify_user,
					"s_pword": spotify_pass,
					"p_uname": pandora_user,
					"p_pword": pandora_pass,
					"user_email": email }
		queue.put(person)
		logging.info("added to queue")


		saved = "Your Spotify playlists are getting Pandorify'd!"

	return render_template('index.html', saved=saved)

def sendEmail(email, pandora_user):
	slist = get_songs_by_user(email)
	playlists = []
	songs = []
	songsValid = []
	playlistName = []
	uri = []
	for elem in slist:
		songInfo = elem.split("~")
		songs.append(songInfo[0])
		uri.append(songInfo[3])
		if songInfo[2] == 1:
			songsValid.append(1)
		else:
			songsValid.append(0)
		playlistName.append(songInfo[1]) #CHECK INDEX FOR PLAYLIST
		if not songInfo[1] in playlists: #CHECK INDEX FOR PLAYLIST
			playlists.append(songInfo[1]) #CHECK INDEX FOR PLAYLIST

	s = sendgrid.Sendgrid('varantz', 'sendPass123', secure=True)
	#message = sendgrid.Message("notifications@pandorify.us", "Pandorify!", "", ("<b>Your playlist" + plural(playlists) + " ready!</b> <br />" + getBody(["Test PL 1", "Test PL 2"], ["song1","song2","song3","song4" ], [1,1,1,1], ["Test PL 1", "Test PL 2", "Test PL 1", "Test PL 2"], ["a","b","gg","aa"])))
	message = sendgrid.Message("notifications@pandorify.us", "Pandorify!", "", ("<b>Your playlist" + plural(playlists) + " ready!</b> " + getBody(playlists, songs, songsValid, playlistName, uri)))
	message.add_to(email, pandora_user)

	s.web.send(message)
	return

def plural(playlists):
	if len(playlists)>1:
		return "s are "
	else:
		return " is "

def getBody(playlists, songs, songsValid, playlistName, uri):
	toRet = ""
	for pl in playlists:
		toRet = toRet + "<p>" + "Songs added from " + pl + ":" + "<br /> "
		toRet = toRet + "<ol>"
		for i in range(len(songs)):
			if songsValid[i] == 1:
				if playlistName[i] == pl:
					toRet = toRet + "	" + songs[i] + " - " + uri[i] +   "<br />"
		toRet = toRet + "</ol>"
		temp =  "<br />"   + "Songs not found on Spotify from " + pl + ":" + "<br />" + "<ol>"
		temp2 = ""
		for i in range(len(songs)):
			if songsValid[i] == 0:
				if playlistName[i] == pl:
					temp2 = temp2 + "	" + songs[i] + "<br />"
		if not temp2 == "":
			toRet = toRet + temp + temp2 + "</ol>"
		toRet = toRet + "<br />" + "</p>"
	return toRet

@app.route('/entries')
def route_entries():
	return render_template('entries.html', content=list_person_entries())

@app.route('/about')
def about():
	return "Built at PennApps 2012."

@app.route('/done')
def about():
	return ""

def get_ruby_songs(u_name):
	lst = []
	for song in db.rubysongs.find():
		lst.append(song['info'])

	return lst

def format_songs(songs, email):
	spl = []
	for song in songs:
		spl = song.split('~')
		if(spl[3]==email):
			print spl
			uri = getUri(spl[0],spl[1])
			if uri != None:	
				song = {"title": spl[0],
						"artist": spl[1],
						"station": spl[2],
						"email": email,
						"uri": uri,
						"has_uri": 1}
			else:
				song = {"title": spl[0],
						"artist": spl[1],
						"station": spl[2],
						"email": email,
						"uri": "",
						"has_uri": 0}
			db.songs.insert(song)

def get_songtable():
	return db.songs.find()

def get_songs_by_user(email):
	lst = []
	for song in db.songs.find({"email": email}):
		lst.append(str(song['title']) + "~"  + str(song['station']) + "~" + str(song['has_uri']) + "~" + str(song['uri']))
	return lst

def list_person_entries():
    content = ''
    ppl = db.people.find()
    for p in ppl:
    	content += 'Pandora Username:'+ p['p_uname']

    return content

if __name__ == '__main__':
	# Bind to PORT if defined, otherwise default to 5000.
	port = int(os.environ.get('PORT', 5000))
	app.run(host='0.0.0.0', port=port)
