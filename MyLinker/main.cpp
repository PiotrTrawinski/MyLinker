#include "DosHeader.h"
#include "FileHeader.h"
#include "OptionalHeader32.h"
#include "OptionalHeader64.h"
#include "DataDirectory.h"
#include "FileHeader.h"
#include "ObjectFile.h"
#include "PeFile.h"
#include "PeHeader.h"
#include "SectionHeader.h"
#include "SymbolTableEntry.h"
#include "errorMessages.h"
#include "BinaryFile.h"

#include <iostream>
#include <fstream>
#include <variant>
#include <vector>
#include <string>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <numeric>

#define NOMINMAX
#include "Windows.h"


dword getPageSize() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

struct ProgramOptions {
    int sizeOfStackReserve = 0x200000;
    int sizeOfStackCommit  = 0x1000;
    int sizeOfHeapReserve  = 0x100000;
    int sizeOfHeapCommit   = 0x1000;
    int sectionAllign = getPageSize();
    int fileAllign = 0x200;
    int imageBase = 0x400000;
    std::string entryPoint = "_main";
    std::string outputFileName = "a.exe";
    bool showDllWarnings = false;
    word subsystem = OptionalHeader32::Subsystem::WindowsCui;
    bool onlyShowHelp = false;
    std::vector<std::string> objFileNames;
    std::vector<std::pair<std::string, HINSTANCE>> dlls;
};

bool programOptionsReadSingleIntArg(const std::vector<char*>& argv, size_t& i, const std::string& option, int& value) {
    i += 1;
    if (i >= argv.size()) {
        return errorMessageBool("expected 1 integer argument for ["+option+"]");
    }
    try {
        value = std::stoi(argv[i++], nullptr, 0);
    } catch (...) {
        return errorMessageBool("argument provided for ["+option+"] was not an integer");
    }

    return true;
}
std::optional<ProgramOptions> getProgramOptions(char* program, const std::vector<char*>& argv) {
    ProgramOptions options;

    size_t i = 0;
    while (i < argv.size()) {
        if (!strcmp("-help", argv[i]) || !strcmp("-h", argv[i]) || !strcmp("?", argv[i])) {
            options.onlyShowHelp = true;
            std::cout << "-help            : show usage\n";
            std::cout << "-stackReserve N  : reserved stack size (N - natural number) [default: N=0x200000]\n";
            std::cout << "-stackCommit N   : starting stack size (N - natural number) [default: N=0x1000]\n";
            std::cout << "-heapReserve N   : reserved heap size  (N - natural number) [default: N=0x100000]\n";
            std::cout << "-heapCommit N    : starting heap size  (N - natural number) [default: N=0x1000]\n";
            std::cout << "-sectionAllign N : allignment for sections in memory to N bytes (N >= PAGE_SIZE and is power of 2)\n"; 
            std::cout << "                   [default: N=PAGE_SIZE]\n";
            std::cout << "-fileAllign N    : allignment for sections in file (512 <= N <= 65536 and N is power of 2)\n"; 
            std::cout << "                   [default: N=512]\n";
            std::cout << "-base N          : virtual address after loading (N is multiple of 65536)\n"; 
            std::cout << "                   [default: N=0x400000]\n";
            std::cout << "-entry FUN       : entry point (FUN is function name) [default: FUN = '_main']\n";
            std::cout << "-out PATH        : path to output file (PATH - chosen path) [default: PATH=a.exe]\n";
            std::cout << "-subsystem STR   : subsystem. possible values for STR:\n";
            std::cout << "                       native, winBoot, winGUI, winCUI, winCE,\n";
            std::cout << "                       posix, os2, efiApp, efiBootDriver,\n";
            std::cout << "                       efiRuntimeDriver, efiRom\n";
            std::cout << "                   [default: STR='winCUI']\n";
            std::cout << "-dllwarn         : show warnings if non-perfect dll symbol matching occured\n";
            std::cout << "-dll DLL_FILE    : path to linked .dll\n";
            std::cout << "OBJ_FILE         : path to linked .obj\n";
            return options;
        } else if (!strcmp("-stackReserve", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-stackReserve", options.sizeOfStackReserve)) return std::nullopt;
        } else if (!strcmp("-stackCommit", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-stackCommit", options.sizeOfStackCommit)) return std::nullopt;
        } else if (!strcmp("-heapReserve", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-heapReserve", options.sizeOfHeapReserve)) return std::nullopt;
        } else if (!strcmp("-heapCommit", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-heapCommit", options.sizeOfHeapCommit)) return std::nullopt;
        } else if (!strcmp("-sectionAllign", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-sectionAllign", options.sectionAllign)) return std::nullopt;
            dword a = options.sectionAllign;
            dword pageSize = getPageSize();
            if (!(a >= pageSize && (a & (a - 1)) == 0)) {
                return errorMessageOpt("[-sectionAllign] value needs to be a power of 2 and be >= " + std::to_string(pageSize) + " (page size)");
            }
        } else if (!strcmp("-fileAllign", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-fileAllign", options.fileAllign)) return std::nullopt;
            auto a = options.fileAllign;
            if (!(a >= 512 && a <= 65536 && (a & (a - 1)) == 0)) {
                return errorMessageOpt("[-fileAllign] value needs to be a power of 2 between 512 and 65536 bytes");
            }
        } else if (!strcmp("-base", argv[i])) {
            if (!programOptionsReadSingleIntArg(argv, i, "-base", options.imageBase)) return std::nullopt;
            if (options.imageBase % 65536 != 0 || options.imageBase <= 0) {
                return errorMessageOpt("[-base] value needs to be a multiple of 65536 bytes");
            }
        } else if (!strcmp("-entry", argv[i])) {
            i += 1;
            if (i >= argv.size()) {
                return errorMessageOpt("expected 1 string argument for [-entry]");
            }
            options.entryPoint = argv[i++];
        } else if (!strcmp("-out", argv[i])) {
            i += 1;
            if (i >= argv.size()) {
                return errorMessageOpt("expected 1 string argument for [-out]");
            }
            options.outputFileName = argv[i++];
        } else if (!strcmp("-dll", argv[i])) {
            i += 1;
            if (i >= argv.size()) {
                return errorMessageOpt("expected 1 string argument for [-dll]");
            }
            std::string dllName = argv[i++];
            auto loadedLib = LoadLibraryA(dllName.c_str()); 
            if (loadedLib != nullptr) { 
                options.dlls.emplace_back(dllName, loadedLib);
            } else {
                warningMessage("couldn't load dynamic library '" + dllName + "'");
            }
        } else if (!strcmp("-dllwarn", argv[i])) {
            i += 1;
            options.showDllWarnings = true;
        } else if (!strcmp("-subsystem", argv[i])) {
            i += 1;
            if (i >= argv.size()) {
                return errorMessageOpt("expected 1 string argument for [-subsystem]");
            }
            static std::unordered_map<std::string, word> argToSubsystem = {
                {"native",           OptionalHeader32::Subsystem::Native},
                {"winBoot",          OptionalHeader32::Subsystem::WindowsBootApplication},
                {"winGUI",           OptionalHeader32::Subsystem::WindowsGui},
                {"winCUI",           OptionalHeader32::Subsystem::WindowsCui},
                {"winCE",            OptionalHeader32::Subsystem::WindowsCeGui},
                {"posix",            OptionalHeader32::Subsystem::PosixCui},
                {"os2",              OptionalHeader32::Subsystem::Os2Cui},
                {"efiApp",           OptionalHeader32::Subsystem::EfiApplication},
                {"efiBootDriver",    OptionalHeader32::Subsystem::EfiBootServiceDriver},
                {"efiRuntimeDriver", OptionalHeader32::Subsystem::EfiRuntimeDriver},
                {"efiRom",           OptionalHeader32::Subsystem::EfiRom}
            };
            std::string chosenSubsystem = argv[i++];
            if (auto found = argToSubsystem.find(chosenSubsystem); found != argToSubsystem.end()) {
                options.subsystem = found->second;
            } else {
                return errorMessageOpt("'" + chosenSubsystem + "' is not a known subsystem (see -help)");
            }
        } else { // input file (.obj)
            options.objFileNames.emplace_back(argv[i++]);
        }
    }

    if (options.objFileNames.empty()) {
        return errorMessageOpt("no object files given");
    }

    if (options.fileAllign > options.sectionAllign) {
        warningMessage(
            "file allignment > sectionAllignment (" + 
            std::to_string(options.fileAllign) + " > " + std::to_string(options.sectionAllign) +
            "). Setting sectionAllign = fileAllign"
        );
        options.sectionAllign = options.fileAllign;
    }

    return options;
}


std::array<byte, 8> strToArray(const std::string& str) {
    std::array<byte, 8> arr;
    size_t i = 0;
    do {
        arr[i] = str[i];
        i += 1;
    } while (i < str.size() && i < 8);
    for (; i < 8; ++i) {
        arr[i] = 0;
    }
    return arr;
}
std::string arrayToStr(const std::array<byte, 8>& arr) {
    std::string str;

    int i = 0;
    while (i < 8 && arr[i] != '\0') {
        str += arr[i++];
    }

    return str;
}
std::optional<std::string> getSymbolName(const std::array<byte, 8>& arr, const std::map<int, std::string>& stringTable) {
    if (arr[0] == 0 && arr[1] == 0 && arr[2] == 0 && arr[3] == 0) {
        int stringOffset = *reinterpret_cast<const int*>(&arr[4]);
        if (auto str = stringTable.find(stringOffset); str != end(stringTable)) {
            return str->second;
        } else {
            return std::nullopt;
        }
    } else {
        return arrayToStr(arr);
    }
}

struct ObjectSectionName {
    const ObjectFile* obj;
    std::array<byte, 8> name;

    ObjectSectionName(const ObjectFile* obj, const std::array<byte, 8>& name) :
        obj(obj),
        name(name)
    {}

    bool operator==(const ObjectSectionName& other) const {
        return obj == other.obj && name == other.name;
    }
};
namespace std {
    template <> struct hash<ObjectSectionName> {
        std::size_t operator()(const ObjectSectionName& os) const {
            return std::hash<const ObjectFile*>()(os.obj) ^ (std::hash<std::string>()(arrayToStr(os.name)) << 1);
        }
    };
}

struct PeSectionPosition {
    int sectionNr;
    int offset;

    PeSectionPosition(const int sectionNr, const int offset) :
        sectionNr(sectionNr),
        offset(offset)
    {}
};

struct ObjectSectionOffset {
    const ObjectFile* obj;
    int offset;

    ObjectSectionOffset(const ObjectFile* obj, const int offset) :
        obj(obj),
        offset(offset)
    {}
};

struct Section {
    std::array<byte, 8> name;
    std::vector<byte> data;
    std::vector<ObjectSectionOffset> objSections;
    dword characteristics;

    bool operator<(const Section& other) {
        auto characteristicValue = [](auto characteristic) -> auto {
            if (characteristic & SectionHeader::Characteristic::ContainsCode)              return 0;
            if (characteristic & SectionHeader::Characteristic::ContainsInitializedData)   return 1;
            if (characteristic & SectionHeader::Characteristic::ContainsUninitializedData) return 2;
            else return 3;
        };
        return characteristicValue(this->characteristics) < characteristicValue(other.characteristics);
    }
};

std::optional<std::pair<std::string, std::string>> tryFindDll(const std::string& functionName, const std::vector<std::pair<std::string, HINSTANCE>>& dlls, bool showDllWarnings) {
    // map of already found functions
    static std::unordered_map<std::string, std::string> functionNameToDll;

    // check if function was found before
    if (auto found = functionNameToDll.find(functionName); found != functionNameToDll.end()) {
        return *found;
    }

    // find function with given name
    for (const auto& dll : dlls) {
        if (GetProcAddress(dll.second, functionName.c_str()) != nullptr) {
            functionNameToDll.emplace(functionName, dll.first);
            return std::pair(functionName, dll.first);
        }
    }
    
    // didn't find function with given name in any dlls. try alternative names
    static std::unordered_set<std::string> reportedNames;

    // create possible alternative names to try
    std::vector<std::string> namesAlternativesToTry;

    std::string guessedNameNoSpecialSymbol = "";
    for (size_t i = 0; i < functionName.size(); ++i) {
        if (isalpha(functionName[i]) || isdigit(functionName[i]) || functionName[i] == '_') {
            guessedNameNoSpecialSymbol += functionName[i];
        } else {
            break;
        }
    }
    namesAlternativesToTry.emplace_back(guessedNameNoSpecialSymbol);

    int i = 0;
    while (guessedNameNoSpecialSymbol[i++] == '_') {
        namesAlternativesToTry.emplace_back(guessedNameNoSpecialSymbol.substr(i));
    }

    // try alternative names
    for (const auto& alternativeName : namesAlternativesToTry) {
        for (const auto& dll : dlls) {
            if (GetProcAddress(dll.second, alternativeName.c_str()) != nullptr) {
                if (reportedNames.find(functionName) == reportedNames.end()) {
                    if (showDllWarnings) warningMessage("couldn't find dll function '" + functionName + "'. instead using '" + alternativeName + "'");
                    reportedNames.insert(functionName);
                }
                functionNameToDll.emplace(alternativeName, dll.first);
                return std::pair(alternativeName, dll.first);
            }
        }
    }

    return std::nullopt;
}

std::optional<PeFile> createPeFromObj(const std::vector<ObjectFile>& objFiles, ProgramOptions options) {
    PeFile peFile;

    // get all sections from all input object files (concatenate sections with the same name)
    std::unordered_map<std::string, Section> sectionsMap;
    for (auto& obj : objFiles) {
        for (auto& objSection : obj.sections) {
            auto sectionName = arrayToStr(objSection.header.name);
            if (auto existingSection = sectionsMap.find(sectionName); existingSection != sectionsMap.end()) {
                auto& peSection = existingSection->second;
                peSection.objSections.emplace_back(&obj, peSection.data.size());
                for (byte b : objSection.data) {
                    peSection.data.emplace_back(b);
                }
            } else {
                Section peSection;
                peSection.data = objSection.data;
                peSection.characteristics = objSection.header.characteristics;
                peSection.objSections.emplace_back(&obj, 0);
                peSection.name = objSection.header.name;
                sectionsMap.emplace(sectionName, peSection);
            }
        }
    }

    // sort sections to group as such: [code sections, initialized data sections, uninitialized data sections] 
    std::vector<Section> sections;
    for (auto& sectionMapEntry : sectionsMap) {
        sections.push_back(sectionMapEntry.second);
    }
    std::sort(begin(sections), end(sections));

    int maxNumberOfSections = sections.size() + 2; // maybe will add sections for imports and dll jumps (+2)
    int sizeOfHeadersInFile = DosHeader::Size() + PeHeader::Size32() + SectionHeader::Size() * maxNumberOfSections;
    int sizeOfHeaders = options.fileAllign * ((sizeOfHeadersInFile / options.fileAllign) + 1); 
    int rawAddress = sizeOfHeaders;
    int virtualAddress = options.sectionAllign * ((sizeOfHeaders / options.sectionAllign) + 1);
    dword sizeOfCode = 0;
    dword sizeOfInitializedData = 0;
    dword sizeOfUninitializedData = 0;
    dword baseOfData = 0;

    auto& peSections = peFile.sections;
    std::unordered_map<ObjectSectionName, PeSectionPosition> objSectionNameToPeSection;
    for (auto& section : sections) {
        peSections.emplace_back();
        auto& peSection = peSections.back();
        peSection.header.name = section.name;
        peSection.header.characteristics = section.characteristics;
        peSection.header.virtualSize = std::max<dword>(4, section.data.size());
        if (section.characteristics & SectionHeader::Characteristic::ContainsUninitializedData) {
            peSection.header.sizeOfRawData = 0;
            peSection.header.pointerToRawData = 0;
        } else {
            peSection.header.sizeOfRawData = ((section.data.size() / options.fileAllign) + 1) * options.fileAllign;
            peSection.header.pointerToRawData = rawAddress;
            rawAddress += peSection.header.sizeOfRawData;
        }
        peSection.header.virtualAddress = virtualAddress;
        auto allignedVirtualSize = ((section.data.size() / options.sectionAllign) + 1) * options.sectionAllign;
        virtualAddress += allignedVirtualSize;

        peSection.header.pointerToRelocations = 0;
        peSection.header.pointerToLineNumbers = 0;
        peSection.header.numberOfRelocations = 0;
        peSection.header.numberOfLineNumbers = 0;

        for (auto& objSection : section.objSections) {
            objSectionNameToPeSection.emplace(ObjectSectionName(objSection.obj, section.name), PeSectionPosition(peSections.size()-1, objSection.offset));
        }
        
        if (!(section.characteristics & SectionHeader::Characteristic::ContainsUninitializedData)) {
            peSection.data = section.data;
        }

        if (section.characteristics & SectionHeader::Characteristic::ContainsCode) {
            sizeOfCode += peSection.header.sizeOfRawData;
            baseOfData = virtualAddress;
        } else if (section.characteristics & SectionHeader::Characteristic::ContainsInitializedData) {
            sizeOfInitializedData += peSection.header.sizeOfRawData;
        } else if (section.characteristics & SectionHeader::Characteristic::ContainsUninitializedData) {
            sizeOfUninitializedData += peSection.header.sizeOfRawData;
        }
    }

    std::unordered_map<std::string, PeSectionPosition> symbolNameToPeSection;
    for (auto& obj : objFiles) {
        for (auto& symbol : obj.symbolTableEntries) {
            if (std::holds_alternative<StandardSymbol>(symbol)) {
                auto& standardSymbol = std::get<StandardSymbol>(symbol);
                if (standardSymbol.storageClass == StandardSymbol::StorageClass::External && standardSymbol.sectionNumber > 0) {
                    auto position = objSectionNameToPeSection.at(ObjectSectionName(&obj, obj.sections[standardSymbol.sectionNumber-1].header.name));
                    position.offset += standardSymbol.value;
                    auto symbolName = getSymbolName(standardSymbol.name, obj.stringTable);
                    if (!symbolName) {
                        return errorMessageOpt("obj file is malformed - missing string table entry");
                    }
                    if (auto foundSymbol = symbolNameToPeSection.find(*symbolName); foundSymbol != end(symbolNameToPeSection)) {
                        return errorMessageOpt("multiple global symbols with same name: '" + foundSymbol->first + "'");
                    }
                    symbolNameToPeSection.emplace(*symbolName, position);
                }
            }
        }
    }

    ImportDirectory32 dllImports;
    std::unordered_map<std::string, dword> dllFunctionToJmpAddress;
    std::unordered_map<std::string, std::string> dllFunctionSymbolNameToRealName;
    int jmpSectionVirtualAddress = options.sectionAllign;

    // get all dll symbols
    for (auto& obj : objFiles) {
        for (auto& section : obj.sections) {
            for (auto& reloc : section.relocationTable) {
                // get the section+offset in PE that coresponds to the section that needs relocation (.text)
                auto [peChangedSectionNumber, changedOffsetInSection] = objSectionNameToPeSection.at({&obj, section.header.name});
                auto& sectionToChange = peFile.sections[peChangedSectionNumber];
                int changedRVA = sectionToChange.header.virtualAddress + changedOffsetInSection;
                auto* dataToChangePtr = &sectionToChange.data[reloc.virtualAddress + changedOffsetInSection];
            
                // get name of the symbol name in obj file that the relocations points to (.bss or .data if section...)
                auto& standardSymbol = std::get<StandardSymbol>(obj.symbolTableEntries[reloc.symbolTableIndex]);
            
                if (standardSymbol.storageClass == StandardSymbol::StorageClass::External) {
                    auto objAddressedSymbolName = getSymbolName(standardSymbol.name, obj.stringTable);
                    if (!objAddressedSymbolName) {
                        return errorMessageOpt("obj file is malformed - missing string table entry");
                    }

                    if (auto foundSymbol = symbolNameToPeSection.find(*objAddressedSymbolName); foundSymbol == symbolNameToPeSection.end()) { // it is symbol from dll
                        if (auto dll = tryFindDll(*objAddressedSymbolName, options.dlls, options.showDllWarnings)) {
                            dllFunctionSymbolNameToRealName.emplace(*objAddressedSymbolName, dll->first);
                            auto jmpAddress = dllFunctionToJmpAddress.find(dll->first);
                            dword dllJmpAddress = jmpSectionVirtualAddress;
                            if (jmpAddress != end(dllFunctionToJmpAddress)) {
                                dllJmpAddress = jmpAddress->second;
                            } else {
                                dllJmpAddress = jmpSectionVirtualAddress + dllFunctionToJmpAddress.size() * 6;
                                dllFunctionToJmpAddress.emplace(dll->first, dllJmpAddress);
                                auto dllImport = std::find_if(begin(dllImports.dlls), end(dllImports.dlls), [&dllName = dll->second](ImportDll32& import){
                                    return import.name == dllName;
                                });
                                if (dllImport != end(dllImports.dlls)) {
                                    auto& importedFunction = dllImport->imports.emplace_back();
                                    importedFunction.hintName.hint =0;
                                    importedFunction.hintName.name = dll->first;
                                } else {
                                    auto& newImportedDll = dllImports.dlls.emplace_back();
                                    auto& importedFunction = newImportedDll.imports.emplace_back();
                                    newImportedDll.name = dll->second;
                                    importedFunction.hintName.hint = 0;
                                    importedFunction.hintName.name = dll->first;
                                }
                            }
                        } else {
                            return errorMessageOpt("symbol '" + *objAddressedSymbolName + "' was not defined in any .obj or dll");
                        }
                    }
                }
            }
        }
    }

    DataDirectory importDataDirectory = {0, 0};
    DataDirectory importAddressTableDirectory = {0, 0};
    if (dllFunctionToJmpAddress.size() > 0) {
        auto sizeOfDllJmpSection = dllFunctionToJmpAddress.size() * 6; // 6 bytes per jmp instruction
        auto sizeOfDllJmpSectionInFile = options.fileAllign * ((sizeOfDllJmpSection / options.fileAllign) + 1);
        auto sizeOfDllJmpSectionInMemory = options.sectionAllign * ((sizeOfDllJmpSection / options.sectionAllign) + 1);

        for (auto& peSection : peSections) {
            peSection.header.pointerToRawData += sizeOfDllJmpSectionInFile;
            peSection.header.virtualAddress += sizeOfDllJmpSectionInMemory;
        }
        sizeOfCode += sizeOfDllJmpSectionInFile;
        baseOfData += sizeOfDllJmpSectionInMemory;
        virtualAddress += sizeOfDllJmpSectionInMemory;
        rawAddress += sizeOfDllJmpSectionInFile;

        for (auto& entry : objSectionNameToPeSection) {
            entry.second.sectionNr += 1;
        }
        for (auto& entry : symbolNameToPeSection) {
            entry.second.sectionNr += 1;
        }

        peSections.emplace(begin(peSections));
        auto dllJmpSection = &peSections[0];
        dllJmpSection->data.resize(dllFunctionToJmpAddress.size()*6);
        dllJmpSection->header.name = strToArray(".dlljmp");
        dllJmpSection->header.virtualSize = dllJmpSection->data.size();
        dllJmpSection->header.virtualAddress = jmpSectionVirtualAddress;
        dllJmpSection->header.sizeOfRawData = sizeOfDllJmpSectionInFile;
        dllJmpSection->header.pointerToRawData = sizeOfHeaders;
        dllJmpSection->header.pointerToRelocations = 0;
        dllJmpSection->header.pointerToLineNumbers = 0;
        dllJmpSection->header.numberOfRelocations = 0;
        dllJmpSection->header.numberOfLineNumbers = 0;
        dllJmpSection->header.characteristics = SectionHeader::Characteristic::ContainsCode
                                             | SectionHeader::Characteristic::CanRead
                                             | SectionHeader::Characteristic::CanExecute;


        // .idata section (import + IAT directory)
        peSections.emplace_back();
        auto& idata = peSections.back();
        dllJmpSection = &peSections[0];

        dword importAddressTableRVAOffset = (dllImports.dlls.size() + dllFunctionToJmpAddress.size())*4;
        dword importLookupTableRVA = virtualAddress + (dllImports.dlls.size() + 1) * 20;
        dword nameRVA = virtualAddress + (dllImports.dlls.size() + 1)*20 + (dllImports.dlls.size() + dllFunctionToJmpAddress.size())*8;
        for (auto& dll : dllImports.dlls) {
            dll.directoryEntry.importLookupTableRVA = importLookupTableRVA;
            dll.directoryEntry.importAddressTableRVA = importLookupTableRVA + importAddressTableRVAOffset;
            for (auto& importedFunction : dll.imports) {
                importedFunction.hintNameTableRva = nameRVA;
                nameRVA += importedFunction.hintName.name.size() + 3;
                int pos = dllFunctionToJmpAddress.at(importedFunction.hintName.name)-jmpSectionVirtualAddress;
                *reinterpret_cast<word*>(&dllJmpSection->data[pos]) = 0x25ff;
                *reinterpret_cast<dword*>(&dllJmpSection->data[pos+2]) = options.imageBase + importLookupTableRVA + importAddressTableRVAOffset;
                importLookupTableRVA += sizeof(dword);
            }
            dll.directoryEntry.nameRVA = nameRVA;
            nameRVA += dll.name.size()+1;
            dll.directoryEntry.forwarderChain = 0;
            dll.directoryEntry.timeDateStamp = 0;
            importLookupTableRVA += sizeof(dword);
        }
        int sizeOfImportSection = nameRVA - virtualAddress;
    
        importDataDirectory.size = sizeOfImportSection;
        importDataDirectory.virtualAddress = virtualAddress;
        importAddressTableDirectory.size = importAddressTableRVAOffset;
        importAddressTableDirectory.virtualAddress = virtualAddress + (dllImports.dlls.size() + 1) * 20;

        idata.header.name = strToArray(".idata");
        idata.header.virtualSize = sizeOfImportSection;
        idata.header.virtualAddress = virtualAddress;
        idata.header.sizeOfRawData = options.fileAllign * ((sizeOfImportSection / options.fileAllign) + 1);
        idata.header.pointerToRawData = rawAddress;
        idata.header.pointerToRelocations = 0;
        idata.header.pointerToLineNumbers = 0;
        idata.header.numberOfRelocations = 0;
        idata.header.numberOfLineNumbers = 0;
        idata.header.characteristics = SectionHeader::Characteristic::ContainsInitializedData
                                     | SectionHeader::Characteristic::CanRead
                                     | SectionHeader::Characteristic::CanWrite;

        idata.data.resize(sizeOfImportSection, 0);
        int dllHeaderOffset = 0;
        for (auto& dll : dllImports.dlls) {
            *reinterpret_cast<dword*>(&idata.data[dllHeaderOffset]) = dll.directoryEntry.importLookupTableRVA;
            *reinterpret_cast<dword*>(&idata.data[dllHeaderOffset += sizeof(dword)]) = dll.directoryEntry.timeDateStamp;
            *reinterpret_cast<dword*>(&idata.data[dllHeaderOffset += sizeof(dword)]) = dll.directoryEntry.forwarderChain;
            *reinterpret_cast<dword*>(&idata.data[dllHeaderOffset += sizeof(dword)]) = dll.directoryEntry.nameRVA;
            *reinterpret_cast<dword*>(&idata.data[dllHeaderOffset += sizeof(dword)]) = dll.directoryEntry.importAddressTableRVA;
            dllHeaderOffset += sizeof(dword);
            for (size_t i = 0; i < dll.name.size(); ++i) {
                idata.data[dll.directoryEntry.nameRVA - virtualAddress + i] = dll.name[i];
            }
            int offset = 0;
            for (auto& importedFunction : dll.imports) {
                *reinterpret_cast<dword*>(&idata.data[dll.directoryEntry.importLookupTableRVA - virtualAddress + offset])  = importedFunction.hintNameTableRva;
                *reinterpret_cast<dword*>(&idata.data[dll.directoryEntry.importAddressTableRVA - virtualAddress + offset]) = importedFunction.hintNameTableRva;
                offset += sizeof(dword);
                for (size_t i = 0; i < importedFunction.hintName.name.size(); ++i) {
                    idata.data[importedFunction.hintNameTableRva - virtualAddress + 2 + i] = importedFunction.hintName.name[i];
                }
            }
        }
    }

    dword sizeOfImage = 0;
    for (auto& peSection : peSections) {
        sizeOfImage += ((peSection.header.sizeOfRawData / options.sectionAllign) + 1) * options.sectionAllign;
    }
    

    // apply relocations
    for (auto& obj : objFiles) {
        for (auto& section : obj.sections) {
            for (auto& reloc : section.relocationTable) {
                // get the section+offset in PE that coresponds to the section that needs relocation
                auto [peChangedSectionNumber, changedOffsetInSection] = objSectionNameToPeSection.at({&obj, section.header.name});
                auto& sectionToChange = peFile.sections[peChangedSectionNumber];
                int changedRVA = sectionToChange.header.virtualAddress + changedOffsetInSection;
                auto* dataToChangePtr = &sectionToChange.data[reloc.virtualAddress + changedOffsetInSection];
            
                // get name of the symbol name in obj file that the relocations points to
                auto& standardSymbol = std::get<StandardSymbol>(obj.symbolTableEntries[reloc.symbolTableIndex]);
            
                if (standardSymbol.storageClass != StandardSymbol::StorageClass::External) {
                    // get the section+offset in PE that coresponds to the objAddressedSection
                    auto objAddressedSymbolName = arrayToStr(standardSymbol.name);
                    auto [peAddressedSectionNumber, addressedOffsetInSection] = objSectionNameToPeSection.at({&obj, strToArray(objAddressedSymbolName)});
                    auto& addressedSection = peFile.sections[peAddressedSectionNumber];
                    int addressedRVA = addressedSection.header.virtualAddress + addressedOffsetInSection;
                
                    if (reloc.type == RelocationEntry::TypeIntel386::Dir32va) {
                        *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA + options.imageBase;
                    } else if (reloc.type == RelocationEntry::TypeIntel386::Dir32rva) {
                        *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA;
                    } else if (reloc.type == RelocationEntry::Rel32) {
                        *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA - changedRVA - 5 - (reloc.virtualAddress - 1);
                    } else if (reloc.type != RelocationEntry::Absolute) {
                        return errorMessageOpt("unsuported relocation entry type");
                    }
                } else if (standardSymbol.storageClass == StandardSymbol::StorageClass::External) {
                    auto objAddressedSymbolName = getSymbolName(standardSymbol.name, obj.stringTable);
                    if (!objAddressedSymbolName) {
                        return errorMessageOpt("obj file is malformed - missing string table entry");
                    }

                    if (auto foundSymbol = symbolNameToPeSection.find(*objAddressedSymbolName); foundSymbol != symbolNameToPeSection.end()) { // it is symbol from other obj
                        auto peAddressedSymbolNumber = foundSymbol->second.sectionNr;
                        auto addressedOffsetInSection = foundSymbol->second.offset;
                        auto& addressedSection = peFile.sections[peAddressedSymbolNumber];
                        int addressedRVA = addressedSection.header.virtualAddress + addressedOffsetInSection;

                        if (reloc.type == RelocationEntry::TypeIntel386::Dir32va) {
                            *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA + options.imageBase;
                        } else if (reloc.type == RelocationEntry::TypeIntel386::Dir32rva) {
                            *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA;
                        } else if (reloc.type == RelocationEntry::Rel32) {
                            *reinterpret_cast<int*>(dataToChangePtr) += addressedRVA - changedRVA - 5 - (reloc.virtualAddress - 1);
                        } else if (reloc.type != RelocationEntry::Absolute) {
                            return errorMessageOpt("unsuported relocation entry type");
                        }
                    } else { // it is symbol from dll
                        auto dllJmpAddress = dllFunctionToJmpAddress.at(dllFunctionSymbolNameToRealName.at(*objAddressedSymbolName));
                        if (reloc.type == RelocationEntry::TypeIntel386::Dir32va) {
                            *reinterpret_cast<int*>(dataToChangePtr) = dllJmpAddress + options.imageBase;
                        } else if (reloc.type == RelocationEntry::TypeIntel386::Dir32rva) {
                            *reinterpret_cast<int*>(dataToChangePtr) = dllJmpAddress;
                        } else if (reloc.type == RelocationEntry::Rel32) {
                            *reinterpret_cast<int*>(dataToChangePtr) = dllJmpAddress - changedRVA - 5 - (reloc.virtualAddress - 1);
                        } else if (reloc.type != RelocationEntry::Absolute) {
                            return errorMessageOpt("unsuported relocation entry type");
                        }
                    }
                }
            }
        }
    }

    // find and set entry point
    auto entryPoint = symbolNameToPeSection.find(options.entryPoint);
    if (entryPoint == end(symbolNameToPeSection)) {
        return errorMessageOpt("couldn't find entry point: '"+options.entryPoint+"'");
    }
    int addressOfEntryPoint = peSections[entryPoint->second.sectionNr].header.virtualAddress + entryPoint->second.offset;
    

    // fileHeader
    auto& fileHeader = peFile.peHeader.fileHeader;

    fileHeader.machine = FileHeader::Machine::I386;
    fileHeader.numberOfSections = static_cast<word>(peSections.size());
    fileHeader.timeDateStamp = static_cast<dword>(time(nullptr));
    fileHeader.pointerToSymbolTable = 0;
    fileHeader.numberOfSymbols = 0;
    fileHeader.sizeOfOptionalHeader = OptionalHeader32::Size();
    fileHeader.characteristics = FileHeader::Characteristic::RelocsStripped
                               | FileHeader::Characteristic::ExecutableImage
                               | FileHeader::Characteristic::Machine32Bit
                               | FileHeader::Characteristic::DebugStripped;

    // optional header
    peFile.peHeader.optionalHeader = OptionalHeader32();
    auto& optionalHeader = std::get<OptionalHeader32>(peFile.peHeader.optionalHeader);

    optionalHeader.sizeOfCode = sizeOfCode;
    optionalHeader.sizeOfInitializedData = sizeOfInitializedData;
    optionalHeader.sizeOfUninitializedData = sizeOfUninitializedData;
    optionalHeader.addressOfEntryPoint = addressOfEntryPoint;
    optionalHeader.baseOfCode = peSections[0].header.virtualAddress;
    optionalHeader.baseOfData = baseOfData;
    optionalHeader.imageBase = options.imageBase;
    optionalHeader.sectionAlignment = options.sectionAllign;
    optionalHeader.fileAlignment = options.fileAllign;

    optionalHeader.majorOperatingSystemVersion = 0x4;
    optionalHeader.minorOperatingSystemVersion = 0x0;
    optionalHeader.majorImageVersion = 0x1;
    optionalHeader.minorImageVersion = 0x0;
    optionalHeader.majorSubsystemVersion = 0x4;
    optionalHeader.minorSubsystemVersion = 0x0;

    optionalHeader.sizeOfImage = sizeOfImage + (((sizeOfHeaders / options.sectionAllign) + 1) * options.sectionAllign);
    optionalHeader.sizeOfHeaders = sizeOfHeaders;

    optionalHeader.checkSum = 0;
    optionalHeader.subsystem = options.subsystem;
    optionalHeader.dllCharacteristics = 0;

    optionalHeader.sizeOfStackReserve = options.sizeOfStackReserve;
    optionalHeader.sizeOfStackCommit = options.sizeOfStackCommit;
    optionalHeader.sizeOfHeapReserve = options.sizeOfHeapReserve;
    optionalHeader.sizeOfHeapCommit = options.sizeOfHeapCommit;

    optionalHeader.numberOfRvaAndSizes = 0x10;
    for (dword i = 0; i < optionalHeader.numberOfRvaAndSizes; ++i) {
        optionalHeader.dataDirectories[i].size = 0;
        optionalHeader.dataDirectories[i].virtualAddress = 0;
    }
    optionalHeader.dataDirectories[OptionalHeader32::DataDirectoryTableId::Import] = importDataDirectory;
    optionalHeader.dataDirectories[OptionalHeader32::DataDirectoryTableId::IAT] = importAddressTableDirectory;

    return peFile;
}


int main(int argc, char** argv) {
    // parse command line arguments
    auto optionsOpt = getProgramOptions(argv[0], std::vector<char*>(argv+1, argv+argc));
    if (!optionsOpt) {
        errorMessageOpt("parsing command line options failed");
        return 1;
    }
    auto& options = *optionsOpt;
    if (options.onlyShowHelp) {
        return 0;
    }

    // read object files
    std::vector<ObjectFile> objFiles;
    for (auto& objFileName : options.objFileNames) {
        auto objFileOpt = readObjectFile<BinaryFile>(objFileName);
        if (!objFileOpt) {
            errorMessageOpt("couldn't read object file '" + objFileName + "'");
            return 2;
        }
        objFiles.emplace_back(*objFileOpt);
    }

    // load system dlls
    std::vector<std::string> dllNames = {
        "kernel32.dll", "user32.dll", "shell32.dll", "msvcrt.dll", 
        "gdi32.dll", "ole32.dll", "advapi32.dll", "comctl32.dll", "wsock32.dll", "mpr.dll"
    };
    for (const auto& dllName : dllNames) {
        auto loadedLib = LoadLibraryA(dllName.c_str()); 
        if (loadedLib != nullptr) { 
            options.dlls.emplace_back(dllName, loadedLib);
        }
    }

    // create PE file structure
    auto peFile = createPeFromObj(objFiles, options);
    if (!peFile) {
        errorMessageOpt("creating PE file structure failed");
        return 3;
    }

    // create PE file
    BinaryFile outFile(options.outputFileName, true);
    if (!write(outFile, *peFile, options.outputFileName)) {
        errorMessageOpt("creating PE file failed");
        remove(options.outputFileName.c_str());
        return 4;
    }

    // free loaded dlls
    for (auto& dll : options.dlls) {
        FreeLibrary(dll.second);
    }

    return 0;
}