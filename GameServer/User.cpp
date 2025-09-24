#include "pch.h"
#include "User.h"

alignas(64) std::atomic<int> jh_content::User::aliveLobbyUserCount = 0;

