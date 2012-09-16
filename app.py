import os
import subprocess
import sendgrid
import Queue
from getUri import *
from flask import Flask, render_template, request
from flaskext.mongoalchemy import MongoAlchemy


app = Flask(__name__)
app.config['DEBUG'] = True
app.config['MONGOALCHEMY_DATABASE'] = 'pandorify'
db = MongoAlchemy(app)

processQueue = Queue()

def manageQueue:
	next = processQueue.get()
	# Run Ruby Process here on next
	sendEmail(next.user_email, next.p_uname)
	if processQueue.empty():
		return False








class Person(db.Document):
	s_uname = db.StringField()
	s_pword = db.StringField()
	p_uname = db.StringField()
	p_pword = db.StringField()
	user_email = db.StringField()
	songs = db.ListField(db.StringField())


@app.route('/', methods=['POST', 'GET'])
def route_root():
	saved = ''
	if request.method == 'POST':
		pandora_user = request.form['pandoraUsername']
		pandora_pass = request.form['pandoraPassword']
		spotify_user = request.form['spotifyUsername']
		spotify_pass = request.form['spotifyPassword']
		email = request.form['email']

		p = Person(s_uname = spotify_user,
					s_pword = spotify_pass,
					p_uname = pandora_user,
					p_pword = pandora_pass,
					user_email = email,
					songs = []
					)
		p.save()
		saved = 'Thanks, we are processing your info and will email you when your playlist is ready!'
		processQueue.put(p)
		manageQueue()
	return render_template('index.html', saved=saved)



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
	s = sendgrid.Sendgrid('varantz', 'sendPass123', secure=True)
	message = sendgrid.Message("vzanoyan@gmail.com", "Pandorify!", "", "<b>Your playlist is ready!</b>")
	message.add_to(email, pandora_user)

	s.web.send(message)

	
	return



@app.route('/entries')
def route_entries():
	return render_template('entries.html', content=list_person_entries())

@app.route('/about')
def about():
	return "Built at PennApps 2012."


def list_person_entries():
    entries = Person.query.all()
    content = '<p>Entries:</p>'
    for entry in entries:
        content += '<p>%s</p>' % entry.s_uname
    return content

def handle_submit(p_user, p_pass, s_user, s_pass):
	out = p_user + ', ' + p_pass + ', ' + s_user + ', ' + s_pass
	return out

if __name__ == '__main__':
    # Bind to PORT if defined, otherwise default to 5000.
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port)