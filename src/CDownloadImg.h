/**
 * \file CDownloadImg.h
 *
 * \section LICENSE
 *
 * Copyright (C) 2011-2012 The InyokaEdit developers
 *
 * This file is part of InyokaEdit.
 *
 * InyokaEdit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * InyokaEdit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InyokaEdit.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \section DESCRIPTION
 * Class definition for download manager for images.
 */

#ifndef CDOWNLOADIMG_H
#define CDOWNLOADIMG_H

#include <QNetworkReply>
#include <QProgressDialog>

#include <QStringList>

class CDownloadImg : public QObject
{
    Q_OBJECT
    QNetworkAccessManager m_NwManager;
    QList<QNetworkReply *> m_listDownloadReplies;

public:
    CDownloadImg( const QString &sAppName );
    void setDLs( const QStringList sListUrls, const QStringList sListSavePath );

public slots:
    void startDownloads();

private slots:
    void downloadFinished( QNetworkReply *reply );
    void cancelDownloads();

signals:
    void finsihedImageDownload();

private:
    void doDownload( const QUrl &url, const QString sSavePath, const QString sBase = "" );
    QUrl redirectUrl( const QUrl& possibleRedirectUrl, const QUrl& oldRedirectUrl) const;

    QString m_sAppname;
    QProgressDialog *m_progessDialog;
    unsigned short nProgress;

    QUrl m_urlRedirectedTo;
    QStringList m_sListUrls;
    QStringList m_sListSavePath;

    QStringList m_sListRepliesPath;
    QStringList m_sListBasename;
    QStringList m_sListRedirect;
};

#endif // CDOWNLOADIMG_H