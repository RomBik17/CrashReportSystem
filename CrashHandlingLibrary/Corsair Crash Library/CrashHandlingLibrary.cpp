#include <iostream>
#include <fstream>
#include <stdio.h>
#include <intrin.h>
#include <windows.h>
#include <Dbghelp.h>
#include <string>
#include <functional>
#include <tlhelp32.h> 
#include <tchar.h> 
#include <winver.h> 
#include <vector> 
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <mmdeviceapi.h>
#include <filesystem>
#include <iomanip>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "user32.lib")
#pragma intrinsic(_ReturnAddress)
#pragma comment(lib, "IPHLPAPI.lib")

#define _WIN32_DCOM
#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
#define A_PRIME 141224297 
#define B_PRIME 9216973
#define C_PRIME 422857 
#define FIRSTH  1531

const int MaxNameLen = 256;
const int DumpSymbolsToRead = 1000;

const std::string Path_For_Dumps = "C:/MiniDumps/";

struct Process_Module {
    std::string Name;
    std::string File_Version;
    std::string Product_Version;
};

extern "C" {
    __declspec(dllexport) int Parse_Exception(
        _EXCEPTION_POINTERS* Exception
    );
}
bool List_Process_Modules(DWORD dwPID, std::vector<Process_Module>& Module_List);
void Print_Error(TCHAR* msg);

std::string Parse_Call_Stack(CONTEXT* ctx) {
    
    std::string Call_Stack_String = "";

    BOOL    result;
    HANDLE  process;
    HANDLE  thread;
    HMODULE hModule;

    STACKFRAME64        stack;
    ULONG               frame;
    DWORD64             displacement;

    DWORD disp;
    IMAGEHLP_LINE64* line;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    char Name[MaxNameLen];
    char module[MaxNameLen];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    memset(&stack, 0, sizeof(STACKFRAME64));

    process = GetCurrentProcess();
    thread = GetCurrentThread();
    displacement = 0;
#if !defined(_M_AMD64)
    stack.AddrPC.Offset = (*ctx).Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = (*ctx).Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = (*ctx).Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;
#endif

    SymInitialize(process, NULL, TRUE); //load symbols

    for (frame = 0; ; frame++) {
        //get next call from stack
        result = StackWalk64
        (
#if defined(_M_AMD64)
            IMAGE_FILE_MACHINE_AMD64
#else
            IMAGE_FILE_MACHINE_I386
#endif
            ,
            process,
            thread,
            &stack,
            ctx,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if (!result) break;

        //get symbol name for address
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        SymFromAddr(process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol);

        line = (IMAGEHLP_LINE64*)malloc(sizeof(IMAGEHLP_LINE64));
        line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        //try to get line
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line)) {
            Call_Stack_String += (std::string(pSymbol->Name) + ' ' + std::to_string(line->LineNumber) + ' ');
        }
        else {
            //failed to get line
            Call_Stack_String += (std::string(pSymbol->Name) + ' ');
            hModule = NULL;
            lstrcpyA(module, "");
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCTSTR)(stack.AddrPC.Offset), &hModule);
        }

        free(line);
        line = NULL;

        if (std::string(pSymbol->Name) == "main") break;
    }
    
    return Call_Stack_String;
}

int Make_MiniDump(_EXCEPTION_POINTERS* Exception, 
    std::string Hash, SYSTEMTIME& Dump_Creation_Time,
    SYSTEMTIME& Dump_Modification_Time) {

    auto hDbgHelp = LoadLibraryA("dbghelp");
    if (hDbgHelp == nullptr) return -1;

    auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (pMiniDumpWriteDump == nullptr) return -1;

    std::string Dump_Name = Path_For_Dumps + Hash + ".dmp";

    FILETIME ftCreate;

    auto Dump_File = CreateFileA(Dump_Name.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (Dump_File == INVALID_HANDLE_VALUE) return -1;

    if (!GetFileTime(Dump_File, &ftCreate, NULL, NULL)) {
        printf("Something wrong!\n");
        return -1;
    }

    FileTimeToSystemTime(&ftCreate, &Dump_Creation_Time);
    GetSystemTime(&Dump_Modification_Time);

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = Exception;
    exceptionInfo.ClientPointers = NULL;

    auto dumped = pMiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        Dump_File,
        MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
        Exception ? &exceptionInfo : nullptr,
        nullptr,
        nullptr);

    CloseHandle(Dump_File);

    return EXCEPTION_EXECUTE_HANDLER;
}

OSVERSIONINFOEXW Get_OS_Version() {

    NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW OS_Info;

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    if (NULL != RtlGetVersion)
    {
        OS_Info.dwOSVersionInfoSize = sizeof(OS_Info);
        RtlGetVersion(&OS_Info);
    }
    return OS_Info;
}

unsigned long long Hash_String(const std::string& String_to_Hash) {
   
    unsigned long long Hash = FIRSTH;
    for (int i = 0; i < String_to_Hash.size(); i ++) {
        Hash = (Hash * A_PRIME) ^ ((long long)String_to_Hash[i] * B_PRIME) + 
            (C_PRIME / (long long)String_to_Hash[i]);
    }

    return Hash; 
}

bool Save_Hardware_Info(std::ofstream& Info_File) {

    Info_File << "\n\nHardware Info:\n";

    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    for (i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }

    Info_File << "\tCPU Type: " << CPUBrandString << '\n';


    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    Info_File << "\tNumber of Cores: " << sysInfo.dwNumberOfProcessors << '\n';

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    Info_File << "\tTotal System Memory: " << (statex.ullTotalPhys / 1024) / 1024 << "MB" << '\n';


    //get info about GPU
    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return 0;
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();
        return false;
    }
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"root\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return false;
    }
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 0;
    }
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn)break;
        VARIANT vtProp;
        hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
        std::wstring ws(vtProp.bstrVal);
        Info_File << "\tGPU Type: " << std::string(ws.begin(), ws.end()) << '\n';
        VariantClear(&vtProp);
    }
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    pclsObj->Release();
    CoUninitialize();

    return true;
}

bool Save_Adapter_Info(std::ofstream& Info_File) {
    
    Info_File << "\n\nAdapters:\n";

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    UINT i;

    struct tm newtime;
    char buffer[32];
    errno_t error;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return false;
    }
    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        FREE(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)MALLOC(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return 1;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        if (!pAdapter) return false;

        Info_File << std::setw(10) << ' ' << std::setw(30) << std::left << "Type" << 
            std::setw(40) << std::left << "Name" << '\n';
        int index = 1;
        while (pAdapter) {
            Info_File << std::setw(10) << std::to_string(index++) + "." << std::setw(30);

            switch (pAdapter->Type) {
            case MIB_IF_TYPE_OTHER:
                Info_File << "Other";
                break;
            case MIB_IF_TYPE_ETHERNET:
                Info_File << "Ethernet";
                break;
            case MIB_IF_TYPE_TOKENRING:
                Info_File << "Token Ring";
                break;
            case MIB_IF_TYPE_FDDI:
                Info_File << "FDDI";
                break;
            case MIB_IF_TYPE_PPP:
                Info_File << "PPP";
                break;
            case MIB_IF_TYPE_LOOPBACK:
                Info_File << "Lookback";
                break;
            case MIB_IF_TYPE_SLIP:
                Info_File << "Slip";
                break;
            default:
                Info_File << "Unknown type " + std::to_string(pAdapter->Type);
                break;
            }

            Info_File << std::setw(30) << std::left << pAdapter->Description << '\n';
            pAdapter = pAdapter->Next;
        }
    }
    else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
        return false;
    }
    if (pAdapterInfo)
        FREE(pAdapterInfo);

    return true;
}

extern "C" {
    __declspec(dllexport) int Parse_Exception(_EXCEPTION_POINTERS* Exception) {

        SYSTEMTIME System_T, Local_T;
        GetSystemTime(&System_T);
        GetLocalTime(&Local_T);

        if (!std::filesystem::exists(Path_For_Dumps)) {
            std::cout << "\nNot Exists\n";
            std::filesystem::create_directory(Path_For_Dumps);
        }

        OSVERSIONINFOEXW OS_Info = Get_OS_Version();

        if (OS_Info.dwMajorVersion != 10) return -1;

        std::string String_to_Hash = "";

        String_to_Hash += (std::to_string(OS_Info.dwMajorVersion) + ' ');
        String_to_Hash += (std::to_string(OS_Info.dwMinorVersion) + ' ');
        String_to_Hash += (std::to_string(OS_Info.dwBuildNumber) + ' ');

        String_to_Hash += Parse_Call_Stack(Exception->ContextRecord);

        std::vector<Process_Module> Module_List;
        List_Process_Modules(GetCurrentProcessId(), Module_List);

        String_to_Hash += (Module_List[0].File_Version + " " + Module_List[0].Product_Version);

        unsigned long long Hash = Hash_String(String_to_Hash);

        std::ofstream Info_File(Path_For_Dumps + std::to_string(Hash) + ".txt");

        Info_File << "Time of Crash:\n";
        Info_File << "\tSystem: " << System_T.wYear << ' ' << System_T.wMonth << ' ' << System_T.wDay << ' ' <<
            System_T.wHour << ' ' << System_T.wMinute << ' ' << System_T.wSecond << '\n';
        Info_File << "\tLocal:  " << Local_T.wYear << ' ' << Local_T.wMonth << ' ' << Local_T.wDay << ' ' <<
            Local_T.wHour << ' ' << Local_T.wMinute << ' ' << Local_T.wSecond << "\n\n\n";

        if (Module_List.size() > 0) {
            Info_File << "List of Process Modules:\n";
            using std::setw;
            using std::left;
            Info_File << setw(10) << ' ' << setw(40) << left << "Name" << setw(20) << left << "File Version" <<
                setw(20) << left << "Product Version" << "\n";
            for (int i = 0; i < Module_List.size(); i++) {
                Info_File << setw(10) << left << std::to_string(i + 1) + '.' << setw(40) << left <<
                    Module_List[i].Name << setw(20) << left << Module_List[i].File_Version << setw(20) << left <<
                    Module_List[i].Product_Version << "\n";

            }
        }

        Save_Hardware_Info(Info_File);
        Save_Adapter_Info(Info_File);
        
        SYSTEMTIME Dump_Creation_Time;
        SYSTEMTIME Dump_Modification_Time;
        
        Make_MiniDump(Exception, std::to_string(Hash), Dump_Creation_Time, Dump_Modification_Time);

        std::string Dump_Key_String = "";
        Dump_Key_String += (std::to_string(Dump_Creation_Time.wYear) + std::to_string(Dump_Creation_Time.wMonth) +
            std::to_string(Dump_Creation_Time.wDay) + std::to_string(Dump_Creation_Time.wHour) +
            std::to_string(Dump_Creation_Time.wMinute) + std::to_string(Dump_Creation_Time.wSecond) + " ");

        Dump_Key_String += (std::to_string(Dump_Modification_Time.wYear) + std::to_string(Dump_Modification_Time.wMonth) +
            std::to_string(Dump_Modification_Time.wDay) + std::to_string(Dump_Modification_Time.wHour) +
            std::to_string(Dump_Modification_Time.wMinute) + std::to_string(Dump_Modification_Time.wSecond) + " ");

        std::ifstream Dump_File(Path_For_Dumps + std::to_string(Hash) + ".dmp", 
            std::ifstream::ate | std::ifstream::binary);
        
        int Dump_Size = Dump_File.tellg();
        Dump_Key_String += (std::to_string(Dump_Size) + " ");

        Dump_File.seekg(0, std::ios::beg);

        char* Dump_Symbols_Buffer = new char[DumpSymbolsToRead];
        Dump_File.read(Dump_Symbols_Buffer, DumpSymbolsToRead);
        for (int i = 0; i < DumpSymbolsToRead; i++) {
            Dump_Key_String += std::to_string((int)Dump_Symbols_Buffer[i]);
        }
        delete[] Dump_Symbols_Buffer;

        unsigned long long Dump_Key_Value = Hash_String(Dump_Key_String);
        Dump_File.close();

        Info_File << "\n\nDump Key Value: " << Dump_Key_Value;
        Info_File.close();
        
        return TRUE;
    }
}

bool Get_Module_Versions(TCHAR* pszFilePath, Process_Module& Module) {
    
    DWORD               dwSize = 0;
    BYTE*               pbVersionInfo = NULL;
    VS_FIXEDFILEINFO*   pFileInfo = NULL;
    UINT                puLenFileInfo = 0;

    // Get the version information for the file requested
    dwSize = GetFileVersionInfoSize(pszFilePath, NULL);
    if (dwSize == 0) {
        printf("Error in GetFileVersionInfoSize: %d\n", GetLastError());
        return false;
    }

    pbVersionInfo = new BYTE[dwSize];

    if (!GetFileVersionInfo(pszFilePath, 0, dwSize, pbVersionInfo)) {
        printf("Error in GetFileVersionInfo: %d\n", GetLastError());
        delete[] pbVersionInfo;
        return false;
    }

    if (!VerQueryValue(pbVersionInfo, TEXT("\\"), (LPVOID*)&pFileInfo, &puLenFileInfo)) {
        printf("Error in VerQueryValue: %d\n", GetLastError());
        delete[] pbVersionInfo;
        return false;
    }

    {
        Module.File_Version += (std::to_string((pFileInfo->dwFileVersionLS >> 24) & 0xff) + ".");
        Module.File_Version += (std::to_string((pFileInfo->dwFileVersionLS >> 16) & 0xff) + ".");
        Module.File_Version += (std::to_string((pFileInfo->dwFileVersionLS >> 8) & 0xff) + ".");
        Module.File_Version += (std::to_string((pFileInfo->dwFileVersionLS >> 0) & 0xff));
    }

    {
        Module.Product_Version += (std::to_string((pFileInfo->dwProductVersionLS >> 24) & 0xff) + ".");
        Module.Product_Version += (std::to_string((pFileInfo->dwProductVersionLS >> 16) & 0xff) + ".");
        Module.Product_Version += (std::to_string((pFileInfo->dwProductVersionLS >> 8) & 0xff) + ".");
        Module.Product_Version += (std::to_string((pFileInfo->dwProductVersionLS >> 0) & 0xff));
    }

    return true;
}

bool List_Process_Modules(DWORD dwPID, std::vector<Process_Module>& Module_List) {
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 Module_Entry_32;

    Process_Module Module;

    //  Take a snapshot of all modules in the specified process. 
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        //Print_Error(TEXT("CreateToolhelp32Snapshot (of modules)"));
        return false;
    }

    //  Set the size of the structure before using it. 
    Module_Entry_32.dwSize = sizeof(MODULEENTRY32);

    //  Retrieve information about the first module, 
    //  and exit if unsuccessful 
    if (!Module32First(hModuleSnap, &Module_Entry_32))
    {
        //Print_Error(TEXT("Module32First"));  // Show cause of failure 
        CloseHandle(hModuleSnap);     // Must clean up the snapshot object! 
        return false;
    }

    //  Now walk the module list of the process, 
    //  and display information about each module 
    while (true) {
        bool Module_Is_Needed = false;
        Module.Name = "";

        for (int i = 0; i < 256 - 3; i++) {
            if (Module_Entry_32.szModule[i] == '.') {
                if (Module_Entry_32.szModule[i + 1] == 'd') {
                    if (Module_Entry_32.szModule[i + 2] == 'l') {
                        if (Module_Entry_32.szModule[i + 3] == 'l') {
                            Module.Name += ".dll";
                            Module_Is_Needed = true;
                            break;
                        }
                    }
                }
                else if (Module_Entry_32.szModule[i + 1] == 'e') {
                    if (Module_Entry_32.szModule[i + 2] == 'x') {
                        if (Module_Entry_32.szModule[i + 3] == 'e') {
                            Module.Name += ".exe";
                            Module_Is_Needed = true;
                            break;
                        }
                    }
                }
            }
            Module.Name += Module_Entry_32.szModule[i];
        }

        if (Module_Is_Needed) {
            Module.File_Version = ""; Module.Product_Version = "";
            if (Get_Module_Versions(Module_Entry_32.szExePath, Module)) {
                Module_List.push_back(Module);
            }
        }

        if (!Module32Next(hModuleSnap, &Module_Entry_32)) break;
    }

    //  Do not forget to clean up the snapshot object. 
    CloseHandle(hModuleSnap);
    return true;
}

void Print_Error(TCHAR* msg) {
    DWORD eNum;
    TCHAR sysMsg[256];
    TCHAR* p;

    eNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, eNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        sysMsg, 256, NULL);

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do { *p-- = 0; } while ((p >= sysMsg) &&
        ((*p == '.') || (*p < 33)));

    // Display the message
    _tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}