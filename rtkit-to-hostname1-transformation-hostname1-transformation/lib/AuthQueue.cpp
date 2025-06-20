/*
    Copyright (c) 2023 Serenity Cyber Security, LLC <license@futurecrew.ru>
                       Author: Gleb Popov <arrowd@FreeBSD.org>
    Copyright (c) 2024-2025 Future Crew, LLC <license@futurecrew.ru>
                       Author: Gleb Popov <arrowd@FreeBSD.org>

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors may
    be used to endorse or promote products derived from this software without specific
    prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "AuthQueue"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <utility>

using namespace PolkitQt1;

AuthQueue * AuthQueue::getInstance()
{
    static auto instance = std::unique_ptr<AuthQueue>(new AuthQueue);
    return instance.get();
}

void AuthQueue::Enqueue(const QString & actionId, const QDBusContext * context,
                        OnBeforeContinuationCheck beforeContinuationCheck, Continuation continuation)
{
    if (!context || !context->calledFromDBus())
    {
        Item item{actionId, {}, DBusSavedContext(), std::move(beforeContinuationCheck), std::move(continuation)};
        return callBack(item, PolkitQt1::Authority::Result::No);
    }

    // bypass Polkit completely when asking authorization for root
    auto caller = context->connection().interface()->serviceUid(context->message().service());
    if (caller == 0) {
        Item item{actionId, {}, DBusSavedContext(context), {}, std::move(continuation)};
        return callBack(item, PolkitQt1::Authority::Result::Yes);
    }

    Item item{actionId, {}, DBusSavedContext(context), std::move(beforeContinuationCheck), std::move(continuation)};

    m_items.enqueue(item);
    if (m_items.size() == 1)
        dispatchItem();
}

void AuthQueue::EnqueueWithDetails(const QString & actionId,
                                   const DetailsMap& details,
                                   const QDBusContext * context,
                                   OnBeforeContinuationCheck beforeContinuationCheck,
                                   Continuation continuation)
{
    if (!context || !context->calledFromDBus())
    {
        Item item{actionId, details, DBusSavedContext(), std::move(beforeContinuationCheck), std::move(continuation)};
        return callBack(item, Authority::Result::No);
    }

    // bypass Polkit completely when asking authorization for root
    auto caller = context->connection().interface()->serviceUid(context->message().service());
    if (caller == 0) {
        Item item{actionId, {}, DBusSavedContext(context), {}, std::move(continuation)};
        return callBack(item, PolkitQt1::Authority::Result::Yes);
    }

    Item item{actionId, details, DBusSavedContext(context), std::move(beforeContinuationCheck), std::move(continuation)};

    m_items.enqueue(item);
    if (m_items.size() == 1)
        dispatchItem();
}

void AuthQueue::onCheckAuthorizationFinished(Authority::Result result)
{
    Item item = m_items.dequeue();

    // start next authentication eagerly
    if (!m_items.empty())
        dispatchItem();

    callBack(item, result);
}

void AuthQueue::callBack(const Item& item, Authority::Result result)
{
    if (item.canContinue && !std::invoke(item.canContinue))
    {
        return;
    }

    if (item.continuation)
    {
        std::invoke(item.continuation, result, &item.context);
    }
}

void AuthQueue::dispatchItem()
{
    const Item & item = m_items.head();
    auto subject = SystemBusNameSubject(item.context.message().service());
    Authority::AuthorizationFlags flags = item.context.message().isInteractiveAuthorizationAllowed()
                                                  ? Authority::AllowUserInteraction
                                                  : Authority::None;
    if (item.details.isEmpty())
        m_authority->checkAuthorization(item.actionId, subject, flags);
    else
        m_authority->checkAuthorizationWithDetails(item.actionId, subject, flags, item.details);
}

AuthQueue::AuthQueue()
{
    m_authority = Authority::instance();

    QObject::connect(m_authority, &Authority::checkAuthorizationFinished, [this](auto result) {
        onCheckAuthorizationFinished(result);
    });
}
