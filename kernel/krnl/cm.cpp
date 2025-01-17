#include <krnl/cm.hpp>
#include <krnl/common.hpp>
#include <krnl/physmgr.hpp>
#include <hal/vcache.hpp>
#include <hal/bus.hpp>
#include <hal/diskphys.hpp>
#include <libk/string.h>
#include <libk/ctype.h>

#pragma GCC optimize ("Os")
#pragma GCC optimize ("-fno-strict-aliasing")
#pragma GCC optimize ("-fno-align-labels")
#pragma GCC optimize ("-fno-align-jumps")
#pragma GCC optimize ("-fno-align-loops")
#pragma GCC optimize ("-fno-align-functions")

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcast-align"


/// <summary>
/// Opens a registry file and loads the registry hive into memory. The file is left open so other 
/// Cm functions may act upon the registry file it must be closed by CmClose otherwise the registry
/// state will be inconsistent. 
/// </summary>
/// <param name="filename">The filepath to the registry file.</param>
/// <returns>A pointer to a registry hive object with the data read into it.</returns>
Reghive* CmOpen(const char* filename)
{
    int br;
    bool dir;
    uint64_t size;

    Reghive* reg = (Reghive*) malloc(sizeof(Reghive));

    reg->f = new File(filename, kernelProcess);
    reg->f->stat(&size, &dir);
    reg->f->open(FileOpenMode::Read);

    uint8_t* data = (uint8_t*) malloc(size);
    reg->f->read(size, data, &br);
    reg->f->close();

    reg->f->open((FileOpenMode) (((int) FileOpenMode::Read) | ((int) FileOpenMode::Write) | ((int) FileOpenMode::CreateAlways)));
    reg->f->write(size, data, &br);
    free(data);

    if (!reg->f) {
        KePanic("REGISTRY FILE COULD NOT BE OPENED");
    }

    reg->f->seek(0);
    reg->f->read(sizeof(reg->header), &reg->header, &br);
    reg->f->seek(0);

    if (memcmp(reg->header.sig, "REGISTRY", 8)) {
        KePanic("REGISTRY SIGNATURE INVALID");
    }

    reg->valid = true;

    return reg;
}


/// <summary>
/// Closes the backend registry file, and closes the registry hive in memory.
/// </summary>
/// <param name="reg"></param>
void CmClose(Reghive* reg)
{
    if (!reg->valid) {
        return;
    }

    reg->f->close();
}


/// <summary>
/// Inserts an extent of a given type in the register under a given parent, with the given data. It also
/// flushes the newly updated registry data to the disk. This is a low level function, designed to be used
/// by the other registry functions. Other higher level functions hould be used to, for example, 
/// create strings, integers and directories.
/// </summary>
/// <param name="reg">The registry hive to add the extent into.</param>
/// <param name="parent">The parent extent to which the new extent should be added to. A value of zero indicates no parent, and therefore this extent must be referenced in some other use case specific form.</param>
/// <param name="type">The type of extent to add.</param>
/// <param name="data">A pointer to the data to copy into the extent (a copy is created).</param>
/// <returns></returns>
int CmCreateExtent(Reghive* reg, int parent, int type, uint8_t* data)
{
    int child = CmFindUnusedExtent(reg);

    Extent parentData;
    memset(&parentData, 0, EXTENT_LENGTH);
    if (parent) CmReadExtent(reg, parent, (uint8_t*) &parentData);

    Extent childData;
    memcpy(&childData, data, EXTENT_LENGTH);

    if (parent) {
        if (parentData.next) {
            childData.next = parentData.next;
            parentData.next = child;
        } else {
            childData.next = 0;
            parentData.next = child;
        }
    } else {
        childData.next = 0;
    }

    CmWriteExtent(reg, child, (uint8_t*) &childData);

    if (parent) {
        CmWriteExtent(reg, parent, (uint8_t*) &parentData);
    }

    return child;
}


/// <summary>
/// Adds a new blank string key in the registry with a given name, under a given parent extent. This 
/// will flush the registry to the file.
/// </summary>
/// <param name="reg">The registry hive to add the string key to.</param>
/// <param name="parent">The parent extent to add the string to.</param>
/// <param name="name">The name of the key. This must be a valid registry key name.</param>
/// <returns></returns>
int CmCreateString(Reghive* reg, int parent, const char* name)
{
    Extent ext;
    memset(&ext, 0, sizeof(Extent));

    CmConvertToInternalFilename(name, ext.s.name);
    ext.type = EXTENT_STRING;

    memset(ext.s.extents, 0, 21);

    return CmCreateExtent(reg, parent, 0, (uint8_t*) &ext);
}


/// <summary>
/// Marks an extent as unused. After this call the old data cannot be recovered, and this extent can be reused.
/// This is a low-level internal function that should only be called by other registry functions here. This will
/// flush the registry data to the file.
/// </summary>
/// <param name="reg">The registry hive where the extent is.</param>
/// <param name="extent">The extent to mark as unused.</param>
void CmFreeExtent(Reghive* reg, int extent)
{
    uint8_t data[EXTENT_LENGTH];
    memset(data, 0, EXTENT_LENGTH);
    CmWriteExtent(reg, extent, data);
}


/// <summary>
/// Returns the string data stored by a given string type extent. The output buffer must be at least 277 bytes long
/// even if the string data is shorter than this. If the string is 277 bytes long, a null character will not be placed
/// after it, and therefore care must be taken to avoid buffer overruns.
/// </summary>
/// <param name="reg">The registry hive where the extent is.</param>
/// <param name="extnum">The extent of the string object.</param>
/// <param name="out">The data stored the the given string type event will be copied here. This buffer must be at least 277 bytes long, even if the string is shorter.</param>
void CmGetString(Reghive* reg, int extnum, char* out)
{
    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);

    for (int i = 0; i < 7; ++i) {

        int num = ext.s.extents[i * 3 + 2];
        num <<= 8;
        num |= ext.s.extents[i * 3 + 1];
        num <<= 8;
        num |= ext.s.extents[i * 3 + 0];

        if (num != 0) {
            Extent data;
            CmReadExtent(reg, num, (uint8_t*) &data);

            memcpy(out + i * 39, data.x.data, 39);
        }
    }
}


/// <summary>
/// Sets the string data for a string type extent. The data can be up to 277 bytes long, and all 277
/// bytes from the data buffer will be copied, regardless of the length of the string.
/// This will flush the new registry data to the file.
/// </summary>
/// <param name="reg">The registry hive where the extent is.</param>
/// <param name="extnum">The extent of the string object.</param>
/// <param name="data">The data to be copied to the string object in the registry.</param>
void CmSetString(Reghive* reg, int extnum, const char* data)
{
    int CmComponents = (int) (strlen(data) + 38) / 39;
    if (CmComponents > 7) {
        KePanic("CmSetString TOO LONG");
    }

    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);

    for (int i = 0; i < 7; ++i) {

        int num = ext.s.extents[i * 3 + 2];
        num <<= 8;
        num |= ext.s.extents[i * 3 + 1];
        num <<= 8;
        num |= ext.s.extents[i * 3 + 0];

        if (i < CmComponents) {
            if (num == 0) {
                Extent blank;
                blank.type = EXTENT_STRING_DATA;
                num = CmCreateExtent(reg, 0, 0, (uint8_t*) &blank);

                ext.s.extents[i * 3] = num & 0xFF;
                ext.s.extents[i * 3 + 1] = (num >> 8) & 0xFF;
                ext.s.extents[i * 3 + 2] = (num >> 16) & 0xFF;
            }

            Extent compo;
            CmReadExtent(reg, num, (uint8_t*) &compo);
            memset(compo.x.data, 0, 39);
            strncpy((char*) compo.x.data, data + i * 39, 39);           //strncpy is correct here
            CmWriteExtent(reg, num, (uint8_t*) &compo);

        } else {
            if (num != 0) {
                CmFreeExtent(reg, num);

                ext.s.extents[i * 3] = 0;
                ext.s.extents[i * 3 + 1] = 0;
                ext.s.extents[i * 3 + 2] = 0;
            }
        }
    }

    CmWriteExtent(reg, extnum, (uint8_t*) &ext);
}


/// <summary>
/// Returns the extent number of the first sub-extent of a given directory.
/// This effectively 'enters the directory' by giving us the first child entry of a directory.
/// </summary>
/// <param name="reg">The registry hive where the directory is.</param>
/// <param name="num">The extent number of the directory to enter into.</param>
/// <returns>The extent number of the first sub-extent, or -1 if a non-directory extent is passed in.</returns>
int CmEnterDirectory(Reghive* reg, int num)
{
    Extent ext;
    CmReadExtent(reg, num, (uint8_t*) &ext);
    if (ext.type == EXTENT_DIRECTORY) {
        return ext.d.start;
    }
    return -1;
}


/// <summary>
/// Given an extent number (i.e. the first sub-extent in a directory from CmEnterDirectory), iterate
/// through the entries within this chain, searching for one with a key name given.
/// 
/// It is likely a bug if a directory extent (instead of the child extent) is passed in as the extent number.
/// </summary>
/// <param name="reg">The registry hive where the search is to be done.</param>
/// <param name="extent">The starting point to search from.</param>
/// <param name="name">The name of the key to search for. It must be a valid key name.</param>
/// <returns>Returns -1 if a key with that name is not found, or the extent number if it is found.</returns>
int CmFindInDirectory(Reghive* reg, int extent, const char* name)
{
    Extent ext;

    uint8_t wantName[18];
    memset(wantName, 0xFF, 18);
    CmConvertToInternalFilename(name, wantName);

    int num = extent;
    while (true) {
        if (!num) {
            return -1;
        }
        CmReadExtent(reg, num, (uint8_t*) &ext);
        if (ext.type == EXTENT_STRING_DATA && ext.type == EXTENT_BLANK) {
            KePanic("CmFindInDirectory ASSERTION FAILED");
        }
        if (!memcmp(wantName, ext.s.name, 18)) {
            return num;
        }
        num = ext.next;
    }
}


/// <summary>
/// Adds a new integer key into the registry, with a given value, parent, name and type.
/// This will flush the registry to the file.
/// </summary>
/// <param name="reg">The registry hive to add the integer to.</param>
/// <param name="parent">The parent extent where the integer should be added to.</param>
/// <param name="name">They key name of the integer. This must be a valid registry key.</param>
/// <param name="value">The value to set the integer to.</param>
/// <param name="type">The type (width) of the integer. Do not set this to anything which isn't an integer type.</param>
/// <returns></returns>
int CmCreateInteger(Reghive* reg, int parent, const char* name, uint64_t value, int type)
{
    Extent ext;
    memset(&ext, 0, sizeof(Extent));

    CmConvertToInternalFilename(name, ext.i.name);
    ext.type = type;
    ext.i.data = value;

    return CmCreateExtent(reg, parent, 0, (uint8_t*) &ext);
}


/// <summary>
/// Given an extent, returns the next extent in the same directory. If the extent passed in
/// is the last extent in the directory, zero will be returned.
/// </summary>
/// <param name="reg">The registry hive the extent is in.</param>
/// <param name="extnum">The current extent number.</param>
/// <returns>The next extent number. Returns zero if there is no next extent.</returns>
int CmGetNext(Reghive* reg, int extnum)
{
    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);
    return ext.next;
}


/// <summary>
/// Creates a new directory in the registry. This will flush the registry
/// to the file.
/// </summary>
/// <param name="reg">The registry hive to add the dircetory to.</param>
/// <param name="parent">The parent extent of the new directory.</param>
/// <param name="name">The key name to give to the directory. This name must be a valid key name.</param>
/// <returns>The extent number of the first sub-entry in the directory (the dummy object). This is *not* the extent number of the directory itself.</returns>
int CmCreateDirectory(Reghive* reg, int parent, const char* name)
{
    Extent ext;
    memset(&ext, 0, sizeof(Extent));

    CmConvertToInternalFilename(name, ext.d.name);
    ext.type = EXTENT_DIRECTORY;
    ext.d.start = 0;

    int next = parent;
    while (parent) {
        int nxt = CmGetNext(reg, next);
        if (nxt != 0) {
            next = nxt;
        } else {
            break;
        }
    }

    int a = CmCreateExtent(reg, next, 0, (uint8_t*) &ext);

    CmReadExtent(reg, a, (uint8_t*) &ext);
    ext.d.start = CmCreateInteger(reg, a, "DUMMY", 0, EXTENT_INTEGER32);
    CmWriteExtent(reg, a, (uint8_t*) &ext);

    return ext.d.start;
}


/// <summary>
/// Reads an extent from the registry file.
/// </summary>
/// <param name="reg">The registry hive to read from.</param>
/// <param name="extnum">The extent to read.</param>
/// <param name="data">The buffer to copy the data to. This must be long enough to store EXTENT_LENGTH bytes.</param>
void CmReadExtent(Reghive* reg, int extnum, uint8_t* data)
{
    if (!reg->valid) {
        return;
    }

    int br;
    reg->f->seek(extnum * EXTENT_LENGTH);
    reg->f->read(EXTENT_LENGTH, data, &br);
}


/// <summary>
/// Writes an extent to the registry file.
/// </summary>
/// <param name="reg">The registry hive to write to.</param>
/// <param name="extnum">The extent to write.</param>
/// <param name="data">The buffer to write the data from. EXTENT_LENGTH bytes will be copied from the buffer to the disk.</param>
void CmWriteExtent(Reghive* reg, int extnum, uint8_t* data)
{
    if (!reg->valid) {
        return;
    }

    int br;
    reg->f->seek(extnum * EXTENT_LENGTH);
    reg->f->write(EXTENT_LENGTH, data, &br);
}


/// <summary>
/// Flushes the registry hive back to the disk. Should be called whenever the header is updated.
/// </summary>
/// <param name="reg">The registry hive whose header needs flushing.</param>
void CmUpdateHeader(Reghive* reg)
{
    if (!reg->valid) {
        return;
    }

    CmWriteExtent(reg, 0, (uint8_t*) &reg->header);
}


/// <summary>
/// Increase the number of extents a registry hive can store. 
/// </summary>
/// <param name="reg">The registry hive to add extents to.</param>
/// <param name="extents">The number of extents to add to the directory.</param>
/// <returns>Returns the old number of extents in the registry, or -1 if the registry is invalid.</returns>
int CmExpand(Reghive* reg, int extents)
{
    if (!reg->valid) {
        return -1;
    }

    reg->f->seek(reg->header.numExtents * EXTENT_LENGTH);

    uint8_t data[EXTENT_LENGTH];
    memset(data, 0, EXTENT_LENGTH);
    for (int i = 0; i < extents; ++i) {
        int br;
        reg->f->write(EXTENT_LENGTH, data, &br);
    }

    int oldNumExtents = reg->header.numExtents;
    reg->header.numExtents += extents;

    CmUpdateHeader(reg);
    return oldNumExtents;
}


/// <summary>
/// Searches the registry for an unused extent, expanding the registry hive if needed. Therefore, it will
/// always find an unused extent.
/// </summary>
/// <param name="reg">The registry hive.</param>
/// <returns>The extent number of an unused extent. If the registry is invalid, -1 will be returned.</returns>
int CmFindUnusedExtent(Reghive* reg)
{
    if (!reg->valid) {
        return -1;
    }

    reg->f->seek(0);

    int extNum = 0;
    while (extNum < reg->header.numExtents) {
        uint8_t type;
        int br;
        reg->f->read(1, &type, &br);
        if (type == 0) {
            return extNum;
        }

        extNum++;
        reg->f->seek(EXTENT_LENGTH * extNum);
    }

    return CmExpand(reg, 64);
}


/// <summary>
/// Valid components of a registry key name. Each of these digraphs and characters take up one
/// character in the internal representation of the name. Any characters not in this table cannot
/// be encoded into a key name. Digraphs not in this table will be stored as two characters.
/// </summary>
char CmComponents[64][4] = {
    "TH", "HE", "IN", "ER", "AN", "RE", "ND", "AT", "ON", "NT", "HA", "ES",
    "ST", "EN", "ED", "TO", "IT", "OU", "EA", "HI", "IS", "OR", "TI", "AS",
    "TE", "ET", "NG", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
    "6", "7", "8", "9", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U",
    "0", "1", "2", "3", "4", "5", "V", "W", "X", "Y", "Z", 0,
};


/// <summary>
/// Checks whether two given strings are in the table of valid digraphs/characters for use in key names.
/// The first string will only be checked if it is asked for, and the second string will only be checked
/// if the first string was not valid, or wasn't checked.
/// </summary>
/// <param name="first">The first string to check, only checked if tryFirst is true.</param>
/// <param name="second">The second string to check, only check if tryFirst is false, or the first string was not in the table.</param>
/// <param name="tryFirst">Whether or not to check the first string.</param>
/// <param name="matchedFirst">Whether or not the match was from the first string (true) or the second string (false) will be stored here.</param>
/// <returns>The index in CmComponents where the matching string was found.</returns>
int CmGetMatch(char* first, char* second, bool tryFirst, bool* matchedFirst)
{
    if (tryFirst) {
        *matchedFirst = true;
        for (int i = 0; i < 63; ++i) {
            if (!strcmp(first, CmComponents[i])) return i;
        }
    }
    *matchedFirst = false;
    for (int i = 0; i < 63; ++i) {
        if (!strcmp(second, CmComponents[i])) return i;
    }

    return -1;
}


/// <summary>
/// Shifts a value of an arbitrary bit length onto a 32 bit shift register, as long as it will fit.
/// </summary>
/// <param name="reg">A pointer to the 32 bit shift register. The value stored here will be updated after calling this function.</param>
/// <param name="count">A pointer containing the number of bits currently on the shift register. The value stored here will be updated after calling this function.</param>
/// <param name="val">The value to be shifted on.</param>
/// <param name="bits">The number of bits used to represent 'val'.</param>
/// <returns>Returns true if val can fit onto the shift register, or false otherwise. If false is returned, 'reg' and 'count' will be unchanged.</returns>
bool CmAddShift(uint32_t* reg, int* count, uint8_t val, int bits)
{
    if (*count + bits < 32) {
        *reg |= (((uint32_t) val) << *count);
        *count += bits;
        return true;

    } else {
        return false;
    }
}


/// <summary>
/// Reads a value of an arbitrary bit length off a shift register, and then shift that register by that number of bits.
/// The shift register is 32 bits long.
/// </summary>
/// <param name="reg">A pointer to the 32 bit shift register. The value stored here will be updated after calling this function.</param>
/// <param name="count">A pointer containing the number of bits currently on the shift register. The value stored here will be updated after calling this function.</param>
/// <param name="bits">The number of bits in the value to return, and hence how many bits to shift off.</param>
/// <param name="success">If there is enough bits in the shift register, true will be written in the location pointed to. Otherwise, false will be written here.</param>
/// <returns>Returns zero if success contains false, or the value if success contains true.</returns>
uint8_t CmGetShift(uint32_t* reg, int* count, int bits, bool* success)
{
    if (*count >= bits) {
        uint8_t ret = (*reg) & ((1 << bits) - 1);
        *reg >>= bits;
        *count -= bits;
        *success = true;
        return ret;

    } else {
        *success = false;
        return 0;
    }
}


/// <summary>
/// Reads the integer value from an integer registry key.
/// </summary>
/// <param name="reg">The registry hive where the integer is.</param>
/// <param name="extnum">The extent number of the integer key.</param>
/// <param name="i">After this function is called, the integer value will be placed into the location pointed to.</param>
void CmGetInteger(Reghive* reg, int extnum, uint64_t* i)
{
    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);
    *i = ext.i.data;
}

/// <summary>
/// Sets the integer value of an integer registry key.
/// </summary>
/// <param name="reg">The registry hive where the integer is.</param>
/// <param name="extnum">The extent number of the integer key.</param>
/// <param name="i">The integer value to set the key's value to.</param>
void CmSetInteger(Reghive* reg, int extnum, uint64_t i)
{
    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);
    ext.i.data = i;
    CmWriteExtent(reg, extnum, (uint8_t*) &ext);
}


/// <summary>
/// For a given extent, returns the key name, and the type of key. The key name is decoded
/// into its human-readable form.
/// </summary>
/// <param name="reg">The register where the extent is in.</param>
/// <param name="extnum">The extent number to get information about.</param>
/// <param name="name">The name of the key, stored in human readable form. This buffer needs to have
/// enough room to store 49 characters./param>
/// <returns>Returns the type of the extent.</returns>
int CmGetNameAndTypeFromExtent(Reghive* reg, int extnum, char* name)
{
    Extent ext;
    CmReadExtent(reg, extnum, (uint8_t*) &ext);
    CmConvertFromInternalFilename(ext.s.name, name);
    return ext.type;
}


/// <summary>
/// Converts an internal key filename into a human-readable format.
/// </summary>
/// <param name="in">The internal key filename.</param>
/// <param name="out">The human-readable key filename will be copied to this buffer. The buffer
/// should be able to store at least 49 characters, regardless of the length of the internal string.</param>
/// <returns>Always returns STATUS_SUCCESS</returns>
int CmConvertFromInternalFilename(const uint8_t* in, char* out)
{
    uint8_t decoded[24];

    uint32_t sreg = 0;
    int sregcnt = 0;
    int ob = 0;

    memset(out, 0xEE, 18);

    for (int i = 0; i < 18; ++i) {
        CmAddShift(&sreg, &sregcnt, in[i], 8);

    retry:;
        bool success;
        uint8_t val = CmGetShift(&sreg, &sregcnt, 6, &success);

        if (success) {
            decoded[ob++] = val;
            goto retry;
        }
    }

    out[0] = 0;
    for (int i = 0; i < 24; ++i) {
        strcat(out, CmComponents[decoded[i]]);
    }

    return STATUS_SUCCESS;
}


/// <summary>
/// Converts a human-readable key name into the internal key name format.
/// </summary>
/// <param name="name">The human-readable key name. This must not be a full path, but just a key name.</param>
/// <param name="out">The internal key name will be stored in this buffer. It should be at least 24 bytes long,
/// regardless of the length of the human-readable name.</param>
/// <returns>Returns STATUS_SUCCESS when it could be successfully converted. If the resulting internal key name
/// is too long to be stored in the registry, STATUS_LONG_NAME will be returned. If there is a character in the
/// human-readable key name which is not allowed in an internal key, STATUS_NOT_ENCODABLE will be returend.</returns>
int CmConvertToInternalFilename(const char* name, uint8_t* out)
{
    char path[48];
    memset(path, 0, 48);
    for (int i = 0; i < strlen(name); ++i) {
        path[i] = toupper(name[i]);
    }

    uint8_t parts[24];
    int next = 0;

    memset(parts, 63, 24);

    for (int i = 0; path[i]; ++i) {
        char digraph[4];
        digraph[0] = path[i];
        digraph[1] = path[i + 1];
        digraph[2] = 0;

        char mono[3];
        mono[0] = path[i];
        mono[1] = 0;

        bool matchedDigraph;

        int thing;
        if (strlen(mono)) {
            thing = CmGetMatch(digraph, mono, true, &matchedDigraph);
        } else {
            thing = CmGetMatch(NULL, digraph, false, &matchedDigraph);
        }
        if (thing == -1) {
            KePanic("STATUS_NOT_ENCODABLE");
            return STATUS_NOT_ENCODABLE;
        }

        if (next == 24) {
            KePanic("STATUS_LONG_NAME");
            return STATUS_LONG_NAME;
        }

        parts[next++] = thing;
        if (matchedDigraph) {
            ++i;
        }

        if (!strlen(mono)) break;
    }

    uint32_t sreg = 0;
    int sregcnt = 0;
    int ob = 0;

    memset(out, 0xEE, 18);

    for (int i = 0; i < 100; ++i) {
        bool canAdd = i >= 24 ? false : CmAddShift(&sreg, &sregcnt, parts[i], 6);
        if (!canAdd) {
            bool success;
            uint8_t val = CmGetShift(&sreg, &sregcnt, 8, &success);

            if (success) {
                out[ob++] = val;
            } else {
                break;
            }

            --i;
        }
    }

    return STATUS_SUCCESS;
}


/// <summary>
/// Implementation of strtok. See the C standard for details.
/// </summary>
/// <param name="str">See C standard for details.</param>
/// <param name="delim">See C standard for details.</param>
/// <returns></returns>
char* zStrtok(char* str, const char* delim)
{
    static char* static_str = 0;      /* var to store last address */
    int index = 0, strlength = 0;           /* integers for indexes */
    int found = 0;                  /* check if delim is found */

    /* delimiter cannot be NULL
    * if no more char left, return NULL as well
    */
    if (delim == 0 || (str == 0 && static_str == 0))
        return 0;

    if (str == 0)
        str = static_str;

    /* get length of string */
    while (str[strlength])
        strlength++;

    /* find the first occurance of delim */
    for (index = 0; index < strlength; index++)
        if (str[index] == delim[0]) {
            found = 1;
            break;
        }

    /* if delim is not contained in str, return str */
    if (!found) {
        static_str = 0;
        return str;
    }

    /* check for consecutive delimiters
    *if first char is delim, return delim
    */
    if (str[0] == delim[0]) {
        static_str = (str + 1);
        return (char*) delim;
    }

    /* terminate the string
    * this assignmetn requires char[], so str has to
    * be char[] rather than *char
    */
    str[index] = '\0';

    /* save the rest of the string */
    if ((str + index + 1) != 0)
        static_str = (str + index + 1);
    else
        static_str = 0;

    return str;
}


/// <summary>
/// Follows a filepath and gets its the extent number of the object pointed to by that path.
/// </summary>
/// <param name="reg">The registry hive where the path is to be followed.</param>
/// <param name="__name">The filepath (it may contain forward slashes) to be followed.</param>
/// <returns>The extent number of the extent at that path, or -1 if the path does not exist, and 1 if you asked for the root folder.</returns>
int CmFindObjectFromPath(Reghive* reg, const char* __name)
{
    char name[256];
    strcpy(name, __name);

    int loc = 1;
    char* p = zStrtok(name, "/");
    if (!p) {
        return 1;
    }

    loc = CmFindInDirectory(reg, loc, p);
    p = zStrtok(NULL, "/");

    while (p) {
        loc = CmEnterDirectory(reg, loc);
        if (loc == -1) return -1;
        loc = CmFindInDirectory(reg, loc, p);
        p = zStrtok(NULL, "/");
    }

    return loc;
}

#pragma GCC diagnostic pop
