#pragma once
#include "usingTypes.h"

#include <optional>
#include <array>

struct SectionHeader {
    enum Characteristic : dword {
        NoPadding                   = 0x00000008, // This flag is obsolete and is replaced by Allign1Byte.
        ContainsCode                = 0x00000020, // The section contains executable code.
        ContainsInitializedData     = 0x00000040, // The section contains initialized data. 
        ContainsUninitializedData   = 0x00000080, // The section contains uninitialized data. 
        LinkInfo                    = 0x00000200, // The section contains comments or other information. The .drectve section has this type. This is valid for object files only. 
        LinkRemove                  = 0x00000800, // The section will not become part of the image. This is valid only for object files. 
        ContainsComdatData          = 0x00001000, // The section contains COMDAT data. For more information, see COMDAT Sections (Object Only). 
        ContainsDataGPReferenced    = 0x00008000, // The section contains data referenced through the global pointer (GP). 
        Allign1Byte                 = 0x00100000, // Align data on 1-byte boundary. Valid only for object files. 
        Allign2Byte                 = 0x00200000, // Align data on 2-byte boundary. Valid only for object files. 
        Allign4Byte                 = 0x00300000, // Align data on 4-byte boundary. Valid only for object files. 
        Allign8Byte                 = 0x00400000, // Align data on 8-byte boundary. Valid only for object files. 
        Allign16Byte                = 0x00500000, // Align data on 16-byte boundary. Valid only for object files. 
        Allign32Byte                = 0x00600000, // Align data on 32-byte boundary. Valid only for object files. 
        Allign64Byte                = 0x00700000, // Align data on 64-byte boundary. Valid only for object files. 
        Allign128Byte               = 0x00800000, // Align data on 128-byte boundary. Valid only for object files. 
        Allign256Byte               = 0x00900000, // Align data on 256-byte boundary. Valid only for object files. 
        Allign512Byte               = 0x00A00000, // Align data on 512-byte boundary. Valid only for object files. 
        Allign1024Byte              = 0x00B00000, // Align data on 1024-byte boundary. Valid only for object files. 
        Allign2048Byte              = 0x00C00000, // Align data on 2048-byte boundary. Valid only for object files. 
        Allign4096Byte              = 0x00D00000, // Align data on 4096-byte boundary. Valid only for object files. 
        Allign8192Byte              = 0x00E00000, // Align data on 8192-byte boundary. Valid only for object files. 
        ContainsExtendedRelocations = 0x01000000, // The section contains extended relocations. 
        CanDiscard                  = 0x02000000, // The section can be discarded as needed. 
        NotCached                   = 0x04000000, // The section cannot be cached. 
        NotPagable                  = 0x08000000, // The section is not pageable. 
        CanShare                    = 0x10000000, // The section can be shared in memory. 
        CanExecute                  = 0x20000000, // The section can be executed as code. 
        CanRead                     = 0x40000000, // The section can be read. 
        CanWrite                    = 0x80000000  // The section can be written to. 
    };

    std::array<byte, 8> name;
    dword virtualSize;
    dword virtualAddress;
    dword sizeOfRawData;
    dword pointerToRawData;
    dword pointerToRelocations;
    dword pointerToLineNumbers;
    word numberOfRelocations;
    word numberOfLineNumbers;
    dword characteristics;

    static int Size() {
        return 40;
    }
};

template<typename Reader> std::optional<SectionHeader> readSectionHeader(Reader& reader) {
    SectionHeader sectionHeader;

    reader >> sectionHeader.name;
    reader >> sectionHeader.virtualSize;
    reader >> sectionHeader.virtualAddress;
    reader >> sectionHeader.sizeOfRawData;
    reader >> sectionHeader.pointerToRawData;
    reader >> sectionHeader.pointerToRelocations;
    reader >> sectionHeader.pointerToLineNumbers;
    reader >> sectionHeader.numberOfRelocations;
    reader >> sectionHeader.numberOfLineNumbers;
    reader >> sectionHeader.characteristics;

    return reader ? sectionHeader : std::optional<SectionHeader>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const SectionHeader& sectionHeader) {
    out << sectionHeader.name
        << sectionHeader.virtualSize
        << sectionHeader.virtualAddress
        << sectionHeader.sizeOfRawData
        << sectionHeader.pointerToRawData
        << sectionHeader.pointerToRelocations
        << sectionHeader.pointerToLineNumbers
        << sectionHeader.numberOfRelocations
        << sectionHeader.numberOfLineNumbers
        << sectionHeader.characteristics;
}