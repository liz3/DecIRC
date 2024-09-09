#!/bin/sh
rm -rf macos_app/DecIrc.app
mkdir -p macos_app/DecIrc.app/Contents/MacOS
mkdir -p macos_app/DecIrc.app/Contents/Resources
cp macos_app/Info.plist macos_app/DecIrc.app/Contents
cp -r assets macos_app/DecIrc.app/Contents/MacOS
cp build/DecIrc macos_app/DecIrc.app/Contents/MacOS/Dec
cp assets/DecIrc.icns macos_app/DecIrc.app/Contents/Resources