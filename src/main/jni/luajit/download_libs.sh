#!/bin/bash
# Quick fix: Download prebuilt LuaJIT libraries for Android

echo "Downloading prebuilt LuaJIT libraries..."

cd LOADER/app/src/main/jni/luajit

# Option 1: Use koreader prebuilts (most reliable)
echo "Downloading from koreader/luajit-rocks..."

# arm64-v8a
curl -L -o arm64-v8a/libluajit.a \
  "https://github.com/koreader/luajit-rocks/releases/download/v2.1.0-beta3/libluajit-arm64-v8a.a"

# armeabi-v7a  
curl -L -o armeabi-v7a/libluajit.a \
  "https://github.com/koreader/luajit-rocks/releases/download/v2.1.0-beta3/libluajit-armeabi-v7a.a"

echo "Done! Check file sizes:"
ls -lh arm64-v8a/libluajit.a
ls -lh armeabi-v7a/libluajit.a

echo ""
echo "If files are small (<100KB), download failed."
echo "Try manual download from:"
echo "https://github.com/koreader/luajit-rocks/releases"
