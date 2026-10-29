#pragma once
#include <xinput.h>
#include <cwchar>
// Read-only Sony HID; layouts follow hid-playstation documentation.
namespace PlayStation {
static inline SHORT axis(BYTE v,bool invert=false) {
    int n=static_cast<int>(v)-128;
    int scaled=n<0?n*256:n*32767/127;
    if(invert)scaled=scaled==-32768?32767:-scaled;
    return static_cast<SHORT>(scaled);
}
static inline bool decode(USHORT product,const BYTE* report,size_t length,XINPUT_STATE& result) {
    bool ds=product==0x0ce6||product==0x0df2;
    if(!ds&&product!=0x05c4&&product!=0x09cc)return false;
    if(length==0)return false;
    size_t start=0;bool simple=false;
    if(report[0]==1) {
        if(ds&&length==64)start=1;
        else if((!ds&&length==64)||length==10){start=1;simple=true;}
        else return false;
    } else if(length==78&&((ds&&report[0]==0x31)||(!ds&&report[0]==0x11))) {
        // Bluetooth CRC includes the 0xA1 prefix.
        DWORD crc=0xffffffff;
        for(size_t i=0;i<length-3;++i) {
            crc^=i==0?0xa1:report[i-1];
            for(int bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320:0);
        }
        DWORD expected=0;memcpy(&expected,report+length-4,4);
        if(~crc!=expected)return false;
        start=ds?2:3;simple=!ds;
    } else return false;
    const BYTE* p=report+start;
    size_t buttons=simple?4:7,trigger=simple?7:4;
    XINPUT_STATE next{};auto& g=next.Gamepad;
    g.sThumbLX=axis(p[0]);g.sThumbLY=axis(p[1],true);
    g.sThumbRX=axis(p[2]);g.sThumbRY=axis(p[3],true);
    g.bLeftTrigger=p[trigger];g.bRightTrigger=p[trigger+1];
    BYTE a=p[buttons],b=p[buttons+1];
    static constexpr WORD hats[]={1,1|8,8,8|2,2,2|4,4,4|1};
    if((a&15)<8)g.wButtons|=hats[a&15];
    if(a&0x10)g.wButtons|=XINPUT_GAMEPAD_X;
    if(a&0x20)g.wButtons|=XINPUT_GAMEPAD_A;
    if(a&0x40)g.wButtons|=XINPUT_GAMEPAD_B;
    if(a&0x80)g.wButtons|=XINPUT_GAMEPAD_Y;
    if(b&1)g.wButtons|=XINPUT_GAMEPAD_LEFT_SHOULDER;
    if(b&2)g.wButtons|=XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if(b&0x10)g.wButtons|=XINPUT_GAMEPAD_BACK;
    if(b&0x20)g.wButtons|=XINPUT_GAMEPAD_START;
    if(b&0x40)g.wButtons|=XINPUT_GAMEPAD_LEFT_THUMB;
    if(b&0x80)g.wButtons|=XINPUT_GAMEPAD_RIGHT_THUMB;
    if(p[buttons+2]&2)g.wButtons|=XINPUT_GAMEPAD_BACK; // Touchpad or Create/Share.
    result=next;return true;
}
}
#ifndef CONTROLS_TEST
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#pragma comment(lib,"setupapi.lib")
#pragma comment(lib,"hid.lib")
namespace PlayStation {
static SRWLOCK lock=SRWLOCK_INIT;
static XINPUT_STATE sample{};
static DWORD updated=0;
static DWORD sequence=0;
static USHORT detectedProduct=0;
static volatile LONG requested=0;
static HANDLE openDevice(USHORT& product,USHORT& reportSize) {
    GUID guid{};HidD_GetHidGuid(&guid);
    HDEVINFO devices=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(devices==INVALID_HANDLE_VALUE)return INVALID_HANDLE_VALUE;
    HANDLE found=INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA entry{};entry.cbSize=sizeof(entry);
    for(DWORD index=0;SetupDiEnumDeviceInterfaces(devices,nullptr,&guid,index,&entry);++index) {
        DWORD required=0;SetupDiGetDeviceInterfaceDetailW(devices,&entry,nullptr,0,&required,nullptr);
        if(required<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)||required>65536)continue;
        auto detail=static_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(HeapAlloc(GetProcessHeap(),0,required));
        if(!detail)continue;
        detail->cbSize=sizeof(*detail);
        HANDLE device=INVALID_HANDLE_VALUE;
        if(SetupDiGetDeviceInterfaceDetailW(devices,&entry,detail,required,nullptr,nullptr)) {
            // Open supported Sony HID devices only.
            CharLowerBuffW(detail->DevicePath,static_cast<DWORD>(wcslen(detail->DevicePath)));
            bool sony=wcsstr(detail->DevicePath,L"vid_054c")||wcsstr(detail->DevicePath,L"vid&0002054c");
            bool pad=wcsstr(detail->DevicePath,L"pid_0ce6")||wcsstr(detail->DevicePath,L"pid_0df2")||
                wcsstr(detail->DevicePath,L"pid_05c4")||wcsstr(detail->DevicePath,L"pid_09cc")||
                wcsstr(detail->DevicePath,L"pid&0ce6")||wcsstr(detail->DevicePath,L"pid&0df2")||
                wcsstr(detail->DevicePath,L"pid&05c4")||wcsstr(detail->DevicePath,L"pid&09cc");
            if(sony&&pad)
                device=CreateFileW(detail->DevicePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
        }
        HeapFree(GetProcessHeap(),0,detail);
        if(device==INVALID_HANDLE_VALUE)continue;
        HIDD_ATTRIBUTES attr{};attr.Size=sizeof(attr);
        bool supported=HidD_GetAttributes(device,&attr)&&attr.VendorID==0x054c&&
            (attr.ProductID==0x0ce6||attr.ProductID==0x0df2||attr.ProductID==0x05c4||attr.ProductID==0x09cc);
        PHIDP_PREPARSED_DATA prep=nullptr;HIDP_CAPS caps{};
        bool gamepad=false;
        if(supported&&HidD_GetPreparsedData(device,&prep)) {
            gamepad=HidP_GetCaps(prep,&caps)==HIDP_STATUS_SUCCESS&&caps.UsagePage==1&&
                (caps.Usage==4||caps.Usage==5)&&caps.InputReportByteLength>=10&&caps.InputReportByteLength<=128;
            HidD_FreePreparsedData(prep);
        }
        if(gamepad){product=attr.ProductID;reportSize=caps.InputReportByteLength;found=device;break;}
        CloseHandle(device);
    }
    SetupDiDestroyDeviceInfoList(devices);return found;
}
static void publish(const XINPUT_STATE* value,USHORT product=0) {
    AcquireSRWLockExclusive(&lock);
    if(value){sample=*value;sample.dwPacketNumber=++sequence;updated=GetTickCount();detectedProduct=product;}
    else {updated=0;sample={};detectedProduct=0;}
    ReleaseSRWLockExclusive(&lock);
}
static DWORD WINAPI worker(void*) {
    // Keep HID I/O off the game input thread.
    HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!event)return 0;
    for(;;) {
        if(!InterlockedCompareExchange(&requested,0,0)){Sleep(100);continue;}
        USHORT product=0,size=0;HANDLE device=openDevice(product,size);
        if(device==INVALID_HANDLE_VALUE){Sleep(1000);continue;}
        bool healthy=true;
        while(healthy&&InterlockedCompareExchange(&requested,0,0)) {
            BYTE bytes[128]{};OVERLAPPED io{};io.hEvent=event;ResetEvent(event);DWORD count=0;
            BOOL complete=ReadFile(device,bytes,size,&count,&io);
            if(!complete&&GetLastError()==ERROR_IO_PENDING) {
                DWORD waited=WAIT_TIMEOUT;
                while(waited==WAIT_TIMEOUT&&InterlockedCompareExchange(&requested,0,0))waited=WaitForSingleObject(event,100);
                if(waited!=WAIT_OBJECT_0)CancelIoEx(device,&io);
                complete=GetOverlappedResult(device,&io,&count,TRUE);
            }
            if(!complete){healthy=false;break;}
            XINPUT_STATE value{};
            if(decode(product,bytes,count,value))publish(&value,product);
        }
        publish(nullptr);CloseHandle(device);
    }
}
static void enable(bool value) {
    static bool started=false;
    InterlockedExchange(&requested,value?1:0);
    if(value&&!started) {
        HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
        if(thread){started=true;CloseHandle(thread);}
    }
}
static bool read(XINPUT_STATE& result) {
    AcquireSRWLockShared(&lock);
    bool valid=updated&&GetTickCount()-updated<500;
    if(valid)result=sample;
    ReleaseSRWLockShared(&lock);return valid;
}
}
#else
namespace PlayStation {
static bool connected=false;
static XINPUT_STATE sample{};
static void enable(bool){}
static bool read(XINPUT_STATE& result){if(!connected)return false;result=sample;return true;}
}
#endif
