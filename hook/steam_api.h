#ifndef STEAM_API_H_INCLUDED
    #define STEAM_API_H_INCLUDED 1

#include <cstdint>

using SteamAPICall_t = std::uint64_t;

using AppId_t = std::uint32_t;
using DepotId_t = std::uint32_t;
using FriendGameInfo_t = void*;
using SteamLeaderboard_t = std::uint64_t;
using SteamLeaderboardEntries_t = std::uint64_t;

using HAuthTicket = std::uint32_t;
using HServerListRequest = void*;
using HServerQuery = int;
using HSteamUser = std::int32_t;

using SNetListenSocket_t = std::uint32_t;
using SNetSocket_t = std::uint32_t;

using UGCHandle_t = std::uint64_t;

using PublishedFileUpdateHandle_t = std::uint64_t;
using PublishedFileId_t = std::uint64_t;

struct CSteamID {
    std::uint64_t Id;
};

struct CGameID {
    std::uint64_t Id;
};

struct MatchMakingKeyValuePair_t {
    char Key[256];
    char Value[256];
};

struct SteamParamStringArray_t {
    const char** Strings;
    std::int32_t NumStrings;
};

enum class EPersonaState {};
enum class EFriendRelationship {};
enum class EChatEntryType {};
enum class EBeginAuthSessionResult {};
enum class EUserHasLicenseForAppResult {};
enum class ELobbyComparison {};
enum class ELobbyDistanceFilter {};
enum class ELobbyType {};
enum class EP2PSend {};
enum class ESNetSocketConnectionType {};
enum class ERemoteStoragePlatform {};
enum class EUGCReadAction {};
enum class EWorkshopVideoProvider {};
enum class ERemoteStoragePublishedFileVisibility {};
enum class EWorkshopFileType {};
enum class EWorkshopFileAction {};
enum class EWorkshopEnumerationType {};
enum class EVoiceResult {};
enum class ELeaderboardSortMethod {};
enum class ELeaderboardDisplayType {};

enum class ELeaderboardDataRequest {
    Global = 0,
    GlobalAroundUser = 1,
    Friends = 2,
    Users = 3,
};

enum class ELeaderboardUploadScoreMethod {
    None = 0,
    KeepBest = 1,
    ForceUpdate = 2,
};

enum class EUniverse {};
enum class ENotificationPosition {};
enum class ESteamAPICallFailure {};
enum class EGamepadTextInputMode {};
enum class EGamepadTextInputLineMode {};

struct CCallbackBase {
    virtual void Run(void* Param) = 0;
    virtual void Run(void* Param, bool IOFailure, SteamAPICall_t SteamAPICall)=0;
    virtual int GetCallbackSizeBytes()=0;

    enum {
        Registered = 0x01,
        GameServer = 0x02
    };
    std::uint8_t CallbackFlags;
    int Callback;
};

struct ISteamMatchmakingServerListResponse;
struct ISteamMatchmakingPingResponse;
struct ISteamMatchmakingPlayersResponse;
struct ISteamMatchmakingRulesResponse;
struct gameserveritem_t;

struct LeaderboardEntry_t {
	CSteamID SteamIdUser;
	std::int32_t GlobalRank;
	std::int32_t Score;
	std::int32_t Details;
	UGCHandle_t Ugc;
};

struct P2PSessionState_t;

struct ISteamApps;
struct ISteamFriends;
struct ISteamGameServer;
struct ISteamMatchmaking;
struct ISteamMatchmakingServers;
struct ISteamNetworking;
struct ISteamRemoteStorage;
struct ISteamUser;
struct ISteamUserStats;
struct ISteamUtils;

struct LeaderboardFindResult_t {
    SteamLeaderboard_t Leaderboard;
    bool Found;
};

struct LeaderboardScoresDownloaded_t {
    SteamLeaderboard_t Leaderboard;
    SteamLeaderboardEntries_t Entries;
    int EntryCount;
};

struct LeaderboardScoreUploaded_t {
    bool Success;
    SteamLeaderboard_t SteamLeaderboard;
    std::int32_t Score;
    bool ScoreChanged;
    int GlobalRankNew;
    int GlobalRankPrevious;
};

using SteamAPIWarningMessageHook_t = void (*)(int, const char*);

bool SteamAPI_Init();

void SteamAPI_RegisterCallResult(CCallbackBase* Callback, SteamAPICall_t ApiCall);
void SteamAPI_RegisterCallback(CCallbackBase* Callback, int Index);
void SteamAPI_RunCallbacks();
void SteamAPI_Shutdown();
void SteamAPI_UnregisterCallResult(CCallbackBase* Callback, SteamAPICall_t ApiCall);
void SteamAPI_UnregisterCallback(CCallbackBase* Callback);

ISteamApps* SteamApps();
ISteamFriends* SteamFriends();
ISteamGameServer* SteamGameServer();
void SteamGameServer_Shutdown();
ISteamMatchmaking* SteamMatchmaking();
ISteamMatchmakingServers* SteamMatchmakingServers();
ISteamNetworking* SteamNetworking();
ISteamRemoteStorage* SteamRemoteStorage();
ISteamUser* SteamUser();
ISteamUserStats* SteamUserStats();
ISteamUtils* SteamUtils();

#endif
