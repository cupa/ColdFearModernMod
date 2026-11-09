#pragma once
struct PayloadFile {int id;const wchar_t* name;const char* hash;};
static constexpr PayloadFile steamPayload[]={
    {101,L"ColdFearThirdPerson.exe","6ac62cd7681ba7e89417ccb50b0ffeb9b59e87c324f550fef545b13b10ad0ef9"},
    {102,L"ColdFearModernControls.dll","4ecd89f944a6765c3578292dd8c9bf56ca0db5eb41491aafcc28c57935a8dcbe"},
    {103,L"dinput8.dll","0ad9d62da8c065a8fd9d3275fc75d700b33f40eb9b65e9495aa29a1948bb4568"},
    {104,L"Camera.ini","9d2e2954eab5b1924fc12cba582b6da4c3825a1d3e86afe6294304fef26bc18f"},
    {105,L"ColdFearDisplay.ini","16530f41698c4f14c9874ee683ecac4490aaa6c31f248746dd194798f00687b9"},
    {106,L"Enable.cmd","f09b37e5a4281296a32502f4f51c5a55982af2e346168ac94683ec4d053a9087"},
    {107,L"Disable.cmd","e8405bfffa85453c67b1ee8fdd0558b865754c854cd5ad4b88719486fa87ed08"},
    {108,L"Toggle.cmd","14e007f2df5f54d5e199d4ab18faf2def9efdd7fca40265ff337e5bc7fca20d7"},
    {109,L"Status.cmd","cdeabce72dd6d7e4ff9ceca9080f46a50bfd4e912597ef400de7bec3ba6bddd8"},
    {110,L"README.md","382440f6648fb93454063dd044f1170ac27201d2d2d7e17b7a736657a356621e"},
    {111,L"CONTROLS.md","02ea09af4186dcb21bd4f38b030a742eebfcbf139cf41f1452ceae414d1f8966"},
    {112,L"LICENSE.md","4151d2bb5907dcb6f09991229babe07cd5636596bc81bfd7aed3f79fbca1b36e"},
    {113,L"INSTALL.md","97ecc1b8529685a674b61f6240f103f228ec386b975c0048b1cdccc1a4f97860"},
    {114,L"Uninstall.exe","f4b93eca3cd5a5be5a4a16de78c1dd06baba0ac3e909a2f5df5f6084c855fa89"},
};
static constexpr PayloadFile gogPayload[]={
    {201,L"ColdFearThirdPerson.exe","6ac62cd7681ba7e89417ccb50b0ffeb9b59e87c324f550fef545b13b10ad0ef9"},
    {202,L"ColdFearModernControls.dll","4ecd89f944a6765c3578292dd8c9bf56ca0db5eb41491aafcc28c57935a8dcbe"},
    {203,L"d3d9.dll","7c2e8d63a9473900ec708420d7da8eae56b8e737bef3b9b0776230949f0078e5"},
    {204,L"Camera.ini","9d2e2954eab5b1924fc12cba582b6da4c3825a1d3e86afe6294304fef26bc18f"},
    {205,L"Enable.cmd","f09b37e5a4281296a32502f4f51c5a55982af2e346168ac94683ec4d053a9087"},
    {206,L"Disable.cmd","e8405bfffa85453c67b1ee8fdd0558b865754c854cd5ad4b88719486fa87ed08"},
    {207,L"Toggle.cmd","14e007f2df5f54d5e199d4ab18faf2def9efdd7fca40265ff337e5bc7fca20d7"},
    {208,L"Status.cmd","cdeabce72dd6d7e4ff9ceca9080f46a50bfd4e912597ef400de7bec3ba6bddd8"},
    {209,L"README.md","382440f6648fb93454063dd044f1170ac27201d2d2d7e17b7a736657a356621e"},
    {210,L"CONTROLS.md","02ea09af4186dcb21bd4f38b030a742eebfcbf139cf41f1452ceae414d1f8966"},
    {211,L"LICENSE.md","4151d2bb5907dcb6f09991229babe07cd5636596bc81bfd7aed3f79fbca1b36e"},
    {212,L"INSTALL.md","97ecc1b8529685a674b61f6240f103f228ec386b975c0048b1cdccc1a4f97860"},
    {213,L"Uninstall.exe","f4b93eca3cd5a5be5a4a16de78c1dd06baba0ac3e909a2f5df5f6084c855fa89"},
};
static constexpr const char* knownProxies[]={
    "0ad9d62da8c065a8fd9d3275fc75d700b33f40eb9b65e9495aa29a1948bb4568",
    "f4763d63b1ca672f792509c3e9d84a9e41c94673aa988022f9d05c295ea003d3",
    "8b8800b12a1a808e5392bd8537a766428c4c15cf55d9e19a88820c0965e6fdc9",
    "c4678759f86e3f93ed07367a0782770fc77261b8629575a22d648722a0c5223c",
    "2ae3afc958d16ef6ae9010af9ebf801587e81744599851110a94ebd4655d781b",
    "d0010f078616b0d8c4fe82f389818f7f2436929b8c8a15a164a255f7807f1b39",
    "ad897be3c24be9256b2bdfc6a01ae2fe0a00e4a5c2ff3237fa5b56e8c930e505",
    "f067bff4edbf882b62fbca0be9ad5b956b136f9585e10e7ef2c06fd1271890c7",
};
