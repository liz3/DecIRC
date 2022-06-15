# DEC
discord-easier-client,  

**Before anything: this is a WIP Project, expect it to crash at any moment!**

This is a completely custom made discord client in c++/opengl with currently only keyboard support.

## Motivation
It sounded like a cool idea, more might follow later.

## Safety
This is not endorsed or supported by Discord in any shape or form and i am not responsible for any lost account.  
**But**: i would argue its very safe since this in its current state dec cannot "replace" the main client, even though that might change in the future.
More importantly: this was designed with legitimate use in mind, not for any botting purposes. dec does not overload systems, will not bypass limitations or monitization based systems from discord.

## Building
You will need a c++17 capable c/c++ compiler and cmake.  
dec vendors all its dependencies through submodules, so building will take a moment.

(ive only tested on macos so far but i will make sure it builds on other platforms in the future)
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
Export your discord token to the env var `DISCORD_TOKEN`

run the executable!

## Features
(as of now)

* Sending/Reading/deleting messages
* basic embed rendering
* basic attachment rendering
* working but semi stable voice chat (guild AND dm)
* responsive



## Shortcuts
C = CTRL
```
C-d - Focus direct message list
C-g - Focus Guild list
C-m - focus text-field again

C-p - Scroll up chat
C-n - Scroll down chat
C-e - Scroll chat to bottom

C-x-e - start/end direct message call

C-shift-p - focus message above
C-shift-n - focus message down

C-x-d - delete focused message 

C-u - get info about a user in dm or of a focused message
```

## Third party dependencies
Since this is a bare bones project there are a couple of deps used, here they are credited:

* [freetype2](https://freetype.org/) - Used for font rendering
* [glfw](https://www.glfw.org/) - Window host and Input controller, this is vendored because i added functionality in regards to getting non text from the clipboard
* [IXWebSocket](https://github.com/machinezone/IXWebSocket) - very very easy abstraction for http requests and websockets, also performs request asyncronious
* [nlohmann/json](https://github.com/nlohmann/json) - easily the best json library available for c++, used for work with json!
* [libjpeg](http://libjpeg.sourceforge.net/) - jpeg decoder
* [sodium](https://github.com/jedisct1/libsodium) - A modern, portable, easy to use crypto library, used for audio encryption
* [libwebp](https://chromium.googlesource.com/webm/libwebp) - webp decoder, discord uses webp in quite some places
* [opus](https://github.com/xiph/opus) - Audio codec used by discord
* [lodepng](https://github.com/lvandeve/lodepng) - PNG decoder
* [RtAudio](https://github.com/thestk/rtaudio) - Cross platform Abstraction for recording and playing back audio

## LICENSE 
The source of dec is free software under GPL 2.0, the deps might have own licenses.