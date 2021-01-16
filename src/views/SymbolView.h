#pragma once
struct UserData;

struct SymbolView {

    enum { kSearchFieldSize = 256 };

    bool open;
    bool case_sensitive;
    int selected_row;

    char searchField[kSearchFieldSize];

    SymbolView();
    void WriteConfig(UserData& config);
    void ReadConfig(strref config);
    void Draw();
};
