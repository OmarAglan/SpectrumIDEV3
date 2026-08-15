#pragma once
#include "QalamSyntaxThemes.h"
#include <QVector>
#include <memory>

class ThemeManager {
public:
    static const QVector<std::shared_ptr<SyntaxTheme>>& getAvailableThemes() {
        static const QVector<std::shared_ptr<SyntaxTheme>> themes = {
            std::make_shared<VSCodeDarkTheme>(),
            std::make_shared<MonokaiTheme>(),
            std::make_shared<OceanicTheme>(),
            std::make_shared<QalamGlowTheme>()
        };
        return themes;
    }
    
    static std::shared_ptr<SyntaxTheme> getThemeByIndex(int index) {
        const auto& themes = getAvailableThemes();
        if (index >= 0 && index < themes.size()) {
            return themes.at(index);
        }
        return themes.at(0); // Default theme
    }
};
