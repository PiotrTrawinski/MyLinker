#pragma once
#include "usingTypes.h"

#include <optional>

struct DataDirectory {
    dword virtualAddress = 0;
    dword size = 0;

    static int Size() {
        return 8;
    }
};

template<typename Reader> std::optional<DataDirectory> readDataDirectory(Reader& reader) {
    DataDirectory dataDirectory;

    reader >> dataDirectory.virtualAddress;
    reader >> dataDirectory.size;

    return reader ? dataDirectory : std::optional<DataDirectory>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const DataDirectory& dataDirectory) {
    out << dataDirectory.virtualAddress
        << dataDirectory.size;
}