#ifndef SERVER_HPP
#define SERVER_HPP

#include <QObject>
#include <QTcpServer>

class Server : public QTcpServer
{
    Q_OBJECT

private:
    quint16 PORT = 9000;

public:
    explicit Server(QObject *parent = nullptr);
    void startMe();


    // QTcpServer interface
protected:
    void incomingConnection(qintptr handle) override;
};

#endif // SERVER_HPP
