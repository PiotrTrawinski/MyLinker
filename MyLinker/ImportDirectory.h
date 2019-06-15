#pragma once
#include "usingTypes.h"
#include <vector>
#include <optional>

struct ImportDirectoryEntry {
    dword importLookupTableRVA;  // The RVA of the import lookup table. This table contains a name or ordinal for each import. 
    dword timeDateStamp = 0;     // The stamp that is set to zero until the image is bound. After the image is bound, this field is set to the time/data stamp of the DLL. 
    dword forwarderChain;        // The index of the first forwarder reference. -1 if no forwarders.
    dword nameRVA;               // The address of an ASCII string that contains the name of the DLL. This address is relative to the image base. 
    dword importAddressTableRVA; // The RVA of the import address table. The contents of this table are identical to the contents of the import lookup table until the image is bound. 
};

struct HintNameTableEntry {
    word hint; // An index into the export name pointer table. A match is attempted first with this value. If it fails, a binary search is performed on the DLL's export name pointer table. 
    std::string name; // An ASCII string that contains the name to import. This is the string that must be matched to the public name in the DLL. This string is case sensitive, terminated by a null byte and aligned to even number of bytes. 
};

struct ImportDll32Entry {
    std::optional<word> ordinal;
    HintNameTableEntry hintName;
    dword hintNameTableRva;
};

struct ImportDll32 {
    std::string name;
    ImportDirectoryEntry directoryEntry;
    std::vector<ImportDll32Entry> imports;
};

struct ImportDirectory32 {
    std::vector<ImportDll32> dlls;
};