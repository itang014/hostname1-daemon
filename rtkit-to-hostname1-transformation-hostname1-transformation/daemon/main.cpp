/*
 * Copyright (c) 2025 FreeBSD Foundation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QCoreApplication>

#include "Hostnamed.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Hostnamed daemon(QDBusConnection::systemBus());

    if(!daemon.Start())
        return -1;

    return app.exec();
}