script_path=`dirname $0`
c_exec=$script_path/spotify_playing

if [ -e $c_exec ]; then
    # Use the C version if it exists.
    $c_exec
else
    pgrep spotify.exe 1>/dev/null 2>&1
    if [ $? != 0 ]; then
        exit 1
    fi

    xwininfo -root -tree | grep '"Spotify": ("spotify.exe" "Wine")' 1>/dev/null 2>&1
    if [ $? != 0 ]; then
        xwininfo -root -tree | grep '("spotify.exe" "Wine")' | grep -v 'has no name' | sed 's/[ ]*0x[0-9a    -f]* "Spotify - \(.*\)": ("spotify.exe" "Wine").*/\1/'
    else
        exit 1
    fi
fi
