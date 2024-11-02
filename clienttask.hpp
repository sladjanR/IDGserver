#ifndef CLIENTTASK_HPP
#define CLIENTTASK_HPP

#include <QRunnable>
#include <QThreadPool>

class QTcpSocket;   // Forward deklaracija

class ClientTask : public QRunnable
{
private:
    qintptr socketDescriptor;

public:
    ClientTask(qintptr socketDescriptor);

    // QRunnable interface
public:
    void run() override;



private:
    void handleRequest(QJsonObject& obj, QTcpSocket &socket);
    void createRepository(const QString &repositoryName);
    void storeBlob(const QString &repositoryName, const QString &blobHash, const QByteArray &content);

    void storeCommit(const QString &repositoryName, const QJsonObject &commitObj);
    void handlePullRepository(const QString &repositoryName, QTcpSocket &clientSocket);
    void handleListRepositories(QTcpSocket &clientSocket);
    void storeTree(const QString &repositoryName, const QString &treeHash, const QJsonObject &treeData);
};

#endif // CLIENTTASK_HPP
