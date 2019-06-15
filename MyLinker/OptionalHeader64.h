#pragma once
#include "usingTypes.h"
#include "DataDirectory.h"

#include <optional>

struct OptionalHeader64 {
    // ----------------------------------------------
    // Standard Fields
    // ----------------------------------------------

    word magicNumber = 0x20b;

    /*
        The linker major version number.
    */
    byte majorLinkerVersion = 0x2;

    /*
        The linker minor version number. 
    */
    byte minorLinkerVersion = 0x18;

    /*
        The size of the code (text) section, or the sum of all code sections if there are multiple sections. 
    */
    dword sizeOfCode;

    /*
        The size of the initialized data section, or the sum of all such sections if there are multiple data sections. 
    */
    dword sizeOfInitializedData;

    /*
        The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections. 
    */
    dword sizeOfUninitializedData;

    /*
        The address of the entry point relative to the image base when the executable file is loaded into memory. 
        For program images, this is the starting address. For device drivers, this is the address of the initialization function. 
        An entry point is optional for DLLs. When no entry point is present, this field must be zero. 
    */
    dword addressOfEntryPoint;

    /*
        The address that is relative to the image base of the beginning-of-code section when it is loaded into memory. 
    */
    dword baseOfCode;


    // ----------------------------------------------
    // Windows-Specific Fields
    // ----------------------------------------------
    enum DllCharacteristic : word {
        HightEntropyVa                = 0x0020, // Image can handle a high entropy 64-bit virtual address space. 
        DynamicBase                   = 0x0040, // DLL can be relocated at load time. 
        ForceIntegrity                = 0x0080, // Code Integrity checks are enforced 
        CompatibleWithNX              = 0x0100, // Image is NX compatible. 
        NoIsolation                   = 0x0200, // Isolation aware, but do not isolate the image. 
        NoStructuredExceptionHandling = 0x0400, // Does not use structured exception (SE) handling. 
        DoNotBindImage                = 0x0800, // Do not bind the image. 
        AppContainer                  = 0x1000, // Image must execute in an AppContainer. 
        WdmDRIVER                     = 0x2000, // A WDM driver. 
        ControlFlowGuardSupport       = 0x4000, // Image supports Control Flow Guard. 
        TerminalServerAware           = 0x8000, // Terminal Server aware. 
    };
    enum Subsystem : word {
        Unknown                = 0,  // An unknown subsystem 
        Native                 = 1,  // Device drivers and native Windows processes 
        WindowsGui             = 2,  // The Windows graphical user interface (GUI) subsystem 
        WindowsCui             = 3,  // The Windows character subsystem 
        Os2Cui                 = 5,  // The OS/2 character subsystem 
        PosixCui               = 7,  // The Posix character subsystem 
        NativeWindows          = 8,  // Native Win9x driver 
        WindowsCeGui           = 9,  // Windows CE 
        EfiApplication         = 10, // Extensible Firmware Interface (EFI) application 
        EfiBootServiceDriver   = 11, // EFI driver with boot services 
        EfiRuntimeDriver       = 12, // EFI driver with run-time services 
        EfiRom                 = 13, // EFI ROM image 
        XBox                   = 14, // XBOX 
        WindowsBootApplication = 16  // Windows boot application. 
    };
    enum DataDirectoryTableId : dword {
        Export                = 0,
        Import                = 1,
        Resource              = 2,
        Exception             = 3,
        Certificate           = 4,
        BaseRelocation        = 5,
        Debug                 = 6,
        Architecture          = 7,
        GlobalPtr             = 8,
        TlsTable              = 9,
        LoadConfig            = 10,
        BoundImport           = 11,
        IAT                   = 12,
        DelayImportDescriptor = 13,
        ClrRuntimeHeader      = 14
    };

    /*
        The preferred address of the first byte of image when loaded into memory; must be a multiple of 64 K. 
        The default for DLLs is 0x10000000. 
        The default for Windows CE EXEs is 0x00010000. 
        The default for Windows NT, Windows 2000, Windows XP, Windows 95, Windows 98, Windows Me is 0x00400000. 
    */
    qword imageBase;

    /*
        The alignment (in bytes) of sections when they are loaded into memory. 
        It must be greater than or equal to fileAlignment. 
        The default is the page size for the architecture. 
    */
    dword sectionAlignment;

    /*
        The alignment factor (in bytes) that is used to align the raw data of sections in the image file. 
        The value should be a power of 2 between 512 and 64 K, inclusive. The default is 512. 
        If the sectionAlignment is less than the architecture's page size, then fileAlignment must match sectionAlignment. 
    */
    dword fileAlignment;

    /*
        The major version number of the required operating system. 
    */
    word majorOperatingSystemVersion;

    /*
        The minor version number of the required operating system. 
    */
    word minorOperatingSystemVersion;

    /*
        The major version number of the image. 
    */
    word majorImageVersion;

    /*
        The minor version number of the image. 
    */
    word minorImageVersion;

    /*
        The major version number of the subsystem. 
    */
    word majorSubsystemVersion;

    /*
        The minor version number of the subsystem. 
    */
    word minorSubsystemVersion;

    /*
        Reserved. Must be 0.
    */
    dword win32VersionValue = 0;

    /*
        The size (in bytes) of the image, including all headers, as the image is loaded in memory. 
        It must be a multiple of sectionAlignment. 
    */
    dword sizeOfImage;

    /*
        The combined size of an MS-DOS stub, PE header, and section headers rounded up to a multiple of fileAlignment. 
    */
    dword sizeOfHeaders;

    /*
        The image file checksum. The algorithm for computing the checksum is incorporated into IMAGHELP.DLL. 
        The following are checked for validation at load time: all drivers, any DLL loaded at boot time
        and any DLL that is loaded into a critical Windows process. 
    */
    dword checkSum;

    /*
        The subsystem that is required to run this image.
    */
    word subsystem;

    /*
        DllCharacteristic flags
    */
    word dllCharacteristics;

    /*
        The size of the stack to reserve. Only sizeOfStackCommit is committed; 
        the rest is made available one page at a time until the reserve size is reached. 
    */
    qword sizeOfStackReserve;

    /*
        The size of the stack to commit. 
    */
    qword sizeOfStackCommit;

    /*
        The size of the local heap space to reserve. Only SizeOfHeapCommit is committed; 
        the rest is made available one page at a time until the reserve size is reached. 
    */
    qword sizeOfHeapReserve;

    /*
        The size of the local heap space to commit. 
    */
    qword sizeOfHeapCommit;

    /*
        Reserved, must be zero. 
    */
    dword loaderFlags = 0;

    /*
        The number of data-directory entries in the remainder of the optional header. 
    */
    dword numberOfRvaAndSizes = 16;

    DataDirectory dataDirectories[16] = {};

    static int Size() {
        return 112 + 16 * DataDirectory::Size(); // 0xf0
    }
};

template<typename Reader> std::optional<OptionalHeader64> readOptionalHeader64(Reader& reader) {
    OptionalHeader64 optionalHeader64;

    reader >> optionalHeader64.magicNumber;
    reader >> optionalHeader64.majorLinkerVersion;
    reader >> optionalHeader64.minorLinkerVersion;
    reader >> optionalHeader64.sizeOfCode;
    reader >> optionalHeader64.sizeOfInitializedData;
    reader >> optionalHeader64.sizeOfUninitializedData;
    reader >> optionalHeader64.addressOfEntryPoint;
    reader >> optionalHeader64.baseOfCode;
    reader >> optionalHeader64.imageBase;
    reader >> optionalHeader64.sectionAlignment;
    reader >> optionalHeader64.fileAlignment;
    reader >> optionalHeader64.majorOperatingSystemVersion;
    reader >> optionalHeader64.minorOperatingSystemVersion;
    reader >> optionalHeader64.majorImageVersion;
    reader >> optionalHeader64.minorImageVersion;
    reader >> optionalHeader64.majorSubsystemVersion;
    reader >> optionalHeader64.minorSubsystemVersion;
    reader >> optionalHeader64.win32VersionValue;
    reader >> optionalHeader64.sizeOfImage;
    reader >> optionalHeader64.sizeOfHeaders;
    reader >> optionalHeader64.checkSum;
    reader >> optionalHeader64.subsystem;
    reader >> optionalHeader64.dllCharacteristics;
    reader >> optionalHeader64.sizeOfStackReserve;
    reader >> optionalHeader64.sizeOfStackCommit;
    reader >> optionalHeader64.sizeOfHeapReserve;
    reader >> optionalHeader64.sizeOfHeapCommit;
    reader >> optionalHeader64.loaderFlags;
    reader >> optionalHeader64.numberOfRvaAndSizes;

    for (int i = 0; i < 16; ++i) {
        auto dataDirectory = DataDirectory::Read(reader);
        if (!dataDirectory) return std::nullopt;
        optionalHeader64.dataDirectories[i] = *dataDirectory;
    }

    return reader ? optionalHeader64 : std::optional<OptionalHeader64>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const OptionalHeader64& optionalHeader64) {
    out << optionalHeader64.magicNumber
        << optionalHeader64.majorLinkerVersion
        << optionalHeader64.minorLinkerVersion
        << optionalHeader64.sizeOfCode
        << optionalHeader64.sizeOfInitializedData
        << optionalHeader64.sizeOfUninitializedData
        << optionalHeader64.addressOfEntryPoint
        << optionalHeader64.baseOfCode
        << optionalHeader64.imageBase
        << optionalHeader64.sectionAlignment
        << optionalHeader64.fileAlignment
        << optionalHeader64.majorOperatingSystemVersion
        << optionalHeader64.minorOperatingSystemVersion
        << optionalHeader64.majorImageVersion
        << optionalHeader64.minorImageVersion
        << optionalHeader64.majorSubsystemVersion
        << optionalHeader64.minorSubsystemVersion
        << optionalHeader64.win32VersionValue
        << optionalHeader64.sizeOfImage
        << optionalHeader64.sizeOfHeaders
        << optionalHeader64.checkSum
        << optionalHeader64.subsystem
        << optionalHeader64.dllCharacteristics
        << optionalHeader64.sizeOfStackReserve
        << optionalHeader64.sizeOfStackCommit
        << optionalHeader64.sizeOfHeapReserve
        << optionalHeader64.sizeOfHeapCommit
        << optionalHeader64.loaderFlags
        << optionalHeader64.numberOfRvaAndSizes;

    for (int i = 0; i < 16; ++i) {
        write(out, optionalHeader64.dataDirectories[i]);
    }
}