#include "defines.h"

#include "steam.h"

#include <mutex>
#include <vector>
#include <unordered_map>

#include <filesystem>

#include <cassert>

#include "debug.h"
#include "state.h"

std::recursive_mutex CallbackMutex;

struct call_result {
    std::vector<unsigned char> Buffer;
    bool IoFailure;
};

struct call_result_entry {
    CCallbackBase* Callback;
    SteamAPICall_t ApiCall;
};

std::unordered_map<SteamAPICall_t, call_result> CallResults = {};
std::vector<call_result_entry> CallResultEntries = {};

struct persona {
    const char* Name;
};

static constexpr persona Personas[] = {
    {"Bethany Ramsey"},
    {"Lorna Trengove"},
    {"Grover Thorn"},
    {"Winton Pemberton"},
    {"Delmar Danniel"},
    {"Quinten Grayson"},
    {"Vernon Gibson"},
    {"Westley Dedrick"},
    {"Chad Sims"},
    {"Michael Glover"},
    {"Jarrett Kitchen"},
    {"Edison Kellogg"},
    {"Candace Barnett"},
    {"Harold Ruff"},
    {"Sabrina Falconer"},
    {"Romaine Waters"},
    {"Bradford Christian"},
    {"Temperance Hawking"},
    {"Daisy Milburn"},
    {"Nona Pickle"},
};

const persona* get_persona(CSteamID Id){
    if(Id.Id == 0 || Id.Id >= std::size(Personas)){
        return nullptr;
    }

    return Personas+(Id.Id-1);
}

static SteamAPICall_t add_call_result(const void* Data, std::size_t Size, bool IoFailure){
    std::lock_guard Lock(CallbackMutex);

    SteamAPICall_t r = 1;

    while(CallResults.contains(r)){
        ++r;
    }

    auto& Result = CallResults[r];
    Result.Buffer.resize(Size);
    std::memcpy(Result.Buffer.data(), Data, Size);

    Result.IoFailure = IoFailure;

    return r;
}

template <typename T>
static SteamAPICall_t add_call_result(const T& t, bool IoFailure = false){
    return add_call_result(&t, sizeof(t), IoFailure);
}

struct ISteamApps {
    struct dlc {
        AppId_t AppId;
        bool Available;
        const char* Name;
        bool Subscribed;
        bool Installed;
    };

    static constexpr dlc Dlcs[] = {
        {
            .AppId = 208570,
            .Available = true,
            .Name = "Dishonored: Dunwall City Trials DLC",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 208575,
            .Available = true,
            .Name = "Dishonored: The Knife of Dunwall",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 212890,
            .Available = true,
            .Name = "Dishonored Shadow Rat Pack",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 212891,
            .Available = false,
            .Name = "Dishonored Backstreet Butcher Pack",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 212892,
            .Available = false,
            .Name = "Dishonored Arcane Assassin Pack",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 212893,
            .Available = true,
            .Name = "Dishonored: Void Walkers Arsenal",
            .Subscribed = true,
            .Installed = true,
        },
        {
            .AppId = 212894,
            .Available = true,
            .Name = "Dishonored: The Brigmore Witches",
            .Subscribed = true,
            .Installed = true,
        },
    };

    static constexpr int DlcCount = std::size(Dlcs);

    static const dlc* get_dlc(AppId_t AppId){
        for(auto& Dlc:Dlcs){
            if(Dlc.AppId == AppId){
                return &Dlc;
            }
        }

        return nullptr;
    }

    virtual bool BIsSubscribed(){ NYI("ISteamApps::BIsSubscribed()\n"); }

    virtual bool BIsLowViolence(){
        DLOG("ISteamApps::BIsLowViolence()\n");
        return false;
    }

    virtual bool BIsCybercafe(){ NYI("ISteamApps::BIsCybercafe()\n"); }
    virtual bool BIsVACBanned(){ NYI("ISteamApps::BIsVACBanned()\n"); }

    virtual const char* GetCurrentGameLanguage(){
        auto r = "english";
        DLOG("ISteamApps::GetCurrentGameLanguage(): \"%s\"\n", r);
        return r;
    }

    virtual const char* GetAvailableGameLanguages(){ NYI("ISteamApps::GetAvailableGameLanguages()\n"); }

    virtual bool BIsSubscribedApp(AppId_t AppId){
        bool r = false;
        if(auto Dlc = get_dlc(AppId)){
            r = Dlc->Subscribed;
        }
        DLOG("ISteamApps::BIsSubscribedApp(%u): %d\n", AppId, r);
        return r;
    }

    virtual bool BIsDlcInstalled(AppId_t AppId){
        bool r = false;
        if(auto Dlc = get_dlc(AppId)){
            r = Dlc->Installed;
        }
        DLOG("ISteamApps::BIsDlcInstalled(%u): %d\n", AppId, r);
        return r;
    }

    virtual std::uint32_t GetEarliestPurchaseUnixTime(AppId_t){ NYI("ISteamApps::GetEarliestPurchaseUnixTime()\n"); }
    virtual bool BIsSubscribedFromFreeWeekend(){ NYI("ISteamApps::BIsSubscribedFromFreeWeekend()\n"); }

    virtual int GetDLCCount(){
        auto r = DlcCount;
        DLOG("ISteamApps::GetDLCCount(): %d\n", r);
        return r;
    }

    virtual bool BGetDLCDataByIndex(int Index, AppId_t* AppId, bool* Available, char* Name, int NameSize){
        bool r = false;
        if(Index < DlcCount){
            auto& Dlc = Dlcs[Index];
            *AppId = Dlc.AppId;
            *Available = Dlc.Available;
            std::snprintf(Name, static_cast<std::size_t>(NameSize), "%s", Dlc.Name);
            r = true;
        }
        DLOG("ISteamApps::BGetDLCDataByIndex(%d, *0x%p = %u, *0x%p = %d, \"%.*s\", %d): %d\n", Index, AppId, *AppId, Available, *Available, NameSize, Name, NameSize, r);
        return r;
    }

    virtual void InstallDLC(AppId_t){ NYI("ISteamApps::InstallDLC()\n"); }
    virtual void UninstallDLC(AppId_t){ NYI("ISteamApps::UninstallDLC()\n"); }
    virtual void RequestAppProofOfPurchaseKey(AppId_t){ NYI("ISteamApps::RequestAppProofOfPurchaseKey()\n"); }
    virtual bool GetCurrentBetaName(char*, int){ NYI("ISteamApps::GetCurrentBetaName()\n"); }
    virtual bool MarkContentCorrupt(bool){ NYI("ISteamApps::MarkContentCorrupt()\n"); }
    virtual std::uint32_t GetInstalledDepots(AppId_t, DepotId_t*, std::uint32_t){ NYI("ISteamApps::GetInstalledDepots()\n"); }
    virtual std::uint32_t GetAppInstallDir(AppId_t, char*, std::uint32_t){ NYI("ISteamApps::GetAppInstallDir()\n"); }
    virtual bool BIsAppInstalled(AppId_t){ NYI("ISteamApps::BIsAppInstalled()\n"); }
    virtual CSteamID GetAppOwner(){ NYI("ISteamApps::GetAppOwner()\n"); }
    virtual const char* GetLaunchQueryParam(const char*){ NYI("ISteamApps::GetLaunchQueryParam()\n"); }
};

struct ISteamFriends {
    virtual const char* GetPersonaName(){
        DLOG("ISteamFriends::GetPersonaName()\n");
        return "TasBot";
    }

    virtual SteamAPICall_t SetPersonaName(const char*){ NYI("ISteamFriends::SetPersonaName()\n"); }
    virtual EPersonaState GetPersonaState(){ NYI("ISteamFriends::GetPersonaState()\n"); }

    virtual int GetFriendCount(int Flags){
        DLOG("ISteamFriends::GetFriendCount(0x%04X)\n", Flags);
        return 0;
    }

    virtual CSteamID GetFriendByIndex(int, int){ NYI("ISteamFriends::GetFriendByIndex()\n"); }
    virtual EFriendRelationship GetFriendRelationship(CSteamID){ NYI("ISteamFriends::GetFriendRelationship()\n"); }
    virtual EPersonaState GetFriendPersonaState(CSteamID){ NYI("ISteamFriends::GetFriendPersonaState()\n"); }

    virtual const char* GetFriendPersonaName(CSteamID Id){
        auto r = "[unknown]";

        if(auto Persona = get_persona(Id)){
            r = Persona->Name;
        }

        DLOG("ISteamFriends::GetFriendPersonaName(%llu): \"%s\"\n", Id.Id, r);

        return r;
    }

    virtual bool GetFriendGamePlayed(CSteamID, FriendGameInfo_t*){ NYI("ISteamFriends::GetFriendGamePlayed()\n"); }
    virtual const char* GetFriendPersonaNameHistory(CSteamID, int){ NYI("ISteamFriends::GetFriendPersonaNameHistory()\n"); }
    virtual bool HasFriend(CSteamID, int){ NYI("ISteamFriends::HasFriend()\n"); }
    virtual int GetClanCount(){ NYI("ISteamFriends::GetClanCount()\n"); }
    virtual CSteamID GetClanByIndex(int){ NYI("ISteamFriends::GetClanByIndex()\n"); }
    virtual const char* GetClanName(CSteamID){ NYI("ISteamFriends::GetClanName()\n"); }
    virtual const char* GetClanTag(CSteamID){ NYI("ISteamFriends::GetClanTag()\n"); }
    virtual bool GetClanActivityCounts(CSteamID, int*, int*, int*){ NYI("ISteamFriends::GetClanActivityCounts()\n"); }
    virtual SteamAPICall_t DownloadClanActivityCounts(CSteamID*, int){ NYI("ISteamFriends::DownloadClanActivityCounts()\n"); }
    virtual int GetFriendCountFromSource(CSteamID){ NYI("ISteamFriends::GetFriendCountFromSource()\n"); }
    virtual CSteamID GetFriendFromSourceByIndex(CSteamID, int){ NYI("ISteamFriends::GetFriendFromSourceByIndex()\n"); }
    virtual bool IsUserInSource(CSteamID, CSteamID){ NYI("ISteamFriends::IsUserInSource()\n"); }
    virtual void SetInGameVoiceSpeaking(CSteamID, bool){ NYI("ISteamFriends::SetInGameVoiceSpeaking()\n"); }
    virtual void ActivateGameOverlay(const char*){ NYI("ISteamFriends::ActivateGameOverlay()\n"); }

    virtual void ActivateGameOverlayToUser(const char* Name, CSteamID Id){
        DLOG("ISteamFriends::ActivateGameOverlayToUser(\"%s\", %llu)\n", Name, Id.Id);
    }

    virtual void ActivateGameOverlayToWebPage(const char*){ NYI("ISteamFriends::ActivateGameOverlayToWebPage()\n"); }

    virtual void ActivateGameOverlayToStore(AppId_t AppId){
        DLOG("ISteamFriends::ActivateGameOverlayToStore(%u)\n", AppId);
    }

    virtual void SetPlayedWith(CSteamID){ NYI("ISteamFriends::SetPlayedWith()\n"); }
    virtual void ActivateGameOverlayInviteDialog(CSteamID){ NYI("ISteamFriends::ActivateGameOverlayInviteDialog()\n"); }
    virtual int GetSmallFriendAvatar(CSteamID){ NYI("ISteamFriends::GetSmallFriendAvatar()\n"); }
    virtual int GetMediumFriendAvatar(CSteamID){ NYI("ISteamFriends::GetMediumFriendAvatar()\n"); }
    virtual int GetLargeFriendAvatar(CSteamID){ NYI("ISteamFriends::GetLargeFriendAvatar()\n"); }
    virtual bool RequestUserInformation(CSteamID, bool){ NYI("ISteamFriends::RequestUserInformation()\n"); }
    virtual SteamAPICall_t RequestClanOfficerList(CSteamID){ NYI("ISteamFriends::RequestClanOfficerList()\n"); }
    virtual CSteamID GetClanOwner(CSteamID){ NYI("ISteamFriends::GetClanOwner()\n"); }
    virtual int GetClanOfficerCount(CSteamID){ NYI("ISteamFriends::GetClanOfficerCount()\n"); }
    virtual CSteamID GetClanOfficerByIndex(CSteamID, int){ NYI("ISteamFriends::GetClanOfficerByIndex()\n"); }
    virtual std::uint32_t GetUserRestrictions(){ NYI("ISteamFriends::GetUserRestrictions()\n"); }

    virtual bool SetRichPresence(const char* Key, const char* Value){
        DLOG("ISteamFriends::SetRichPresence(\"%s\", \"%s\")\n", Key, Value);
        return true;
    }

    virtual void ClearRichPresence(){ NYI("ISteamFriends::ClearRichPresence()\n"); }
    virtual const char* GetFriendRichPresence(CSteamID, const char*){ NYI("ISteamFriends::GetFriendRichPresence()\n"); }
    virtual int GetFriendRichPresenceKeyCount(CSteamID){ NYI("ISteamFriends::GetFriendRichPresenceKeyCount()\n"); }
    virtual const char* GetFriendRichPresenceKeyByIndex(CSteamID, int){ NYI("ISteamFriends::GetFriendRichPresenceKeyByIndex()\n"); }
    virtual void RequestFriendRichPresence(CSteamID){ NYI("ISteamFriends::RequestFriendRichPresence()\n"); }
    virtual bool InviteUserToGame(CSteamID, const char*){ NYI("ISteamFriends::InviteUserToGame()\n"); }
    virtual int GetCoplayFriendCount(){ NYI("ISteamFriends::GetCoplayFriendCount()\n"); }
    virtual CSteamID GetCoplayFriend(int){ NYI("ISteamFriends::GetCoplayFriend()\n"); }
    virtual int GetFriendCoplayTime(CSteamID){ NYI("ISteamFriends::GetFriendCoplayTime()\n"); }
    virtual AppId_t GetFriendCoplayGame(CSteamID){ NYI("ISteamFriends::GetFriendCoplayGame()\n"); }
    virtual SteamAPICall_t JoinClanChatRoom(CSteamID){ NYI("ISteamFriends::JoinClanChatRoom()\n"); }
    virtual bool LeaveClanChatRoom(CSteamID){ NYI("ISteamFriends::LeaveClanChatRoom()\n"); }
    virtual int GetClanChatMemberCount(CSteamID){ NYI("ISteamFriends::GetClanChatMemberCount()\n"); }
    virtual CSteamID GetChatMemberByIndex(CSteamID, int){ NYI("ISteamFriends::GetChatMemberByIndex()\n"); }
    virtual bool SendClanChatMessage(CSteamID, const char*){ NYI("ISteamFriends::SendClanChatMessage()\n"); }
    virtual int GetClanChatMessage(CSteamID, int, void*, int, EChatEntryType*, CSteamID*){ NYI("ISteamFriends::GetClanChatMessage()\n"); }
    virtual bool IsClanChatAdmin(CSteamID, CSteamID){ NYI("ISteamFriends::IsClanChatAdmin()\n"); }
    virtual bool IsClanChatWindowOpenInSteam(CSteamID){ NYI("ISteamFriends::IsClanChatWindowOpenInSteam()\n"); }
    virtual bool OpenClanChatWindowInSteam(CSteamID){ NYI("ISteamFriends::OpenClanChatWindowInSteam()\n"); }
    virtual bool CloseClanChatWindowInSteam(CSteamID){ NYI("ISteamFriends::CloseClanChatWindowInSteam()\n"); }
    virtual bool SetListenForFriendsMessages(bool){ NYI("ISteamFriends::SetListenForFriendsMessages()\n"); }
    virtual bool ReplyToFriendMessage(CSteamID, const char*){ NYI("ISteamFriends::ReplyToFriendMessage()\n"); }
    virtual int GetFriendMessage(CSteamID, int, void*, int, EChatEntryType*){ NYI("ISteamFriends::GetFriendMessage()\n"); }
    virtual SteamAPICall_t GetFollowerCount(CSteamID){ NYI("ISteamFriends::GetFollowerCount()\n"); }
    virtual SteamAPICall_t IsFollowing(CSteamID){ NYI("ISteamFriends::IsFollowing()\n"); }
    virtual SteamAPICall_t EnumerateFollowingList(std::uint32_t){ NYI("ISteamFriends::EnumerateFollowingList()\n"); }
};

struct ISteamGameServer {
    virtual bool InitGameServer(std::uint32_t, std::uint16_t, std::uint16_t, std::uint32_t, AppId_t, const char*){ NYI("ISteamGameServer::InitGameServer()\n"); }
    virtual void SetProduct(const char*){ NYI("ISteamGameServer::SetProduct()\n"); }
    virtual void SetGameDescription(const char*){ NYI("ISteamGameServer::SetGameDescription()\n"); }
    virtual void SetModDir(const char*){ NYI("ISteamGameServer::SetModDir()\n"); }
    virtual void SetDedicatedServer(bool){ NYI("ISteamGameServer::SetDedicatedServer()\n"); }
    virtual void LogOn(const char*){ NYI("ISteamGameServer::LogOn()\n"); }
    virtual void LogOnAnonymous(){ NYI("ISteamGameServer::LogOnAnonymous()\n"); }
    virtual void LogOff(){ NYI("ISteamGameServer::LogOff()\n"); }
    virtual bool BLoggedOn(){ NYI("ISteamGameServer::BLoggedOn()\n"); }
    virtual bool BSecure(){ NYI("ISteamGameServer::BSecure()\n"); }

    virtual CSteamID GetSteamID(){
        DLOG("ISteamGameServer::GetSteamID()\n");
        return {5};
    }

    virtual bool WasRestartRequested(){ NYI("ISteamGameServer::WasRestartRequested()\n"); }
    virtual void SetMaxPlayerCount(int){ NYI("ISteamGameServer::SetMaxPlayerCount()\n"); }
    virtual void SetBotPlayerCount(int){ NYI("ISteamGameServer::SetBotPlayerCount()\n"); }
    virtual void SetServerName(const char*){ NYI("ISteamGameServer::SetServerName()\n"); }
    virtual void SetMapName(const char*){ NYI("ISteamGameServer::SetMapName()\n"); }
    virtual void SetPasswordProtected(bool){ NYI("ISteamGameServer::SetPasswordProtected()\n"); }
    virtual void SetSpectatorPort(std::uint16_t){ NYI("ISteamGameServer::SetSpectatorPort()\n"); }
    virtual void SetSpectatorServerName(const char*){ NYI("ISteamGameServer::SetSpectatorServerName()\n"); }
    virtual void ClearAllKeyValues(){ NYI("ISteamGameServer::ClearAllKeyValues()\n"); }
    virtual void SetKeyValue(const char*, const char*){ NYI("ISteamGameServer::SetKeyValue()\n"); }
    virtual void SetGameTags(const char*){ NYI("ISteamGameServer::SetGameTags()\n"); }
    virtual void SetGameData(const char*){ NYI("ISteamGameServer::SetGameData()\n"); }
    virtual void SetRegion(const char*){ NYI("ISteamGameServer::SetRegion()\n"); }
    virtual bool SendUserConnectAndAuthenticate(std::uint32_t, const void*, std::uint32_t, CSteamID*){ NYI("ISteamGameServer::SendUserConnectAndAuthenticate()\n"); }
    virtual CSteamID CreateUnauthenticatedUserConnection(){ NYI("ISteamGameServer::CreateUnauthenticatedUserConnection()\n"); }
    virtual void SendUserDisconnect(CSteamID){ NYI("ISteamGameServer::SendUserDisconnect()\n"); }
    virtual bool BUpdateUserData(CSteamID, const char*, std::uint32_t){ NYI("ISteamGameServer::BUpdateUserData()\n"); }
    virtual HAuthTicket GetAuthSessionTicket(void*, int, std::uint32_t*){ NYI("ISteamGameServer::GetAuthSessionTicket()\n"); }
    virtual EBeginAuthSessionResult BeginAuthSession(const void*, int, CSteamID){ NYI("ISteamGameServer::BeginAuthSession()\n"); }
    virtual void EndAuthSession(CSteamID){ NYI("ISteamGameServer::EndAuthSession()\n"); }
    virtual void CancelAuthTicket(HAuthTicket){ NYI("ISteamGameServer::CancelAuthTicket()\n"); }
    virtual EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID, AppId_t){ NYI("ISteamGameServer::UserHasLicenseForApp()\n"); }
    virtual bool RequestUserGroupStatus(CSteamID, CSteamID){ NYI("ISteamGameServer::RequestUserGroupStatus()\n"); }
    virtual void GetGameplayStats(){ NYI("ISteamGameServer::GetGameplayStats()\n"); }
    virtual SteamAPICall_t GetServerReputation(){ NYI("ISteamGameServer::GetServerReputation()\n"); }
    virtual std::uint32_t GetPublicIP(){ NYI("ISteamGameServer::GetPublicIP()\n"); }
    virtual bool HandleIncomingPacket(const void*, int, std::uint32_t, std::uint16_t){ NYI("ISteamGameServer::HandleIncomingPacket()\n"); }
    virtual int GetNextOutgoingPacket(void*, int, std::uint32_t*, std::uint16_t*){ NYI("ISteamGameServer::GetNextOutgoingPacket()\n"); }
    virtual void EnableHeartbeats(bool){ NYI("ISteamGameServer::EnableHeartbeats()\n"); }
    virtual void SetHeartbeatInterval(int){ NYI("ISteamGameServer::SetHeartbeatInterval()\n"); }
    virtual void ForceHeartbeat(){ NYI("ISteamGameServer::ForceHeartbeat()\n"); }
    virtual SteamAPICall_t AssociateWithClan(CSteamID){ NYI("ISteamGameServer::AssociateWithClan()\n"); }
    virtual SteamAPICall_t ComputeNewPlayerCompatibility(CSteamID){ NYI("ISteamGameServer::ComputeNewPlayerCompatibility()\n"); }
};

struct ISteamMatchmaking {
    virtual int GetFavoriteGameCount(){ NYI("ISteamMatchmaking::GetFavoriteGameCount()\n"); }
    virtual bool GetFavoriteGame(int, AppId_t*, std::uint32_t*, std::uint16_t*, std::uint16_t*, std::uint32_t*, std::uint32_t*){ NYI("ISteamMatchmaking::GetFavoriteGame()\n"); }
    virtual int AddFavoriteGame(AppId_t, std::uint32_t, std::uint16_t, std::uint16_t, std::uint32_t, std::uint32_t){ NYI("ISteamMatchmaking::AddFavoriteGame()\n"); }
    virtual bool RemoveFavoriteGame(AppId_t, std::uint32_t, std::uint16_t, std::uint16_t, std::uint32_t){ NYI("ISteamMatchmaking::RemoveFavoriteGame()\n"); }
    virtual SteamAPICall_t RequestLobbyList(){ NYI("ISteamMatchmaking::RequestLobbyList()\n"); }
    virtual void AddRequestLobbyListStringFilter(const char*, const char*, ELobbyComparison){ NYI("ISteamMatchmaking::AddRequestLobbyListStringFilter()\n"); }
    virtual void AddRequestLobbyListNumericalFilter(const char*, int, ELobbyComparison){ NYI("ISteamMatchmaking::AddRequestLobbyListNumericalFilter()\n"); }
    virtual void AddRequestLobbyListNearValueFilter(const char*, int){ NYI("ISteamMatchmaking::AddRequestLobbyListNearValueFilter()\n"); }
    virtual void AddRequestLobbyListFilterSlotsAvailable(int){ NYI("ISteamMatchmaking::AddRequestLobbyListFilterSlotsAvailable()\n"); }
    virtual void AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter){ NYI("ISteamMatchmaking::AddRequestLobbyListDistanceFilter()\n"); }
    virtual void AddRequestLobbyListResultCountFilter(int){ NYI("ISteamMatchmaking::AddRequestLobbyListResultCountFilter()\n"); }
    virtual void AddRequestLobbyListCompatibleMembersFilter(CSteamID){ NYI("ISteamMatchmaking::AddRequestLobbyListCompatibleMembersFilter()\n"); }
    virtual CSteamID GetLobbyByIndex(int){ NYI("ISteamMatchmaking::GetLobbyByIndex()\n"); }
    virtual SteamAPICall_t CreateLobby(ELobbyType, int){ NYI("ISteamMatchmaking::CreateLobby()\n"); }
    virtual SteamAPICall_t JoinLobby(CSteamID){ NYI("ISteamMatchmaking::JoinLobby()\n"); }
    virtual void LeaveLobby(CSteamID){ NYI("ISteamMatchmaking::LeaveLobby()\n"); }
    virtual bool InviteUserToLobby(CSteamID, CSteamID){ NYI("ISteamMatchmaking::InviteUserToLobby()\n"); }
    virtual int GetNumLobbyMembers(CSteamID){ NYI("ISteamMatchmaking::GetNumLobbyMembers()\n"); }
    virtual CSteamID GetLobbyMemberByIndex(CSteamID, int){ NYI("ISteamMatchmaking::GetLobbyMemberByIndex()\n"); }
    virtual const char* GetLobbyData(CSteamID, const char*){ NYI("ISteamMatchmaking::GetLobbyData()\n"); }
    virtual bool SetLobbyData(CSteamID, const char*, const char*){ NYI("ISteamMatchmaking::SetLobbyData()\n"); }
    virtual int GetLobbyDataCount(CSteamID){ NYI("ISteamMatchmaking::GetLobbyDataCount()\n"); }
    virtual bool GetLobbyDataByIndex(CSteamID, int, char*, int, char*, int){ NYI("ISteamMatchmaking::GetLobbyDataByIndex()\n"); }
    virtual bool DeleteLobbyData(CSteamID, const char*){ NYI("ISteamMatchmaking::DeleteLobbyData()\n"); }
    virtual const char* GetLobbyMemberData(CSteamID, CSteamID, const char*){ NYI("ISteamMatchmaking::GetLobbyMemberData()\n"); }
    virtual void SetLobbyMemberData(CSteamID, const char*, const char*){ NYI("ISteamMatchmaking::SetLobbyMemberData()\n"); }
    virtual bool SendLobbyChatMsg(CSteamID, const void*, int){ NYI("ISteamMatchmaking::SendLobbyChatMsg()\n"); }
    virtual int GetLobbyChatEntry(CSteamID, int, CSteamID*, void*, int, EChatEntryType*){ NYI("ISteamMatchmaking::GetLobbyChatEntry()\n"); }
    virtual bool RequestLobbyData(CSteamID){ NYI("ISteamMatchmaking::RequestLobbyData()\n"); }
    virtual void SetLobbyGameServer(CSteamID, std::uint32_t, std::uint16_t, CSteamID){ NYI("ISteamMatchmaking::SetLobbyGameServer()\n"); }
    virtual bool GetLobbyGameServer(CSteamID, std::uint32_t*, std::uint16_t*, CSteamID*){ NYI("ISteamMatchmaking::GetLobbyGameServer()\n"); }
    virtual bool SetLobbyMemberLimit(CSteamID, int){ NYI("ISteamMatchmaking::SetLobbyMemberLimit()\n"); }
    virtual int GetLobbyMemberLimit(CSteamID){ NYI("ISteamMatchmaking::GetLobbyMemberLimit()\n"); }
    virtual bool SetLobbyType(CSteamID, ELobbyType){ NYI("ISteamMatchmaking::SetLobbyType()\n"); }
    virtual bool SetLobbyJoinable(CSteamID, bool){ NYI("ISteamMatchmaking::SetLobbyJoinable()\n"); }
    virtual CSteamID GetLobbyOwner(CSteamID){ NYI("ISteamMatchmaking::GetLobbyOwner()\n"); }
    virtual bool SetLobbyOwner(CSteamID, CSteamID){ NYI("ISteamMatchmaking::SetLobbyOwner()\n"); }
    virtual bool SetLinkedLobby(CSteamID, CSteamID){ NYI("ISteamMatchmaking::SetLinkedLobby()\n"); }
};

struct ISteamMatchmakingServers {
    virtual HServerListRequest RequestInternetServerList(AppId_t, MatchMakingKeyValuePair_t* *, std::uint32_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestInternetServerList()\n"); }
    virtual HServerListRequest RequestLANServerList(AppId_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestLANServerList()\n"); }
    virtual HServerListRequest RequestFriendsServerList(AppId_t, MatchMakingKeyValuePair_t* *, std::uint32_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestFriendsServerList()\n"); }
    virtual HServerListRequest RequestFavoritesServerList(AppId_t, MatchMakingKeyValuePair_t* *, std::uint32_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestFavoritesServerList()\n"); }
    virtual HServerListRequest RequestHistoryServerList(AppId_t, MatchMakingKeyValuePair_t* *, std::uint32_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestHistoryServerList()\n"); }
    virtual HServerListRequest RequestSpectatorServerList(AppId_t, MatchMakingKeyValuePair_t* *, std::uint32_t, ISteamMatchmakingServerListResponse*){ NYI("ISteamMatchmakingServers::RequestSpectatorServerList()\n"); }
    virtual void ReleaseRequest(HServerListRequest){ NYI("ISteamMatchmakingServers::ReleaseRequest()\n"); }
    virtual gameserveritem_t* GetServerDetails(HServerListRequest, int){ NYI("ISteamMatchmakingServers::GetServerDetails()\n"); }
    virtual void CancelQuery(HServerListRequest){ NYI("ISteamMatchmakingServers::CancelQuery()\n"); }
    virtual void RefreshQuery(HServerListRequest){ NYI("ISteamMatchmakingServers::RefreshQuery()\n"); }
    virtual bool IsRefreshing(HServerListRequest){ NYI("ISteamMatchmakingServers::IsRefreshing()\n"); }
    virtual int GetServerCount(HServerListRequest){ NYI("ISteamMatchmakingServers::GetServerCount()\n"); }
    virtual void RefreshServer(HServerListRequest, int){ NYI("ISteamMatchmakingServers::RefreshServer()\n"); }
    virtual HServerQuery PingServer(std::uint32_t, std::uint16_t, ISteamMatchmakingPingResponse*){ NYI("ISteamMatchmakingServers::PingServer()\n"); }
    virtual HServerQuery PlayerDetails(std::uint32_t, std::uint16_t, ISteamMatchmakingPlayersResponse*){ NYI("ISteamMatchmakingServers::PlayerDetails()\n"); }
    virtual HServerQuery ServerRules(std::uint32_t, std::uint16_t, ISteamMatchmakingRulesResponse*){ NYI("ISteamMatchmakingServers::ServerRules()\n"); }
    virtual void CancelServerQuery(HServerQuery){ NYI("ISteamMatchmakingServers::CancelServerQuery()\n"); }
};

struct ISteamNetworking {
    virtual bool SendP2PPacket(CSteamID, const void*, std::uint32_t, EP2PSend, int){ NYI("ISteamNetworking::SendP2PPacket()\n"); }
    virtual bool IsP2PPacketAvailable(std::uint32_t*, int){ NYI("ISteamNetworking::IsP2PPacketAvailable()\n"); }
    virtual bool ReadP2PPacket(void*, std::uint32_t, std::uint32_t*, CSteamID*, int){ NYI("ISteamNetworking::ReadP2PPacket()\n"); }
    virtual bool AcceptP2PSessionWithUser(CSteamID){ NYI("ISteamNetworking::AcceptP2PSessionWithUser()\n"); }
    virtual bool CloseP2PSessionWithUser(CSteamID){ NYI("ISteamNetworking::CloseP2PSessionWithUser()\n"); }
    virtual bool CloseP2PChannelWithUser(CSteamID, int){ NYI("ISteamNetworking::CloseP2PChannelWithUser()\n"); }
    virtual bool GetP2PSessionState(CSteamID, P2PSessionState_t*){ NYI("ISteamNetworking::GetP2PSessionState()\n"); }

    virtual bool AllowP2PPacketRelay(bool Allow){
        DLOG("ISteamNetworking::AllowP2PPacketRelay(%d)\n",  Allow);
        return true;
    }

    virtual SNetListenSocket_t CreateListenSocket(int, std::uint32_t, std::uint16_t, bool){ NYI("ISteamNetworking::CreateListenSocket()\n"); }
    virtual SNetSocket_t CreateP2PConnectionSocket(CSteamID, int, int, bool){ NYI("ISteamNetworking::CreateP2PConnectionSocket()\n"); }
    virtual SNetSocket_t CreateConnectionSocket(std::uint32_t, std::uint16_t, int){ NYI("ISteamNetworking::CreateConnectionSocket()\n"); }
    virtual bool DestroySocket(SNetSocket_t, bool){ NYI("ISteamNetworking::DestroySocket()\n"); }
    virtual bool DestroyListenSocket(SNetListenSocket_t, bool){ NYI("ISteamNetworking::DestroyListenSocket()\n"); }
    virtual bool SendDataOnSocket(SNetSocket_t, void*, std::uint32_t, bool){ NYI("ISteamNetworking::SendDataOnSocket()\n"); }
    virtual bool IsDataAvailableOnSocket(SNetSocket_t, std::uint32_t*){ NYI("ISteamNetworking::IsDataAvailableOnSocket()\n"); }
    virtual bool RetrieveDataFromSocket(SNetSocket_t, void*, std::uint32_t, std::uint32_t*){ NYI("ISteamNetworking::RetrieveDataFromSocket()\n"); }
    virtual bool IsDataAvailable(SNetListenSocket_t, std::uint32_t*, SNetSocket_t*){ NYI("ISteamNetworking::IsDataAvailable()\n"); }
    virtual bool RetrieveData(SNetListenSocket_t, void*, std::uint32_t, std::uint32_t*, SNetSocket_t*){ NYI("ISteamNetworking::RetrieveData()\n"); }
    virtual bool GetSocketInfo(SNetSocket_t, CSteamID*, int*, std::uint32_t*, std::uint16_t*){ NYI("ISteamNetworking::GetSocketInfo()\n"); }
    virtual bool GetListenSocketInfo(SNetListenSocket_t, std::uint32_t*, std::uint16_t*){ NYI("ISteamNetworking::GetListenSocketInfo()\n"); }
    virtual ESNetSocketConnectionType GetSocketConnectionType(SNetSocket_t){ NYI("ISteamNetworking::GetSocketConnectionType()\n"); }
    virtual int GetMaxPacketSize(SNetSocket_t){ NYI("ISteamNetworking::GetMaxPacketSize()\n"); }
};

constexpr std::int32_t TotalCloudQuota = 1024*1024*1024;

struct file_deleter {
    void operator()(std::FILE* File) const {
        std::fclose(File);
    }
};

using unique_file = std::unique_ptr<std::FILE, file_deleter>;

struct ISteamRemoteStorage {
    std::mutex FileMutex;

    struct file_entry {
        std::string Name;
        std::vector<unsigned char> Buffer;
        std::int64_t Timestamp;
    };

    std::vector<file_entry> Files = {};
    std::int64_t Timestamp = 0;

    using file_it = std::vector<file_entry>::iterator;

    file_it get_file(const char* Name, bool Create = false){
        for(auto it = Files.begin(), End = Files.end(); it != End; ++it){
            if(_stricmp(it->Name.data(), Name) == 0){
                return it;
            }
        }

        if(Create){
            Files.push_back({
                .Name = Name,
                .Buffer = {},
                .Timestamp = 0,
            });

            return Files.end()-1;
        }else{
            return Files.end();
        }
    }

    void load_files(){
        for(auto& Entry:fs::directory_iterator(CmdArgs.Userdata)){
            if(!Entry.is_regular_file()){
                continue;
            }

            unique_file File(_wfopen(Entry.path().c_str(), L"rb"));
            if(!File){
                throw std::runtime_error("Unable to open cloud save.");
            }

            auto Time = Entry.last_write_time().time_since_epoch().count();

            auto Name = Entry.path().filename();

            auto Size = static_cast<DWORD>(Entry.file_size());
            if(Size != Entry.file_size()){
                throw std::runtime_error("Cloud save is too large.");
            }

            file_entry FileEntry = {
                .Name = Entry.path().filename().string(),
                .Buffer = std::vector<unsigned char>(Size),
                .Timestamp = Time,
            };

            if(std::fread(FileEntry.Buffer.data(), 1, Size, File.get()) != Size){
                throw std::runtime_error("Unable to read cloud save.");
            }

            Files.push_back(std::move(FileEntry));
        }

        std::sort(Files.begin(), Files.end(), [](auto& Lhs, auto& Rhs){
            if(Lhs.Timestamp == Rhs.Timestamp){
                return Lhs.Name < Rhs.Name;
            }else{
                return Lhs.Timestamp < Rhs.Timestamp;
            }
        });

        for(auto& File:Files){
            File.Timestamp = ++Timestamp;
        }
    }

    bool save_files(const wchar_t* Name){
        try {
            auto Path = fs::weakly_canonical(Name);
            if(fs::exists(Path) && fs::equivalent(Path, CmdArgs.Userdata)){
                throw std::runtime_error("Trying to overwrite userdata!\n");
            }

            fs::create_directories(Path);

            for(auto& Data:Files){
                auto Filepath = Path/Data.Name;

                unique_file File(_wfopen(Filepath.c_str(), L"wb"));
                if(!File){
                    throw std::runtime_error("Unable to open cloudsave.");
                }

                auto Size = Data.Buffer.size();
                if(std::fwrite(Data.Buffer.data(), 1, Size, File.get()) != Size){
                    throw std::runtime_error("Unable to write cloudsave.");
                }
            }

            return true;
        }catch(std::exception& e){
            std::fprintf(stderr, "Error: %s\n", e.what());
            return false;
        }
    }

    virtual bool FileWrite(const char* File, const unsigned char* Buffer, std::int32_t Size){
        std::lock_guard Lock(FileMutex);

        DLOG("ISteamRemoteStorage::FileWrite(\"%s\", 0x%p, %d)\n", File, Buffer, Size);

        auto it = get_file(File, true);
        it->Buffer.assign(Buffer, Buffer+Size);
        it->Timestamp = ++Timestamp;

        return true;
    }

    virtual std::int32_t FileRead(const char* File, unsigned char* Buffer, std::int32_t Size){
        std::lock_guard Lock(FileMutex);

        std::int32_t r = 0;
        if(auto it = get_file(File); it != Files.end()){
            r = std::min(static_cast<std::int32_t>(it->Buffer.size()), Size);

            if(r > 0){
                std::memcpy(Buffer, it->Buffer.data(), static_cast<std::size_t>(r));
            }
        }

        DLOG("ISteamRemoteStorage::FileRead(\"%s\", 0x%p, %d): %d\n", File, Buffer, Size, r);

        return r;
    }

    virtual bool FileForget(const char* File){ NYI("ISteamRemoteStorage::FileForget(\"%s\")\n", File); }
    virtual bool FileDelete(const char* File){ NYI("ISteamRemoteStorage::FileDelete(\"%s\")\n", File); }
    virtual SteamAPICall_t FileShare(const char* File){ NYI("ISteamRemoteStorage::FileShare(\"%s\")\n", File); }
    virtual bool SetSyncPlatforms(const char* File, ERemoteStoragePlatform Platform){ NYI("ISteamRemoteStorage::SetSyncPlatforms(\"%s\", %08X)\n", File, Platform); }

    virtual bool FileExists(const char* File){
        auto r = get_file(File) != Files.end();
        DLOG("ISteamRemoteStorage::FileExists(\"%s\"): %d\n", File, r);
        return r;
    }

    virtual bool FilePersisted(const char* File){ NYI("ISteamRemoteStorage::FilePersisted(\"%s\")\n", File); }

    virtual std::int32_t GetFileSize(const char* File){
        std::lock_guard Lock(FileMutex);

        std::int32_t r = 0;
        if(auto it = get_file(File); it != Files.end()){
            r = static_cast<std::int32_t>(it->Buffer.size());
        }

        DLOG("ISteamRemoteStorage::GetFileSize(\"%s\"): %d\n", File, r);

        return r;
    }

    virtual std::int64_t GetFileTimestamp(const char* File){
        std::lock_guard Lock(FileMutex);

        std::int64_t r = 0;
        if(auto it = get_file(File); it != Files.end()){
            r = it->Timestamp;
        }

        DLOG("ISteamRemoteStorage::GetFileTimestamp(\"%s\"): %lld\n", File, r);

        return r;
    }

    virtual ERemoteStoragePlatform GetSyncPlatforms(const char* File){ NYI("ISteamRemoteStorage::GetSyncPlatforms(\"%s\")\n", File); }

    virtual std::int32_t GetFileCount(){
        std::lock_guard Lock(FileMutex);
        DLOG("ISteamRemoteStorage::GetFileCount()\n");
        return static_cast<std::int32_t>(Files.size());
    }

    virtual const char* GetFileNameAndSize(int Index, std::int32_t* Size){
        std::lock_guard Lock(FileMutex);

        auto& File = Files[static_cast<std::size_t>(Index)];

        *Size = static_cast<std::int32_t>(File.Buffer.size());

        DLOG("ISteamRemoteStorage::GetFileNameAndSize(%d, *0x%p = %d): \"%s\"\n", Index, Size, *Size, File.Name.data());

        return File.Name.data();
    }

    virtual bool GetQuota(std::int32_t* Total, std::int32_t* Available){
        std::lock_guard Lock(FileMutex);

        *Total = TotalCloudQuota;
        *Available = TotalCloudQuota;

        for(auto& File:Files){
            *Available -= static_cast<std::int32_t>(File.Buffer.size());
        }

        DLOG("ISteamRemoteStorage::GetQuota(*0x%p = %d, *0x%p = %d)\n", Total, *Total, Available, *Available);

        return true;
    }

    virtual bool IsCloudEnabledForAccount(){
        DLOG("ISteamRemoteStorage::IsCloudEnabledForAccount()\n");
        return false;
    }

    virtual bool IsCloudEnabledForApp(){ NYI("ISteamRemoteStorage::IsCloudEnabledForApp()\n"); }
    virtual void SetCloudEnabledForApp(bool){ NYI("ISteamRemoteStorage::SetCloudEnabledForApp()\n"); }
    virtual SteamAPICall_t UGCDownload(UGCHandle_t, std::uint32_t){ NYI("ISteamRemoteStorage::UGCDownload()\n"); }
    virtual bool GetUGCDownloadProgress(UGCHandle_t, std::int32_t*, std::int32_t*){ NYI("ISteamRemoteStorage::GetUGCDownloadProgress()\n"); }
    virtual bool GetUGCDetails(UGCHandle_t, AppId_t*, char* *, std::int32_t*, CSteamID*){ NYI("ISteamRemoteStorage::GetUGCDetails()\n"); }
    virtual std::int32_t UGCRead(UGCHandle_t, void*, std::int32_t, std::uint32_t, EUGCReadAction){ NYI("ISteamRemoteStorage::UGCRead()\n"); }
    virtual std::int32_t GetCachedUGCCount(){ NYI("ISteamRemoteStorage::GetCachedUGCCount()\n"); }
    virtual UGCHandle_t GetCachedUGCHandle(std::int32_t){ NYI("ISteamRemoteStorage::GetCachedUGCHandle()\n"); }
    virtual SteamAPICall_t PublishVideo(EWorkshopVideoProvider, const char*, const char*, const char*, AppId_t, const char*, const char*, ERemoteStoragePublishedFileVisibility, SteamParamStringArray_t*){ NYI("ISteamRemoteStorage::PublishVideo()\n"); }
    virtual SteamAPICall_t PublishWorkshopFile(const char*, const char*, AppId_t, const char*, const char*, ERemoteStoragePublishedFileVisibility, SteamParamStringArray_t*, EWorkshopFileType){ NYI("ISteamRemoteStorage::PublishWorkshopFile()\n"); }
    virtual PublishedFileUpdateHandle_t CreatePublishedFileUpdateRequest(PublishedFileId_t){ NYI("ISteamRemoteStorage::CreatePublishedFileUpdateRequest()\n"); }
    virtual bool UpdatePublishedFileFile(PublishedFileUpdateHandle_t, const char*){ NYI("ISteamRemoteStorage::UpdatePublishedFileFile()\n"); }
    virtual bool UpdatePublishedFilePreviewFile(PublishedFileUpdateHandle_t, const char*){ NYI("ISteamRemoteStorage::UpdatePublishedFilePreviewFile()\n"); }
    virtual bool UpdatePublishedFileTitle(PublishedFileUpdateHandle_t, const char*){ NYI("ISteamRemoteStorage::UpdatePublishedFileTitle()\n"); }
    virtual bool UpdatePublishedFileDescription(PublishedFileUpdateHandle_t, const char*){ NYI("ISteamRemoteStorage::UpdatePublishedFileDescription()\n"); }
    virtual bool UpdatePublishedFileVisibility(PublishedFileUpdateHandle_t, ERemoteStoragePublishedFileVisibility){ NYI("ISteamRemoteStorage::UpdatePublishedFileVisibility()\n"); }
    virtual bool UpdatePublishedFileTags(PublishedFileUpdateHandle_t, SteamParamStringArray_t*){ NYI("ISteamRemoteStorage::UpdatePublishedFileTags()\n"); }
    virtual SteamAPICall_t CommitPublishedFileUpdate(PublishedFileUpdateHandle_t){ NYI("ISteamRemoteStorage::CommitPublishedFileUpdate()\n"); }
    virtual SteamAPICall_t GetPublishedFileDetails(PublishedFileId_t, std::uint32_t){ NYI("ISteamRemoteStorage::GetPublishedFileDetails()\n"); }
    virtual SteamAPICall_t DeletePublishedFile(PublishedFileId_t){ NYI("ISteamRemoteStorage::DeletePublishedFile()\n"); }
    virtual SteamAPICall_t EnumerateUserPublishedFiles(std::uint32_t){ NYI("ISteamRemoteStorage::EnumerateUserPublishedFiles()\n"); }
    virtual SteamAPICall_t SubscribePublishedFile(PublishedFileId_t){ NYI("ISteamRemoteStorage::SubscribePublishedFile()\n"); }
    virtual SteamAPICall_t EnumerateUserSubscribedFiles(std::uint32_t){ NYI("ISteamRemoteStorage::EnumerateUserSubscribedFiles()\n"); }
    virtual SteamAPICall_t UnsubscribePublishedFile(PublishedFileId_t){ NYI("ISteamRemoteStorage::UnsubscribePublishedFile()\n"); }
    virtual bool UpdatePublishedFileSetChangeDescription(PublishedFileUpdateHandle_t, const char*){ NYI("ISteamRemoteStorage::UpdatePublishedFileSetChangeDescription()\n"); }
    virtual SteamAPICall_t GetPublishedItemVoteDetails(PublishedFileId_t){ NYI("ISteamRemoteStorage::GetPublishedItemVoteDetails()\n"); }
    virtual SteamAPICall_t UpdateUserPublishedItemVote(PublishedFileId_t, bool){ NYI("ISteamRemoteStorage::UpdateUserPublishedItemVote()\n"); }
    virtual SteamAPICall_t GetUserPublishedItemVoteDetails(PublishedFileId_t){ NYI("ISteamRemoteStorage::GetUserPublishedItemVoteDetails()\n"); }
    virtual SteamAPICall_t EnumerateUserSharedWorkshopFiles(CSteamID, std::uint32_t, SteamParamStringArray_t*, SteamParamStringArray_t*){ NYI("ISteamRemoteStorage::EnumerateUserSharedWorkshopFiles()\n"); }
    virtual SteamAPICall_t SetUserPublishedFileAction(PublishedFileId_t, EWorkshopFileAction){ NYI("ISteamRemoteStorage::SetUserPublishedFileAction()\n"); }
    virtual SteamAPICall_t EnumeratePublishedFilesByUserAction(EWorkshopFileAction, std::uint32_t){ NYI("ISteamRemoteStorage::EnumeratePublishedFilesByUserAction()\n"); }
    virtual SteamAPICall_t EnumeratePublishedWorkshopFiles(EWorkshopEnumerationType, std::uint32_t, std::uint32_t, std::uint32_t, SteamParamStringArray_t*, SteamParamStringArray_t*){ NYI("ISteamRemoteStorage::EnumeratePublishedWorkshopFiles()\n"); }
    virtual SteamAPICall_t UGCDownloadToLocation(UGCHandle_t, const char*, std::uint32_t){ NYI("ISteamRemoteStorage::UGCDownloadToLocation()\n"); }
};

struct ISteamUser {
    virtual HSteamUser GetHSteamUser(){ NYI("ISteamUser::GetHSteamUser()\n"); }

    virtual bool BLoggedOn(){
        // DLOG("ISteamUser::BLoggedOn()\n");
        return true;
    }

    virtual CSteamID GetSteamID(){
        DLOG("ISteamUser::GetSteamID()\n");
        return {1337};
    }

    virtual int InitiateGameConnection(void*, int, CSteamID, std::uint32_t, std::uint16_t, bool){ NYI("ISteamUser::InitiateGameConnection()\n"); }
    virtual void TerminateGameConnection(std::uint32_t, std::uint16_t){ NYI("ISteamUser::TerminateGameConnection()\n"); }
    virtual void TrackAppUsageEvent(CGameID, int, const char*){ NYI("ISteamUser::TrackAppUsageEvent()\n"); }

    virtual bool GetUserDataFolder(char* Buffer, int Size){
        auto r = std::snprintf(Buffer, static_cast<std::size_t>(Size), "%ls",
            CmdArgs.Userdata.c_str());
        DLOG("ISteamUser::GetUserDataFolder(0x%p \"%.*s\", %d)\n", Buffer, Size, Buffer, Size);
        return 0 <= r && r < Size-1;
    }

    virtual void StartVoiceRecording(){ NYI("ISteamUser::StartVoiceRecording()\n"); }
    virtual void StopVoiceRecording(){ NYI("ISteamUser::StopVoiceRecording()\n"); }
    virtual EVoiceResult GetAvailableVoice(std::uint32_t*, std::uint32_t*, std::uint32_t){ NYI("ISteamUser::GetAvailableVoice()\n"); }
    virtual EVoiceResult GetVoice(bool, void*, std::uint32_t, std::uint32_t*, bool, void*, std::uint32_t, std::uint32_t*, std::uint32_t){ NYI("ISteamUser::GetVoice()\n"); }
    virtual EVoiceResult DecompressVoice(const void*, std::uint32_t, void*, std::uint32_t, std::uint32_t*, std::uint32_t){ NYI("ISteamUser::DecompressVoice()\n"); }
    virtual std::uint32_t GetVoiceOptimalSampleRate(){ NYI("ISteamUser::GetVoiceOptimalSampleRate()\n"); }
    virtual HAuthTicket GetAuthSessionTicket(void*, int, std::uint32_t*){ NYI("ISteamUser::GetAuthSessionTicket()\n"); }
    virtual EBeginAuthSessionResult BeginAuthSession(const void*, int, CSteamID){ NYI("ISteamUser::BeginAuthSession()\n"); }
    virtual void EndAuthSession(CSteamID){ NYI("ISteamUser::EndAuthSession()\n"); }
    virtual void CancelAuthTicket(HAuthTicket){ NYI("ISteamUser::CancelAuthTicket()\n"); }
    virtual EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID, AppId_t){ NYI("ISteamUser::UserHasLicenseForApp()\n"); }
    virtual bool BIsBehindNAT(){ NYI("ISteamUser::BIsBehindNAT()\n"); }
    virtual void AdvertiseGame(CSteamID, std::uint32_t, std::uint16_t){ NYI("ISteamUser::AdvertiseGame()\n"); }
    virtual SteamAPICall_t RequestEncryptedAppTicket(void*, int){ NYI("ISteamUser::RequestEncryptedAppTicket()\n"); }
    virtual bool GetEncryptedAppTicket(void*, int, std::uint32_t*){ NYI("ISteamUser::GetEncryptedAppTicket()\n"); }
    virtual int GetGameBadgeLevel(int, bool){ NYI("ISteamUser::GetGameBadgeLevel()\n"); }
    virtual int GetPlayerSteamLevel(){ NYI("ISteamUser::GetPlayerSteamLevel()\n"); }
};

struct ISteamUserStats {
    struct leaderboard_entry {
        CSteamID SteamIdUser;
        std::int32_t GlobalRank;
        std::int32_t Score;
        std::int32_t DetailCount;
        std::int32_t Details[10];
    };

    struct leaderboard {
        const char* Name;
        const leaderboard_entry* Entries;
        int EntryCount;
    };

    static constexpr leaderboard_entry LeaderboardEntries[] = {
        {{ 1},  1, 100000, 0, {}},
        {{ 2},  2,  95000, 0, {}},
        {{ 3},  3,  90000, 0, {}},
        {{ 4},  4,  85000, 0, {}},
        {{ 5},  5,  80000, 0, {}},
        {{ 6},  6,  75000, 0, {}},
        {{ 7},  7,  70000, 0, {}},
        {{ 8},  8,  65000, 0, {}},
        {{ 9},  9,  60000, 0, {}},
        {{10}, 10,  55000, 0, {}},
        {{11}, 11,  50000, 0, {}},
        {{12}, 12,  45000, 0, {}},
        {{13}, 13,  40000, 0, {}},
        {{14}, 14,  35000, 0, {}},
        {{15}, 15,  30000, 0, {}},
        {{16}, 16,  25000, 0, {}},
        {{17}, 17,  20000, 0, {}},
        {{18}, 18,  15000, 0, {}},
        {{19}, 19,  10000, 0, {}},
    };

    static constexpr leaderboard Leaderboards[] = {
        {
            .Name = "NORMAL_CHALLENGE0",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE1",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE2",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE3",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE4",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE5",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE6",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE7",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE8",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "NORMAL_CHALLENGE9",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE0",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE1",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE2",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE3",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE4",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE5",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE6",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE7",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE8",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
        {
            .Name = "EXTREM_CHALLENGE9",
            .Entries = LeaderboardEntries,
            .EntryCount = std::size(LeaderboardEntries),
        },
    };

    virtual bool RequestCurrentStats(){
        DLOG("ISteamUserStats::RequestCurrentStats()\n");
        return true;
    }

    virtual bool GetStatF(const char*, float*){ NYI("ISteamUserStats::GetStatF()\n"); }
    virtual bool GetStatI(const char*, std::int32_t*){ NYI("ISteamUserStats::GetStatI()\n"); }
    virtual bool SetStatF(const char*, float){ NYI("ISteamUserStats::SetStatF()\n"); }
    virtual bool SetStatI(const char*, std::int32_t){ NYI("ISteamUserStats::SetStatI()\n"); }
    virtual bool UpdateAvgRateStat(const char*, float, double){ NYI("ISteamUserStats::UpdateAvgRateStat()\n"); }

    virtual bool GetAchievement(const char* Name, bool* r){
        DLOG("ISteamUserStats::GetAchievement(\"%s\", 0x%p)\n", Name, r);
        *r = false;
        return true;
    }

    virtual bool SetAchievement(const char* Name){
        DLOG("ISteamUserStats::SetAchievement(\"%s\")\n", Name);
        return true;
    }

    virtual bool ClearAchievement(const char* Name){
        DLOG("ISteamUserStats::ClearAchievement(\"%s\")\n", Name);
        return true;
    }

    virtual bool GetAchievementAndUnlockTime(const char*, bool*, std::uint32_t*){ NYI("ISteamUserStats::GetAchievementAndUnlockTime()\n"); }

    virtual bool StoreStats(){
        DLOG("ISteamUserStats::StoreStats()\n");
        return true;
    }

    virtual int GetAchievementIcon(const char*){ NYI("ISteamUserStats::GetAchievementIcon()\n"); }
    virtual const char* GetAchievementDisplayAttribute(const char*, const char*){ NYI("ISteamUserStats::GetAchievementDisplayAttribute()\n"); }
    virtual bool IndicateAchievementProgress(const char*, std::uint32_t, std::uint32_t){ NYI("ISteamUserStats::IndicateAchievementProgress()\n"); }
    virtual SteamAPICall_t RequestUserStats(CSteamID){ NYI("ISteamUserStats::RequestUserStats()\n"); }
    virtual bool GetUserStatF(CSteamID, const char*, float*){ NYI("ISteamUserStats::GetUserStatF()\n"); }
    virtual bool GetUserStatI(CSteamID, const char*, std::int32_t*){ NYI("ISteamUserStats::GetUserStatI()\n"); }
    virtual bool GetUserAchievement(CSteamID, const char*, bool*){ NYI("ISteamUserStats::GetUserAchievement()\n"); }
    virtual bool GetUserAchievementAndUnlockTime(CSteamID, const char*, bool*, std::uint32_t*){ NYI("ISteamUserStats::GetUserAchievementAndUnlockTime()\n"); }
    virtual bool ResetAllStats(bool){ NYI("ISteamUserStats::ResetAllStats()\n"); }
    virtual SteamAPICall_t FindOrCreateLeaderboard(const char*, ELeaderboardSortMethod, ELeaderboardDisplayType){ NYI("ISteamUserStats::FindOrCreateLeaderboard()\n"); }

    virtual SteamAPICall_t FindLeaderboard(const char* Name){
        LeaderboardFindResult_t Result = {
            .Leaderboard = 0,
            .Found = false,
        };

        for(std::size_t i = 0; auto& Leaderboard:Leaderboards){
            if(std::strcmp(Leaderboard.Name, Name) == 0){
                Result.Leaderboard = i;
                Result.Found = true;
                break;
            }

            ++i;
        }

        auto r = add_call_result(Result);

        DLOG("ISteamUserStats::FindLeaderboard(\"%s\"): %llu\n", Name, r);

        return r;
    }

    virtual const char* GetLeaderboardName(SteamLeaderboard_t){ NYI("ISteamUserStats::GetLeaderboardName()\n"); }

    virtual int GetLeaderboardEntryCount(SteamLeaderboard_t Index){
        int r = 0;
        if(Index < std::size(Leaderboards)){
            auto& Leaderboard = Leaderboards[Index];
            r = Leaderboard.EntryCount;
        }

        DLOG("ISteamUserStats::GetLeaderboardEntryCount(%llu): %d\n", Index, r);

        return r;
    }

    virtual ELeaderboardSortMethod GetLeaderboardSortMethod(SteamLeaderboard_t){ NYI("ISteamUserStats::GetLeaderboardSortMethod()\n"); }
    virtual ELeaderboardDisplayType GetLeaderboardDisplayType(SteamLeaderboard_t){ NYI("ISteamUserStats::GetLeaderboardDisplayType()\n"); }

    virtual SteamAPICall_t DownloadLeaderboardEntries(
        SteamLeaderboard_t Index, ELeaderboardDataRequest Request, int Start, int End
    ){
        assert(Index < std::size(Leaderboards));

        auto& Leaderboard = Leaderboards[Index];

        int p0 = 0;
        int p1 = Leaderboard.EntryCount;

        int Self = 5;

        switch(Request){
            case ELeaderboardDataRequest::Global: {
                p0 = std::clamp(Start-1, p0, p1);
                p1 = std::clamp(End, p0, p1);
                break;
            }
            case ELeaderboardDataRequest::GlobalAroundUser: {
                p0 = std::clamp(Start+Self-1, p0, p1);
                p1 = std::clamp(End+Self, p0, p1);
                break;
            }
            case ELeaderboardDataRequest::Friends:
            case ELeaderboardDataRequest::Users: break;
        }

        LeaderboardScoresDownloaded_t Result = {
            .Leaderboard = Index,
            .Entries = reinterpret_cast<SteamLeaderboardEntries_t>(Leaderboard.Entries+p0),
            .EntryCount = p1-p0,
        };

        auto r = add_call_result(Result);

        DLOG("ISteamUserStats::DownloadLeaderboardEntries(%llu, %d, %d, %d): %llu\n",
            Index, Request, Start, End, r);

        return r;
    }

    virtual SteamAPICall_t DownloadLeaderboardEntriesForUsers(SteamLeaderboard_t, CSteamID*, int){ NYI("ISteamUserStats::DownloadLeaderboardEntriesForUsers()\n"); }

    virtual bool GetDownloadedLeaderboardEntry(SteamLeaderboardEntries_t Entries, int Index, LeaderboardEntry_t* LeaderboardEntry, std::int32_t* Details, int DetailCount){
        auto& Entry = reinterpret_cast<const leaderboard_entry*>(Entries)[Index];

        LeaderboardEntry->SteamIdUser = Entry.SteamIdUser;
        LeaderboardEntry->GlobalRank = Entry.GlobalRank;
        LeaderboardEntry->Score = Entry.Score;
        LeaderboardEntry->Details = Entry.DetailCount;

        DLOG("ISteamUserStats::GetDownloadedLeaderboardEntry(0x%llX, %d, 0x%p, 0x%p, %d)\n", Entries, Index, LeaderboardEntry, Details, DetailCount);

        for(int i = 0, End = std::min(Entry.DetailCount, DetailCount); i < End; ++i){
            Details[i] = Entry.Details[i];
        }

        return true;
    }

    virtual SteamAPICall_t UploadLeaderboardScore(SteamLeaderboard_t Index, ELeaderboardUploadScoreMethod Method, std::int32_t Score, const std::int32_t* Details, int DetailCount){
        LeaderboardScoreUploaded_t Result = {
            .Success = true,
            .SteamLeaderboard = Index,
            .Score = Score,
            .ScoreChanged = true,
            .GlobalRankNew = 5,
            .GlobalRankPrevious = 5,
        };

        auto r = add_call_result(Result);

        DLOG("ISteamUserStats::UploadLeaderboardScore(%llu, %u, %d, 0x%p, %d): %llu\n", Index, Method, Score, Details, DetailCount, r);

        return r;
    }

    virtual SteamAPICall_t AttachLeaderboardUGC(SteamLeaderboard_t, UGCHandle_t){ NYI("ISteamUserStats::AttachLeaderboardUGC()\n"); }
    virtual SteamAPICall_t GetNumberOfCurrentPlayers(){ NYI("ISteamUserStats::GetNumberOfCurrentPlayers()\n"); }
    virtual SteamAPICall_t RequestGlobalAchievementPercentages(){ NYI("ISteamUserStats::RequestGlobalAchievementPercentages()\n"); }
    virtual int GetMostAchievedAchievementInfo(char*, std::uint32_t, float*, bool*){ NYI("ISteamUserStats::GetMostAchievedAchievementInfo()\n"); }
    virtual int GetNextMostAchievedAchievementInfo(int, char*, std::uint32_t, float*, bool*){ NYI("ISteamUserStats::GetNextMostAchievedAchievementInfo()\n"); }
    virtual bool GetAchievementAchievedPercent(const char*, float*){ NYI("ISteamUserStats::GetAchievementAchievedPercent()\n"); }
    virtual SteamAPICall_t RequestGlobalStats(int){ NYI("ISteamUserStats::RequestGlobalStats()\n"); }
    virtual bool GetGlobalStatD(const char*, double*){ NYI("ISteamUserStats::GetGlobalStatD()\n"); }
    virtual bool GetGlobalStatI(const char*, std::int64_t*){ NYI("ISteamUserStats::GetGlobalStatI()\n"); }
    virtual std::int32_t GetGlobalStatHistoryU(const char*, double*, std::uint32_t){ NYI("ISteamUserStats::GetGlobalStatHistoryU()\n"); }
    virtual std::int32_t GetGlobalStatHistoryI(const char*, std::int64_t*, std::uint32_t){ NYI("ISteamUserStats::GetGlobalStatHistoryI()\n"); }
};

struct ISteamUtils {
    virtual std::uint32_t GetSecondsSinceAppActive(){ NYI("ISteamUtils::GetSecondsSinceAppActive()\n"); }
    virtual std::uint32_t GetSecondsSinceComputerActive(){ NYI("ISteamUtils::GetSecondsSinceComputerActive()\n"); }
    virtual EUniverse GetConnectedUniverse(){ NYI("ISteamUtils::GetConnectedUniverse()\n"); }
    virtual std::uint32_t GetServerRealTime(){ NYI("ISteamUtils::GetServerRealTime()\n"); }
    virtual const char* GetIPCountry(){ NYI("ISteamUtils::GetIPCountry()\n"); }
    virtual bool GetImageSize(int, std::uint32_t*, std::uint32_t*){ NYI("ISteamUtils::GetImageSize()\n"); }
    virtual bool GetImageRGBA(int, std::uint8_t*, int){ NYI("ISteamUtils::GetImageRGBA()\n"); }
    virtual bool GetCSERIPPort(std::uint32_t*, std::uint16_t*){ NYI("ISteamUtils::GetCSERIPPort()\n"); }
    virtual std::uint8_t GetCurrentBatteryPower(){ NYI("ISteamUtils::GetCurrentBatteryPower()\n"); }

    virtual std::uint32_t GetAppID(){
        return 205100;
    }

    virtual void SetOverlayNotificationPosition(ENotificationPosition Position){
        DLOG("ISteamUtils::SetOverlayNotificationPosition(%d)\n",  Position);
    }

    virtual bool IsAPICallCompleted(SteamAPICall_t, bool*){ NYI("ISteamUtils::IsAPICallCompleted()\n"); }
    virtual ESteamAPICallFailure GetAPICallFailureReason(SteamAPICall_t){ NYI("ISteamUtils::GetAPICallFailureReason()\n"); }
    virtual bool GetAPICallResult(SteamAPICall_t, void*, int, int, bool*){ NYI("ISteamUtils::GetAPICallResult()\n"); }
    virtual void RunFrame(){ NYI("ISteamUtils::RunFrame()\n"); }
    virtual std::uint32_t GetIPCCallCount(){ NYI("ISteamUtils::GetIPCCallCount()\n"); }

    virtual void SetWarningMessageHook(SteamAPIWarningMessageHook_t Hook){
        DLOG("ISteamUtils::SetWarningMessageHook(0x%p)\n",  Hook);
    }

    virtual bool IsOverlayEnabled(){ NYI("ISteamUtils::IsOverlayEnabled()\n"); }
    virtual bool BOverlayNeedsPresent(){ NYI("ISteamUtils::BOverlayNeedsPresent()\n"); }
    virtual SteamAPICall_t CheckFileSignature(const char*){ NYI("ISteamUtils::CheckFileSignature()\n"); }
    virtual bool ShowGamepadTextInput(EGamepadTextInputMode, EGamepadTextInputLineMode, const char*, std::uint32_t, const char*){ NYI("ISteamUtils::ShowGamepadTextInput()\n"); }
    virtual std::uint32_t GetEnteredGamepadTextLength(){ NYI("ISteamUtils::GetEnteredGamepadTextLength()\n"); }
    virtual bool GetEnteredGamepadTextInput(char*, std::uint32_t){ NYI("ISteamUtils::GetEnteredGamepadTextInput()\n"); }
    virtual const char* GetSteamUILanguage(){ NYI("ISteamUtils::GetSteamUILanguage()\n"); }
    virtual bool IsSteamRunningInVR(){ NYI("ISteamUtils::IsSteamRunningInVR()\n"); }
};

ISteamApps SteamApps_Instance = {};
ISteamFriends SteamFriends_Instance = {};
ISteamGameServer SteamGameServer_Instance = {};
ISteamMatchmaking SteamMatchmaking_Instance = {};
ISteamMatchmakingServers SteamMatchmakingServers_Instance = {};
ISteamNetworking SteamNetworking_Instance = {};
ISteamRemoteStorage SteamRemoteStorage_Instance = {};
ISteamUser SteamUser_Instance = {};
ISteamUserStats SteamUserStats_Instance = {};
ISteamUtils SteamUtils_Instance = {};

void load_steam_cloud(){
    SteamRemoteStorage_Instance.load_files();
}

bool save_steam_cloud(const wchar_t* Name){
    return SteamRemoteStorage_Instance.save_files(Name);
}

bool SteamAPI_Init_Hook(){
    auto r = true;

    DLOG("SteamAPI_Init(): %d\n", r);

    return r;
}

void SteamAPI_RegisterCallResult_Hook(CCallbackBase* Callback, SteamAPICall_t ApiCall){
    DLOG("SteamAPI_RegisterCallResult(0x%p, 0x%016llX)\n", Callback, ApiCall);

    std::lock_guard Lock(CallbackMutex);

    CallResultEntries.push_back({
        .Callback = Callback,
        .ApiCall = ApiCall,
    });
}

void SteamAPI_RegisterCallback_Hook(CCallbackBase* Callback, int Index){
    const char* Name = nullptr;
    switch(Index){
        case  101: Name = "SteamServersConnected_t"; break;
        case  103: Name = "SteamServersDisconnected_t"; break;
        case  206: Name = "GSClientAchievementStatus_t"; break;
        case  331: Name = "GameOverlayActivated_t"; break;
        case  332: Name = "GameServerChangeRequested_t"; break;
        case  334: Name = "AvatarImageLoaded_t"; break;
        case  337: Name = "GameRichPresenceJoinRequested_t"; break;
        case  704: Name = "SteamShutdown_t"; break;
        case 1005: Name = "DlcInstalled_t"; break;
    }

    if(Name){
        DLOG("SteamAPI_RegisterCallback(0x%p, %s)\n", Callback, Name);
    }else{
        DLOG("SteamAPI_RegisterCallback(0x%p, %d)\n", Callback, Index);
    }
}

void SteamAPI_RunCallbacks_Hook(){
    std::lock_guard Lock(CallbackMutex);

    // `CallResultEntries` can change as a result of a callback.
    // Hance, we need to use indices and `.size()`.
    for(std::size_t i = 0; i < CallResultEntries.size(); ++i){
        auto& Entry = CallResultEntries[i];
        auto it = CallResults.find(Entry.ApiCall);
        assert(it != CallResults.end());

        auto& [Buffer, IoFailure] = it->second;

        Entry.Callback->Run(Buffer.data(), IoFailure, Entry.ApiCall);

        CallResults.erase(it);
    }

    CallResultEntries.clear();
}

void SteamAPI_Shutdown_Hook(){
    DLOG("SteamAPI_Shutdown()\n");
}

void SteamAPI_UnregisterCallResult_Hook(CCallbackBase* Callback, SteamAPICall_t ApiCall){
    NYI("SteamAPI_UnregisterCallResult(0x%p, 0x%016llX)\n", Callback, ApiCall);
}

void SteamAPI_UnregisterCallback_Hook(CCallbackBase* Callback){
    NYI("SteamAPI_UnregisterCallback(0x%p)\n", Callback);
}

ISteamApps* SteamApps_Hook(){
    DLOG("SteamApps()\n");
    return &SteamApps_Instance;
}

ISteamFriends* SteamFriends_Hook(){
    DLOG("SteamFriends()\n");
    return &SteamFriends_Instance;
}

ISteamGameServer* SteamGameServer_Hook(){
    DLOG("SteamGameServer()\n");
    return &SteamGameServer_Instance;
}

void SteamGameServer_Shutdown_Hook(){
    DLOG("SteamGameServer_Shutdown()\n");
}

ISteamMatchmaking* SteamMatchmaking_Hook(){
    DLOG("SteamMatchmaking()\n");
    return &SteamMatchmaking_Instance;
}

ISteamMatchmakingServers* SteamMatchmakingServers_Hook(){
    DLOG("SteamMatchmakingServers()\n");
    return &SteamMatchmakingServers_Instance;
}

ISteamNetworking* SteamNetworking_Hook(){
    DLOG("SteamNetworking()\n");
    return &SteamNetworking_Instance;
}

ISteamRemoteStorage* SteamRemoteStorage_Hook(){
    DLOG("SteamRemoteStorage()\n");
    return &SteamRemoteStorage_Instance;
}

ISteamUser* SteamUser_Hook(){
    DLOG("SteamUser()\n");
    return &SteamUser_Instance;
}

ISteamUserStats* SteamUserStats_Hook(){
    DLOG("SteamUserStats()\n");
    return &SteamUserStats_Instance;
}

ISteamUtils* SteamUtils_Hook(){
    DLOG("SteamUtils()\n");
    return &SteamUtils_Instance;
}
