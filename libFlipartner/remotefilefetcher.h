#ifndef FILEFETCHER_H
#define FILEFETCHER_H

#include <QObject>

#include "flipperupdates.h"

class QNetworkAccessManager;

class RemoteFileFetcher : public QObject
{
    Q_OBJECT

public:
    RemoteFileFetcher(QObject *parent = nullptr);
    virtual ~RemoteFileFetcher();

    bool fetch(const QString &remoteUrl);
    bool fetch(const Flipper::Updates::FileInfo &fileInfo);

signals:
    void progressChanged(double);
    void finished(const QByteArray&);

private slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onErrorOccured(int code);

private:
    QNetworkAccessManager *m_manager;
};

#endif // FILEFETCHER_H