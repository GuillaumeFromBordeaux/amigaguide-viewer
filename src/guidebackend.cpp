// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "guidebackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QVariantMap>

namespace {
// Recherche insensible à la casse (héritage Amiga), avec ou sans ".guide"
QString findFile(const QString &dir, QString name)
{
    name.replace(u'\\', u'/');
    name = name.mid(name.lastIndexOf(u':') + 1);
    name = name.mid(name.lastIndexOf(u'/') + 1).toLower();
    if (name.isEmpty())
        return {};
    const QDir d(dir);
    for (const QString &f : d.entryList(QDir::Files)) {
        const QString low = f.toLower();
        if (low == name || low == name + QStringLiteral(".guide"))
            return d.absoluteFilePath(f);
    }
    return {};
}
} // namespace

GuideBackend::GuideBackend(QObject *parent) : QObject(parent)
{
    // Rechargement automatique quand le fichier est modifié (par Kate par exemple).
    // Le délai regroupe les notifications multiples d'une même sauvegarde.
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(300);
    connect(&m_reloadTimer, &QTimer::timeout, this, &GuideBackend::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_reloadTimer.start(); });
}

const Guide *GuideBackend::guide() const
{
    const auto it = m_cache.constFind(m_path);
    return it == m_cache.constEnd() ? nullptr : &it.value();
}

const Node *GuideBackend::node() const
{
    const Guide *g = guide();
    if (!g)
        return nullptr;
    const auto it = g->nodes.constFind(m_key);
    return it == g->nodes.constEnd() ? nullptr : &it.value();
}

QString GuideBackend::guideTitle() const
{
    const Guide *g = guide();
    return g ? g->title() : QString();
}

QString GuideBackend::nodeTitle() const
{
    const Node *n = node();
    return n ? (n->title.isEmpty() ? n->name : n->title) : QString();
}

bool GuideBackend::wrap() const
{
    const Guide *g = guide();
    return g && g->wrap;
}

QVariantList GuideBackend::nodes() const
{
    QVariantList list;
    if (const Guide *g = guide()) {
        for (const QString &k : g->order) {
            const Node &n = g->nodes[k];
            list << QVariantMap{{QStringLiteral("key"), k},
                                {QStringLiteral("title"), n.title.isEmpty() ? n.name : n.title}};
        }
    }
    return list;
}

QString GuideBackend::relative(bool next) const
{
    const Guide *g = guide();
    const Node *n = node();
    if (!n)
        return {};
    const QString &explicitName = next ? n->next : n->prev;
    if (!explicitName.isEmpty()) {
        const QString k = explicitName.toLower();
        return g->nodes.contains(k) ? k : QString();
    }
    const int i = g->order.indexOf(m_key) + (next ? 1 : -1);
    return (i >= 0 && i < g->order.size()) ? g->order.at(i) : QString();
}

QString GuideBackend::special(Special which) const
{
    const Guide *g = guide();
    const Node *n = node();
    if (!n)
        return {};
    QString name;
    QString def;
    switch (which) {
    case Toc:   name = n->toc; def = QStringLiteral("main"); break;
    case Index: name = n->index.isEmpty() ? g->index : n->index; def = QStringLiteral("index"); break;
    case Help:  name = n->help.isEmpty() ? g->help : n->help; def = QStringLiteral("help"); break;
    }
    const QString k = (name.isEmpty() ? def : name).toLower();
    return g->nodes.contains(k) ? k : QString();
}

bool GuideBackend::load(const QString &absPath)
{
    if (!m_cache.contains(absPath)) {
        Guide g;
        QString err;
        if (!AmigaGuide::parse(absPath, g, err)) {
            Q_EMIT message(QStringLiteral("Impossible d'ouvrir %1 : %2")
                               .arg(QFileInfo(absPath).fileName(), err));
            return false;
        }
        m_cache.insert(absPath, g);
    }
    if (m_cache[absPath].nodes.isEmpty()) {
        Q_EMIT message(QStringLiteral("%1 ne contient aucun nœud AmigaGuide.")
                           .arg(QFileInfo(absPath).fileName()));
        return false;
    }
    return true;
}

void GuideBackend::show(const QString &path, const QString &key, bool push)
{
    if (push && !m_path.isEmpty() && !m_key.isEmpty() && (path != m_path || key != m_key))
        m_history.append({m_path, m_key});
    if (path != m_watched) {
        if (!m_watched.isEmpty())
            m_watcher.removePath(m_watched);
        m_watcher.addPath(path);
        m_watched = path;
    }
    m_path = path;
    m_key = key;
    m_html = AmigaGuide::renderNode(m_cache[path].nodes[key], QFileInfo(path).absolutePath());
    Q_EMIT stateChanged();
}

void GuideBackend::go(const QString &key)
{
    if (!key.isEmpty())
        show(m_path, key, true);
}

bool GuideBackend::openPath(const QString &path, const QString &nodeName, bool push)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    if (!load(abs))
        return false;
    const Guide &g = m_cache[abs];
    QString key = nodeName.toLower();
    if (key.isEmpty() || !g.nodes.contains(key))
        key = g.startKey();
    show(abs, key, push);
    return true;
}

void GuideBackend::openUrl(const QUrl &url)
{
    const auto saved = m_history;
    m_history.clear();
    if (!openPath(url.toLocalFile(), QString(), false))
        m_history = saved; // échec : on garde l'état précédent
}

void GuideBackend::gotoNode(const QString &key)
{
    const Guide *g = guide();
    if (g && g->nodes.contains(key))
        show(m_path, key, true);
}

void GuideBackend::followLink(const QString &href)
{
    const int colon = href.indexOf(u':');
    const QString scheme = href.left(colon);
    const QString rest = href.mid(colon + 1);

    if (scheme == u"amiga") {
        const int c2 = rest.indexOf(u':');
        const QString kind = rest.left(c2);
        const QString target = QUrl::fromPercentEncoding(rest.mid(c2 + 1).toUtf8());
        if (kind != u"close" && kind != u"quit")
            Q_EMIT message(QStringLiteral("Commande Amiga non exécutée (%1 %2).")
                               .arg(kind.toUpper(), target));
        return;
    }

    const QString target = QUrl::fromPercentEncoding(rest.toUtf8());
    const Guide *g = guide();
    if (g && g->nodes.contains(target.toLower())) {
        show(m_path, target.toLower(), true);
        return;
    }

    // lien vers un autre fichier : "autre.guide/NOEUD" ou "autre.guide"
    QString filePart = target;
    QString nodePart = QStringLiteral("main");
    const int slash = target.lastIndexOf(u'/');
    if (slash > 0) {
        filePart = target.left(slash);
        nodePart = target.mid(slash + 1);
    }
    if (g) {
        const QString found = findFile(QFileInfo(m_path).absolutePath(), filePart);
        if (!found.isEmpty()) {
            openPath(found, nodePart, true);
            return;
        }
    }
    Q_EMIT message(QStringLiteral("Nœud ou fichier introuvable : %1").arg(target));
}

void GuideBackend::goBack()
{
    if (m_history.isEmpty())
        return;
    const auto [path, key] = m_history.takeLast();
    if (load(path))
        show(path, key, false);
}

QVariantList GuideBackend::search(const QString &query) const
{
    QVariantList results;
    const Guide *g = guide();
    const QString q = query.trimmed();
    if (!g || q.isEmpty())
        return results;

    constexpr int maxResults = 300;
    constexpr int snippetLen = 110;

    for (const QString &k : g->order) {
        const Node &n = g->nodes[k];
        const QString title = n.title.isEmpty() ? n.name : n.title;
        bool hit = title.contains(q, Qt::CaseInsensitive);
        int count = 0;
        QString snippet;

        for (const QString &raw : AmigaGuide::plainText(n)) {
            const QString line = raw.simplified();
            const int idx = line.indexOf(q, 0, Qt::CaseInsensitive);
            if (idx < 0)
                continue;
            hit = true;
            ++count;
            if (snippet.isEmpty()) { // extrait centré sur la première occurrence
                const int start = qMax(0, idx - snippetLen / 3);
                snippet = line.mid(start, snippetLen);
                if (start > 0)
                    snippet.prepend(u'…');
                if (start + snippetLen < line.size())
                    snippet.append(u'…');
            }
        }
        if (!hit)
            continue;
        results << QVariantMap{{QStringLiteral("key"), k},
                               {QStringLiteral("title"), title},
                               {QStringLiteral("snippet"), snippet},
                               {QStringLiteral("count"), count}};
        if (results.size() >= maxResults)
            break;
    }
    return results;
}

void GuideBackend::editInKate()
{
    const Guide *g = guide();
    const Node *n = node();
    if (!g || !n)
        return;

    const QString kate = QStandardPaths::findExecutable(QStringLiteral("kate"));
    if (kate.isEmpty()) {
        Q_EMIT message(QStringLiteral("Kate est introuvable. Installez-le avec : sudo pacman -S kate"));
        return;
    }

    QStringList args;
    if (n->line > 0) // Kate s'ouvre directement sur le @node de la section affichée
        args << QStringLiteral("--line") << QString::number(n->line)
             << QStringLiteral("--column") << QStringLiteral("1");
    if (g->latin1)
        args << QStringLiteral("--encoding") << QStringLiteral("ISO-8859-1");
    args << g->path;

    if (!QProcess::startDetached(kate, args))
        Q_EMIT message(QStringLiteral("Impossible de lancer Kate."));
}

void GuideBackend::reload()
{
    if (m_path.isEmpty())
        return;
    const QString path = m_path;
    if (!QFileInfo::exists(path))
        return; // sauvegarde en cours (remplacement du fichier) ou fichier supprimé

    // Certains éditeurs remplacent le fichier : la surveillance doit être réarmée
    if (!m_watcher.files().contains(path))
        m_watcher.addPath(path);

    const QString key = m_key;
    m_cache.remove(path);
    if (!load(path))
        return;
    const Guide &g = m_cache[path];
    show(path, g.nodes.contains(key) ? key : g.startKey(), false);
}

// Sommaire affiché dans le volet de gauche : les liens du nœud principal (MAIN).
// Si ce nœud ne contient aucun lien, on retombe sur la liste de tous les nœuds.
QVariantList GuideBackend::contents() const
{
    QVariantList list;
    const Guide *g = guide();
    if (!g)
        return list;

    const auto encode = [](const QString &s) {
        return QStringLiteral("node:") + QString::fromLatin1(QUrl::toPercentEncoding(s));
    };

    const QString mainKey = g->startKey();
    const auto mainIt = g->nodes.constFind(mainKey);
    if (mainIt != g->nodes.constEnd()) {
        QSet<QString> seen;
        for (const AmigaGuide::Link &l : AmigaGuide::links(mainIt.value())) {
            if (l.kind != u"link" && l.kind != u"alink" && l.kind != u"guide")
                continue; // on ignore system, rx, quit...
            const QString label = l.label.simplified();
            const QString key = l.target.toLower();
            if (label.isEmpty() || l.target.isEmpty() || key == mainKey)
                continue; // pas de lien « retour au sommaire » dans le sommaire
            const QString href = encode(l.target);
            if (seen.contains(href))
                continue;
            seen.insert(href);
            list << QVariantMap{{QStringLiteral("label"), label},
                                {QStringLiteral("href"), href},
                                {QStringLiteral("key"), g->nodes.contains(key) ? key : QString()}};
        }
    }

    if (list.isEmpty()) {
        for (const QString &k : g->order) {
            const Node &n = g->nodes[k];
            list << QVariantMap{{QStringLiteral("label"), n.title.isEmpty() ? n.name : n.title},
                                {QStringLiteral("href"), encode(n.name)},
                                {QStringLiteral("key"), k}};
        }
    }
    return list;
}
