
#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>

#define A_PRIME 141224297 
#define B_PRIME 9216973
#define C_PRIME 422857 
#define FIRSTH  1531

const int DumpSymbolsToRead = 1000;

unsigned long long Hash_String(const std::string& String_to_Hash) {

    unsigned long long Hash = FIRSTH;
    for (int i = 0; i < String_to_Hash.size(); i++) {
        Hash = (Hash * A_PRIME) ^ ((long long)String_to_Hash[i] * B_PRIME) +
            (C_PRIME / (long long)String_to_Hash[i]);
    }

    return Hash;
}

extern "C" __declspec(dllexport) bool Identify_Dump(unsigned long long Dump_Key_Value, BSTR Path_For_Dumps) {

    FILETIME ftWrite;
    FILETIME ftCreate;
    SYSTEMTIME Dump_Modification_Time;
    SYSTEMTIME Dump_Creation_Time;

    FileTimeToSystemTime(&ftWrite, &Dump_Modification_Time);
    FileTimeToSystemTime(&ftCreate, &Dump_Creation_Time);

    std::string Creation_Time = std::to_string(Dump_Creation_Time.wYear) +
        std::to_string(Dump_Creation_Time.wMonth) + std::to_string(Dump_Creation_Time.wDay) +
        std::to_string(Dump_Creation_Time.wHour) + std::to_string(Dump_Creation_Time.wMinute) +
        std::to_string(Dump_Creation_Time.wSecond);

    std::string Modification_Time = std::to_string(Dump_Modification_Time.wYear) +
        std::to_string(Dump_Modification_Time.wMonth) + std::to_string(Dump_Modification_Time.wDay) +
        std::to_string(Dump_Modification_Time.wHour) + std::to_string(Dump_Modification_Time.wMinute) +
        std::to_string(Dump_Modification_Time.wSecond);

    std::ifstream Dump_File(Path_For_Dumps, std::ifstream::ate | std::ifstream::binary);
    int Dump_Size = Dump_File.tellg();

    Dump_File.seekg(0, std::ios::beg);

    char* Dump_Symbols_Buffer = new char[DumpSymbolsToRead];
    Dump_File.read(Dump_Symbols_Buffer, DumpSymbolsToRead);
    std::string Dump_Symbols = "";
    for (int i = 0; i < DumpSymbolsToRead; i++) {
        Dump_Symbols += std::to_string((int)Dump_Symbols_Buffer[i]);
    }

    delete[] Dump_Symbols_Buffer;
    Dump_File.close();

    std::string New_Dump_Key_String = Creation_Time + " " + Modification_Time + " " +
        std::to_string(Dump_Size) + " " + Dump_Symbols;

    auto New_Dump_Key_Value = Hash_String(New_Dump_Key_String);

    if (Dump_Key_Value == New_Dump_Key_Value) return true;

    return false;
}