# -*- coding: utf-8 -*-

"""
This module is used to get data from a Spotify client. You can use it to find
the currently playing song. See the SpotifyClient class and the main function.

**Note:** The module has only been tested in Linux running Spotify through 
          Wine.

Copyright (c) 2009 Simon Pantzare <simon@spantz.org>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

"""

from __future__ import with_statement
from string import digits, letters


def base62_encode(hex):
    dec, b62, alnum, base = int(hex, 16), [], digits + letters, 62
    for _ in range(22):
        b62.append(alnum[dec % base])
        dec = dec // base
    b62.reverse()
    return "".join(b62)


def parse_metadata(file):
    meta = {}
    with open(file) as f:
        data = f.read()
        data = data[data.find("\r\n") + 2:]
        artists_d, _, songs_d = data.split("\r\n\r\n")
    
        artists = {}
        for entry in artists_d.split("\r\n"):
            idx, name = entry.split(chr(1))
            artists[idx] = name
        
        meta = {}
        for entry in songs_d.split("\r\n"):
            try:
                idx, title, artist_idx, _ = entry.split(chr(1), 3)
            except ValueError:
                continue
            meta[idx] = {
                'artist': artists[artist_idx[:32]], 
                'title': title
            }
    return meta


def now_playing_idx(file):
    idx = None
    with open(file) as f:
        data = f.read()
        key = '"playing_track_id":"'
        start = data.find(key) + len(key)
        end = data.find('"', start)
        idx = data[start:end]
    return idx


class SpotifyEntry(object):
    def __init__(self, artist, title, idx):
        self.artist = artist
        self.title = title
        self.idx = idx

    def http_link(self):
        return "http://open.spotify.com/track/" + base62_encode(self.idx)


    def uri(self):
        return "spotify:track:" + base62_encode(self.idx)


    def __str__(self):
        return "%s - %s (%s)" % (self.artist, self.title, self.uri())


class SpotifyClient(object):
    def __init__(self, dir):
        self.dir = dir


    def update(self):
        self.meta = parse_metadata(self.dir + '/metadata')
        return self


    def get(self, idx):
        entry = self.meta[idx]
        return SpotifyEntry(entry['artist'], entry['title'], idx)


    def now_playing(self):
        idx = now_playing_idx(self.dir + '/guistate')
        if idx is None:
            return None
        return self.get(idx)


def main():
    import os, sys

    if os.name != 'posix':
        sys.exit(1)
    
    try:
        home_dir = os.environ["HOME"]
        sys_user = os.environ["USER"]
        spotify_login = os.environ["SPOTIFY_LOGIN"]
        spotify_data_dir = "%s/.wine/drive_c/windows/profiles/%s" \
                           "/Application Data/Spotify/Users/%s-user" \
                           % (home_dir, sys_user, spotify_login)

        client = SpotifyClient(spotify_data_dir)
        print client.update().now_playing()
        sys.exit(0)
    except:
        sys.exit(1)


if __name__ == '__main__':
    main()
