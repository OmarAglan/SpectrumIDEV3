#pragma once

struct BaaSelectionRange
{
    int line{-1};
    int character{-1};
    int endLine{-1};
    int endCharacter{-1};

    bool isValid() const
    {
        return line >= 0 and character >= 0 and endLine >= line and
               endCharacter >= 0 and
               (endLine > line or endCharacter >= character);
    }
};
