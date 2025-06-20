/*
 * Copyright (c) 2025 Gleb Popov <arrowd@FreeBSD.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <functional>

#include <sys/types.h>

#include <qtypes.h>

class Process
{
public:
    using Ptr = std::shared_ptr<Process>;

    static void ForEach(const std::function<void(Process*)>& f);

    Process(qulonglong process);
    Process(pid_t process, uid_t user, qulonglong startTime);

    bool SetHighPriority(qulonglong thread, int priority) const;
    bool SetRealtimePriority(qulonglong thread, uint priority) const;
    bool SetIdlePriority(qulonglong thread, uint priority) const;
    bool ResetAllPriorities(qulonglong thread) const;

    bool IsValid() const;
    bool HasNonStandardSchedulingPolicy() const;
    bool ContainsThread(qulonglong thread) const;
    pid_t Pid() const { return m_process; }
    uid_t Uid() const { return m_user; }

    auto tie() const
    {
        return std::tie(m_process, m_user, m_startTime);
    }
    bool operator==(const Process& rhs) const
    {
        return tie() == rhs.tie();
    }
private:
    pid_t m_process;
    uid_t m_user = -1u;
    qulonglong m_startTime = 0;
};
