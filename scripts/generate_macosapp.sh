#!/bin/sh
rm -rf macos_app/DecIrc.app
mkdir -p macos_app/DecIrc.app/Contents/MacOS
cp macos_app/Info.plist macos_app/DecIrc.app/Contents
cp -r assets macos_app/DecIrc.app/Contents/MacOS
cp build/dec macos_app/DecIrc.app/Contents/MacOS/Dec