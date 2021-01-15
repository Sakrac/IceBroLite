#pragma once
struct UserData;

struct SymbolView {
    bool open;

    SymbolView();
    void WriteConfig(UserData& config);
    void ReadConfig(strref config);
    void Draw();
};
