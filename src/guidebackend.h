// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <QHash>
#include <QObject>
#include <QFileSystemWatcher>
#include <QPair>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include "amigaguide.h"

class GuideBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
    Q_PROPERTY(QString html READ html NOTIFY stateChanged)
    Q_PROPERTY(QString guideTitle READ guideTitle NOTIFY stateChanged)
    Q_PROPERTY(QString nodeTitle READ nodeTitle NOTIFY stateChanged)
    Q_PROPERTY(QString currentKey READ currentKey NOTIFY stateChanged)
    Q_PROPERTY(bool wrap READ wrap NOTIFY stateChanged)
    Q_PROPERTY(QVariantList nodes READ nodes NOTIFY stateChanged)
    Q_PROPERTY(QVariantList contents READ contents NOTIFY stateChanged)
    Q_PROPERTY(bool canBack READ canBack NOTIFY stateChanged)
    Q_PROPERTY(bool canNext READ canNext NOTIFY stateChanged)
    Q_PROPERTY(bool canPrev READ canPrev NOTIFY stateChanged)
    Q_PROPERTY(bool canContents READ canContents NOTIFY stateChanged)
    Q_PROPERTY(bool hasIndex READ hasIndex NOTIFY stateChanged)
    Q_PROPERTY(bool hasHelp READ hasHelp NOTIFY stateChanged)

public:
    explicit GuideBackend(QObject *parent = nullptr);

    bool loaded() const { return guide() != nullptr; }
    QString html() const { return m_html; }
    QString guideTitle() const;
    QString nodeTitle() const;
    QString currentKey() const { return m_key; }
    bool wrap() const;
    QVariantList nodes() const;
    QVariantList contents() const;
    bool canBack() const { return !m_history.isEmpty(); }
    bool canNext() const { return !relative(true).isEmpty(); }
    bool canPrev() const { return !relative(false).isEmpty(); }
    bool canContents() const { return !special(Toc).isEmpty(); }
    bool hasIndex() const { return !special(Index).isEmpty(); }
    bool hasHelp() const { return !special(Help).isEmpty(); }

    bool openPath(const QString &path, const QString &node = QString(), bool push = true);
    Q_INVOKABLE QVariantList search(const QString &query) const;

public Q_SLOTS:
    void openUrl(const QUrl &url);
    void gotoNode(const QString &key);
    void followLink(const QString &href);
    void goBack();
    void goNext() { go(relative(true)); }
    void goPrev() { go(relative(false)); }
    void goContents() { go(special(Toc)); }
    void goIndex() { go(special(Index)); }
    void goHelp() { go(special(Help)); }
    void editInKate();

Q_SIGNALS:
    void stateChanged();
    void message(const QString &text);

private:
    enum Special { Toc, Index, Help };

    const Guide *guide() const;
    const Node *node() const;
    QString relative(bool next) const;
    QString special(Special which) const;
    bool load(const QString &absPath);
    void show(const QString &path, const QString &key, bool push);
    void go(const QString &key);
    void reload();

    QHash<QString, Guide> m_cache;
    QVector<QPair<QString, QString>> m_history;
    QString m_path, m_key, m_html;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
    QString m_watched;
};
