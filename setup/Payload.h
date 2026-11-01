#pragma once
struct PayloadFile {int id;const wchar_t* name;const char* hash;};
static constexpr PayloadFile steamPayload[]={
    {101,L"ColdFearModern.exe","b9e9dfc262d8df33a40db411fdffcdc5c86fa7c7fd69e29eb34601bfcc3df0b4"},
    {102,L"ColdFearThirdPerson.exe","eb4af050824f97e11a830afe0f710c0494f65e87888bb632e854f0d6f2ccef3e"},
    {103,L"ColdFearModernControls.dll","bac5dbb0d7bafda660241bc48950c37c44460085e85488db8559827eba5658b9"},
    {104,L"dinput8.dll","c4678759f86e3f93ed07367a0782770fc77261b8629575a22d648722a0c5223c"},
    {105,L"Camera.ini","9864d3a8721d7ae76c94514be926ea73dd7b690cff5c5057fec45b2150343d45"},
    {106,L"ColdFearDisplay.ini","1175d31ae4da01bcaf7a1d0a8a3d5bca78500c97fd76c6cabb189d124d425f7a"},
    {107,L"Enable.cmd","f09b37e5a4281296a32502f4f51c5a55982af2e346168ac94683ec4d053a9087"},
    {108,L"Disable.cmd","e8405bfffa85453c67b1ee8fdd0558b865754c854cd5ad4b88719486fa87ed08"},
    {109,L"Toggle.cmd","14e007f2df5f54d5e199d4ab18faf2def9efdd7fca40265ff337e5bc7fca20d7"},
    {110,L"Status.cmd","cdeabce72dd6d7e4ff9ceca9080f46a50bfd4e912597ef400de7bec3ba6bddd8"},
    {111,L"README.md","c485a0b0deb789ca796fc29ff4909476df683bdc5dc6d8335fc64ad79f44712b"},
    {112,L"CONTROLS.md","02ea09af4186dcb21bd4f38b030a742eebfcbf139cf41f1452ceae414d1f8966"},
    {113,L"LICENSE.md","4151d2bb5907dcb6f09991229babe07cd5636596bc81bfd7aed3f79fbca1b36e"},
    {114,L"INSTALL.md","c5f650bd2f4fad2f0b6de4b79d562759c7eb30154a6eeb0281690ae4639697bc"},
    {115,L"Uninstall.exe","903551412616a041af251d09511edae38ee622aea02ba548ce5eadf50312152b"},
};
static constexpr PayloadFile gogPayload[]={
    {201,L"ColdFearModern.exe","b9e9dfc262d8df33a40db411fdffcdc5c86fa7c7fd69e29eb34601bfcc3df0b4"},
    {202,L"ColdFearThirdPerson.exe","eb4af050824f97e11a830afe0f710c0494f65e87888bb632e854f0d6f2ccef3e"},
    {203,L"ColdFearModernControls.dll","bac5dbb0d7bafda660241bc48950c37c44460085e85488db8559827eba5658b9"},
    {204,L"Camera.ini","9864d3a8721d7ae76c94514be926ea73dd7b690cff5c5057fec45b2150343d45"},
    {205,L"Enable.cmd","f09b37e5a4281296a32502f4f51c5a55982af2e346168ac94683ec4d053a9087"},
    {206,L"Disable.cmd","e8405bfffa85453c67b1ee8fdd0558b865754c854cd5ad4b88719486fa87ed08"},
    {207,L"Toggle.cmd","14e007f2df5f54d5e199d4ab18faf2def9efdd7fca40265ff337e5bc7fca20d7"},
    {208,L"Status.cmd","cdeabce72dd6d7e4ff9ceca9080f46a50bfd4e912597ef400de7bec3ba6bddd8"},
    {209,L"README.md","c485a0b0deb789ca796fc29ff4909476df683bdc5dc6d8335fc64ad79f44712b"},
    {210,L"CONTROLS.md","02ea09af4186dcb21bd4f38b030a742eebfcbf139cf41f1452ceae414d1f8966"},
    {211,L"LICENSE.md","4151d2bb5907dcb6f09991229babe07cd5636596bc81bfd7aed3f79fbca1b36e"},
    {212,L"INSTALL.md","c5f650bd2f4fad2f0b6de4b79d562759c7eb30154a6eeb0281690ae4639697bc"},
    {213,L"Uninstall.exe","903551412616a041af251d09511edae38ee622aea02ba548ce5eadf50312152b"},
};
static constexpr const char* knownProxies[]={
    "c4678759f86e3f93ed07367a0782770fc77261b8629575a22d648722a0c5223c",
    "2ae3afc958d16ef6ae9010af9ebf801587e81744599851110a94ebd4655d781b",
    "d0010f078616b0d8c4fe82f389818f7f2436929b8c8a15a164a255f7807f1b39",
    "ad897be3c24be9256b2bdfc6a01ae2fe0a00e4a5c2ff3237fa5b56e8c930e505",
    "f067bff4edbf882b62fbca0be9ad5b956b136f9585e10e7ef2c06fd1271890c7",
};
