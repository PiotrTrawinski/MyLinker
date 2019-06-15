#pragma once
#include "usingTypes.h"
#include "FileHeader.h"
#include "OptionalHeader32.h"
#include "OptionalHeader64.h"

#include <variant>
#include <optional>

struct PeHeader {
    /* 
        Signature that identifies the file as a PE format image file.
        It's equal to 0x00004550 ("PE\0\0")
    */
    dword signature = 0x00004550;

    FileHeader fileHeader;
    std::variant<OptionalHeader32, OptionalHeader64> optionalHeader = OptionalHeader32();

    static int Size32() {
        return sizeof(dword) + FileHeader::Size() + OptionalHeader32::Size();
    }
    static int Size64() {
        return sizeof(dword) + FileHeader::Size() + OptionalHeader64::Size();
    }
    int size() {
        if (std::holds_alternative<OptionalHeader32>(optionalHeader)) {
            return sizeof(dword) + FileHeader::Size() + OptionalHeader32::Size();
        } else {
            return sizeof(dword) + FileHeader::Size() + OptionalHeader64::Size();
        }
    }
};

template<typename Reader> std::optional<PeHeader> readPeHeader(Reader& reader) {
    return std::nullopt;
}

template<typename Writer> void write(Writer& out, const PeHeader& peHeader) {
    out << peHeader.signature;
    write(out, peHeader.fileHeader);
    if (std::holds_alternative<OptionalHeader32>(peHeader.optionalHeader)) {
        write(out, std::get<OptionalHeader32>(peHeader.optionalHeader));
    } else {
        write(out, std::get<OptionalHeader64>(peHeader.optionalHeader));
    }
}