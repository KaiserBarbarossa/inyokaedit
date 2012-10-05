/**
 * \file CDownload.cpp
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
 * Download functions: Styles, article, images
 */

#include <QDebug>

#include "CDownload.h"

extern bool bDEBUG;

CDownload::CDownload( QWidget *pParent, const QString &sAppName, const QString &sAppDir, const QString &sStylesDir,
                      const QString &sImgDir, const QString &sInyokaUrl, const bool bAutomaticImgDL ) :
    m_pParent(pParent),
    m_sAppName(sAppName),
    m_sAppDir(sAppDir),
    m_sStylesDir(sStylesDir),
    m_sImgDir(sImgDir),
    m_sInyokaUrl(sInyokaUrl),
    m_bAutomaticImageDownload(bAutomaticImgDL)
{
    qDebug() << "Start" << Q_FUNC_INFO;

    m_NwManager = new QNetworkAccessManager(m_pParent);
    connect( m_NwManager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(replyFinished(QNetworkReply*)) );

    m_DlImages = new CDownloadImg( m_sAppName );

    connect( m_DlImages, SIGNAL(finsihedImageDownload()),
             this, SLOT(showArticle()) );

    qDebug() << "End" << Q_FUNC_INFO;
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
// Call download script for Inyoka styles (ONLY working with Linux!)

bool CDownload::loadInyokaStyles()
{
    qDebug() << "Start" << Q_FUNC_INFO;
    int iRet = QMessageBox::question( m_pParent, tr("Download styles"),
                                      tr("In order to preview articles correctly, Inyoka resources have to be downloaded. This process may take a few minutes.\n\nDo you want to download these files now?"),
                                      QMessageBox::Yes | QMessageBox::No );

    if ( QMessageBox::Yes== iRet )
    {
        CProgressDialog *myArticleDownloadProgress;

        // Path from normal installation
        if ( QFile::exists("/usr/share/" + m_sAppName.toLower() + "/GetInyokaStyles") && !bDEBUG )
        {
            myArticleDownloadProgress = new CProgressDialog( "/usr/share/" + m_sAppName.toLower() +"/GetInyokaStyles",
                                                             QStringList() << m_sStylesDir,
                                                             m_sAppName, m_pParent );
        }
        // No installation: Use app path
        else
        {
            myArticleDownloadProgress = new CProgressDialog( m_sAppDir + "/GetInyokaStyles",
                                                             QStringList() << m_sStylesDir,
                                                             m_sAppName, m_pParent );
        }

        myArticleDownloadProgress->open();

        qDebug() << "End" << Q_FUNC_INFO;
        return true;
    }
    return false;
}
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
// DOWNLOAD EXISTING INYOKA WIKI ARTICLE

void CDownload::downloadArticle()
{
    // Check for internet connection
#if QT_VERSION >= 0x040700
    QNetworkConfigurationManager mgr;
    if( !mgr.isOnline() )
    {
        QMessageBox::warning( m_pParent, m_sAppName,
                              tr("Download not possible, no active internet connection found!") );
        return;
    }
#endif

    QString sUrl("");
    bool ok; // Buttons of input dialog (click on "OK" -> ok = true, click on "Cancel" -> ok = false)

    // Show input dialog
    m_sSitename = QInputDialog::getText( m_pParent, m_sAppName,
                                         tr("Please insert name of the article which should be downloaded:"),
                                         QLineEdit::Normal,
                                         tr("Category/Article", "Msg: Input dialog DL article example text"),
                                         &ok );

    // Click on "cancel" or string is empty
    if ( false == ok || m_sSitename.isEmpty() )
    {
        return;
    }

    // Replace non valid characters
    m_sSitename.replace( QString::fromUtf8("ä"), "a", Qt::CaseInsensitive );
    m_sSitename.replace( QString::fromUtf8("ö"), "o", Qt::CaseInsensitive );
    m_sSitename.replace( QString::fromUtf8("ü"), "u", Qt::CaseInsensitive );
    m_sSitename.replace( " ", "_" );

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif 

    sUrl = m_sInyokaUrl + "/" + m_sSitename + "?action=export&format=raw";

    // Download specific revision
    if ( m_sSitename.contains("@rev=") )
    {
        QString sRevision = m_sSitename.mid( m_sSitename.indexOf("@rev=") );
        sRevision.remove("@rev=");
        m_sSitename.remove( m_sSitename.indexOf("@rev="), m_sSitename.length() );
        sUrl += "&rev=" + sRevision;
    }

    m_bDownloadArticle = true;
    QNetworkRequest request(sUrl);
    m_Reply = m_NwManager->get(request);
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
// DOWNLOAD IN ARTICLES INCLUDED IMAGES

void CDownload::downloadImages()
{
    m_bDownloadArticle = false;

    QString sUrl( m_sInyokaUrl + "/" + m_sSitename +"?action=metaexport" );
    QNetworkRequest request(sUrl);
    m_Reply = m_NwManager->get(request);
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

void CDownload::replyFinished( QNetworkReply *reply )
{
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    QIODevice *data(reply);

    if( reply->error() != QNetworkReply::NoError )
    {
        QMessageBox::critical( m_pParent, m_sAppName, data->errorString() );
        return;
    }
    else
    {
        QString sTmpArticle = data->readAll();

        if( m_bDownloadArticle )
        {
            sTmpArticle.replace("\r\r\n", "\n");  // Replace windows specific newlines

            m_Reply->deleteLater();

            // Site does not exist etc.
            if ( "" == sTmpArticle )
            {
                QMessageBox::information( m_pParent, m_sAppName, tr("Could not download the article.") );
                return;
            }

            m_sArticleText = sTmpArticle;
            downloadImages();
        }
        // ----------------------------------------------------------------------------------------
        // Download article images metadata
        else
        {
            int iRet = 0;
            QStringList sListTmp, sListMetadata, sListSaveFolder;

            // Site does not exist etc.
            if ( "" == sTmpArticle )
            {
                QMessageBox::information( m_pParent, m_sAppName, tr("Could not find meta data.") );
                return;
            }

            // Copy metadata line by line in list
            sListTmp << sTmpArticle.split("\n");

            m_Reply->deleteLater();

            // Get only attachments article metadata
            for ( int i = 0; i < sListTmp.size(); i++ )
            {
                if ( sListTmp[i].startsWith("X-Attach: " + m_sSitename + "/", Qt::CaseInsensitive) )
                {
                    sListMetadata << sListTmp[i].remove("X-Attach: ");  // Remove "X-Attach: "
                    sListMetadata.last().remove("\r");  // Remove windows specific newline \r
                    sListMetadata.last() = m_sInyokaUrl + "/_image?target=" + sListMetadata.last().toLower();
                    sListSaveFolder << m_sImgDir;

                    //qDebug() << sListMetadata.last();
                }
            }

            // If attachments exist
            if ( sListMetadata.size() > 0 )
            {
                // Ask if images should be downloaded (if not enabled by default in settings)
                if ( false == m_bAutomaticImageDownload )
                {
                    iRet = QMessageBox::question( m_pParent, m_sAppName,
                                                  tr("Do you want to download the images which are attached to the article?"),
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
                }
                else
                {
                    iRet = QMessageBox::Yes;
                }

                if ( QMessageBox::Yes == iRet )
                {
                    qDebug() << "Starting image download...";
                    m_DlImages->setDLs( sListMetadata, sListSaveFolder );
                    QTimer::singleShot(0, m_DlImages, SLOT(startDownloads()));
                }
                else
                {
                    this->showArticle();
                }
            }
        }
    }
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

void CDownload::showArticle()
{
    emit sendArticleText( m_sArticleText, m_sSitename );
}