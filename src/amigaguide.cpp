// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "amigaguide.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QUrl>

QString Guide::title() const
{
    return name.isEmpty() ? QFileInfo(path).completeBaseName() : name;
}

QString Guide::startKey() const
{
    if (nodes.contains(QStringLiteral("main")))
        return QStringLiteral("main");
    return order.isEmpty() ? QString() : order.first();
}

namespace AmigaGuide {

QVector<Token> splitTokens(const QString &s)
{
    QVector<Token> toks;
    const int n = s.size();
    int i = 0;
    while (i < n) {
        if (s[i].isSpace()) {
            ++i;
        } else if (s[i] == u'"') {
            int j = i + 1;
            QString buf;
            while (j < n && s[j] != u'"') {
                if (s[j] == u'\\' && j + 1 < n)
                    ++j;
                buf += s[j];
                ++j;
            }
            toks.push_back({true, buf});
            i = j + 1;
        } else {
            int j = i;
            while (j < n && !s[j].isSpace())
                ++j;
            toks.push_back({false, s.mid(i, j - i)});
            i = j;
        }
    }
    return toks;
}

namespace {

const QSet<QString> &knownCmds()
{
    static const QSet<QString> s = {
        "node", "endnode", "database", "master", "author", "$ver", "font", "index",
        "help", "toc", "next", "prev", "title", "rem", "remark", "width", "height",
        "wordwrap", "smartwrap", "proportional", "keywords", "macro", "onopen",
        "onclose", "copyright", "tab"};
    return s;
}

QString firstToken(const QString &arg)
{
    const auto t = splitTokens(arg);
    return t.isEmpty() ? QString() : t.first().text;
}

QString expandTabs(const QString &s)
{
    QString out;
    for (const QChar c : s) {
        if (c == u'\t')
            out += QString(8 - out.size() % 8, u' ');
        else
            out += c;
    }
    return out;
}

int findClose(const QString &s, int i)
{
    bool inQuote = false;
    while (i < s.size()) {
        const QChar c = s[i];
        if (c == u'\\') {
            i += 2;
            continue;
        }
        if (c == u'"')
            inQuote = !inQuote;
        else if (c == u'}' && !inQuote)
            return i;
        ++i;
    }
    return -1;
}

QString colorFor(const QString &name)
{
    if (name == u"shine") return QStringLiteral("#c08000");
    if (name == u"shadow") return QStringLiteral("#666666");
    if (name == u"fill") return QStringLiteral("#3465a4");
    if (name == u"filltext") return QStringLiteral("#ffffff");
    if (name == u"highlight") return QStringLiteral("#cc0000");
    return {}; // text, background, inconnu : couleur du thème
}

QString enc(const QString &s)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(s));
}

// Chemin AmigaDOS -> fichier réel, relatif au dossier du guide, sans tenir compte de la casse.
// "GUIDEDIR:img.iff" et "Vol:img.iff" -> "img.iff" ; un '/' initial désigne le dossier parent.
QString resolveAmigaPath(const QString &baseDir, QString p)
{
    const QString original = p;
    const auto fallback = [&]() -> QString { // chemin Linux absolu fourni tel quel
        const QFileInfo fi(original);
        return (fi.isAbsolute() && fi.isFile()) ? fi.absoluteFilePath() : QString();
    };

    p.replace(u'\\', u'/');
    const int colon = p.indexOf(u':');
    if (colon >= 0)
        p = p.mid(colon + 1);
    QString prefix;
    while (p.startsWith(u'/')) {
        prefix += QStringLiteral("../");
        p.remove(0, 1);
    }
    p.replace(QStringLiteral("//"), QStringLiteral("/../"));
    p = prefix + p;

    QDir dir(baseDir);
    const QStringList parts = p.split(u'/', Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        const QString &part = parts[i];
        if (part == u".")
            continue;
        if (part == u"..") {
            if (!dir.cdUp())
                return fallback();
            continue;
        }
        QString found = dir.exists(part) ? part : QString();
        if (found.isEmpty()) {
            const auto entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
            for (const QString &e : entries) {
                if (e.compare(part, Qt::CaseInsensitive) == 0) {
                    found = e;
                    break;
                }
            }
        }
        if (found.isEmpty())
            return fallback();
        if (i == parts.size() - 1) {
            const QString full = dir.absoluteFilePath(found);
            return QFileInfo(full).isFile() ? full : fallback();
        }
        if (!dir.cd(found))
            return fallback();
    }
    return fallback();
}

// Balise <img> pour une image de guide ; texte de remplacement si elle est introuvable
// ou si Qt ne sait pas la lire (l'IFF demande le plugin kimageformats).
QString imageTag(const QString &baseDir, const QString &amigaPath)
{
    const QString file = baseDir.isEmpty() ? QString() : resolveAmigaPath(baseDir, amigaPath);
    if (!file.isEmpty()) {
        QImageReader reader(file);
        QSize size = reader.size();
        if (!size.isValid()) {
            const QImage img = reader.read();
            if (!img.isNull())
                size = img.size();
        }
        if (size.isValid()) {
            const QString url = QUrl::fromLocalFile(file).toString(QUrl::FullyEncoded);
            return QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\"/>")
                .arg(url.toHtmlEscaped())
                .arg(size.width())
                .arg(size.height());
        }
    }
    return QStringLiteral("<i>[image : %1]</i>").arg(amigaPath.toHtmlEscaped());
}

class Renderer
{
public:
    explicit Renderer(QString baseDir) : m_baseDir(std::move(baseDir)) {}

    QString renderLine(QString line)
    {
        line = expandTabs(line);
        static const QRegularExpression lindent(QStringLiteral("^@\\{lindent\\s+(\\d+)\\}"));
        const auto m = lindent.match(line);
        if (m.hasMatch()) { // l'indentation en début de ligne s'applique à cette ligne
            m_indent = m.captured(1).toInt();
            line = line.mid(m.capturedLength());
        }

        QString out;
        m_open = false;
        restyle(out);
        if (m_indent > 0)
            out += QString(m_indent, u' ');

        const int n = line.size();
        int i = 0;
        while (i < n) {
            const QChar c = line[i];
            if (c == u'\\' && i + 1 < n && (line[i + 1] == u'@' || line[i + 1] == u'\\')) {
                out += QString(line[i + 1]).toHtmlEscaped();
                i += 2;
                continue;
            }
            if (c == u'@' && i + 1 < n && line[i + 1] == u'{') {
                const int end = findClose(line, i + 2);
                if (end != -1) {
                    tag(line.mid(i + 2, end - i - 2), out);
                    i = end + 1;
                    continue;
                }
            }
            out += QString(c).toHtmlEscaped();
            ++i;
        }
        if (m_open)
            out += QStringLiteral("</span>");
        if (out.isEmpty())
            out = QStringLiteral("&nbsp;");
        return QStringLiteral("<p align=\"%1\" style=\"margin:0; white-space:pre-wrap\">%2</p>")
            .arg(m_align, out);
    }

private:
    void restyle(QString &out)
    {
        if (m_open)
            out += QStringLiteral("</span>");
        QStringList css;
        if (m_b) css << QStringLiteral("font-weight:bold");
        if (m_i) css << QStringLiteral("font-style:italic");
        if (m_u) css << QStringLiteral("text-decoration:underline");
        if (!m_fg.isEmpty()) css << QStringLiteral("color:") + m_fg;
        if (!m_bg.isEmpty()) css << QStringLiteral("background-color:") + m_bg;
        m_open = !css.isEmpty();
        if (m_open)
            out += QStringLiteral("<span style=\"%1\">").arg(css.join(QStringLiteral("; ")));
    }

    void tag(const QString &content, QString &out)
    {
        const auto toks = splitTokens(content);
        if (toks.isEmpty())
            return;

        if (toks[0].quoted) { // lien : @{"Label" link Cible [ligne]}
            const QString kind = toks.size() > 1 ? toks[1].text.toLower() : QStringLiteral("link");
            if (kind == u"img") { // @{"fichier" img}
                out += imageTag(m_baseDir, toks[0].text);
                return;
            }
            const QString target = toks.size() > 2 ? toks[2].text : QString();
            const QString href = (kind == u"link" || kind == u"alink" || kind == u"guide")
                ? QStringLiteral("node:") + enc(target)
                : QStringLiteral("amiga:") + kind + u':' + enc(target);
            //out += QStringLiteral("<a href=\"%1\">%2</a>").arg(href, toks[0].text.toHtmlEscaped());
            out += QStringLiteral(
            "<a href=\"%1\" "
            "style=\"color:#222; "
            "background-color:#fff;\">%2</a>"
            ).arg(href, toks[0].text.toHtmlEscaped());
            return;
        }

        const QString cmd = toks[0].text.toLower();
        const QString arg = toks.size() > 1 ? toks[1].text.toLower() : QString();

        if (cmd == u"b" || cmd == u"ub") { m_b = (cmd == u"b"); restyle(out); }
        else if (cmd == u"i" || cmd == u"ui") { m_i = (cmd == u"i"); restyle(out); }
        else if (cmd == u"u" || cmd == u"uu") { m_u = (cmd == u"u"); restyle(out); }
        else if (cmd == u"plain") {
            m_b = m_i = m_u = false;
            m_fg.clear();
            m_bg.clear();
            restyle(out);
        }
        else if (cmd == u"fg") { m_fg = colorFor(arg); restyle(out); }
        else if (cmd == u"bg") { m_bg = colorFor(arg); restyle(out); }
        else if (cmd == u"jleft") m_align = QStringLiteral("left");
        else if (cmd == u"jcenter") m_align = QStringLiteral("center");
        else if (cmd == u"jright") m_align = QStringLiteral("right");
        else if (cmd == u"lindent") {
            bool ok = false;
            const int v = arg.toInt(&ok);
            if (ok) m_indent = qMax(0, v);
        }
        else if (cmd == u"img") { // @{img "fichier"} : forme documentée pour AmigaOS 3.2 (V47)
            if (toks.size() > 1)
                out += imageTag(m_baseDir, toks[1].text);
        }
        else if (cmd == u"tab") out += QString(8, u' ');
        else if (cmd == u"line" || cmd == u"par") out += QStringLiteral("<br/>");
    }

    QString m_baseDir;
    bool m_b = false, m_i = false, m_u = false, m_open = false;
    QString m_fg, m_bg, m_align = QStringLiteral("left");
    int m_indent = 0;
};

} // namespace

bool parse(const QString &path, Guide &g, QString &error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        error = f.errorString();
        return false;
    }
    const QByteArray data = f.readAll();
    QStringDecoder dec(QStringConverter::Utf8);
    QString text = dec(data);
    const bool latin1 = dec.hasError();
    if (latin1)
        text = QString::fromLatin1(data); // encodage historique de l'Amiga

    g = Guide();
    g.latin1 = latin1;
    g.path = QFileInfo(path).absoluteFilePath();

    static const QRegularExpression reCmd(QStringLiteral("^@(\\$?[A-Za-z]+):?\\s*(.*)$"));
    static const QRegularExpression reNl(QStringLiteral("\r\n|\r|\n"));

    bool inNode = false;
    QString curKey;
    int lineNo = 0;

    for (const QString &raw : text.split(reNl)) {
        ++lineNo;
        const auto m = reCmd.match(raw);
        const QString cmd = m.hasMatch() ? m.captured(1).toLower() : QString();
        if (!knownCmds().contains(cmd)) {
            if (inNode)
                g.nodes[curKey].lines << raw;
            continue;
        }

        const QString arg = m.captured(2).trimmed();
        if (cmd == u"wordwrap" || cmd == u"smartwrap") {
            g.wrap = true;
        } else if (cmd == u"node") {
            const auto toks = splitTokens(arg);
            if (toks.isEmpty())
                continue;
            Node n;
            n.line = lineNo;
            n.name = toks[0].text;
            n.title = toks.size() > 1 ? toks[1].text : n.name;
            curKey = n.name.toLower();
            if (!g.nodes.contains(curKey))
                g.order << curKey;
            g.nodes.insert(curKey, n);
            inNode = true;
        } else if (cmd == u"endnode") {
            inNode = false;
        } else if (inNode) {
            Node &n = g.nodes[curKey];
            if (cmd == u"title") {
                QString t = arg;
                t.remove(u'"');
                n.title = t;
            } else if (cmd == u"toc") n.toc = firstToken(arg);
            else if (cmd == u"next") n.next = firstToken(arg);
            else if (cmd == u"prev") n.prev = firstToken(arg);
            else if (cmd == u"index") n.index = firstToken(arg);
            else if (cmd == u"help") n.help = firstToken(arg);
        } else {
            if (cmd == u"database") g.name = firstToken(arg);
            else if (cmd == u"index") g.index = firstToken(arg);
            else if (cmd == u"help") g.help = firstToken(arg);
        }
    }
    return true;
}

QString renderNode(const Node &node, const QString &baseDir)
{
    Renderer r(baseDir);
    QString html;
    for (const QString &line : node.lines)
        html += r.renderLine(line);
    return html;
}

QStringList plainText(const Node &node)
{
    QStringList out;
    for (const QString &line : node.lines) {
        QString s;
        const int n = line.size();
        int i = 0;
        while (i < n) {
            const QChar c = line[i];
            if (c == u'\\' && i + 1 < n && (line[i + 1] == u'@' || line[i + 1] == u'\\')) {
                s += line[i + 1];
                i += 2;
                continue;
            }
            if (c == u'@' && i + 1 < n && line[i + 1] == u'{') {
                const int end = findClose(line, i + 2);
                if (end != -1) {
                    const auto toks = splitTokens(line.mid(i + 2, end - i - 2));
                    const bool isImg = toks.size() > 1 && toks[1].text.compare(u"img", Qt::CaseInsensitive) == 0;
                    if (!toks.isEmpty() && toks[0].quoted && !isImg) // lien : on garde son libellé
                        s += toks[0].text;
                    i = end + 1;
                    continue;
                }
            }
            s += c;
            ++i;
        }
        out << s;
    }
    return out;
}

QVector<Link> links(const Node &node)
{
    QVector<Link> result;
    for (const QString &line : node.lines) {
        const int n = line.size();
        int i = 0;
        while (i < n) {
            if (line[i] == u'\\' && i + 1 < n && (line[i + 1] == u'@' || line[i + 1] == u'\\')) {
                i += 2;
                continue;
            }
            if (line[i] == u'@' && i + 1 < n && line[i + 1] == u'{') {
                const int end = findClose(line, i + 2);
                if (end != -1) {
                    const auto toks = splitTokens(line.mid(i + 2, end - i - 2));
                    if (!toks.isEmpty() && toks[0].quoted) {
                        result.push_back({toks[0].text,
                                          toks.size() > 1 ? toks[1].text.toLower() : QStringLiteral("link"),
                                          toks.size() > 2 ? toks[2].text : QString()});
                    }
                    i = end + 1;
                    continue;
                }
            }
            ++i;
        }
    }
    return result;
}

} // namespace AmigaGuide
