#include <QCoreApplication>
#include "server.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Server server;
    server.startMe();

    return a.exec();
}
