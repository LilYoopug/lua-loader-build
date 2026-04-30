// --- C Standard ---
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <math.h>
#include <limits>

// --- C++ Standard ---
#include <thread>
#include <vector>
#include <fstream>
#include <string>
#include <time.h>

// --- Android / System / Dynamic loading ---
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/system_properties.h>

// --- Graphics ---
#include <EGL/egl.h>
#include <GLES3/gl3.h>

// --- External libs / hooks / utilities ---
#include <xdl.h>
#include <SubstrateHook.h>
#include <CydiaSubstrate.h>

// --- KittyMemory (memory tools) ---
#include <KittyMemory/KittyMemory.h>
#include <KittyMemory/MemoryPatch.h>
#include <KittyMemory/KittyScanner.h>
#include <KittyMemory/KittyUtils.h>
#include <KittyUtils.h>

// --- ImGui ---
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

// --- Project / Local headers ---
#include "include/Tools.hpp"
#include "include/obfuscate.h"
#include "include/Theme.h"
#include "imgui/Font.h"
#include "imgui/Roboto-Regular.h"
#include "QQInj.h"
#include "imgui/Icon.h"
#include "imgui/Iconcpp.h"
#include "AutoUpdate/IL2CppSDKGenerator/Il2Cpp.h"
#include "AutoUpdate/Tools/Call_Tools.h"
#include "Zygisk.hpp"

// --- LuaJIT ---
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

// ============================================
// GLOBAL STATE
// ============================================
inline static int g_GlHeight, g_GlWidth;
inline static bool g_IsSetup = false;
inline int prevWidth, prevHeight;

// Lua state
static lua_State* g_LuaState = nullptr;
static bool g_LuaScriptLoaded = false;
static std::string g_LuaScriptContent;
static double g_LastScriptCheck = 0.0;

// Online script URL - CHANGE THIS TO YOUR SERVER
const char* SCRIPT_URL = "https://raw.githubusercontent.com/LilYoopug/lua-menu-scripts/main/menu.lua";
const char* SCRIPT_VERSION_URL = "https://raw.githubusercontent.com/LilYoopug/lua-menu-scripts/main/version.txt";

// Local cache paths
const char* CACHE_DIR = "/storage/emulated/0/Android/data/com.kukouri.wizworld/files/";
const char* SCRIPT_CACHE = "/storage/emulated/0/Android/data/com.kukouri.wizworld/files/menu.lua";
const char* VERSION_CACHE = "/storage/emulated/0/Android/data/com.kukouri.wizworld/files/version.txt";

// Your Game Package Name
char packageName[] = "com.kukouri.wizworld";

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

// ============================================
// ============================================
// IMGUI THEME SETUP
// ============================================
// IMGUI THEME SETUP
// ============================================
void SetupImGuiStyle() {
    SetDarkGreenBlueTheme();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 22.0f;
    io.Fonts->AddFontDefault(&font_cfg);
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 23.0f;
    io.Fonts->AddFontFromMemoryCompressedTTF(Font_compressed_data, Font_compressed_size, 23.0f, &icons_config, icons_ranges);
}

// ============================================
void SetupImGuiStyle() {
    SetDarkGreenBlueTheme();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 22.0f;
    io.Fonts->AddFontDefault(&font_cfg);
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 23.0f;
    io.Fonts->AddFontFromMemoryCompressedTTF(Font_compressed_data, Font_compressed_size, 23.0f, &icons_config, icons_ranges);
}

// LOGGING
// ============================================
void writeLog(const std::string& logMessage, const std::string& filename = "/storage/emulated/0/Android/data/com.waxmoon.ma.gp/files/log.txt") {
    std::ofstream logFile(filename, std::ios::app);
    if (logFile.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        logFile << "[" << dt << "] " << logMessage << std::endl;
        logFile.close();
    }
}

std::string GetProp(const char* key) {
    char value[PROP_VALUE_MAX];
    __system_property_get(key, value);
    return std::string(value);
}

// ============================================
// TIME HELPER
// ============================================
static double NowSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// ============================================
// HTTP DOWNLOAD HELPER
// ============================================
std::string DownloadString(const char* url) {
    // Simple curl-based download
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -L '%s' 2>/dev/null", url);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        writeLog("Failed to execute curl for: " + std::string(url));
        return "";
    }
    
    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

bool DownloadFile(const char* url, const char* outputPath) {
    std::string content = DownloadString(url);
    if (content.empty()) {
        return false;
    }
    
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        writeLog("Failed to write to: " + std::string(outputPath));
        return false;
    }
    
    outFile << content;
    outFile.close();
    return true;
}

// ============================================
// LUA C API BINDINGS
// ============================================

// Expose logging to Lua
static int lua_WriteLog(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    writeLog(msg);
    return 0;
}

// Expose ImGui functions to Lua
static int lua_ImGui_Begin(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    bool open = true;
    ImGuiWindowFlags flags = 0;
    
    if (lua_gettop(L) >= 2) {
        flags = (ImGuiWindowFlags)luaL_checkinteger(L, 2);
    }
    
    bool result = ImGui::Begin(name, &open, flags);
    lua_pushboolean(L, result);
    return 1;
}

static int lua_ImGui_End(lua_State* L) {
    ImGui::End();
    return 0;
}

static int lua_ImGui_Text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    ImGui::Text("%s", text);
    return 0;
}

static int lua_ImGui_Button(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool clicked = ImGui::Button(label);
    lua_pushboolean(L, clicked);
    return 1;
}

static int lua_ImGui_Checkbox(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool value = lua_toboolean(L, 2);
    bool changed = ImGui::Checkbox(label, &value);
    lua_pushboolean(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

static int lua_ImGui_SliderFloat(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    float value = (float)luaL_checknumber(L, 2);
    float min = (float)luaL_checknumber(L, 3);
    float max = (float)luaL_checknumber(L, 4);
    bool changed = ImGui::SliderFloat(label, &value, min, max);
    lua_pushnumber(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

static int lua_ImGui_SliderInt(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    int value = (int)luaL_checkinteger(L, 2);
    int min = (int)luaL_checkinteger(L, 3);
    int max = (int)luaL_checkinteger(L, 4);
    bool changed = ImGui::SliderInt(label, &value, min, max);
    lua_pushinteger(L, value);
    lua_pushboolean(L, changed);
    return 2;
}

static int lua_ImGui_Separator(lua_State* L) {
    ImGui::Separator();
    return 0;
}

static int lua_ImGui_SameLine(lua_State* L) {
    ImGui::SameLine();
    return 0;
}

static int lua_ImGui_Spacing(lua_State* L) {
    ImGui::Spacing();
    return 0;
}

static int lua_ImGui_TextColored(lua_State* L) {
    float r = (float)luaL_checknumber(L, 1);
    float g = (float)luaL_checknumber(L, 2);
    float b = (float)luaL_checknumber(L, 3);
    float a = (float)luaL_checknumber(L, 4);
    const char* text = luaL_checkstring(L, 5);
    ImGui::TextColored(ImVec4(r, g, b, a), "%s", text);
    return 0;
}

static int lua_ImGui_BeginTabBar(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    bool result = ImGui::BeginTabBar(id);
    lua_pushboolean(L, result);
    return 1;
}

static int lua_ImGui_EndTabBar(lua_State* L) {
    ImGui::EndTabBar();
    return 0;
}

static int lua_ImGui_BeginTabItem(lua_State* L) {
    const char* label = luaL_checkstring(L, 1);
    bool result = ImGui::BeginTabItem(label);
    lua_pushboolean(L, result);
    return 1;
}

static int lua_ImGui_EndTabItem(lua_State* L) {
    ImGui::EndTabItem();
    return 0;
}

static int lua_ImGui_GetScreenWidth(lua_State* L) {
    lua_pushinteger(L, g_GlWidth);
    return 1;
}

static int lua_ImGui_GetScreenHeight(lua_State* L) {
    lua_pushinteger(L, g_GlHeight);
    return 1;
}

// Expose IL2CPP hook functions to Lua
static int lua_HookMethod(lua_State* L) {
    const char* assembly = luaL_checkstring(L, 1);
    const char* namespaze = luaL_checkstring(L, 2);
    const char* klass = luaL_checkstring(L, 3);
    const char* method = luaL_checkstring(L, 4);
    int paramCount = (int)luaL_checkinteger(L, 5);
    
    void* methodPtr = Il2CppGetMethodOffset(assembly, namespaze, klass, method, paramCount);
    
    if (methodPtr) {
        lua_pushlightuserdata(L, methodPtr);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_DobbyHook(lua_State* L) {
    void* target = lua_touserdata(L, 1);
    void* replacement = lua_touserdata(L, 2);
    
    if (target && replacement) {
        void* original = nullptr;
        DobbyHook(target, replacement, &original);
        lua_pushlightuserdata(L, original);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

// ============================================
// LUA INITIALIZATION
// ============================================
void InitLua() {
    if (g_LuaState) {
        lua_close(g_LuaState);
    }
    
    g_LuaState = luaL_newstate();
    luaL_openlibs(g_LuaState);
    
    // Register C functions to Lua
    lua_register(g_LuaState, "WriteLog", lua_WriteLog);
    lua_register(g_LuaState, "HookMethod", lua_HookMethod);
    lua_register(g_LuaState, "DobbyHook", lua_DobbyHook);
    
    // Register ImGui functions
    lua_register(g_LuaState, "ImGui_Begin", lua_ImGui_Begin);
    lua_register(g_LuaState, "ImGui_End", lua_ImGui_End);
    lua_register(g_LuaState, "ImGui_Text", lua_ImGui_Text);
    lua_register(g_LuaState, "ImGui_Button", lua_ImGui_Button);
    lua_register(g_LuaState, "ImGui_Checkbox", lua_ImGui_Checkbox);
    lua_register(g_LuaState, "ImGui_SliderFloat", lua_ImGui_SliderFloat);
    lua_register(g_LuaState, "ImGui_SliderInt", lua_ImGui_SliderInt);
    lua_register(g_LuaState, "ImGui_Separator", lua_ImGui_Separator);
    lua_register(g_LuaState, "ImGui_SameLine", lua_ImGui_SameLine);
    lua_register(g_LuaState, "ImGui_Spacing", lua_ImGui_Spacing);
    lua_register(g_LuaState, "ImGui_TextColored", lua_ImGui_TextColored);
    lua_register(g_LuaState, "ImGui_BeginTabBar", lua_ImGui_BeginTabBar);
    lua_register(g_LuaState, "ImGui_EndTabBar", lua_ImGui_EndTabBar);
    lua_register(g_LuaState, "ImGui_BeginTabItem", lua_ImGui_BeginTabItem);
    lua_register(g_LuaState, "ImGui_EndTabItem", lua_ImGui_EndTabItem);
    lua_register(g_LuaState, "ImGui_GetScreenWidth", lua_ImGui_GetScreenWidth);
    lua_register(g_LuaState, "ImGui_GetScreenHeight", lua_ImGui_GetScreenHeight);
    
    writeLog("Lua initialized successfully");
}

// ============================================
// SCRIPT LOADING
// ============================================
bool LoadLuaScript(const char* scriptPath) {
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        writeLog("Failed to open Lua script: " + std::string(scriptPath));
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty()) {
        writeLog("Lua script is empty");
        return false;
    }
    
    g_LuaScriptContent = content;
    
    // Execute the script
    if (luaL_dostring(g_LuaState, content.c_str()) != LUA_OK) {
        const char* error = lua_tostring(g_LuaState, -1);
        writeLog("Lua error: " + std::string(error));
        lua_pop(g_LuaState, 1);
        return false;
    }
    
    writeLog("Lua script loaded successfully");
    g_LuaScriptLoaded = true;
    return true;
}

void CheckAndUpdateScript() {
    double now = NowSeconds();
    
    // Check for updates every 60 seconds
    if (now - g_LastScriptCheck < 60.0) {
        return;
    }
    
    g_LastScriptCheck = now;
    
    writeLog("Checking for script updates...");
    
    // Download version file
    std::string onlineVersion = DownloadString(SCRIPT_VERSION_URL);
    if (onlineVersion.empty()) {
        writeLog("Failed to check version");
        return;
    }
    
    // Read local version
    std::string localVersion;
    std::ifstream versionFile(VERSION_CACHE);
    if (versionFile.is_open()) {
        std::getline(versionFile, localVersion);
        versionFile.close();
    }
    
    // Compare versions
    if (onlineVersion != localVersion) {
        writeLog("New version detected, downloading...");
        
        if (DownloadFile(SCRIPT_URL, SCRIPT_CACHE)) {
            // Save new version
            std::ofstream outVersion(VERSION_CACHE);
            if (outVersion.is_open()) {
                outVersion << onlineVersion;
                outVersion.close();
            }
            
            // Reload script
            LoadLuaScript(SCRIPT_CACHE);
            writeLog("Script updated successfully");
        } else {
            writeLog("Failed to download new script");
        }
    } else {
        writeLog("Script is up to date");
    }
}

// ============================================
// IMGUI RENDERING
// ============================================
void DrawMenu() {
    if (!g_LuaScriptLoaded) {
        // Fallback UI if script not loaded
        ImGui::Begin("Loader", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Loading script from server...");
        ImGui::Text("Please wait...");
        ImGui::End();
        return;
    }
    
    // Call Lua's DrawMenu function
    lua_getglobal(g_LuaState, "DrawMenu");
    if (lua_isfunction(g_LuaState, -1)) {
        if (lua_pcall(g_LuaState, 0, 0, 0) != LUA_OK) {
            const char* error = lua_tostring(g_LuaState, -1);
            writeLog("Lua DrawMenu error: " + std::string(error));
            lua_pop(g_LuaState, 1);
        }
    } else {
        lua_pop(g_LuaState, 1);
    }
}

// ============================================
// OPENGL HOOKS
// ============================================
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (dpy != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE) {
        if (!g_IsSetup) {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL;
            
            ImGui_ImplOpenGL3_Init("#version 300 es");
            ImGui_ImplAndroid_Init(nullptr);
            
            SetupImGuiStyle();
            
            // Initialize Lua
            InitLua();
            
            // Try to load cached script first
            if (!LoadLuaScript(SCRIPT_CACHE)) {
                // Download from server
                writeLog("Downloading script from server...");
                if (DownloadFile(SCRIPT_URL, SCRIPT_CACHE)) {
                    LoadLuaScript(SCRIPT_CACHE);
                } else {
                    writeLog("Failed to download script");
                }
            }
            
            g_IsSetup = true;
        }
        
        ImGuiIO& io = ImGui::GetIO();
        
        glGetIntegerv(GL_VIEWPORT, (GLint*)&g_GlWidth);
        g_GlWidth = g_GlWidth * 2;
        g_GlHeight = g_GlHeight * 2;
        
        if (prevWidth != g_GlWidth || prevHeight != g_GlHeight) {
            io.DisplaySize = ImVec2((float)g_GlWidth, (float)g_GlHeight);
            prevWidth = g_GlWidth;
            prevHeight = g_GlHeight;
        }
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();
        
        // Check for script updates
        CheckAndUpdateScript();
        
        // Draw menu
        DrawMenu();
        
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    return orig_eglSwapBuffers(dpy, surface);
}

// ============================================
// INITIALIZATION
// ============================================
void InitHooks() {
    writeLog("Initializing hooks...");
    
    // Hook eglSwapBuffers
    void* eglSwapBuffersAddr = DobbySymbolResolver(nullptr, "eglSwapBuffers");
    if (eglSwapBuffersAddr) {
        DobbyHook(eglSwapBuffersAddr, (void*)hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        writeLog("eglSwapBuffers hooked");
    } else {
        writeLog("Failed to find eglSwapBuffers");
    }
}

// ============================================
// ENTRY POINT
// ============================================
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, [](void*) -> void* {
        sleep(3);
        
        writeLog("=== Lua Loader Started ===");
        writeLog("Package: " + std::string(packageName));
        
        // Initialize IL2CPP
        Il2CppAttach();
        
        // Initialize hooks
        InitHooks();
        
        writeLog("Initialization complete");
        return nullptr;
    }, nullptr);
}

// ============================================
// ZYGISK IMPLEMENTATION
// ============================================
class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        
        preSpecialize(package_name, app_data_dir);
        
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            std::thread([]() {
                sleep(1);
                lib_main();
            }).detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack = false;

    void preSpecialize(const char *package, const char *app_dir) {
        if (package && strcmp(package, packageName) == 0) {
            enable_hack = true;
            writeLog("Target package detected: " + std::string(package));
        }
    }
};

REGISTER_ZYGISK_MODULE(MyModule)
