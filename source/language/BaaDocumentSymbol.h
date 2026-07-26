#pragma once

#include <QString>
#include <QVector>

struct BaaDocumentSymbol
{
    QString name;
    QString detail;
    int kind{};
    int line{1};
    int column{1};
    int endLine{1};
    int endColumn{1};
    QVector<BaaDocumentSymbol> children;

    bool isValid() const { return not name.isEmpty() and line > 0 and column > 0; }
};
