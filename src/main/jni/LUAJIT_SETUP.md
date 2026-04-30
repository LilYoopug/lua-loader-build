# LuaJIT Download and Setup Guide

## You need to add LuaJIT prebuilt libraries to your project

### Directory Structure
```
LOADER/app/src/main/jni/luajit/
├── include/
│   ├── lua.h
│   ├── lualib.h
│   ├── lauxlib.h
│   ├── luaconf.h
│   └── luajit.h
├── arm64-v8a/
│   └── libluajit.a
└── armeabi-v7a/
    └── libluajit.a
```

## Option 1: Download Prebuilt (Easiest)

### From LuaJIT Official
1. Visit: https://luajit.org/download.html
2. Download latest version (2.1.x)
3. Extract and copy files

### From GitHub Releases
```bash
cd LOADER/app/src/main/jni/
mkdir -p luajit/arm64-v8a luajit/armeabi-v7a luajit/include

# Download prebuilt from community repos
# Example: https://github.com/koreader/luajit-rocks/releases
```

## Option 2: Build from Source

### Prerequisites
```bash
sudo apt-get install build-essential git
```

### Build for Android
```bash
# Clone LuaJIT
git clone https://github.com/LuaJIT/LuaJIT.git
cd LuaJIT

# Set Android NDK path
export NDK=/path/to/android-ndk
export NDKABI=21  # Minimum API level

# Build for arm64-v8a
make clean
make HOST_CC="gcc -m64" CROSS=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$NDKABI- \
     TARGET_SYS=Linux TARGET_FLAGS="-DLUAJIT_ENABLE_GC64"
cp src/libluajit.a ../LOADER/app/src/main/jni/luajit/arm64-v8a/

# Build for armeabi-v7a
make clean
make HOST_CC="gcc -m32" CROSS=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi$NDKABI-
cp src/libluajit.a ../LOADER/app/src/main/jni/luajit/armeabi-v7a/

# Copy headers
cp src/lua*.h ../LOADER/app/src/main/jni/luajit/include/
```

## Option 3: Use Pre-compiled from This Repo (Recommended)

I'll provide download links for prebuilt LuaJIT libraries:

### Quick Setup
```bash
cd LOADER/app/src/main/jni/

# Create directories
mkdir -p luajit/arm64-v8a luajit/armeabi-v7a luajit/include

# Download prebuilt libraries (you need to host these or use community builds)
# For now, use placeholder - you'll need to get actual files
```

## Option 4: Download from My Server (If Available)

```bash
cd LOADER/app/src/main/jni/luajit/

# Download arm64
curl -L -o arm64-v8a/libluajit.a https://your-server.com/libluajit-arm64.a

# Download arm32
curl -L -o armeabi-v7a/libluajit.a https://your-server.com/libluajit-arm32.a

# Download headers
curl -L -o include/lua.h https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lua.h
curl -L -o include/lualib.h https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lualib.h
curl -L -o include/lauxlib.h https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lauxlib.h
curl -L -o include/luaconf.h https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/luaconf.h
curl -L -o include/luajit.h https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/luajit.h
```

## Verify Installation

After adding LuaJIT files, verify:

```bash
# Check files exist
ls -lh LOADER/app/src/main/jni/luajit/arm64-v8a/libluajit.a
ls -lh LOADER/app/src/main/jni/luajit/armeabi-v7a/libluajit.a
ls -lh LOADER/app/src/main/jni/luajit/include/*.h

# File sizes should be:
# libluajit.a (arm64): ~600-800 KB
# libluajit.a (arm32): ~500-700 KB
```

## Build the Project

```bash
cd LOADER
./gradlew assembleDebug

# Or with Android Studio:
# Build > Make Project
```

## Troubleshooting

### Error: libluajit.a not found
- Check file paths match Android.mk
- Ensure files are not empty (check file size)

### Error: undefined reference to lua_*
- Check LOCAL_STATIC_LIBRARIES includes libluajit
- Verify header files are in include/

### Error: wrong architecture
- Make sure you built for correct ABI (arm64-v8a and armeabi-v7a)
- Check NDK version compatibility

## Alternative: Use Lua Instead of LuaJIT

If LuaJIT is too complex, you can use standard Lua 5.4:

1. Download from: https://www.lua.org/download.html
2. Smaller and easier to build
3. Slightly slower but still fast enough
4. Change includes in Main.cpp from luajit.h to lua.h

## Need Help?

If you can't get LuaJIT working, I can:
1. Provide direct download links
2. Help you build from source
3. Switch to standard Lua instead
4. Use a different scripting solution (JSON config, etc.)
