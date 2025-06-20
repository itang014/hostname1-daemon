/*
 * Copyright (c) 2025 Gleb Popov <arrowd@FreeBSD.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QtGlobal>

#include <limits.h>
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/thr.h>
#include <sys/rtprio.h>

#include "OSDep.h"

static kvm_t* KVM = nullptr;
static char KVMErrorBuf[_POSIX2_LINE_MAX];
static bool Entered = false;

namespace OSDep
{

bool Init()
{
    if (KVM)
        Fini();

    KVM = kvm_openfiles(nullptr, "/dev/null", nullptr, O_RDONLY, KVMErrorBuf);

    return KVM != nullptr;
}
void Fini()
{
    if (KVM) {
        kvm_close(KVM);
        KVM = nullptr;
    }
}

void ForEachProcess(const std::function<void(pid_t, uid_t, qulonglong)>& f)
{
    Q_ASSERT(!Entered);
    Entered = true;

    int count = 0;
    struct kinfo_proc *kinfo = kvm_getprocs(KVM, KERN_PROC_PROC, 0, &count);
    for (int i = 0; i < count; i++)
        f(kinfo->ki_pid, kinfo->ki_uid, kinfo->ki_start.tv_sec);

    Entered = false;
}

std::optional<uid_t> GetUIDForPID(pid_t process)
{
    Q_ASSERT(!Entered);
    Entered = true;

    int count = 0;
    struct kinfo_proc *kinfo = kvm_getprocs(KVM, KERN_PROC_PID, static_cast<pid_t>(process), &count);

    Entered = false;

    if (count < 1)
        return {};
    return kinfo->ki_uid;
}

bool PIDContainsTID(pid_t process, qulonglong thread)
{
    return thr_kill2(process, static_cast<long>(thread), 0) == 0;
}

bool PIDHasNonStandardSchedulingPolicy(pid_t process)
{
    int policy = sched_getscheduler(process);
    return policy != SCHED_OTHER;
}

void ResolvePID(pid_t process, uid_t* userOut, qulonglong* startTimeOut)
{
    Q_ASSERT(!Entered);
    Entered = true;

    int count = 0;
    struct kinfo_proc *kinfo = kvm_getprocs(KVM, KERN_PROC_PID, static_cast<pid_t>(process), &count);

    Entered = false;

    if (count < 1)
        return;

    *userOut = kinfo->ki_uid;
    *startTimeOut = kinfo->ki_start.tv_sec;
}

bool SetHighPriority(pid_t process, qulonglong thread, int priority)
{
    struct rtprio rtp;
    rtp.prio = 0;
    rtp.type = RTP_PRIO_NORMAL;
    bool ret = rtprio_thread(RTP_SET, static_cast<lwpid_t>(thread), &rtp) == 0;

    ret &= setpriority(PRIO_PROCESS, static_cast<pid_t>(process), priority) == 0;
    return ret;
}

bool SetRealtimePriority(pid_t process, qulonglong thread, uint priority)
{
    Q_UNUSED(process);

    struct rtprio rtp;
    rtp.prio = priority;
    rtp.type = RTP_PRIO_REALTIME;

    return rtprio_thread(RTP_SET, static_cast<lwpid_t>(thread), &rtp) == 0;
}

bool SetIdlePriority(pid_t process, qulonglong thread, uint priority)
{
    Q_UNUSED(process);

    struct rtprio rtp;
    rtp.prio = priority;
    rtp.type = RTP_PRIO_IDLE;

    return rtprio_thread(RTP_SET, static_cast<lwpid_t>(thread), &rtp) == 0;
}

bool ResetAllPriorities(pid_t process, qulonglong thread)
{
    struct rtprio rtp;
    rtp.prio = 0;
    rtp.type = RTP_PRIO_NORMAL;

    if (thread)
        return rtprio_thread(RTP_SET, static_cast<lwpid_t>(thread), &rtp) == 0;
    else
        return rtprio(RTP_SET, static_cast<pid_t>(process), &rtp) == 0;
}

}
