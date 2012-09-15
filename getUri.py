#!/usr/bin/env python
# file: getUri.py

import requests
import json

success = False
def getUri(trackGiven, artistGiven):
    success = False
    track = trackGiven
    artist = artistGiven
    for x in json.loads(requests.get("http://ws.spotify.com/search/1/track.json", params={"q":track}).text)['tracks']:
        if(x['artists'][0]['name']==artist):
            return x['href']
    return None
