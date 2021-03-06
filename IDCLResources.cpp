// IDC Resource loader by Scorp0rx0r 09/13/20
//   Reader and writer for IDCLResources.
//   Based on the QuickBMS script from ZenHAX.

#pragma pack(push, 1)
typedef struct _IDCL_HEADER {
    // idstring "IDCL"
    long IdString;
    // get VER long  # 12
    long Version;
    // get DUMMY longlong
    long long Dummy;
    // get ZERO long
    long Zero;
    // get DUMMY long  # 1
    // get DUMMY long  #  - 1
    // get DUMMY long
    long unk[3];
    // #get DUMMY longlong
    // get FILES long
    long FileCount;
    // get DUMMY_NUM long
    // get DUMMY2_NUM long
    long unk1[2];
    // get FILES_2 long
    long IndexCount;
    // get ZERO longlong
    long long Zero2;
    // get DUMMY2 longlong
    long long unk2;
    // get NAMES_OFF longlong
    long long NamesOffset;
    // get DUMMY4_OFF longlong
    long long StringTableEnd;
    // get INFO_OFF longlong
    long long InfoOffset;
    // get DUMMY6_OFF longlong
    long long unk4;
    // get DUMMY7_OFF longlong
    long long FileOffset;
    // get DATA_OFF longlong
    long long DataOffset;
} IDCL_HEADER;

typedef struct _IDCL_FILE_DESCRIPTOR {
    // get ZERO longlong
    long long Zero;
    // get DUMMY longlong      # 1
    long long unk0;
    // get DUMMY longlong      #  - 1
    long long unk1;
    // get TYPE_ID longlong
    unsigned long long TypeId;
    // get NAME_ID longlong
    unsigned long long NameId;
    // get ZERO longlong
    long long Zero1;
    // get ZERO longlong
    long long Zero2;
    // get OFFSET longlong
    long long Offset;
    // get ZSIZE longlong
    long long CompressedSize;
    // get SIZE longlong
    long long UncompressedSize;
    // get DUMMY longlong
    long long unk2;
    // get DUMMY long
    long unk3;
    // get DUMMY long
    long unk4;
    // get DUMMY longlong
    long long unk5;
    // get DUMMY long          # ZIP related ?
    long unk6;
    // get DUMMY long          # ZIP related ?
    long unk7;
    // get ZIP_FLAGS longlong  # 0, 2, 4, 0x210002, 0x410002 and so on
    long long ZipFlags;
    // get ZERO longlong
    long long Zero3;
    // get FLAGS long          # 2
    long Flags;
    // get FLAGS2 long
    long Flags2;
    // get ZERO longlong
    long long Zero4;
} IDCL_FILE_DESCRIPTOR;

#pragma pack (pop)

#include <vector>
#include <string>
#include <stdio.h>

typedef struct _IDCT_FILE {
    long long Offset;
    unsigned long long Size;
    long long SizeUncompressed;
    long long NameOffset;
    long long TypeIdOffset;
    long long NameIdOffset;
    long long InfoOffset;
    IDCL_FILE_DESCRIPTOR FileInfo;
    std::string FileName;
    std::string TypeName;
} IDCT_FILE;

typedef struct _IDCT_STRING {
    long long NameOffset;
    std::string String;
} IDCT_STRING;

class IDCLReader {
    FILE* m_Handle;
    std::vector<IDCT_FILE> m_FileList;
    std::vector<IDCT_STRING> m_IdclStrings;
    bool ParseFileList();

public:
    std::string m_FileName;
    bool OpenFile(const char* FileName);
    std::vector<IDCT_FILE> GetFileList() { return m_FileList; }
    bool ReadFile(size_t Index, char* Buffer, size_t BufferSize);
    bool ReadFile(IDCT_FILE& File, char* Buffer, size_t Size);
    bool WriteFile(IDCT_FILE& File, char* Buffer, size_t BufferSize);
};

bool IDCLReader::OpenFile(const char* FileName)
{
    m_Handle = nullptr;
    int Error = fopen_s(&m_Handle, FileName, "rb");
    if (m_Handle == nullptr) {
        return false;
    }

    m_FileList.clear();
    m_IdclStrings.clear();
    m_FileName = std::string(FileName);
    bool Result = ParseFileList();
    fclose(m_Handle);
    return Result;
}

bool IDCLReader::ParseFileList() {
    FILE* Handle = m_Handle;

    _IDCL_HEADER Header;
    fread_s(&Header, sizeof(Header), sizeof(Header), 1, Handle);

    if (Header.IdString != 'LCDI') {
        return false;
    }

    //goto NAMES_OFF
    fseek(Handle, (long)Header.NamesOffset, SEEK_SET);
    
    //get NAMES longlong
    long long StringCount;
    fread(&StringCount, sizeof(StringCount), 1, Handle);

    // for i = 0 < NAMES
    //     get NAME_OFF longlong
    //     putarray 0 i NAME_OFF
    // next i
    // savepos NAMES_OFF
    // for i = 0 < NAMES
    //     getarray NAME_OFF 0 i
    //     math NAME_OFF + NAMES_OFF
    //     goto NAME_OFF
    //     get NAME string
    //     string NAME % "$"
    //     string NAME | "#"
    //     putarray 0 i NAME
    // next i
    // 
    // # thanks toxic72 https ://zenhax.com/viewtopic.php?p=27774#p27774

    m_IdclStrings.resize(StringCount);
    for (long long i = 0; i < StringCount; i += 1) {
        long long NameOffset;
        fread(&NameOffset, sizeof(NameOffset), 1, Handle);
        m_IdclStrings[i].NameOffset = NameOffset;
    }

    // Read the entire string table.
    long long StringTableSize = Header.StringTableEnd - ftell(Handle);
    std::vector<char> StringTable;
    StringTable.resize(StringTableSize);
    fread(&StringTable[0], 1, StringTableSize, Handle);
    for (long long i = 0; i < StringCount; i += 1) {
        char *StringStart = &StringTable[0] + m_IdclStrings[i].NameOffset;
        char* StringChar = StringStart;
        //while (*StringChar != 0 && *StringChar != '$' && *StringChar !='#') {
        while (*StringChar != 0) {
            StringChar += 1;
        }

        m_IdclStrings[i].String = std::string(StringStart, (size_t)(StringChar - StringStart));
    }

    //goto DUMMY7_OFF
    //for i = 0 < DUMMY2_NUM
    //    get DUMMY long
    //next i
    //savepos DUMMY7_OFF

    // Load the index table.
    size_t FileIdOffset = Header.FileOffset + (sizeof(long) * Header.unk1[1]);
    std::vector<unsigned long long> IndexTable;
    IndexTable.resize(Header.IndexCount);
    fseek(m_Handle, (long)FileIdOffset, SEEK_SET);
    fread(&(IndexTable[0]), sizeof(long long), Header.IndexCount, m_Handle);

    //goto INFO_OFF
    fseek(Handle, (long)(Header.InfoOffset), SEEK_SET);

    //for i = 0 < FILES
    //    get x IDCL_FILE_DESCRIPTOR
    unsigned long long MaxId = 0;
    m_FileList.resize(Header.FileCount);
    for (size_t i = 0; i < m_FileList.size(); i += 1) {
        fread(&(m_FileList[i].FileInfo), sizeof(m_FileList[i].FileInfo), 1, Handle);

        //    math TYPE_ID * 8
        //    math TYPE_ID + DUMMY7_OFF
        //    math NAME_ID + 1
        //    math NAME_ID * 8
        //    math NAME_ID + DUMMY7_OFF

        // TODOne: The index table should really just be loaded wholesale.
        //size_t TypeIdOffset = FileIdOffset + sizeof(long long) * m_FileList[i].FileInfo.TypeId;
        //size_t NameIdOffset = FileIdOffset + sizeof(long long) * (m_FileList[i].FileInfo.NameId + 1);

        //    savepos TMP
        //size_t TmpOffset = ftell(Handle);

        //    goto TYPE_ID
        //    get TYPE_ID longlong
        //    goto NAME_ID
        //    get NAME_ID longlong
        //unsigned long long TypeId;
        //unsigned long long NameId;
        //fseek(Handle, (long)TypeIdOffset, SEEK_SET);
        //fread(&TypeId, sizeof(TypeId), 1, Handle);
        //fseek(Handle, (long)NameIdOffset, SEEK_SET);
        //fread(&NameId, sizeof(NameId), 1, Handle);

        //    goto TMP
        //fseek(Handle, (long)TmpOffset, SEEK_SET);
        
        //    #getarray STR1 0 TYPE_ID
        //    getarray STR2 0 NAME_ID
        //    #string NAME p "%s/%s" STR1 STR2
        //    string NAME p "%s" STR2
        size_t TypeId = IndexTable[m_FileList[i].FileInfo.TypeId];
        size_t NameId = IndexTable[(m_FileList[i].FileInfo.NameId + 1)];
        m_FileList[i].Size = m_FileList[i].FileInfo.CompressedSize;
        m_FileList[i].SizeUncompressed = m_FileList[i].FileInfo.UncompressedSize;
        m_FileList[i].Offset = m_FileList[i].FileInfo.Offset;
        m_FileList[i].FileName = m_IdclStrings[NameId].String;
        m_FileList[i].TypeName = m_IdclStrings[TypeId].String;
        m_FileList[i].InfoOffset = Header.InfoOffset + sizeof(m_FileList[i].FileInfo) * i;
    }

    return true;
}

bool IDCLReader::ReadFile(IDCT_FILE &File, char* Buffer, size_t Size)
{
    if (File.Size > Size) {
        return false;
    }

    int Error = fopen_s(&m_Handle, m_FileName.c_str(), "rb");
    if (m_Handle == nullptr) {
        return false;
    }

    fseek(m_Handle,(long) File.Offset, SEEK_SET);
    fread(Buffer, File.Size, 1, m_Handle);
    fclose(m_Handle);
    return true;
}

bool IDCLReader::ReadFile(size_t Index, char *Buffer, size_t Size)
{
    if (Index > m_FileList.size()) {
        return false;
    }

    return ReadFile(m_FileList[Index], Buffer, Size);
}

bool IDCLReader::WriteFile(IDCT_FILE& File, char* Buffer, size_t Size)
{
    fopen_s(&m_Handle, m_FileName.c_str(), "rb+");
    if (m_Handle == nullptr) {
        return false;
    }

    fseek(m_Handle, 0, SEEK_END);
    size_t FileSize = ftell(m_Handle);
    if ((Size <= File.Size) || ((File.Size + File.Offset) == FileSize)) {
        fseek(m_Handle, (long)File.Offset, SEEK_SET);
        fwrite(Buffer, 1, Size, m_Handle);
        long long Padding = (int)(File.Size - Size);
        if (Padding > 0) {
            std::vector<char> Zero;
            Zero.resize(Padding);
            fwrite(&(Zero[0]), 1, Padding, m_Handle);
        }

        if (Padding != 0) {
            // Modify the file info.
            IDCT_FILE NewFileInfo = File;
            fseek(m_Handle, (long)File.InfoOffset, SEEK_SET);
            NewFileInfo.FileInfo.CompressedSize = Size;
            NewFileInfo.FileInfo.UncompressedSize = Size;
            fwrite(&(NewFileInfo.FileInfo), sizeof(NewFileInfo.FileInfo), 1, m_Handle);
        }

    } else {
        // Append at end.
        fseek(m_Handle, 0, SEEK_END);
        size_t NewOffset = ftell(m_Handle);
        fwrite(Buffer, 1, Size, m_Handle);

        // Modify the file info.
        IDCT_FILE NewFileInfo = File;
        fseek(m_Handle, (long)File.InfoOffset, SEEK_SET);
        NewFileInfo.FileInfo.Offset = NewOffset;
        NewFileInfo.FileInfo.CompressedSize = Size;
        NewFileInfo.FileInfo.UncompressedSize = Size;
        fwrite(&(NewFileInfo.FileInfo), sizeof(NewFileInfo.FileInfo), 1, m_Handle);

        // Zero out the original file.
        fseek(m_Handle, (long)File.Offset, SEEK_SET);
        std::vector<char> Zero;
        Zero.resize(File.FileInfo.CompressedSize);
        fwrite(&(Zero[0]), 1, Size, m_Handle);
    }

    fflush(m_Handle);
    fclose(m_Handle);
    return true;
}