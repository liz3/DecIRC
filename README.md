# DecIRC

**Before anything: this is a WIP Project, expect it to crash at any moment!**

Originally this was a custom discord client, but after some dead ends and a loss in interest i decided to rewrite it to a IRC client, which besides not being
a grey area seamed and is a lot of fun.

## About
DecIRC(the name "Dec" still comes from the discord times) is a WIP c++ client for IRC written to directly target OpenGL, it has a very very basic but working GUI framework for rendering: rectangles, text, images and so on. Doing that anything relating to the UI is custom implemented, which means its very rudimentary but also fast.
The controls are entirely keyboard based for now.

![image](https://github.com/liz3/DecIRC/blob/irc/screenshots/screenshot1.png?raw=true)
![image](https://github.com/liz3/DecIRC/blob/irc/screenshots/screenshot2.png?raw=true)
![image](https://github.com/liz3/DecIRC/blob/irc/screenshots/screenshot3.png?raw=true)


## Building
You will need a c++17 capable c/c++ compiler and cmake.  
On Gnu/Linux you will need glib2, gio2 and gdk-pixbuf since libnotify uses them.
Dec vendors all its dependencies through submodules, so building will take a moment.

Dec should build natively on all 3 big platforms with clang, gcc and msvc.
```
git submodule update --init
mkdir build
cd build
cmake ..
# un*x / mac
make -j4
# windows
cmake --build . --config Release

```
**Note: **: Currently you need to start the executable from the folder its in as it uses relative paths.**

run the executable!

## Features
(as of now)

* Adding and connecting to networks, no SASL support yet.
* Query, names, list, msg, join, part, whois are the commands which have basic implementations, most of which come with ui abstractions.
* a raw mode which is basically telnet with the ircd for everything missing(a lot).
* native integration with Mac and windows and Gnu/linux(freedesktop notification over libnotify) notification centers

## Dec Specific commands
```
/addserver - add a network to the list, the syntax is "/addserver host" followed by a list 
of key=value pairs for settings, values can be enclosed with " to include spaces. keys are 
- name: required, network display name
- nick: required, nickname to send to ircd, currently theres no fallback logic
- username: Username
- realname: Realname to use
- pass: Login password
- port: specify port, defaults to 6697
- ssl: "true" or "false", set if a tls connection should be established, false by default
- verify-ssl: "true" or "false" whether to verify certificates with local CAs, false by default
- auto-connect: "true" or "false", whether to connect on open
A example could be: /addserver host.tld name="My Network" nick=liz3 username=liz3 realname=liz3 ssl=true

/delserver - delete active server

/disconnect - close connection, currently also resets channels and users states

/raw - switch between normal "rendering" and "raw buffer"

/notify - Toggle notifications in current channel. this does not give feedback rn. 
```

# Raw Buffer
The `/raw` command will toggle into a buffer which when you type a message will be send 1:1 to the Ircd and received message will, besides being normally processed also be appended to that buffer.



## Shortcuts
Generally navigation is done with the arrow keys and enter.
C = CTRL
```
C-d - Focus channel list(if network active)
C-g - Focus Networks list
C-m - focus text-field again

C-k - clear input

C-p - Scroll up active buffer
C-n - Scroll down active buffer
C-e - Scroll active biffer to bottom

C-shift-p - focus message above
C-shift-n - focus message below

C-c Copy focused message content

d-[1-9] - if focused message has links open the respective link

C-u - open channel users
```

## Third party dependencies
Since this is a bare bones project there are a couple of deps used, here they are credited:

* [freetype2](https://freetype.org/) - Used for font rendering
* [glfw](https://www.glfw.org/) - Window host and Input controller, this is vendored because i added functionality in regards to getting non text from the clipboard
* [IXWebSocket](https://github.com/machinezone/IXWebSocket) - very very easy abstraction for http requests and websockets, I only use its TLS socket api.
* [nlohmann/json](https://github.com/nlohmann/json) - easily the best json library available for c++, used for work with json!
* [libjpeg](http://libjpeg.sourceforge.net/) - jpeg decoder
* [libwebp](https://chromium.googlesource.com/webm/libwebp) - webp decoder.
* [lodepng](https://github.com/lvandeve/lodepng) - PNG decoder
* [wintoast](https://github.com/mohabouje/WinToast) - small api wrapper around windows notifications.
* [libpng](http://www.libpng.org/pub/png/libpng.html) - Yes, its a second png decoder. I needed to include this because freetype2 requires it for png fonts, freetype please add a option to use lodepng...
* [libnotify](https://gitlab.gnome.org/GNOME/libnotify) - Wrapper around freedesktop Notifications. I ported this lib to cmake.

## LICENSE 
The source of dec is free software under GPL 2.0, the deps might have own licenses.