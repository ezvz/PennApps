import os
import subprocess
#import sendgrid
import Queue
from getUri import *
from flask import Flask, render_template, request
from flaskext.mongoalchemy import MongoAlchemy
from pymongo import Connection
from subprocess import call


app = Flask(__name__)
app.config['DEBUG'] = True
#app.config['MONGOALCHEMY_DATABASE'] = 'prod'
#db = MongoAlchemy(app)

connection = Connection('localhost', 27017)
db = connection['prod']

# Run Ruby Process here on next
#sendEmail(next.user_email, next.p_uname)

@app.route('/', methods=['POST', 'GET'])
def route_root():
	saved = ''
	songs = ''
	error = ''
	if request.method == 'POST':
		#if request.form['songs']:
		#	return request.form['songs']

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
		ppl = db.people
		ppl.insert(person)

		saved = 'Thanks, we are processing your info and will email you when your playlist is ready!'
		flag = call(["ruby", "watir.rb", pandora_user, pandora_pass])
		songs = get_ruby_songs(pandora_user)
		format_songs(songs, email)
		if flag == 0:
			error = "Failed to login to Pandora."
		#out = subprocess.Popen(["ruby", "watir.rb", pandora_user, pandora_pass], stdout=subprocess.PIPE)
		#songs, err = out.communicate()
		#processQueue.put(p)
		#manageQueue()
	return render_template('index.html', saved=saved, songs=songs, error=error)

def sendEmail(email, pandora_user):
	#p = Person.query.filter(Person.user_email==email).first()
	#songs = []
	#songsValid = []
	#songsInvalid = []
	#for elem in p.songs:
	#	songInfo = elem.split("~")
	#	if getUri(songInfo[0],songInfo[1]) == None:
	#		songsInvalid.append(songInfo[0])
	#	else:
	#		songsValid.append(songInfo[0])
	#s = sendgrid.Sendgrid('varantz', 'sendPass123', secure=True)
	#message = sendgrid.Message("vzanoyan@gmail.com", "Pandorify!", "", "<b>Your playlist is ready!</b>")
	#message.add_to(email, pandora_user)

	#s.web.send(message)

	return

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
	for song in db.songs.find({"email": "asdf"}):
		lst.append(song['title'] + "~" + song['artist'] + "~" + song['station'] + "~" + song['email'])
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
