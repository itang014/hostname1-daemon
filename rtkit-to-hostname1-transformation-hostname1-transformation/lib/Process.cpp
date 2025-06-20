/*
 * Copyright (c) 2025 Gleb Popov <arrowd@FreeBSD.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Process.h"
#include "OSDep.h"

Process::Process(qulonglong process)
    : m_process(process)
{
    OSDep::ResolvePID(m_process, &m_user, &m_startTime);
}

Process::Process(pid_t process, uid_t user, qulonglong startTime)
    : m_process(process), m_user(user), m_startTime(startTime)
{}

void Process::ForEach(const std::function<void (Process*)>& f)
{
    OSDep::ForEachProcess([f](auto pid, auto uid, auto startTime) {
        Process p(pid, uid, startTime);
        f(&p);
    });
}

bool Process::IsValid() const
{
    if (m_user == -1u || m_startTime == 0)
        return false;
    return *this == Process(m_process);
}

bool Process::HasNonStandardSchedulingPolicy() const
{
    return OSDep::PIDHasNonStandardSchedulingPolicy(m_process);
}

bool Process::ContainsThread(qulonglong thread) const
{
    return OSDep::PIDContainsTID(m_process, thread);
}

bool Process::SetHighPriority(qulonglong thread, int priority) const
{
    return OSDep::SetHighPriority(m_process, thread, priority);
}

bool Process::SetRealtimePriority(qulonglong thread, uint priority) const
{
    return OSDep::SetRealtimePriority(m_process, thread, priority);
}

bool Process::SetIdlePriority(qulonglong thread, uint priority) const
{
    return OSDep::SetIdlePriority(m_process, thread, priority);
}

bool Process::ResetAllPriorities(qulonglong thread) const
{
    return OSDep::ResetAllPriorities(m_process, thread);
}
