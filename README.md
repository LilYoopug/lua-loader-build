# Lua Loader Build

Automated build system for Android Lua loader with LuaJIT.

## 🚀 Features

- ✅ Stripped Main.cpp (no cheat code)
- ✅ LuaJIT integration
- ✅ Online script loading from GitHub
- ✅ Auto-update system
- ✅ GitHub Actions auto-builds LuaJIT libraries

## 📦 Build Status

[![Build LuaJIT Libraries](https://github.com/LilYoopug/lua-loader-build/actions/workflows/build-luajit.yml/badge.svg)](https://github.com/LilYoopug/lua-loader-build/actions/workflows/build-luajit.yml)

## 🔧 How It Works

1. **GitHub Actions** automatically builds LuaJIT libraries
2. Libraries are committed to the repo
3. You can then build the APK with prebuilt libraries

## 📁 Structure

```
src/main/jni/
├── Main.cpp              # Stripped Lua loader
├── Android.mk            # Build configuration
├── luajit/
│   ├── include/          # LuaJIT headers
│   ├── arm64-v8a/        # Built by GitHub Actions
│   │   └── libluajit.a
│   └── armeabi-v7a/      # Built by GitHub Actions
│       └── libluajit.a
└── [other files...]
```

## 🎯 Building

### Option 1: Wait for GitHub Actions
1. Push to main branch
2. GitHub Actions builds LuaJIT automatically
3. Libraries are committed back to repo
4. Pull the changes
5. Build APK with AIDE or Android Studio

### Option 2: Manual Build
```bash
cd src/main/jni
ndk-build
```

## 📝 Online Scripts

Scripts are loaded from: https://github.com/LilYoopug/lua-menu-scripts

- `menu.lua` - Main menu script
- `version.txt` - Version for auto-update

## 🔄 Updating Menu

1. Edit `menu.lua` in lua-menu-scripts repo
2. Increment `version.txt`
3. Push changes
4. All users auto-update in 60 seconds!

## 📚 Documentation

- Main project: [LOADER](../LOADER/)
- Online scripts: [lua-menu-scripts](https://github.com/LilYoopug/lua-menu-scripts)
- Setup guide: [SETUP_GUIDE.md](../SETUP_GUIDE.md)

## ⚙️ Configuration

Edit `Main.cpp` to change script URLs:
```cpp
const char* SCRIPT_URL = "https://raw.githubusercontent.com/LilYoopug/lua-menu-scripts/main/menu.lua";
const char* SCRIPT_VERSION_URL = "https://raw.githubusercontent.com/LilYoopug/lua-menu-scripts/main/version.txt";
```

## 🎉 Credits

- LuaJIT: https://luajit.org/
- ImGui: https://github.com/ocornut/imgui
- KittyMemory: https://github.com/MJx0/KittyMemory
