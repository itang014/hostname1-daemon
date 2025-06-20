/*
 * Copyright (c) 2025 Gleb Popov <arrowd@FreeBSD.org>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QCoreApplication>
#include <QDateTime>

#include <AuthQueue>
#include <DBusSavedContext>

#include "Daemon.h"
#include "Process.h"
#include "OSDep.h"

#include "RealtimeKit1Adaptor.h"

static const QString RTKitService = QStringLiteral("org.freedesktop.RealtimeKit1");
static const QString RTKit1ObjectPath = QStringLiteral("/org/freedesktop/RealtimeKit1");

Daemon::Daemon(const QDBusConnection& bus)
    : m_bus(bus), m_daemonPid(getpid())
{
}

Daemon::~Daemon()
{
    OSDep::Fini();
}

bool Daemon::Start()
{
    new RealtimeKit1Adaptor(this);

    if(!OSDep::Init()) {
        qCritical() << "Could not perform OS-dependend initialization";
        return false;
    }

    if(!m_bus.registerObject(RTKit1ObjectPath, this)) {
        qCritical() << "Could not register" << RTKit1ObjectPath << "object";
        return false;
    }

    if(!m_bus.registerService(RTKitService)) {
        qCritical() << "Could not register" << RTKitService << "service";
        return false;
    }

    return true;
}

void Daemon::Exit()
{
    ResetKnown();
    // StopCanary();

    if (auto* app = QCoreApplication::instance())
        app->quit();
    else
        qWarning() << "No QCoreApplication running, can't Exit()";
}

bool Daemon::SetPriorityAuthorized(const std::shared_ptr<Process>& process,
                                   qulonglong thread,
                                   PriorityType priorityType,
                                   qlonglong priorityValue,
                                   uint callerUid,
                                   const DBusSavedContext* context)
{
    if (!process->IsValid())
        DBUS_THROW_CONTEXT("org.freedesktop.DBus.Error.InvalidArgs", "The requested process was not found");

    if (process->Uid() != callerUid)
        DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.AccessDenied", "The requested process does not belong to you");

    if (!process->ContainsThread(thread))
        DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.InvalidArgs", "The specified thread does not belong to this process");

    switch (priorityType)
    {
    case PriorityType::High:
        if (!process->SetHighPriority(thread, priorityValue))
            DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.Failed", "Failed to set priority");
        break;
    case PriorityType::Realtime:
        if (!process->SetRealtimePriority(thread, priorityValue))
            DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.Failed", "Failed to set priority");
        break;
    case PriorityType::Idle:
        if (!process->SetIdlePriority(thread, priorityValue))
            DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.Failed", "Failed to set priority");
        break;
    default:
        DBUS_THROW_CONTEXT ("org.freedesktop.DBus.Error.Failed", "Unknown priority type");
    }

    if (!process->IsValid()) {
        process->ResetAllPriorities(thread);
        DBUS_THROW_CONTEXT("org.freedesktop.DBus.Error.InvalidArgs", "The requested process was not found");
    }

    m_knownProcesses.push_back(process);

    return true;
}

void Daemon::MakeThreadHighPriority(qulonglong thread, int priority)
{
    auto* context = this;

    auto process = connection().interface()->servicePid(message().service());
    if (!process.isValid())
        DBUS_RETHROW_CONTEXT_VOID(process);

    MakeThreadHighPriorityWithPID(process, thread, priority);
}

void Daemon::MakeThreadHighPriorityWithPID(qulonglong process, qulonglong thread, int priority)
{
    auto* context = this;

    if (!process)
        DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.InvalidArgs", "The requested process was not found");

    auto callerUid = connection().interface()->serviceUid(message().service());
    if (!callerUid.isValid())
        DBUS_RETHROW_CONTEXT_VOID(callerUid);

    if (!checkBursting(callerUid))
        DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.AccessDenied", "You are calling too often");

    garbageCollect();

    std::shared_ptr<Process> proc = std::make_shared<Process>(process);

    AuthQueue::getInstance()->Enqueue(QStringLiteral("org.freedesktop.RealtimeKit1.acquire-high-priority"),
                                      context, {},
                                      [=, callerUid=callerUid.value(), this](auto r, auto* context)
    {
        if (r != PolkitQt1::Authority::Result::Yes)
            DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.AccessDenied", "You are not allowed to set high priority");

        if (!SetPriorityAuthorized(proc, thread, PriorityType::High, priority, callerUid, context))
            return; // error already reported

        context->sendReply();
    });
}

void Daemon::MakeThreadRealtime(qulonglong thread, uint priority)
{
    auto* context = this;

    auto callerUid = connection().interface()->serviceUid(message().service());
    if (!callerUid.isValid())
        DBUS_RETHROW_CONTEXT_VOID(callerUid);

    auto process = connection().interface()->servicePid(message().service());
    if (!process.isValid())
        DBUS_RETHROW_CONTEXT_VOID(process);

    MakeThreadRealtimeWithPID(process, thread, priority);
}

void Daemon::MakeThreadRealtimeWithPID(qulonglong process, qulonglong thread, uint priority)
{
    auto* context = this;

    if (!process)
        DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.InvalidArgs", "The requested process was not found");

    auto callerUid = connection().interface()->serviceUid(message().service());
    if (!callerUid.isValid())
        DBUS_RETHROW_CONTEXT_VOID(callerUid);

    if (!checkBursting(callerUid))
        DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.AccessDenied", "You are calling too often");

    garbageCollect();

    std::shared_ptr<Process> proc = std::make_shared<Process>(process);

    AuthQueue::getInstance()->Enqueue(QStringLiteral("org.freedesktop.RealtimeKit1.acquire-real-time"),
                                      context, {},
                                      [=, callerUid=callerUid.value(), this](auto r, auto* context)
    {
        if (r != PolkitQt1::Authority::Result::Yes)
            DBUS_THROW_CONTEXT_VOID("org.freedesktop.DBus.Error.AccessDenied", "You are not allowed to set realtime priority");

        if (!SetPriorityAuthorized(proc, thread, PriorityType::Realtime, priority, callerUid, context))
            return; // error already reported

        context->sendReply();
    });
}

void Daemon::ResetAll()
{
    Process::ForEach([this](Process* proc) {
        if (proc->Pid() == m_daemonPid)
            return;

        if (proc->Uid() == 0)
            return;

        if (!proc->HasNonStandardSchedulingPolicy())
            return;

        proc->ResetAllPriorities(0);
    });

    garbageCollect();
}

void Daemon::ResetKnown()
{
    for (const auto& proc : m_knownProcesses)
        if (proc->IsValid())
            proc->ResetAllPriorities(0);

    garbageCollect();
}

int Daemon::MaxRealtimePriority() const
{
    return sched_get_priority_max(SCHED_RR);
}

int Daemon::MinNiceLevel() const
{
    return -15; // rtkit default
}

qlonglong Daemon::RTTimeUSecMax() const
{
    return 0;
}

void Daemon::garbageCollect()
{
    erase_if(m_knownProcesses, [](const auto& proc) {
        return !proc->IsValid() || !proc->HasNonStandardSchedulingPolicy();
    });
}

bool Daemon::checkBursting(uint userId)
{
    // rtkit defaults
    const auto burstInterval = 20;
    const auto maxActionsPerBurst = 25;

    auto& bi = m_burstInfos[userId];
    auto now = QDateTime::currentDateTime().toSecsSinceEpoch();

    if (now > bi.second + burstInterval) {
        bi.first = 0;
        bi.second = now;
        return true;
    }

    if (bi.first >= maxActionsPerBurst)
        return false;

    bi.first++;

    return true;
}
