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

#include <QDBusContext>

#include "DBusSavedContext"

DBusSavedContext::DBusSavedContext()
    : m_connection(QDBusConnection::systemBus())
    , m_message()
{
}

DBusSavedContext::DBusSavedContext(const QDBusContext * context)
    : m_connection(context->connection())
    , m_message(context->message())
{
    context->setDelayedReply(true);
}

DBusSavedContext::DBusSavedContext(const DBusSavedContext& context)
    : m_connection(context.m_connection)
    , m_message(context.m_message)
{
}

QDBusConnection DBusSavedContext::connection() const
{
    return m_connection;
}

const QDBusMessage & DBusSavedContext::message() const
{
    return m_message;
}

void DBusSavedContext::sendErrorReply(const QString & name, const QString & msg) const
{
    m_connection.send(m_message.createErrorReply(name, msg));
}

void DBusSavedContext::sendReply() const
{
    m_connection.send(m_message.createReply());
}

void DBusSavedContext::sendReply(const QVariant & arg) const
{
    m_connection.send((m_message.createReply(arg)));
}

void DBusSavedContext::sendReply(const QList<QVariant> & args) const
{
    m_connection.send((m_message.createReply(args)));
}

// no need to do anything here
void DBusSavedContext::setDelayedReply(bool enable) const
{
}
