#include "clienttask.hpp"

// Qt
#include <QTcpSocket>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTcpSocket>

ClientTask::ClientTask(qintptr socketDescriptor) :
    socketDescriptor{socketDescriptor}
{}

void ClientTask::run()
{
    QTcpSocket clientSocket;
    // socketDescriptor - otvorena mrezna veza na Sistemskom nivou
    // Sada povezujemo QTcpSocket sa socketDescriptorom - rukujemo sada sa mreznom vezom koja je vec prihvacena (kada  dodelimo clientSocket-u mozemo da kontrolisemo istu)
    // Dobra je praksa samo to dodeljivanje staviti u if(){} jer se moze desiti da je vec nesto drugo preuzelo tu vezu
    clientSocket.setSocketDescriptor(socketDescriptor);

    // Blokiramo thread u kome se trenutno nalazimo i cekamo da nam stigne podatak koji je SPREMAN za citanje
    // Mozemo staviti kao argument timer koji otkucava...
    if (clientSocket.waitForReadyRead())
    {
        QByteArray data = clientSocket.readAll();
        QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
        QJsonObject obj = jsonDocument.object();

        handleRequest(obj, clientSocket);

        // Potrebno je i odgovoriti korisniku da je sve uspesno odradjeno
        clientSocket.write("Request handled successfully");
        clientSocket.waitForBytesWritten();
    }

    // Konekcija je samo tu kada se prenose podaci, u suprotnom se ona zatvara
    clientSocket.disconnectFromHost();
}

// Metoda za pracenje akcija, vrsimo izvrsavanje odredjenih operacija za poslatu akciju od strane servera
void ClientTask::handleRequest(QJsonObject& obj, QTcpSocket &socket)
{
    QString action = obj["action"].toString();
    // U slucaju da je potrebno sacuvati blob (Byte Large Object) na nas server, uzimamo potrebne infocmacije iz JSON-a
    if (action == "storeBlob")
    {
        QString repositoryName = obj["repositoryName"].toString();
        QString blobHash = obj["blobHash"].toString();
        // Iz 64 baze (string utf8) dekodira ga nazad u originalan binarni format koje mozemo dalje obradjivati
        QByteArray content = QByteArray::fromBase64(obj["content"].toString().toUtf8());
        // Pozivamo metodu koja upisuje na nas server blob na odredjenu lokaciju (repositoryname) i sa sadrzajem koji smo poslali kao argument
        storeBlob(repositoryName, blobHash, content);
    }
    else if (action == "storeTree")
    {
        // Klijent nam salje stablo koje ce u sebi imati svoje blob-ove i druga stabla
        QString repositoryName = obj["repositoryName"].toString();
        QString treeHash = obj["treeHash"].toString();
        QJsonObject treeData = obj["treeData"].toObject();
        storeTree(repositoryName, treeHash, treeData);

    }
    else if (action == "commit")
    {
        QString repositoryName = obj["repositoryName"].toString();
        // Obratiti paznju to kada uzimamo value koristimo i .toObject() u ovom slucaju
        QJsonObject commitObj = obj["commit"].toObject();
        qDebug() << "Primljeni repozitorijum:" << repositoryName;
        // Slicno kao i kod blob-a cuvamo u nasem repozitorijumu i commit
        qDebug() << "Kod commitTree: " << repositoryName;
        storeCommit(repositoryName, commitObj);
    }
    else if (action == "pullRepository")
    {
        // Za ovaj zahtev potrebno je da korisniku vratimo izgled repozitorijuma na serveru, ceo direktorijum
        QString repositoryName = obj["repositoryName"].toString();
        handlePullRepository(repositoryName, socket);
    }
    else if (action == "listRepositories")
    {
        handleListRepositories(socket);
    }
    else
    {
        qCritical() << "Akcija nije definisana!";
    }
}

// Metoda zaduzena za kreiranje repozitorijuma na strani servera
void ClientTask::createRepository(const QString& repositoryName)
{
    qDebug() << "ime je: " << repositoryName;
    QDir dir("repositories/" + repositoryName);
    if (!dir.exists())
    {
        dir.mkpath(".");
        dir.mkdir(".commits");
        dir.mkdir(".blobs");
        dir.mkdir(".trees");
        qDebug() << "Repozitorijum: " << repositoryName << " je kreiran sa commit, blob, and tree direktorijumima";
    }
}

// Cuvanje blob-a, prosledjenog od strane klijenta
void ClientTask::storeBlob(const QString& repositoryName, const QString& blobHash, const QByteArray& content)
{
    if (!QDir("repositories/" + repositoryName).exists())
    {
        createRepository(repositoryName);
    }

    QString filePath = "repositories/" + repositoryName + "/.blobs/blob_" + blobHash + ".txt";
    QFile blobFile(filePath);

    if (blobFile.open(QIODevice::WriteOnly))
    {
        blobFile.write(content);
        blobFile.close();
        qDebug() << "Blob: " << blobHash << " je sacuvan u " << repositoryName;
    }
    else
    {
        qDebug() << "Nije uspelo cuvanje blob-a: " << blobHash << " u " << repositoryName;
    }
}

// Skladistenje svih stabala koje smo dobili od klijenta
void ClientTask::storeTree(const QString& repositoryName, const QString& treeHash, const QJsonObject& treeData)
{
    if (!QDir("repositories/" + repositoryName).exists())
    {
        createRepository(repositoryName);
    }

    QString filePath = "repositories/" + repositoryName + "/.trees/tree_" + treeHash + ".json";
    QFile treeFile(filePath);

    if (treeFile.open(QIODevice::WriteOnly))
    {
        QJsonDocument treeDoc(treeData);
        QByteArray treeDataBytes = treeDoc.toJson();
        treeFile.write(treeDataBytes);
        treeFile.close();
        qDebug() << "Stablo: " << treeHash << " je sacuvano u " << repositoryName;
    }
    else
    {
        qDebug() << "Nije uspelo cuvanje tree-a: " << treeHash << " u " << repositoryName;
    }
}

void ClientTask::storeCommit(const QString& repositoryName, const QJsonObject& commitObj)
{
    if (!QDir("repositories/" + repositoryName).exists())
    {
        createRepository(repositoryName);
    }

    QString commitHash = commitObj["commitHash"].toString();
    QString commitFileName = "commit_" + commitHash + ".json";
    QString filePath = "repositories/" + repositoryName + "/.commits/" + commitFileName;
    QFile commitFile(filePath);

    if (commitFile.open(QIODevice::WriteOnly))
    {
        QJsonDocument commitDoc(commitObj);
        QByteArray commitData = commitDoc.toJson();
        commitFile.write(commitData);
        commitFile.close();
        qDebug() << "Commit: " << commitHash << " je sacuvan u " << repositoryName;
    }
    else
    {
        qDebug() << "Nije uspelo cuvanje commit-a: " << commitHash << " u " << repositoryName;
    }
}

void ClientTask::handlePullRepository(const QString& repositoryName, QTcpSocket& clientSocket)
{
    QString repoPath = "repositories/" + repositoryName;
    // Potrebno je od direktorijuma napraviti arhivu radi jednostavnijeg i pouzdanijeg slanja celog direktorijuma
    // Qt6 kao ne poseduje trenutno ni jednu standarnu biblioteku (citav dan potrosio na ovo) koja bi omogucila .zip kompresiju
    // Postoje biblioteke trece strane, ali je njihovo koriscenje zavisno od verzije (moguce je da nakon azuriranja verzije trenutna ne funkcionise vise)
    // Korisitmo .tar koja je prisutna kod UNIX, LINUX, ali isto tako moze se koristiti i na Windows masinama
    QString tarFilePath = repoPath + ".tar";

    // Pravimo .tar fajl pomocu QProcess objekta
    QProcess tarProcess;

    // --------------------------------------------------
    // Obratiti paznju da je drugi argument QStringList!:
    // -cvf:
        // c - znaci da ce da se kreira arhiva
        // v - prikazivanje samih detalja o procesu
        // f - zelimo da damo specificno ime datoteke
    // tarFilePath - putanja ka odredisnoj datoteci
    // -C:
        // Menja radni direktorijum za .tar pre nego sto pocne da arhivira
    tarProcess.start("tar", QStringList() << "-cvf" << tarFilePath << "-C" << "repositories" << repositoryName);
    tarProcess.waitForFinished();

    // Postavljanje elemenata tar fajla kao QByteArray
    QFile tarFile(tarFilePath);
    if (tarFile.open(QIODevice::ReadOnly))
    {
        QByteArray tarData = tarFile.readAll();
        tarFile.close();

        // Saljemo klijentu nas fajl
        clientSocket.write(tarData);

        // Obavezno stopirati trenutni thread sve dok se svi podaci ne upisu
        clientSocket.waitForBytesWritten();

        // Sada bi bilo dobro i obrisati privremeni .tar fajl
        QFile::remove(tarFilePath);
    }
    else
    {
        qDebug() << "Nije uspesno kreiranje .tar arhive za odabran repozitorijum:" << repositoryName;
    }
}

void ClientTask::handleListRepositories(QTcpSocket &clientSocket)
{
    QDir reposDir("repositories");
    // Dobija listu svih pod-direktorijuma, ali iskljucuje one nevidiljive, tacnije .dir ili ..dir
    // QDir::Dirs - filtrira tako sto ukljucuje samo direktorijume
    QStringList repos = reposDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QJsonArray reposArray;
    for (const QString& repository : repos)
    {
        reposArray.append(repository);
    }

    QJsonObject responseObj;
    // Pravimo sada mi json Objekat koji ce biti odgovor klijentu
    responseObj["action"] = "listRepositories";
    responseObj["repositories"] = reposArray;

    QJsonDocument responseDoc(responseObj);
    QByteArray responseData = responseDoc.toJson();

    // Sada saljemo celu listu repozitorijuma
    clientSocket.write(responseData);
    // Mozemo staviti cak i u argumentu od waitForBytesWritten koliko zelimo da nas thread ceka da se svi podaci upisu
    clientSocket.waitForBytesWritten();

    qDebug() << "Lista repozitorijuma sa servera poslata je klijentu";
}






