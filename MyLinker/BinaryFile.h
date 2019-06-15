#pragma once

#include <vector>
#include <optional>
#include <fstream>

class BinaryFile {
public:
    BinaryFile(const BinaryFile& other)=delete;
    BinaryFile(const std::string& filePath, bool createFile=false) {
        open(filePath, createFile);
    }
    virtual ~BinaryFile() {
        file.close();
    }

    BinaryFile operator=(const BinaryFile& other)=delete;

    void close() {
        file.close();
    }

    bool open(const std::string& newFilePath, bool createFile=false) {
        filePath = newFilePath;
        if (createFile) {
            file.open(filePath, std::ios::trunc | std::ios::in | std::ios::out | std::ios::binary);
        } else {
            file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
        }
        return bool(file);
    }

    void flush() {
        file.flush();
    }

    void setPosition(long newPosition) {
        file.seekg(newPosition);
        file.seekp(newPosition);
    }
    long getPosition() {
        return static_cast<long>(file.tellg());
    }

    bool read(char* resultBuffer, int sizeInBytes) {
        return bool(file.read(resultBuffer, sizeInBytes));
    }
    template<typename T, int Size> bool read(T (&resultBuffer)[Size]) {
        return read(reinterpret_cast<char*>(resultBuffer), Size * sizeof(T));
    }
    template<typename T> std::optional<T> read() {
        char resultBuffer[sizeof(T)];
        if (read(reinterpret_cast<char*>(&resultBuffer[0]), sizeof(T))) {
            return *reinterpret_cast<T*>(resultBuffer);
        } else {
            return std::nullopt;
        }
    }
    template<> std::optional<char> read<char>() {
        char c;
        file >> c;
        if (file) {
            return c;
        } else {
            return std::nullopt;
        }
    }

    template<typename T> BinaryFile& operator>>(T& value) {
        auto readResult = read<T>();
        if (readResult) value = *readResult;
        return *this;
    }
    template<typename T, int Size> BinaryFile& operator>>(T (&array)[Size]) {
        read(array);
        return *this;
    }

    bool write(const char* bytes, const int sizeInBytes) {
        return bool(file.write(bytes, sizeInBytes));
    }

    template<typename T> bool write(const T t) {
        return write(reinterpret_cast<const char*>(&t), sizeof(T));
    }
    template<typename T, int Size> bool write(const T (&t)[Size]) {
        return write(reinterpret_cast<const char*>(t), sizeof(T)*Size);
    }
    template<> bool write(const char c) {
        return bool(file << c);
    }

    template<typename T> BinaryFile& operator<<(const T c) {
        write(c);
        return *this;
    }
    template<typename T, int Size> BinaryFile& operator<<(const T (&c)[Size]) {
        write(c);
        return *this;
    }

    operator bool() {
        return bool(file);
    }

    std::string getFilePath() {
        return filePath;
    }

private:
    std::string filePath = "";
    std::fstream file;
};