#include "userbackupoperation.h"

#include <QUrl>
#include <QFile>
#include <QTimer>
#include <QDebug>

#include "flipperzero/devicestate.h"
#include "flipperzero/commandinterface.h"
#include "flipperzero/cli/storagereadoperation.h"

#include "getfiletreeoperation.h"

using namespace Flipper;
using namespace Zero;

UserBackupOperation::UserBackupOperation(CommandInterface *cli, DeviceState *deviceState, const QString &backupPath, QObject *parent):
    AbstractUtilityOperation(cli, deviceState, parent),
    m_backupDir(backupPath),
    m_deviceDirName(QByteArrayLiteral("/int"))
{}

const QString UserBackupOperation::description() const
{
    return QStringLiteral("Backup %1 @%2").arg(m_deviceDirName, deviceState()->name());
}

void UserBackupOperation::advanceOperationState()
{
    if(operationState() == BasicOperationState::Ready) {
        deviceState()->setStatusString(QStringLiteral("Backing up internal storage..."));

        setOperationState(State::CreatingDirectory);
        createBackupDirectory();

    } else if(operationState() == State::CreatingDirectory) {
        setOperationState(State::GettingFileTree);
        getFileTree();

    } else if(operationState() == State::GettingFileTree) {
        setOperationState(State::ReadingFiles);
        readFiles();
    }
}

void UserBackupOperation::createBackupDirectory()
{
    if(!m_deviceDirName.startsWith('/')) {
        finishWithError(QStringLiteral("Expecting absolute path for device directory"));
        return;
    }

    const auto &subdir = deviceState()->deviceInfo().name;
    QFileInfo targetDirInfo(m_backupDir, subdir);

    if(targetDirInfo.isDir()) {
        QDir d(targetDirInfo.absoluteFilePath());
        if(!d.removeRecursively()) {
            finishWithError(QStringLiteral("Failed to remove old directory (1)"));
            return;
        }

    } else if(targetDirInfo.isFile()) {
        qWarning() << "Deleting a conflicting regular file";

        QFile f(targetDirInfo.absoluteFilePath());
        if(!f.remove()) {
            finishWithError(QStringLiteral("Failed to remove old directory (2)"));
            return;
        }
    }

    targetDirInfo.refresh();

    if(targetDirInfo.exists()) {
        finishWithError(QStringLiteral("Failed to remove old directory (3)"));
    } else if(!m_backupDir.mkpath(subdir + m_deviceDirName) || !m_backupDir.cd(subdir)) {
        finishWithError(QStringLiteral("Failed to create backup directory"));
    } else{
        QTimer::singleShot(0, this, &UserBackupOperation::advanceOperationState);
    }
}

void UserBackupOperation::getFileTree()
{
    auto *operation = new GetFileTreeOperation(cli(), deviceState(), m_deviceDirName, this);

    connect(operation, &AbstractOperation::finished, this, [=]() {
        if(operation->isError()) {
            finishWithError(operation->errorString());
        } else {
            m_fileList = operation->files();
            QTimer::singleShot(0, this, &UserBackupOperation::advanceOperationState);
        }

        operation->deleteLater();
    });

    operation->start();
}

void UserBackupOperation::readFiles()
{
    // Temporary fix: do not read files of size 0
    m_fileList.erase(std::remove_if(m_fileList.begin(), m_fileList.end(), [](const FileInfo &arg) {
        return (arg.type == FileType::RegularFile) && (arg.size == 0);
    }), m_fileList.end());

    auto numFiles = std::count_if(m_fileList.cbegin(), m_fileList.cend(), [](const FileInfo &arg) {
        return arg.type == FileType::RegularFile;
    });

    for(const auto &fileInfo: qAsConst(m_fileList)) {
        const auto filePath = fileInfo.absolutePath.mid(1);

        if(fileInfo.type == FileType::Directory) {
            if(!m_backupDir.mkdir(filePath)) {
                finishWithError(QStringLiteral("Failed to create directory: %1").arg(QString(filePath)));
                return;
            }

        } else if(fileInfo.type == FileType::RegularFile) {
            const auto isLastFile = (--numFiles == 0);

            auto *file = new QFile(m_backupDir.absoluteFilePath(filePath), this);
            if(!file->open(QIODevice::WriteOnly)) {
                file->deleteLater();
                finishWithError(QStringLiteral("Failed to open file for writing: %1").arg(QString(filePath)));
                return;
            }

            auto *op = cli()->storageRead(fileInfo.absolutePath, file);
            connect(op, &AbstractOperation::finished, this, [=]() {
                if(op->isError()) {
                    finishWithError(op->errorString());
                } else if(isLastFile) {
                    finish();
                }

                file->close();
                file->deleteLater();
            });
        }
    }
}