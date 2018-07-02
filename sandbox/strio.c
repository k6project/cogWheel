/*
set string beginning to current position
scan buffer characters:
  if character is new line, stop and return result
  if reached the end:
    if number of bytes scanned is equal to buffer size:
        allocate new buffer of 1.5 size
    else:
        set current buffer as target
        if number of bytes scanned is more then half of buffer set flag to use memmove
    copy range from string beginning to start of the buffer
    set string beginning to start of the buffer
    set current position to length of copied rest data
*/

#include <stdio.h>
#include <stdlib.h>

#define FSFile FILE

struct FSTextFile
{
    FSFile* file;
    char* rdBuff;
    size_t rdMax;
    size_t rdPos;
};

#define FSREADLINE_MIN_BUFFER 1024

#define fsReadFile(b,s,f) fread(b,s,1,f)

/*
ioReadLine(ioInput_t src, char* dest, size_t size);
read line from src (terminated by LF or EOF), or (size-1)
io->readLine(input)
*/

const char* fsReadLine(struct FSTextFile* tFile)
{
    bool scan = true;
    size_t scanned = 0;
    const char* str = tFile->rdBuff + tFile->rdPos;
    while (scan)
    {
        if (tFile->rdPos == tFile->rdMax)
        {
            size_t toRead = 0;
            //todo: If EOF is reached, break out
            if (scanned == tFile->rdMax)
            {
                //Buffer is full, but the EOL is not found - increase buffer size
                toRead = (tFile->rdMax) ? (tFile->rdMax >> 1) : FSREADLINE_MIN_BUFFER;
                tFile->rdMax = toRead + tFile->rdMax;
                tFile->rdBuff = (char*)realloc(tFile->rdBuff, tFile->rdMax);
            }
            else
            {
                //Reached the end of the buffer without EOL, but there is reusable space; memory may overlap, hence memmove
                memmove(tFile->rdBuff, str, scanned);
                toRead = tFile->rdMax - scanned;
                tFile->rdPos = scanned;
            }
            assert(toRead > 0); //Should never happen )))
            //Read bytes into the free portion of the buffer
            void* buff = &tFile->rdBuff[tFile->rdPos];
            fsReadFile(buff, toRead, tFile->file);
        }
        switch (tFile->rdBuff[tFile->rdPos])
        {
        '\n':
            tFile->rdBuff[tFile->rdPos] = 0;
            scan = false;
            break;
        default:
            scanned++;
            break;
        }
        tFile->rdPos++;
    }
    return str;
}
