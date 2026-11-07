// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct Node {
    QString name, title, toc, next, prev, index, help;
    QStringList lines;
    int line = 0; // numéro (à partir de 1) de la ligne @node dans le fichier
};

struct Guide {
    QString path;
    QString name;
    QHash<QString, Node> nodes; // clé = nom en minuscules
    QStringList order;
    QString index, help;
    bool wrap = false;
    bool latin1 = false; // fichier non UTF-8 : lu en ISO-8859-1

    QString title() const;
    QString startKey() const;
};

namespace AmigaGuide {

struct Token {
    bool quoted;
    QString text;
};

QVector<Token> splitTokens(const QString &s);
bool parse(const QString &path, Guide &out, QString &error);
// baseDir : dossier du fichier .guide, pour retrouver les images (@{img ...})
QString renderNode(const Node &node, const QString &baseDir = QString());
QStringList plainText(const Node &node); // une entrée par ligne, sans les commandes @{...}

struct Link {
    QString label, kind, target;
};
QVector<Link> links(const Node &node); // liens @{"texte" link cible} dans l'ordre du nœud

} // namespace AmigaGuide
