#ifndef BLOB_H
#define BLOB_H

// Qt
#include <QFile>
#include <QDir>
#include <QByteArray>
#include <QString>
#include <QCryptographicHash>

// Staro
class Blob
{
public:
    QByteArray content;
    QString hash;
public:
    Blob() = default;
    // Konstruktor sa argumentima koji cuva blob na disku
    Blob(const QString& filePath, const QString& rootPath)
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray fileContent = file.readAll();
            content = fileContent;
            hash = generateHash(content);
            file.close();

            // Na kraju cuvamo napravljeni blob u memoriju
            saveToDisk(rootPath);
        }
    }

    static Blob fromDisk(const QString& blobHash, const QString& rootPath)
    {
        QString blobFilePath = rootPath + "/.blobs/blob_" + blobHash + ".txt";
        QFile blobFile(blobFilePath);
        Blob blob;
        if (blobFile.open(QIODevice::ReadOnly))
        {
            blob.content = blobFile.readAll();
            blob.hash = blobHash;
            blobFile.close();
        }
        return blob;
    }

    QString getHash() const
    {
        return hash;
    }
    QByteArray getContent() const
    {
        return content;
    }

public:
    // Metoda koja generise hash na osnovu kontenta (od bytearray-a)
    QString generateHash(const QByteArray& data)
    {
        QByteArray hashData = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
        return hashData.toHex();
    }

    // Metoda za cuvanje bloba
    bool saveToDisk(const QString& rootPath) const
    {
        // Proveravamo prvo da li direktorijum postoji
        QString blobDir = rootPath + "/.blobs";

        if (!QDir(blobDir).exists())
        {
            QDir().mkpath(blobDir);
        }

        // Definisemo putanju do naseg blob-a (do fajla)
        QString blobFilePath = blobDir + "/blob_" + hash + ".txt";

        // Upisujemo sadrzaj u blob
        QFile blobFile(blobFilePath);
        if (blobFile.open(QIODevice::WriteOnly))
        {
            blobFile.write(content);
            blobFile.close();
            return true;
        }
        return false;
    }
};


#endif // BLOB_H
