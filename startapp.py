import app
from cherrypy import wsgiserver

server = wsgiserver.CherryPyWSGIServer(('0.0.0.0', 80), app.app)
server.start()
