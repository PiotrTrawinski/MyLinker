#pragma once
#include "usingTypes.h"

#include <optional>
#include <variant>
#include <array>
#include <vector>

struct AuxiliarySymbolFunctionDefinition {
    dword tagIndex;              // The symbol-table index of the corresponding .bf (begin function) symbol record. 
    dword totalSize;             // The size of the executable code for the function itself. If the function is in its own section, the SizeOfRawData in the section header is greater or equal to this field, depending on alignment considerations. 
    dword pointerToLineNumber;   // The file offset of the first COFF line-number entry for the function, or zero if none exists. 
    dword pointerToNextFunction; // The symbol-table index of the record for the next function. If the function is the last in the symbol table, this field is set to zero. 
};

struct AuxiliarySymbolFunctionBeginEndSymbols {
    word lineNumber;             // The actual ordinal line number (1, 2, 3, and so on) within the source file, corresponding to the .bf or .ef record. 
    dword pointerToNextFunction; // The symbol-table index of the next .bf symbol record. If the function is the last in the symbol table, this field is set to zero. It is not used for .ef records. 
};

struct AuxiliarySymbolWeakExternals {
    enum Characteristics : dword {
        IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY = 1, // no library search for unresolved external symbol should be performed. 
        IMAGE_WEAK_EXTERN_SEARCH_LIBRARY = 2, // library search for unresolved external symbol should be performed. 
        IMAGE_WEAK_EXTERN_SEARCH_ALIAS = 3 // unresolved external symbol is an alias for symbol with index = tagIndex. 
    };

    dword tagIndex; // The symbol-table index of symbol, the symbol to be linked if unresolved external symbol is not found. 
    dword characteristics;
};

struct AuxiliarySymbolFile {
    byte fileName[18]; // An ANSI string that gives the name of the source file. This is padded with nulls if it is less than the maximum length. 
};

struct AuxiliarySymbolSectionDefinition {
    dword length;             // The size of section data; the same as SizeOfRawData in the section header. 
    word numberOfRelocations; // The number of relocation entries for the section. 
    word numberOfLinenumbers; // The number of line-number entries for the section. 
    dword checkSum;           // The checksum for communal data. 
    word number;              // One-based index into the section table for the associated section. This is used when the COMDAT selection setting is 5. 
    byte selection;           // The COMDAT selection number. This is applicable if the section is a COMDAT section. 
};

using AuxiliarySymbol = std::variant<
    AuxiliarySymbolFunctionDefinition, 
    AuxiliarySymbolFunctionBeginEndSymbols,
    AuxiliarySymbolWeakExternals,
    AuxiliarySymbolFile,
    AuxiliarySymbolSectionDefinition
>;


struct StandardSymbol {
    enum SpecialSectionNumberValues : word {
        SymbolUndefined =  0, // The symbol record is not yet assigned a section. A value of zero indicates that a reference to an external symbol is defined elsewhere.
        SymbolAbsolute  = word(-1), // The symbol has an absolute (non-relocatable) value and is not an address. 
        SymbolDebug     = word(-2), // The symbol provides general type or debugging information but does not correspond to a section.
    };
    enum StorageClass : byte {
        External = 2, 
        Static = 3, 
        Function = 101, 
        File = 103 
    };
    enum Type : word {
        NotFunction = 0,
        IsFunction = 0x20
    };

    std::array<byte, 8> name; // The name of the symbol, represented by a union of three structures. An array of 8 bytes is used if the name is not more than 8 bytes long.
    dword value;              // The value that is associated with the symbol. The interpretation of this field depends on SectionNumber and StorageClass. A typical meaning is the relocatable address. 
    word sectionNumber;       // The signed integer that identifies the section, using a one-based index into the section table. Some values have special meaning.
    word type;                // A number that represents type. Microsoft tools set this field to 0x20 (function) or 0x0 (not a function). 
    byte storageClass;        // An enumerated value that represents storage class. 
    byte numberOfAuxSymbols;  // The number of auxiliary symbol table entries that follow this record. 
};

using SymbolTableEntry = std::variant<
    StandardSymbol, 
    AuxiliarySymbol
>;


template<typename Reader> bool readSymbolTableEntry(Reader& reader, std::vector<SymbolTableEntry>& symbolTable, int& i) {
    StandardSymbol standardSymbol;

    reader >> standardSymbol.name;
    reader >> standardSymbol.value;
    reader >> standardSymbol.sectionNumber;
    reader >> standardSymbol.type;
    reader >> standardSymbol.storageClass;
    reader >> standardSymbol.numberOfAuxSymbols;

    i += standardSymbol.numberOfAuxSymbols;

    symbolTable.emplace_back(standardSymbol);

    for (int i = 0; i < standardSymbol.numberOfAuxSymbols; ++i) {
        if (standardSymbol.storageClass == StandardSymbol::StorageClass::External
            && standardSymbol.type == StandardSymbol::Type::IsFunction
            && standardSymbol.sectionNumber > 0)
        {
            AuxiliarySymbolFunctionDefinition functionDefinition;
            reader >> functionDefinition.tagIndex;
            reader >> functionDefinition.totalSize;
            reader >> functionDefinition.pointerToLineNumber;
            reader >> functionDefinition.pointerToNextFunction;
            byte b;
            reader >> b >> b;
            symbolTable.emplace_back(functionDefinition);
        }
        else if (standardSymbol.storageClass == StandardSymbol::StorageClass::Function) {
            AuxiliarySymbolFunctionBeginEndSymbols functionBeginEndSymbols;
            byte b;
            reader >> b >> b >> b >> b;
            reader >> functionBeginEndSymbols.lineNumber;
            reader >> b >> b >> b >> b >> b >> b;
            reader >> functionBeginEndSymbols.pointerToNextFunction;
            reader >> b >> b;
            symbolTable.emplace_back(functionBeginEndSymbols);
        }
        else if (standardSymbol.storageClass == StandardSymbol::StorageClass::External
            && standardSymbol.sectionNumber == StandardSymbol::SpecialSectionNumberValues::SymbolUndefined
            && standardSymbol.value == 0) 
        {
            AuxiliarySymbolWeakExternals weakExternals;
            reader >> weakExternals.tagIndex;
            reader >> weakExternals.characteristics;
            byte b;
            reader >> b >> b >> b >> b >> b >> b >> b >> b >> b >> b;
            symbolTable.emplace_back(weakExternals);
        }
        else if (standardSymbol.storageClass == StandardSymbol::StorageClass::File) {
            AuxiliarySymbolFile file;
            reader >> file.fileName;
            symbolTable.emplace_back(file);
        }
        else if (standardSymbol.storageClass == StandardSymbol::StorageClass::Static) {
            AuxiliarySymbolSectionDefinition sectionDefinition;
            reader >> sectionDefinition.length;
            reader >> sectionDefinition.numberOfRelocations;
            reader >> sectionDefinition.numberOfLinenumbers;
            reader >> sectionDefinition.checkSum;
            reader >> sectionDefinition.number;
            reader >> sectionDefinition.selection;
            byte b;
            reader >> b >> b >> b;
            symbolTable.emplace_back(sectionDefinition);
        }
    }

    return reader;
}