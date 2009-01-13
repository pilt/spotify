use strict;
use vars qw($VERSION %IRSSI);

use Irssi;

$VERSION = '1.0';
%IRSSI = (
	authors		=> 'Simon Pantzare',
	contact		=> 'simon@spantz.org',
	name		=> 'Spotify Now Playing',
	description	=> 'Say what you are listening to in Spotify.', 
	license		=> 'X11',
    url         => 'http://github.com/pilt/spotify/',
    changed     => '12 January 2009'
);

# FIXME: Code duplication in spotify_playing and spotify_show.

sub spotify_playing
{
	my($data, $server, $witem, $text, $login, $script) = @_;
	return unless $witem;
	$login = Irssi::settings_get_str("login");
	$script = Irssi::settings_get_str("script_path");
    $text = `SPOTIFY_LOGIN=$login python $script`;
    if ($? eq 0) {
        Irssi::active_win->command("/me ♪ $text $data");
    }
    else {
        Irssi::print("Could not get currently playing song."); 
    }
}

sub spotify_show
{
	my($data, $server, $witem, $text, $login, $script) = @_;
	return unless $witem;
	$login = Irssi::settings_get_str("login");
	$script = Irssi::settings_get_str("script_path");
    $text = `SPOTIFY_LOGIN=$login python $script`;
    if ($? eq 0) {
        $text =~ s/\s+$//;
        Irssi::print("♪ $text $data");
    }
    else {
        Irssi::print("Could not get currently playing song."); 
    }
}

Irssi::command_bind np => \&spotify_playing;
Irssi::command_bind nps => \&spotify_show;
Irssi::settings_add_str("spotify", "login", "b702003");
Irssi::settings_add_str("spotify", "script_path", "~/.irssi/scripts/spotify.py");
