#pragma once
#include "usingTypes.h"
#include "FileHeader.h"
#include "SectionHeader.h"
#include "SymbolTableEntry.h"
#include "errorMessages.h"

#include <vector>
#include <optional>
#include <map>
#include <iomanip>
#include <iostream>

struct RelocationEntry {
    enum TypeIntel386 {
        Absolute = 0x0, // The relocation is ignored. 
        Dir32va  = 0x6, // The target's 32-bit VA. 
        Dir32rva = 0x7, // The target's 32-bit RVA. 
        Section  = 0xa, // The 16-bit section index of the section that contains the target. This is used to support debugging information. 
        Secrel   = 0xb, // The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage. 
        Token    = 0xc, // The CLR token. 
        Secrel7  = 0xd, // A 7-bit offset from the base of the section that contains the target. 
        Rel32    = 0x14 // The 32-bit relative displacement to the target. This supports the x86 relative branch and call instructions. 
    };

    /*
        The address of the item to which relocation is applied. 
        This is the offset from the beginning of the section, plus the value of the section's RVA/Offset field
    */
    dword virtualAddress; 

    /*
        A zero-based index into the symbol table. This symbol gives the address that is to be used for the relocation. 
        If the specified symbol has section storage class, then the symbol's address is the address with the first section of the same name. 
    */
    dword symbolTableIndex;

    /*
        A value that indicates the kind of relocation that should be performed. Valid relocation types depend on machine type. 
    */
    word type;
};

template<typename Reader> std::optional<RelocationEntry> readRelocationEntry(Reader& reader) {
    RelocationEntry relocationEntry;

    reader >> relocationEntry.virtualAddress;
    reader >> relocationEntry.symbolTableIndex;
    reader >> relocationEntry.type;

    return reader ? relocationEntry : std::optional<RelocationEntry>(std::nullopt);
}

struct ObjectSection {
    SectionHeader header;
    std::vector<byte> data;
    std::vector<RelocationEntry> relocationTable;
};

struct ObjectFile {
    FileHeader fileHeader;
    std::vector<ObjectSection> sections;
    std::vector<SymbolTableEntry> symbolTableEntries;
    std::map<int, std::string> stringTable;
};

template <typename Reader> std::optional<ObjectFile> readObjectFile(const std::string& objFileName) {
    Reader inFile(objFileName, false);
    if (!inFile) return std::nullopt;

    ObjectFile objFile;

    // read file header
    auto fileHeader = readFileHeader(inFile);
    if (!fileHeader) return errorMessageOpt("Couldn't read file header in obj file '" + objFileName + "'");
    objFile.fileHeader = *fileHeader;

    // read section headers
    objFile.sections.resize(objFile.fileHeader.numberOfSections);
    for (auto& section : objFile.sections) {
        auto sectionHeader = readSectionHeader(inFile);
        if (!sectionHeader) return errorMessageOpt("Couldn't read section header in obj file '" + objFileName + "'");
        section.header = *sectionHeader;
    }

    // read sections data
    for (auto& section : objFile.sections) {
        inFile.setPosition(section.header.pointerToRawData);
        section.data.resize(section.header.sizeOfRawData);
        inFile.read(reinterpret_cast<char*>(section.data.data()), static_cast<int>(section.data.size()));

        if (section.header.numberOfRelocations > 0) {
            inFile.setPosition(section.header.pointerToRelocations);
            section.relocationTable.resize(section.header.numberOfRelocations);
            for (auto& relocationEntry : section.relocationTable) {
                auto relocation = readRelocationEntry(inFile);
                if (!relocation) return errorMessageOpt("Couldn't read relocation entry in obj file '" + objFileName + "'");
                relocationEntry = *relocation;
            }
        }
    }

    // read symbol table entries
    inFile.setPosition(objFile.fileHeader.pointerToSymbolTable);
    objFile.symbolTableEntries.reserve(objFile.fileHeader.numberOfSymbols);
    for (int i = 0; i < static_cast<int>(objFile.fileHeader.numberOfSymbols); ++i) {
        if (!readSymbolTableEntry(inFile, objFile.symbolTableEntries, i)) {
            return errorMessageOpt("Couldn't read symbol table entry in obj file '" + objFileName + "'");
        }
    }

    // read string table
    dword stringTableSize = 0;
    inFile >> stringTableSize;

    int stringByteIndex = 4;
    while (inFile) {
        byte b = 0;
        std::string stringEntry = "";
        while (true) {
            inFile >> b;
            if (b == 0) break;
            stringEntry += b;
        }
        if (!stringEntry.empty()) {
            objFile.stringTable.emplace(stringByteIndex, stringEntry);
            stringByteIndex += stringEntry.size() + 1;
        }
    }

    return objFile;
}


void dump(const ObjectFile& objFile) {
    std::cout << std::hex;
    std::cout << "machine              : " << objFile.fileHeader.machine << '\n';
    std::cout << "numberOfSections     : " << objFile.fileHeader.numberOfSections << '\n';
    std::cout << "timeDateStamp        : " << objFile.fileHeader.timeDateStamp << '\n';
    std::cout << "pointerToSymbolTable : " << objFile.fileHeader.pointerToSymbolTable << '\n';
    std::cout << "numberOfSymbols      : " << objFile.fileHeader.numberOfSymbols << '\n';
    std::cout << "sizeOfOptionalHeader : " << objFile.fileHeader.sizeOfOptionalHeader << '\n';
    std::cout << "characteristics      : " << objFile.fileHeader.characteristics << '\n';
    std::cout << '\n';

    for (auto& section : objFile.sections) {
        std::cout << "section '";
        for (byte b : section.header.name) std::cout << b;
        std::cout << "':\n";
        std::cout << "  " << "header: \n";
        std::cout << "    virtualSize          : " << section.header.virtualSize << '\n';
        std::cout << "    virtualAddress       : " << section.header.virtualAddress << '\n';
        std::cout << "    sizeOfRawData        : " << section.header.sizeOfRawData << '\n';
        std::cout << "    pointerToRawData     : " << section.header.pointerToRawData << '\n';
        std::cout << "    pointerToRelocations : " << section.header.pointerToRelocations << '\n';
        std::cout << "    pointerToLineNumbers : " << section.header.pointerToLineNumbers << '\n';
        std::cout << "    numberOfRelocations  : " << section.header.numberOfRelocations << '\n';
        std::cout << "    numberOfLineNumbers  : " << section.header.numberOfLineNumbers << '\n';
        std::cout << "    characteristics      : " << section.header.characteristics << '\n';
        std::cout << "  " << "data: \n";
        std::cout << "    ";
        for (auto b : section.data) {
            std::cout << (int)b << ' ';
        }
        std::cout << '\n';
        std::cout << "  " << "relocation table: \n";
        for (auto& relocationEntry : section.relocationTable) {
            std::cout << "    entry: \n";
            std::cout << "      virtualAddress   : " << relocationEntry.virtualAddress << '\n';
            std::cout << "      symbolTableIndex : " << relocationEntry.symbolTableIndex << '\n';
            std::cout << "      type             : " << relocationEntry.type << '\n';
        }
    }

    std::cout << '\n';

    for (size_t i = 0; i < objFile.symbolTableEntries.size(); ++i) {
        auto& symbolTableEntry = objFile.symbolTableEntries[i];
        if (std::holds_alternative<StandardSymbol>(symbolTableEntry)) {
            auto symbol = std::get<StandardSymbol>(symbolTableEntry);
            std::cout << "standard symbol '";
            for (byte b : symbol.name) std::cout << b;
            std::cout << "':\n";
            std::cout << "value              : " << (int)symbol.value << '\n';
            std::cout << "sectionNumber      : " << (int)symbol.sectionNumber << '\n';
            std::cout << "type               : " << (int)symbol.type << '\n';
            std::cout << "storageClass       : " << (int)symbol.storageClass << '\n';
            std::cout << "numberOfAuxSymbols : " << (int)symbol.numberOfAuxSymbols << '\n';
        } else {
            auto symbol = std::get<AuxiliarySymbol>(symbolTableEntry);
            std::cout << "Auxiliary Symbol: \n";
            if (std::holds_alternative<AuxiliarySymbolFunctionDefinition>(symbol)) {
                auto& functionDefinition = std::get<AuxiliarySymbolFunctionDefinition>(symbol);
                std::cout << "tagIndex              : " << (int)functionDefinition.tagIndex << '\n';
                std::cout << "totalSize             : " << (int)functionDefinition.totalSize << '\n';
                std::cout << "pointerToLineNumber   : " << (int)functionDefinition.pointerToLineNumber << '\n';
                std::cout << "pointerToNextFunction : " << (int)functionDefinition.pointerToNextFunction << '\n';
            } else if (std::holds_alternative<AuxiliarySymbolFunctionBeginEndSymbols>(symbol)) {
                auto& functionBeginEndSymbols = std::get<AuxiliarySymbolFunctionBeginEndSymbols>(symbol);
                std::cout << "lineNumber            : " << (int)functionBeginEndSymbols.lineNumber << '\n';
                std::cout << "pointerToNextFunction : " << (int)functionBeginEndSymbols.pointerToNextFunction << '\n';
            } else if (std::holds_alternative<AuxiliarySymbolWeakExternals>(symbol)) {
                auto& weakExternals = std::get<AuxiliarySymbolWeakExternals>(symbol);
                std::cout << "tagIndex              : " << (int)weakExternals.tagIndex << '\n';
                std::cout << "characteristics       : " << (int)weakExternals.characteristics << '\n';
            } else if (std::holds_alternative<AuxiliarySymbolFile>(symbol)) {
                auto& file = std::get<AuxiliarySymbolFile>(symbol);
                std::cout << "fileName              : " << file.fileName << '\n';
            } else if (std::holds_alternative<AuxiliarySymbolSectionDefinition>(symbol)) {
                auto& sectionDefinition = std::get<AuxiliarySymbolSectionDefinition>(symbol);
                std::cout << "length                : " << (int)sectionDefinition.length << '\n';
                std::cout << "numberOfRelocations   : " << (int)sectionDefinition.numberOfRelocations << '\n';
                std::cout << "numberOfLinenumbers   : " << (int)sectionDefinition.numberOfLinenumbers << '\n';
                std::cout << "checkSum              : " << (int)sectionDefinition.checkSum << '\n';
                std::cout << "number                : " << (int)sectionDefinition.number << '\n';
                std::cout << "selection             : " << (int)sectionDefinition.selection << '\n';
            }
        }
        std::cout << '\n';
    }

    std::cout << "String table:\n";
    std::cout << std::left;
    for (auto& stringEntry : objFile.stringTable) {
        std::cout << std::setw(5) << stringEntry.first << stringEntry.second << '\n';
    }
    std::cout << '\n';
}
