#pragma once
#include "usingTypes.h"

#include <optional>

struct DosHeader {
    /* 
        e_magic.
        Magic number of an PE file. It is always equal to 0x5a4d ('MZ')
    */
    word magicNumber = 0x5a4d;

    /*
        e_cblp.
        Value in range <0, 512>. value of 0 is effectivly the same as 512.
        Number of used bytes on the last page (512 byte sized blocks) of the file.
        In other words it's (sizeOfImage % 512)
    */
    word lastPageSize = 0x90;

    /*
        e_cp.
        Number of pages (512 byte sized blocks) in file.
        For example, if the file contains 1024 bytes, this field would equal to 2.
        In other words: sizeOfImage = (numberOfPages-1)*512 + lastPageSize
    */
    word numberOfPages = 0x03;

    /*
        e_crlc.
        Number of entries that exist in the relocation pointer table stored after the header.
    */
    word numberOfRelocationEntries = 0;

    /*
        e_cparhdr.
        Size of the header in paragraphs (16 byte sized blocks).
        The header includes the relocation entries.
        The header always spans an even number of paragraphs.
        Note that some OSs and/or programs may fail if the header is not a multiple of 512 bytes.
    */
    word headerSize = 0x4;

    /*
        e_minalloc.
        Minimum number of extra paragraphs (16 byte sized blocks) required by program.
        In other words required memory size is sizeOfLoadModule (=PE size wihout header) + minExtraMemory.
        The program can't be loaded if there isn't at least this much memory available to it.
    */
    word minExtraMemory = 0;

    /*
        e_maxalloc.
        Maximum number of extra paragraphs (16 byte sized blocks) on top of the minExtraMemory
        that the program would like allocated to it before it begins execution.
        Normally, the OS reserves all the remaining conventional memory for your program, 
        but you can limit it with this field.
    */
    word maxExtraMemory = 0xffff;

    /*
        e_ss.
        Initial value of the Stack Segment register.
        It contains the address of the stack segment in paragraphs (16 byte sized blocks)
        relative to the start of the load module
    */
    word stackSegmentInitialValue = 0;

    /*
        e_sp.
        Initial value of the Stack Pointer register.
    */
    word stackPointerInitialValue = 0xb8;

    /*
        e_csum.
        Complemented Checksum. It is equal to the one's complement of the 16-bit sum of all words in file
        Its value is rarely checked, but its purpose is to ensure the integrity of the data within the file.
        Usually, this isn't filled in.
    */
    word complementedCheckSum = 0;

    /*
        e_ip.
        Initial value of the Instruction Pointer register.
    */
    word instructionPointerInitialValue = 0;

    /*
        e_cs.
        Initial value of the Code Segment register, relative to the start of the load module
    */
    word codeSegmentInitialValue = 0;

    /*
        e_lfarlc.
        File address of the relocation table (offset of the first relocation table entry).
    */
    word relocationTableOffset = 0x40;

    /*
        e_ovno.
        Overlay number.
        It's usually set to 0, because few programs actually have overlays. 
        It changes only in files containing programs that use overlays
    */
    word overlayNumber = 0;

    /*
        e_res.
        reserved
    */
    byte reserved1[8] = {};

    /*
        e_oemid.
        Specifies the identifier for the OEM for oemInfo.
    */
    word oemId = 0;

    /*
        e_oeminfo.
        Specifies the OEM information for a specific value of e_oeminfo.
    */
    word oemInfo = 0;

    /*
        e_res2.
        reserved
    */
    byte reserved2[20] = {};

    /*
        e_lfanew.
        File address of PE header (Offset to the 'PE\0\0' signature relative to the beginning of the file)
    */
    dword peHeaderOffset = 0x40;

    int size() {
        return peHeaderOffset;
    }

    static int Size() {
        return 0x40;
    }
};

template<typename Reader> std::optional<DosHeader> readDosHeader(Reader& reader) {
    DosHeader dosHeader;

    reader >> dosHeader.magicNumber;
    reader >> dosHeader.lastPageSize;
    reader >> dosHeader.numberOfPages;
    reader >> dosHeader.numberOfRelocationEntries;
    reader >> dosHeader.headerSize;
    reader >> dosHeader.minExtraMemory;
    reader >> dosHeader.maxExtraMemory;
    reader >> dosHeader.stackSegmentInitialValue;
    reader >> dosHeader.stackPointerInitialValue;
    reader >> dosHeader.complementedCheckSum;
    reader >> dosHeader.instructionPointerInitialValue;
    reader >> dosHeader.codeSegmentInitialValue;
    reader >> dosHeader.relocationTableOffset;
    reader >> dosHeader.overlayNumber;
    reader >> dosHeader.reserved1;
    reader >> dosHeader.oemId;
    reader >> dosHeader.oemInfo;
    reader >> dosHeader.reserved2;
    reader >> dosHeader.peHeaderOffset;

    for (int i = 0x40; i < dosHeader.peHeaderOffset; ++i) {
        byte b = 0;
        reader >> b;
    }

    return reader ? dosHeader : std::optional<DosHeader>(std::nullopt);
}

template<typename Writer> void write(Writer& out, const DosHeader& dosHeader) {
    out << dosHeader.magicNumber
        << dosHeader.lastPageSize
        << dosHeader.numberOfPages
        << dosHeader.numberOfRelocationEntries
        << dosHeader.headerSize
        << dosHeader.minExtraMemory
        << dosHeader.maxExtraMemory
        << dosHeader.stackSegmentInitialValue
        << dosHeader.stackPointerInitialValue
        << dosHeader.complementedCheckSum
        << dosHeader.instructionPointerInitialValue
        << dosHeader.codeSegmentInitialValue
        << dosHeader.relocationTableOffset
        << dosHeader.overlayNumber
        << dosHeader.reserved1
        << dosHeader.oemId
        << dosHeader.oemInfo
        << dosHeader.reserved2
        << dosHeader.peHeaderOffset;

    for (dword i = 0x40; i < dosHeader.peHeaderOffset; ++i) {
        byte b = 0;
        out << b;
    }
}