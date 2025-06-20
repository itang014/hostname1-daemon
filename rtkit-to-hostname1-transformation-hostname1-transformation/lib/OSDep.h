/*
 * Copyright (c) 2025 Gleb Popov <arrowd@FreeBSD.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <optional>
#include <functional>

#include <sys/types.h>

#include <qtypes.h>

namespace OSDep
{

bool Init();
void Fini();

void ForEachProcess(const std::function<void(pid_t, uid_t, qulonglong)>& f);
std::optional<uid_t> GetUIDForPID(pid_t process);
bool PIDContainsTID(pid_t process, qulonglong thread);
bool PIDHasNonStandardSchedulingPolicy(pid_t process);
void ResolvePID(pid_t process, uid_t* userOut, qulonglong* startTimeOut);
bool SetHighPriority(pid_t process, qulonglong thread, int priority);
bool SetRealtimePriority(pid_t process, qulonglong thread, uint priority);
bool SetIdlePriority(pid_t process, qulonglong thread, uint priority);
bool ResetAllPriorities(pid_t process, qulonglong thread);

}
