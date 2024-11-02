#ifndef TREE_HPP
#define TREE_HPP

#include "blob.hpp"

// Qt
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QString>

class Tree
{
private:
    QMap<QString, QString> blobs;  // Maps filenames to blob hashes
    QMap<QString, QString> trees;  // Maps directory names to subtree hashes
    mutable QString hash;          // Cached hash of the current tree

public:
    // Add a blob (file) to the tree
    void addBlob(const QString& name, const Blob& blob)
    {
        blobs[name] = blob.getHash();
    }

    // Add a subtree (directory) to the tree
    void addTree(const QString& name, const Tree& tree)
    {
        trees[name] = tree.getHash();
    }

    // Return the hash of the tree, generating it if necessary
    QString getHash() const
    {
        if (hash.isEmpty()) // Generate the hash only if it hasn't been generated already
        {
            QByteArray combinedData;
            for (const auto& blobHash : blobs)
            {
                combinedData.append(blobHash.toUtf8());
            }
            for (const auto& treeHash : trees)
            {
                combinedData.append(treeHash.toUtf8());
            }

            hash = generateHash(combinedData);
        }
        return hash;
    }

    // Get all blobs in the tree
    QMap<QString, QString> getBlobs() const
    {
        return blobs;
    }

    // Get all subtrees in the tree
    QMap<QString, QString> getTrees() const
    {
        return trees;
    }

    // Convert the tree to JSON format
    QJsonObject toJson() const
    {
        QJsonObject json;
        QJsonObject treeJson;
        QJsonObject blobJson;

        for (auto it = blobs.begin(); it != blobs.end(); ++it)
        {
            blobJson[it.key()] = it.value();
        }

        for (auto it = trees.begin(); it != trees.end(); ++it)
        {
            treeJson[it.key()] = it.value();
        }

        json["blobs"] = blobJson;
        json["trees"] = treeJson;

        return json;
    }

    // Create a Tree object from JSON data
    static Tree fromJson(const QJsonObject& json)
    {
        Tree tree;
        QJsonObject blobJson = json["blobs"].toObject();
        QJsonObject treeJson = json["trees"].toObject();

        for (const QString& key : blobJson.keys())
        {
            tree.blobs[key] = blobJson[key].toString();
        }

        for (const QString& key : treeJson.keys())
        {
            tree.trees[key] = treeJson[key].toString();
        }

        return tree;
    }

    // Save the tree to disk
    bool saveToDisk(const QString& rootPath) const
    {
        QString treeDir = rootPath + "/.trees";
        if (!QDir(treeDir).exists())
        {
            QDir().mkpath(treeDir);
        }

        QString treeFilePath = treeDir + "/tree_" + getHash() + ".json";
        QFile treeFile(treeFilePath);

        if (treeFile.open(QIODevice::WriteOnly))
        {
            QJsonDocument doc(toJson());
            treeFile.write(doc.toJson());
            treeFile.close();
            return true;
        }
        return false;
    }

    // Load a Tree from disk
    static Tree fromDisk(const QString& treeHash, const QString& rootPath)
    {
        QString treeFilePath = rootPath + "/.trees/tree_" + treeHash + ".json";
        QFile treeFile(treeFilePath);
        Tree tree;

        if (treeFile.open(QIODevice::ReadOnly))
        {
            QByteArray treeData = treeFile.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(treeData);
            QJsonObject jsonObj = doc.object();

            tree = Tree::fromJson(jsonObj);
            tree.hash = treeHash; // Assign the hash to the loaded tree
            treeFile.close();
        }
        return tree;
    }

private:
    // Generate a SHA-1 hash for the tree
    QString generateHash(const QByteArray& data) const
    {
        QByteArray hashData = QCryptographicHash::hash(data, QCryptographicHash::Sha1);
        return hashData.toHex();
    }
};

#endif // TREE_HPP
