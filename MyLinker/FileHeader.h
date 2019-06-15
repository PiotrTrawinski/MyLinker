#pragma once
#include "usingTypes.h"

#include <optional>

struct FileHeader {
    enum Machine : word {
        Unknown   = 0x0,    // The contents of this field are assumed to be applicable to any machine type 
        AM33      = 0x1d3,  // Matsushita AM33
        AMD64     = 0x8664, // x64 
        ARM       = 0x1c0,  // ARM little endian 
        ARM64     = 0xaa64, // ARM64 little endian 
        ARMNT     = 0x1c4,  // ARM Thumb-2 little endian 
        EBC       = 0xebc,  // EFI byte code 
        I386      = 0x14c,  // Intel 386 or later processors and compatible processors
        IA64      = 0x200,  // Intel Itanium processor family
        M32R      = 0x9041, // Mitsubishi M32R little endian 
        MIPS16    = 0x266,  // MIPS16 
        MIPSFPU   = 0x366,  // MIPS with FPU 
        MIPS16FPU = 0x466,  // MIPS16 with FPU 
        PowerPc   = 0x1f0,  // Power PC little endian 
        PowerPcFp = 0x1f1,  // Power PC with floating point support 
        R4000     = 0x166,  // MIPS little endian 
        RISCV32   = 0x5032, // RISC-V 32-bit address space 
        RISCV64   = 0x5064, // RISC-V 64-bit address space 
        RISCV128  = 0x5128, // RISC-V 128-bit address space 
        SH3       = 0x1a2,  // Hitachi SH3 
        SH3DSP    = 0x1a3,  // Hitachi SH3 DSP 
        SH4       = 0x1a6,  // Hitachi SH4 
        SH5       = 0x1a8,  // Hitachi SH5 
        THUMB     = 0x1c2,  // Thumb 
        WCEMIPSV2 = 0x169   // MIPS little-endian WCE v2 
    };
    enum Characteristic : word {
        RelocsStripped       = 0x0001, // Image only, Windows CE, and Microsoft Windows NT and later. This indicates that the file does not contain base relocations and must therefore be loaded at its preferred base address. If the base address is not available, the loader reports an error. The default behavior of the linker is to strip base relocations from executable (EXE) files. 
        ExecutableImage      = 0x0002, // Image only. This indicates that the image file is valid and can be run. If this flag is not set, it indicates a linker error. 
        NumsStripped         = 0x0004, // COFF line numbers have been removed. This flag is deprecated and should be zero. 
        SymsStripped         = 0x0008, // COFF symbol table entries for local symbols have been removed. This flag is deprecated and should be zero. 
        AggressiveWsTrim     = 0x0010, // Obsolete. Aggressively trim working set. This flag is deprecated for Windows 2000 and later and must be zero. 
        LargeAddressAware    = 0x0020, // Application can handle > 2GB addresses. 
        LittleEndian         = 0x0080, // This flag is deprecated and should be zero. 
        Machine32Bit         = 0x0100, // Machine is based on a 32-bit-word architecture. 
        DebugStripped        = 0x0200, // Debugging information is removed from the image file.
        RemovableRunFromSwap = 0x0400, // If the image is on removable media, fully load it and copy it to the swap file. 
        NetRunFromSwap       = 0x0800, // If the image is on network media, fully load it and copy it to the swap file. 
        System               = 0x1000, // The image file is a system file, not a user program. 
        DLL                  = 0x2000, // The image file is a dynamic-link library (DLL). Such files are considered executable files for almost all purposes, although they cannot be directly run. 
        UpSystemOnly         = 0x4000, // The file should be run only on a uniprocessor machine. 
        BigEndian            = 0x8000  // This flag is deprecated and should be zero. 
    };

    /*
        Identificator of the target machine (architecture)
    */
    word machine = Machine::I386;

    /*
        Size of the section table
    */
    word numberOfSections; // = 0xc;

    /*
        The low 32 bits of the number of seconds since 00:00 January 1, 1970, 
        that indicates when the file was created.
    */
    dword timeDateStamp; // = 0x1500ca0;

    /*
        The file offset of the COFF symbol table, or zero if no COFF symbol table is present. 
        This value should be zero for an image because COFF debugging information is deprecated. 
    */
    dword pointerToSymbolTable; // = 0x3600;

    /*
        The number of entries in the symbol table. This data can be used to locate the string table, 
        which immediately follows the symbol table. 
        This value should be zero for an image because COFF debugging information is deprecated. 
    */
    dword numberOfSymbols;// = 0x279;

    /*
        The size of the optional header
    */
    word sizeOfOptionalHeader; // = 0xe0;

    /*
        The flags that indicate the attributes of the file
    */
    word characteristics; // = 0x107;

    static int Size() {
        return 20;
    }
};

template<typename Reader> std::optional<FileHeader> readFileHeader(Reader& reader) {
    FileHeader fileHeader;

    reader >> fileHeader.machine;
    reader >> fileHeader.numberOfSections;
    reader >> fileHeader.timeDateStamp;
    reader >> fileHeader.pointerToSymbolTable;
    reader >> fileHeader.numberOfSymbols;
    reader >> fileHeader.sizeOfOptionalHeader;
    reader >> fileHeader.characteristics;

    return reader ? fileHeader : std::optional<FileHeader>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const FileHeader& fileHeader) {
    out << fileHeader.machine
        << fileHeader.numberOfSections
        << fileHeader.timeDateStamp
        << fileHeader.pointerToSymbolTable
        << fileHeader.numberOfSymbols
        << fileHeader.sizeOfOptionalHeader
        << fileHeader.characteristics;
}