/**
 * \file CParseLinks.cpp
 *
 * \section LICENSE
 *
 * Copyright (C) 2011-2016 The InyokaEdit developers
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
 * Parse all kind of links (external, InterWiki, etc.)
 */

#include <QDebug>
#include "./CParseLinks.h"

CParseLinks::CParseLinks(const QString &sUrlToWiki,
                         const QList<QStringList> sListIWiki,
                         const QList<QStringList> sListIWikiUrl,
                         const bool bCheckLinks,
                         const QString &sTransAnchor,
                         const QString &sTransAttach,
                         const QString &sTmpFilePath)
    : m_sWikiUrl(sUrlToWiki),
      m_bCheckLinks(bCheckLinks),
      m_sTransAnchor(sTransAnchor),
      m_sTransAttach(sTransAttach),
      m_sTmpFilePath(sTmpFilePath),
      m_NWreply(NULL) {
    qDebug() << "Calling" << Q_FUNC_INFO;

    // Copy interwiki links to lists
    for (int i = 0; i < sListIWiki.size(); i++) {
        for (int j = 0; j < sListIWiki[i].size(); j++) {
            this->m_sListInterwikiKey << sListIWiki[i][j];
            this->m_sListInterwikiLink << sListIWikiUrl[i][j];
        }
    }

    m_NWAManager = new QNetworkAccessManager(this);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CParseLinks::updateSettings(const QString sUrlToWiki,
                                 const bool bCheckLinks) {
    m_sWikiUrl = sUrlToWiki;
    m_bCheckLinks = bCheckLinks;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CParseLinks::startParsing(QTextDocument *pRawDoc) {
    this->replaceHyperlinks(pRawDoc);
    this->replaceInyokaWikiLinks(pRawDoc);
    this->replaceInterwikiLinks(pRawDoc);
    this->replaceAnchorLinks(pRawDoc);
    this->replaceKnowledgeBoxLinks(pRawDoc);
    this->createAnchor(pRawDoc);
    this->replaceAttachments(pRawDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// External links [http://www.ubuntu.com]
void CParseLinks::replaceHyperlinks(QTextDocument *pRawDoc) {
    QRegExp findHyperlink("\\[{1,1}\\b(http|https|ftp|ftps|file|ssh|mms|svn"
                          "|git|dict|nntp|irc|rsync|smb|apt)\\b://");
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength;
    QString sLink;
    int nSpace;

    nIndex = findHyperlink.indexIn(sDoc);
    while (nIndex >= 0) {
        // Found end of link
        if (sDoc.indexOf("]", nIndex) != -1) {
            nLength = sDoc.indexOf("]", nIndex) - nIndex + 1;  // End of link
            sLink = sDoc.mid(nIndex, nLength);
            // qDebug() << "FOUND: " << sLink;

            sLink.remove("[");
            sLink.remove("]");

            nSpace = sLink.indexOf(" ", 0);
            // Link with description
            if (nSpace != -1) {
                QString sHref = sLink;
                sDoc.replace(nIndex, nLength,
                               "<a href=\"" + sHref.remove(nSpace, nLength)
                               + "\" rel=\"nofollow\" class=\"external\">"
                               + sLink.remove(0, nSpace + 1) + "</a>");
            } else {
                // Plain link
                sDoc.replace(nIndex, nLength,
                               "<a href=\"" + sLink
                               + "\" rel=\"nofollow\" class=\"external\">"
                               + sLink + "</a>");
            }

            // Go on with next
            nIndex = findHyperlink.indexIn(sDoc, nIndex + nLength);
        } else {
            // Skip not closed link and go on with next
            nIndex = findHyperlink.indexIn(sDoc, nIndex + 1);
        }
    }

    pRawDoc->setPlainText(sDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Inyoka wiki links [:Wikipage:]
void CParseLinks::replaceInyokaWikiLinks(QTextDocument *pRawDoc) {
    QRegExp findInyokaWikiLink("\\[{1,1}\\:[0-9A-Za-z:.]");
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength = 0;
    QString sLink("");
    QString sLinkURL("");
    bool bIsOnline = false;

    // Check for internet connection
    #if QT_VERSION >= 0x040700
        QNetworkConfigurationManager mgr;
        bIsOnline = mgr.isOnline();
    #endif

    nIndex = findInyokaWikiLink.indexIn(sDoc);
    while (nIndex >= 0) {
        // Found end of link
        if (sDoc.indexOf("]", nIndex) != -1) {
            nLength = sDoc.indexOf("]", nIndex) - nIndex + 1;  // End of link
            sLink = sDoc.mid(nIndex, nLength);
            if (2 <= sLink.count(":")) {
                // qDebug() << "FOUND: " << sLink;
                sLink.remove("[:");

                // No description
                if (sLink.endsWith(":]")) {
                    sLink.remove(":]");
                    // qDebug() << sLink;
                    QString sLink2 = sLink;
                    sLink2.replace("_", " ");
                    sLinkURL = m_sWikiUrl + "/" + sLink;

                    m_sLinkClassAddition = "";
                    if (bIsOnline && m_bCheckLinks) {
                        m_NWreply = m_NWAManager->get(
                                    QNetworkRequest(
                                        QUrl(sLinkURL + "/a/export/meta/")));
                        QEventLoop loop;  // Workaround getting synchron reply
                        connect(m_NWreply, SIGNAL(finished()),
                                &loop, SLOT(quit()));
                        loop.exec();

                        if (QNetworkReply::NoError == m_NWreply->error()) {
                            m_sLinkClassAddition = "";
                        } else {
                            m_sLinkClassAddition = " missing";
                        }
                    }
                    sDoc.replace(nIndex, nLength,
                                   "<a href=\"" + sLinkURL
                                   + "\" class=\"internal"
                                   + m_sLinkClassAddition + "\">"
                                   + sLink2 + "</a>");
                } else {
                    sLink.remove("]");
                    // qDebug() << sLink.mid(0, sLink.indexOf(":"))
                    //          << " - "
                    //          << sLink.mid(sLink.indexOf(":") + 1, nLength);
                    sLinkURL = m_sWikiUrl + "/"
                               + sLink.mid(0, sLink.indexOf(":"));
                    if (bIsOnline && m_bCheckLinks) {
                        m_NWreply = m_NWAManager->get(
                                    QNetworkRequest(
                                        QUrl(sLinkURL + "/a/export/meta/")));
                        QEventLoop loop;
                        connect(m_NWreply, SIGNAL(finished()),
                                &loop, SLOT(quit()));
                        loop.exec();

                        if (QNetworkReply::NoError == m_NWreply->error()) {
                            m_sLinkClassAddition = "";
                        } else {
                            m_sLinkClassAddition = " missing";
                        }
                    }
                    sDoc.replace(nIndex, nLength,
                                 "<a href=\"" + sLinkURL
                                 + "\" class=\"internal"
                                 + m_sLinkClassAddition + "\">"
                                 + sLink.mid(sLink.indexOf(":")
                                             + 1, nLength).trimmed() + "</a>");
                }
            }

            // Go on with next
            nIndex = findInyokaWikiLink.indexIn(sDoc, nIndex + nLength);
        } else {
            // Skip not closed link and go on with next
            nIndex = findInyokaWikiLink.indexIn(sDoc, nIndex + 1);
        }
    }

    pRawDoc->setPlainText(sDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Interwiki links [wikipedia:Site:Text]
void CParseLinks::replaceInterwikiLinks(QTextDocument *pRawDoc) {
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength = 0;
    QString sLink;
    QStringList sListLink;
    QString sClass;

    // Generate pattern
    QString sPattern = "\\[{1,1}\\b(";
    for (int i = 0; i < m_sListInterwikiKey.size(); i++) {
        sPattern += m_sListInterwikiKey[i];
        if (i != m_sListInterwikiKey.size() -1) {
            sPattern += "|";
        }
    }
    sPattern += ")\\b:";
    QRegExp findInterwikiLink(sPattern);
    // qDebug() << sPattern;

    nIndex = findInterwikiLink.indexIn(sDoc);
    while (nIndex >= 0) {
        // Found end of link
        if (sDoc.indexOf("]", nIndex) != -1) {
            nLength = sDoc.indexOf("]", nIndex) - nIndex + 1;  // End of link
            sLink = sDoc.mid(nIndex, nLength);
            if (2 <= sLink.count(":")) {
                sLink.remove("[");
                sLink.remove("]");
                sListLink = sLink.split(":");
                // qDebug() << sListLink;

                if (sListLink.size() > 1) {
                    if (sListLink[0] == "user") {
                        sClass = "crosslink user";
                    } else if (sListLink[0] == "ikhaya") {
                        sClass = "crosslink ikhaya";
                    } else {
                        sClass = "interwiki interwiki-" + sListLink[0];
                    }

                    if (sListLink.size() >= 3) {
                        QString sTmpUrl(
                                    m_sListInterwikiLink[m_sListInterwikiKey.indexOf(sListLink[0])]);
                        // Check for iWikilink with $Page
                        if (sTmpUrl.contains("$Page", Qt::CaseInsensitive)) {
                            sTmpUrl.replace("$Page", sListLink[1],
                                            Qt::CaseInsensitive);
                        } else {
                            sTmpUrl.append(sListLink[1]);
                        }

                        // Default: Description = sitename
                        QString sTmpDescr(sListLink[1]);

                        // With description
                        if (!sListLink[2].isEmpty()) {
                            sTmpDescr = sListLink[2];
                            // Append description with ":" if any exist
                            for (int i = 3; i < sListLink.size(); i++) {
                                sTmpDescr.append(":" + sListLink[i]);
                            }
                        }

                        // Replace link
                        sDoc.replace(nIndex, nLength,
                                       "<a href=\"" + sTmpUrl + "\" class=\""
                                       + sClass + "\">" + sTmpDescr + "</a>");
                    }
                }
            }

            // Go on with next
            nIndex = findInterwikiLink.indexIn(sDoc, nIndex + nLength);
        } else {
            // Skip not closed link and go on with next
            nIndex = findInterwikiLink.indexIn(sDoc, nIndex + 1);
        }
    }

    pRawDoc->setPlainText(sDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Anchor [#Headline Text]
void CParseLinks::replaceAnchorLinks(QTextDocument *pRawDoc) {
    QRegExp findAnchorLink("\\[{1,1}\\#");
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength = 0;
    QString sLink;
    int nSplit;

    nIndex = findAnchorLink.indexIn(sDoc);
    while (nIndex >= 0) {
        // Found end of link
        if (sDoc.indexOf("]", nIndex) != -1) {
            nLength = sDoc.indexOf("]", nIndex) - nIndex + 1;  // End of link
            sLink = sDoc.mid(nIndex, nLength);

            sLink.remove("[#");
            sLink.remove("]");
            nSplit = sLink.indexOf(" ");
            // qDebug() << sLink.mid(0, nSplit)
            //          << sLink.mid(nSplit + 1 , nLength);

            // With description
            if (nSplit != -1) {
                sDoc.replace(nIndex, nLength,
                               "<a href=\"#" + sLink.mid(0, nSplit)
                               + "\" class=\"crosslink\">"
                               + sLink.mid(nSplit + 1 , nLength) + "</a>");
            } else {
                // Without descrition
                sDoc.replace(nIndex, nLength,
                               "<a href=\"#" + sLink.mid(0, nSplit)
                               + "\" class=\"crosslink\">#"
                               + sLink.mid(0, nSplit) + "</a>");
            }

            // Go on with next
            nIndex = findAnchorLink.indexIn(sDoc, nIndex + nLength);
        } else {
            // Skip not closed link and go on with next
            nIndex = findAnchorLink.indexIn(sDoc, nIndex + 1);
        }
    }

    pRawDoc->setPlainText(sDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Link to knowledge box entry
void CParseLinks::replaceKnowledgeBoxLinks(QTextDocument *pRawDoc) {
    QRegExp findKnowledgeBoxLink("\\[{1,1}[0-9]{1,}\\]{1,1}");
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength;
    QString sLink;

    nIndex = findKnowledgeBoxLink.indexIn(sDoc);
    while (nIndex >= 0) {
        nLength = findKnowledgeBoxLink.matchedLength();
        sLink = findKnowledgeBoxLink.cap();
        // qDebug() << sLink;

        sLink.remove("[");
        sLink.remove("]");

        if (sLink.toUShort() != 0) {
            sDoc.replace(nIndex, nLength,
                           "<sup><a href=\"#source-" + sLink + "\">&#091;"
                           + sLink + "&#093;</a></sup>");
        }

        // Go on with next
        nIndex = findKnowledgeBoxLink.indexIn(sDoc, nIndex + nLength);
    }

    pRawDoc->setPlainText(sDoc);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// Create anchor
void CParseLinks::createAnchor(QTextDocument *pRawDoc) {
    QRegExp findAnchor("\\[{2,2}\\b("
                       + m_sTransAnchor
                       + ")\\([A-Za-z_\\s-0-9]+\\)\\]{2,2}");
    QString sDoc(pRawDoc->toPlainText());
    int nIndex;
    int nLength;
    QString sAnchor;

    nIndex = findAnchor.indexIn(sDoc);
    while (nIndex >= 0) {
        nLength = findAnchor.matchedLength();
        sAnchor = findAnchor.cap();
        // qDebug() << sAnchor;

        sAnchor.remove("[[" + m_sTransAnchor + "(");
        sAnchor.remove(")]]");
        sAnchor = sAnchor.trimmed();

        // Replace characters for valid links (ä, ü, ö, spaces)
        sAnchor.replace(" ", "-");
        sAnchor.replace(QString::fromUtf8("Ä"), "Ae");
        sAnchor.replace(QString::fromUtf8("Ü"), "Ue");
        sAnchor.replace(QString::fromUtf8("Ö"), "Oe");
        sAnchor.replace(QString::fromUtf8("ä"), "ae");
        sAnchor.replace(QString::fromUtf8("ü"), "ue");
        sAnchor.replace(QString::fromUtf8("ö"), "oe");

        // Replace
        sDoc.replace(nIndex, nLength,
                       "<a id=\"" + sAnchor + "\" href=\"#" + sAnchor
                       + "\" class=\"crosslink anchor\"> </a>");

        // Go on with RegExp-Search
        nIndex = findAnchor.indexIn(sDoc, nIndex + nLength);
    }

    pRawDoc->setPlainText(sDoc);
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CParseLinks::replaceAttachments(QTextDocument *p_RawDoc) {
    QString sDoc(p_RawDoc->toPlainText());
    QString sRegExp("\\[\\[" + m_sTransAttach + "\\(.*\\)\\]\\]");
    QRegExp findMacro(sRegExp, Qt::CaseInsensitive);
    findMacro.setMinimal(true);
    QString sMacro;
    int nPos = 0;

    while ((nPos = findMacro.indexIn(sDoc, nPos)) != -1) {
        sMacro = findMacro.cap(0);
        sMacro.remove("[[" + m_sTransAttach + "(");
        sMacro.remove(")]]");
        sMacro.remove('"');

        sMacro = "<a href=\"" + m_sTmpFilePath + "/" + sMacro +
                "\" class=\"crosslink\">" + sMacro + "</a>";

        sDoc.replace(nPos, findMacro.matchedLength(), sMacro);
        // Go on with new start position
        nPos += sMacro.length();
    }

    p_RawDoc->setPlainText(sDoc);
}
