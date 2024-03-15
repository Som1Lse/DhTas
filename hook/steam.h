#ifndef STEAM_H_INCLUDED
    #define STEAM_H_INCLUDED 1

#include "steam_api.h"

bool SteamAPI_Init_Hook();
void SteamAPI_RegisterCallResult_Hook(CCallbackBase* Callback, SteamAPICall_t ApiCall);
void SteamAPI_RegisterCallback_Hook(CCallbackBase* Callback, int Index);
void SteamAPI_RunCallbacks_Hook();
void SteamAPI_Shutdown_Hook();
void SteamAPI_UnregisterCallResult_Hook(CCallbackBase* Callback, SteamAPICall_t ApiCall);
void SteamAPI_UnregisterCallback_Hook(CCallbackBase* Callback);
ISteamApps* SteamApps_Hook();
ISteamFriends* SteamFriends_Hook();
ISteamGameServer* SteamGameServer_Hook();
void SteamGameServer_Shutdown_Hook();
ISteamMatchmaking* SteamMatchmaking_Hook();
ISteamMatchmakingServers* SteamMatchmakingServers_Hook();
ISteamNetworking* SteamNetworking_Hook();
ISteamRemoteStorage* SteamRemoteStorage_Hook();
ISteamUser* SteamUser_Hook();
ISteamUserStats* SteamUserStats_Hook();
ISteamUtils* SteamUtils_Hook();

void load_steam_cloud();
bool save_steam_cloud(const wchar_t* Name);

#endif
