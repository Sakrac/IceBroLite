#pragma once
struct UserData;

struct SymbolView {

    enum { kSearchFieldSize = 256, kContextSymbolSize = 64 };

    bool open;
    bool case_sensitive;
    int selected_row;
    int context_row;
    uint32_t context_address;

    char searchField[kSearchFieldSize];
    char contextLabel[kContextSymbolSize];

    SymbolView();
    void WriteConfig(UserData& config);
    void ReadConfig(strref config);
    void Draw();
};
