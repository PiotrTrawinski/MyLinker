#pragma once
#include "usingTypes.h"
#include "DataDirectory.h"

#include <optional>

struct OptionalHeader32 {
    // ----------------------------------------------
    // Standard Fields
    // ----------------------------------------------

    word magicNumber = 0x10b;

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

    /*
        The address that is relative to the image base of the beginning-of-data section when it is loaded into memory. 
    */
    dword baseOfData;


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
    dword imageBase;

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
    dword sizeOfStackReserve;

    /*
        The size of the stack to commit. 
    */
    dword sizeOfStackCommit;

    /*
        The size of the local heap space to reserve. Only SizeOfHeapCommit is committed; 
        the rest is made available one page at a time until the reserve size is reached. 
    */
    dword sizeOfHeapReserve;

    /*
        The size of the local heap space to commit. 
    */
    dword sizeOfHeapCommit;

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
        return 96 + 16 * DataDirectory::Size(); // 0xe0
    }
};

template<typename Reader> std::optional<OptionalHeader32> readOptionalHeader32(Reader& reader) {
    OptionalHeader32 optionalHeader32;
    
    reader >> optionalHeader32.magicNumber;
    reader >> optionalHeader32.majorLinkerVersion;
    reader >> optionalHeader32.minorLinkerVersion;
    reader >> optionalHeader32.sizeOfCode;
    reader >> optionalHeader32.sizeOfInitializedData;
    reader >> optionalHeader32.sizeOfUninitializedData;
    reader >> optionalHeader32.addressOfEntryPoint;
    reader >> optionalHeader32.baseOfCode;
    reader >> optionalHeader32.baseOfData;
    reader >> optionalHeader32.imageBase;
    reader >> optionalHeader32.sectionAlignment;
    reader >> optionalHeader32.fileAlignment;
    reader >> optionalHeader32.majorOperatingSystemVersion;
    reader >> optionalHeader32.minorOperatingSystemVersion;
    reader >> optionalHeader32.majorImageVersion;
    reader >> optionalHeader32.minorImageVersion;
    reader >> optionalHeader32.majorSubsystemVersion;
    reader >> optionalHeader32.minorSubsystemVersion;
    reader >> optionalHeader32.win32VersionValue;
    reader >> optionalHeader32.sizeOfImage;
    reader >> optionalHeader32.sizeOfHeaders;
    reader >> optionalHeader32.checkSum;
    reader >> optionalHeader32.subsystem;
    reader >> optionalHeader32.dllCharacteristics;
    reader >> optionalHeader32.sizeOfStackReserve;
    reader >> optionalHeader32.sizeOfStackCommit;
    reader >> optionalHeader32.sizeOfHeapReserve;
    reader >> optionalHeader32.sizeOfHeapCommit;
    reader >> optionalHeader32.loaderFlags;
    reader >> optionalHeader32.numberOfRvaAndSizes;
    
    for (int i = 0; i < 16; ++i) {
        auto dataDirectory = DataDirectory::Read(reader);
        if (!dataDirectory) return std::nullopt;
        optionalHeader32.dataDirectories[i] = *dataDirectory;
    }

    return reader ? optionalHeader32 : std::optional<OptionalHeader32>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const OptionalHeader32& optionalHeader32) {
    out << optionalHeader32.magicNumber
        << optionalHeader32.majorLinkerVersion
        << optionalHeader32.minorLinkerVersion
        << optionalHeader32.sizeOfCode
        << optionalHeader32.sizeOfInitializedData
        << optionalHeader32.sizeOfUninitializedData
        << optionalHeader32.addressOfEntryPoint
        << optionalHeader32.baseOfCode
        << optionalHeader32.baseOfData
        << optionalHeader32.imageBase
        << optionalHeader32.sectionAlignment
        << optionalHeader32.fileAlignment
        << optionalHeader32.majorOperatingSystemVersion
        << optionalHeader32.minorOperatingSystemVersion
        << optionalHeader32.majorImageVersion
        << optionalHeader32.minorImageVersion
        << optionalHeader32.majorSubsystemVersion
        << optionalHeader32.minorSubsystemVersion
        << optionalHeader32.win32VersionValue
        << optionalHeader32.sizeOfImage
        << optionalHeader32.sizeOfHeaders
        << optionalHeader32.checkSum
        << optionalHeader32.subsystem
        << optionalHeader32.dllCharacteristics
        << optionalHeader32.sizeOfStackReserve
        << optionalHeader32.sizeOfStackCommit
        << optionalHeader32.sizeOfHeapReserve
        << optionalHeader32.sizeOfHeapCommit
        << optionalHeader32.loaderFlags
        << optionalHeader32.numberOfRvaAndSizes;

    for (int i = 0; i < 16; ++i) {
        write(out, optionalHeader32.dataDirectories[i]);
    }
}