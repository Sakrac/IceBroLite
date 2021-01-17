#pragma once
struct UserData;

struct SymbolView {

    enum { kSearchFieldSize = 256, kContextSymbolSize = 64 };

    bool open;
    bool case_sensitive;
    uint32_t start, end;

    char searchField[kSearchFieldSize];
    char contextLabel[kContextSymbolSize];
    char startStr[32];
    char endStr[32];

    SymbolView();
    void WriteConfig(UserData& config);
    void ReadConfig(strref config);
    void Draw();
};
