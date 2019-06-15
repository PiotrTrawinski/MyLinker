#pragma once
#include "usingTypes.h"
#include "DosHeader.h"
#include "PeHeader.h"
#include "SectionHeader.h"
#include "ImportDirectory.h"

#include <vector>
#include <optional>
#include <algorithm>
#include <string_view>
#include <filesystem>

namespace fs = std::filesystem;

struct PeSection {
    SectionHeader header;
    std::vector<byte> data;
};

struct PeFile {
    DosHeader dosHeader;
    PeHeader peHeader;
    std::vector<PeSection> sections;
};

template<typename Writer> bool write(Writer& out, const PeFile& peFile, std::string_view filePath) {
    // set appropriate file size and fill it with zeros
    out.setPosition(0);
    auto& lastSectionHeader = peFile.sections.back().header;
    fs::resize_file(filePath, lastSectionHeader.pointerToRawData + lastSectionHeader.sizeOfRawData);
    /*for (int i = 0; i < lastSectionHeader.pointerToRawData + lastSectionHeader.sizeOfRawData; ++i) {
        out << (byte)0;
    }*/

    // write headers at the start of the file
    out.setPosition(0);
    write(out, peFile.dosHeader);
    write(out, peFile.peHeader);
    for (auto& imageSection : peFile.sections) {
        write(out, imageSection.header);
    }

    // write sections contents
    for (auto& imageSection : peFile.sections) {
        out.setPosition(imageSection.header.pointerToRawData);
        out.write(reinterpret_cast<const char*>(imageSection.data.data()), imageSection.data.size());
    }

    return bool(out);
}