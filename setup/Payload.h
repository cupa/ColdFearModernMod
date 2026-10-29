#pragma once
struct PayloadFile {int id;const wchar_t* name;const char* hash;};
static constexpr PayloadFile payload[]={
    {101,L"ColdFearThirdPerson.exe","74a680584f907ee6474743bfe2d05d4f04a01564e23bdb7b4b206cd66d552f1c"},
    {102,L"ColdFearModernControls.dll","2f3feae7c20d999328f24ed2ad9f181d597bc3ff661ab688ab45c506c765bd33"},
    {103,L"dinput8.dll","d0010f078616b0d8c4fe82f389818f7f2436929b8c8a15a164a255f7807f1b39"},
    {104,L"Camera.ini","9864d3a8721d7ae76c94514be926ea73dd7b690cff5c5057fec45b2150343d45"},
    {105,L"ColdFearDisplay.ini","9e6d22ccd49ae0f80dd94c86c1dd9f2caadbe4bbb4f403ec51fe376dee103d77"},
    {106,L"Enable.cmd","4882430de399ada0a1b803db40a3ad7038262875c858dc3ec495a19df835e3cb"},
    {107,L"Disable.cmd","77bb2f8b94e766da71118e7da51f0c6c3311d9957e258db9ae1c9f7c86dc09f2"},
    {108,L"Toggle.cmd","5d0d98ea5b4e8c51c65a39fc4c5edd9bc18daa725336a2a1eb28f0d4887da231"},
    {109,L"Status.cmd","634ab0391a072e16053f8947930c7039293b2bb3aee5e3b14537950e2bb4e9fe"},
    {110,L"README.md","f8d193b67758010cb0469f0b9fd85fceb180a321641dc7344e5b1760e178df0b"},
    {111,L"CONTROLS.md","02ea09af4186dcb21bd4f38b030a742eebfcbf139cf41f1452ceae414d1f8966"},
    {112,L"LICENSE.md","4151d2bb5907dcb6f09991229babe07cd5636596bc81bfd7aed3f79fbca1b36e"},
    {113,L"INSTALL.md","8e8a61bf42e6077d7b303d7b69cf7a1d2ade49074b321e9f337db48a81b2f8e0"},
    {114,L"Uninstall.exe","68f2acf5d07303d8f151dc636626ab62939e88542e1acfd386520a232621c00a"},
};
static constexpr const char* knownProxies[]={
    "d0010f078616b0d8c4fe82f389818f7f2436929b8c8a15a164a255f7807f1b39",
    "ad897be3c24be9256b2bdfc6a01ae2fe0a00e4a5c2ff3237fa5b56e8c930e505",
    "f067bff4edbf882b62fbca0be9ad5b956b136f9585e10e7ef2c06fd1271890c7",
};
