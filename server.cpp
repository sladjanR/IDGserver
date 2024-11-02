#include "server.hpp"
#include "clienttask.hpp"

Server::Server(QObject *parent)
    : QTcpServer{parent}
{}

// Pokretanje servera, postavljanje adrese na koju slusa, port
void Server::startMe()
{
    // Soket koji se veze sa ovom adresom moze da slusa i na IPV6 i IPV4 interfejsima
    if (!this->listen(QHostAddress::Any, PORT))
    {
        qDebug() << "Nije uspelo pokretanje servera!";
        return;
    }
    qDebug() << "Server je uspesno pokrenut...";
}

void Server::incomingConnection(qintptr handle)
{
    // Pravimo worker-a kome prosledjujemo kao argument socketDescriptor dobijen od Operativnog Sistema
    ClientTask *task = new ClientTask(handle);
    // Umesto da stavimo sve u neki niz thread-ova, koristicemo thread pool
    // Mogucnosti koje nam donosi jesu: pracenje threadova, njihova grupna kontrola, drzanje na okupu, dodeljivanje nakon slobodnih resursa
    // QThreadPool je u stvari globalna instanca koju mozemo dobiti pomocu staticke metode globalInstance()
    // Nas QRunnable (task) ubacujemo pomocu: start(task)
    QThreadPool::globalInstance()->start(task);
}


