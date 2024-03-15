#ifndef STATE_H_INCLUDED
    #define STATE_H_INCLUDED 1

#include <atomic>
#include <string>
#include <vector>
#include <filesystem>

#include <windows.h>
#include <psapi.h>
#include <xinput.h>

namespace fs = std::filesystem;

enum class game_version {
    V12, V14,
};

struct cmdargs {
    fs::path Documents;
    fs::path Userdata;
    fs::path Scripts;
    std::wstring Main;
};

extern MODULEINFO GameModule;

extern game_version GameVersion;

extern cmdargs CmdArgs;

// Timer
extern LARGE_INTEGER QpcFrequency_Orig;

constexpr std::int64_t QpcFrequency = 10'000'000;

extern std::atomic<std::int64_t> Qpc;
extern std::uint64_t SystemFileTime;

extern int FrameTime;
extern bool FrameWait;

constexpr auto TickConversion = QpcFrequency/1000;

// Windowing
extern thread_local bool IsBinkThread;
extern bool IsInLoadScreen;
extern bool IsInMovie;

#endif
