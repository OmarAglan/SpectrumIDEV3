#include "QalamSyntaxDefinition.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ==================== Language Definition ====================

LanguageDefinition::LanguageDefinition() {
    if (!loadFromJson(":/lang/resources/baa-language.json")) {
        loadDefaults();
    }
    buildSets();
}

bool LanguageDefinition::loadFromJson(const QString &resourcePath) {
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError or !doc.isObject()) return false;

    QJsonObject root = doc.object();

    // Keywords
    for (const QJsonValue &v : root["keywords"].toArray())
        keywordList << v.toString();

    // Builtins
    for (const QJsonValue &v : root["builtins"].toArray())
        builtinList << v.toString();

    // Preprocessors
    for (const QJsonValue &v : root["preprocessors"].toArray())
        preprocessorList << v.toString();

    // Magics (optional)
    QStringList magics;
    for (const QJsonValue &v : root["magics"].toArray())
        magics << v.toString();
    magicSet = QSet<QString>(magics.begin(), magics.end());

    // Regex patterns
    QJsonObject patterns = root["patterns"].toObject();
    hexPattern = QRegularExpression(patterns["hex"].toString(R"(\b0[xX][0-9a-fA-F]+\b)"));
    numberPattern = QRegularExpression(patterns["number"].toString(R"(\b[\d٠-٩]+\b)"));

    return !keywordList.isEmpty(); // sanity check
}

void LanguageDefinition::buildSets() {
    keywordSet = QSet<QString>(keywordList.begin(), keywordList.end());
    builtinSet = QSet<QString>(builtinList.begin(), builtinList.end());
    preprocessorSet = QSet<QString>(preprocessorList.begin(), preprocessorList.end());
}

void LanguageDefinition::loadDefaults() {
    keywordList = {
        // Types (§3.1)
        "صحيح", "نص", "منطقي",
        // Constants (§4)
        "ثابت",
        // Boolean literals (§3.1)
        "صواب", "خطأ",
        // Control flow (§7)
        "إذا", "وإلا", "طالما", "لكل",
        // Switch (§7.5)
        "اختر", "حالة", "افتراضي",
        // Loop control (§7.4)
        "توقف", "استمر",
        // Functions (§5)
        "إرجع",
        // Logical Operators (§8)
        "و", "أو", "ليس",
        // Entry point
        "الرئيسية"
    };

    builtinList = {
        // I/O (§6)
        "اطبع", "اقرأ"
    };

    preprocessorList = {
        "#تضمين",      // Include (§2.1)
        "#تعريف",      // Define (§2.2)
        "#الغاء_تعريف", // Undefine (§2.4)
        "#إذا_عرف",    // Ifdef (§2.3)
        "#وإلا",       // Else (§2.3)
        "#نهاية"       // Endif (§2.3)
    };

    hexPattern = QRegularExpression(R"(\b0[xX][0-9a-fA-F]+\b)");
    numberPattern = QRegularExpression(R"(\b[\d٠-٩]+\b)");
}

const LanguageDefinition& LanguageDefinition::instance() {
    static const LanguageDefinition def;
    return def;
}
