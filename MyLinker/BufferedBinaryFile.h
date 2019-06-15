#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <vector>
#include <optional>
#include <algorithm>
#include <memory>


class BufferedBinaryFile {
public:
    BufferedBinaryFile(const BufferedBinaryFile& other)=delete;
    BufferedBinaryFile(int bufferSize=4096) :
        buffer(bufferSize)
    {}
    BufferedBinaryFile(const std::string& filePath, bool createFile=false, int bufferSize=4096) :
        filePath(filePath),
        buffer(bufferSize)
    {
        open(filePath, createFile);
    }
    virtual ~BufferedBinaryFile() {
        close();
    }
    
    BufferedBinaryFile operator=(const BufferedBinaryFile& other)=delete;

    void close() {
        if (file) {
            if (currentOperation == Operation::Writing) {
                writeBuffer();
            }
            fclose(file);
            file = nullptr;
        }
    }

    bool open(const std::string& newFilePath, bool createFile=false) {
        filePath = newFilePath;
        clearFail();
        close();
        bufferOffset = 0;
        bufferFillSize = 0;

        if (createFile) {
            file = fopen(filePath.c_str(), "w+b");
            currentOperation = Operation::Writing;
        } else {
            file = fopen(filePath.c_str(), "r+b");
            currentOperation = Operation::Reading;
        }

        return file;
    }

    void clearFail() {
        failFlag = false;
    }

    void flush() {
        if (!file) return;
        if (currentOperation == Operation::Writing) {
            writeBuffer();
        }
        fflush(file);
    }

    void setPosition(long newPosition) {
        if (!file) return;
        finalizeIo();
        clearFail();
        fseek(file, newPosition, SEEK_SET);
        bufferOffset = 0;
        bufferFillSize = 0;
    }
    long getPosition() {
        if (file) {
            return ftell(file) - bufferFillSize + bufferOffset;
        } else {
            return 0;
        }
    }

    int read(char* resultBuffer, int sizeInBytes) {
        if (!*this) return 0;
        switchToReading();

        int totalReadBytes = 0;
        do {
            int readBytes = readMaxPossible(resultBuffer + totalReadBytes, sizeInBytes - totalReadBytes);
            if (readBytes <= 0) {
                return totalReadBytes;
            }
            totalReadBytes += readBytes;
        } while (totalReadBytes < sizeInBytes);

        return sizeInBytes;
    }
    template<typename T, int Size> int read(T (&resultBuffer)[Size]) {
        return read(reinterpret_cast<char*>(resultBuffer), Size * sizeof(T));
    }
    template<typename T> std::optional<T> read() {
        if (!*this) return std::nullopt;
        switchToReading();
        
        char resultBuffer[sizeof(T)];
        int totalReadBytes = 0;
        do {
            int readBytes = readMaxPossible(&resultBuffer[0]+totalReadBytes, sizeof(T) - totalReadBytes);
            if (readBytes <= 0) {
                return std::nullopt;
            }
            totalReadBytes += readBytes;
        } while (totalReadBytes < sizeof(T));

        return *reinterpret_cast<T*>(resultBuffer);
    }
    template<> std::optional<char> read<char>() {
        if (!*this) return std::nullopt;
        switchToReading();

        if (bufferOffset >= bufferFillSize) {
            readBuffer();
            if (bufferOffset >= bufferFillSize) {
                return std::nullopt;
            }
        }

        return buffer[bufferOffset++];
    }

    template<typename T> BufferedBinaryFile& operator>>(T& value) {
        auto readResult = read<T>();
        if (readResult) value = *readResult;
        return *this;
    }
    template<typename T, int Size> BufferedBinaryFile& operator>>(T (&array)[Size]) {
        read(array);
        return *this;
    }

    bool write(const char* bytes, const int sizeInBytes) {
        if (!*this) return false;
        switchToWriting();

        int totalWrittenBytes = 0;
        do {
            int writtenBytes = writeMaxPossible(bytes, sizeInBytes - totalWrittenBytes);
            if (writtenBytes <= 0) {
                return false;
            }
            bytes += writtenBytes;
            totalWrittenBytes += writtenBytes;
        } while (totalWrittenBytes < sizeInBytes);

        return true;
    }

    template<typename T> bool write(const T t) {
        return write(reinterpret_cast<const char*>(&t), sizeof(T));
    }
    template<typename T, int Size> bool write(const T (&t)[Size]) {
        return write(reinterpret_cast<const char*>(t), sizeof(T)*Size);
    }
    template<> bool write(const char c) {
        if (!*this) return false;
        switchToWriting();

        if (bufferOffset >= buffer.size()) {
            if (!writeBuffer()) return false;
        }
        buffer[bufferOffset++] = c;
        return true;
    }

    template<typename T> BufferedBinaryFile& operator<<(const T c) {
        write(c);
        return *this;
    }
    template<typename T, int Size> BufferedBinaryFile& operator<<(const T (&c)[Size]) {
        write(c);
        return *this;
    }

    operator bool() {
        return file && !failFlag;
    }

    std::string getFilePath() {
        return filePath;
    }

private:
    enum class Operation {Writing, Reading};

    int writeMaxPossible(const char* bytes, int sizeInBytes) {
        if (bufferOffset >= buffer.size()) {
            if(!writeBuffer()) return 0;
        }
        if (bufferOffset + sizeInBytes >= buffer.size()) {
            int partSize = static_cast<int>(buffer.size()) - bufferOffset;
            memcpy(buffer.data()+bufferOffset, bytes, partSize);
            bufferOffset = static_cast<int>(buffer.size());
            if(!writeBuffer()) return 0;
            return partSize;
        } else {
            memcpy(buffer.data()+bufferOffset, bytes, sizeInBytes);
            bufferOffset += sizeInBytes;
            return sizeInBytes;
        }
    }

    int readMaxPossible(char* resultBuffer, int sizeInBytes) {
        if (bufferOffset >= bufferFillSize) {
            readBuffer();
        }
        if (failFlag) {
            return 0;
        }
        if (bufferOffset + sizeInBytes > bufferFillSize) {
            int partSize = std::max(0, static_cast<int>(bufferFillSize - bufferOffset));
            memcpy(resultBuffer, buffer.data()+bufferOffset, partSize);
            readBuffer();
            return partSize;
        } else {
            memcpy(resultBuffer, buffer.data()+bufferOffset, sizeInBytes);
            bufferOffset += sizeInBytes;
            return sizeInBytes;
        }
    }

    void switchToWriting() {
        if (currentOperation == Operation::Reading) {
            if (bufferOffset < bufferFillSize) {
                fseek(file, bufferOffset - bufferFillSize, SEEK_CUR);
            }
            bufferOffset = 0;
            fflush(file);
            currentOperation = Operation::Writing;
        }
    }
    void switchToReading() {
        if (currentOperation == Operation::Writing) {
            writeBuffer();
            bufferOffset = 0;
            bufferFillSize = 0;
            fflush(file);
            currentOperation = Operation::Reading;
        }
    }

    void finalizeIo() {
        if (currentOperation == Operation::Writing) {
            writeBuffer();
        } else {
            bufferOffset = 0;
        }
    }

    bool writeBuffer() {
        if (bufferOffset > 0) {
            if (!fwrite(buffer.data(), bufferOffset, 1, file)) {
                return false;
            }
            bufferOffset = 0;
        }
        return true;
    }
    void readBuffer() {
        // read only if last read buffer was full or it is first time reading after opening/changing position
        if (bufferFillSize == 0 || bufferFillSize >= buffer.size()) {
            bufferFillSize = static_cast<int>(fread(buffer.data(), 1, buffer.size(), file));
        } else {
            bufferFillSize = 0;
        }
        bufferOffset = 0;
        if (bufferFillSize <= 0) {
            failFlag = true;
        }
    }

    FILE* file = nullptr;
    std::string filePath = "";
    bool failFlag = false;
    Operation currentOperation = Operation::Reading;
    std::vector<char> buffer; 
    int bufferOffset = 0;
    int bufferFillSize = 0; // for reading only
};