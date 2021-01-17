#pragma once
struct UserData;

struct SectionView {
    bool open;

    SectionView();
    void WriteConfig(UserData& config);
    void ReadConfig(strref config);
    void Draw();
};
