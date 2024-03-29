/*! \file attrcache.cpp
 * \brief Attribute caching module.
 *
 * $Id$
 *
 * The functions here manage the upper-level attribute value cache for
 * disk-based mode. It's not used in memory-based builds. The lower-level
 * cache is managed in svdhash.cpp
 *
 * The upper-level cache is organized by a CHashTable and a linked list. The
 * former allows random access while the linked list helps find the
 * least-recently-used attribute.
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"
#include "externs.h"

#if !defined(MEMORY_BASED)

static CHashFile hfAttributeFile;
static bool cache_initted = false;

static bool cache_redirected = false;
#define N_TEMP_FILES 8
static FILE *TempFiles[N_TEMP_FILES];

CLinearTimeAbsolute cs_ltime;

#pragma pack(1)
typedef struct tagAttrRecord
{
    Aname attrKey;
    UTF8 attrText[LBUF_SIZE];
} ATTR_RECORD, *PATTR_RECORD;
#pragma pack()

static ATTR_RECORD TempRecord;

typedef struct tagCacheEntryHeader
{
    struct tagCacheEntryHeader *pPrevEntry;
    struct tagCacheEntryHeader *pNextEntry;
    Aname attrKey;
    size_t nSize;
} CENT_HDR, *PCENT_HDR;

static PCENT_HDR pCacheHead = 0;
static PCENT_HDR pCacheTail = 0;
static size_t CacheSize = 0;

int cache_init(const UTF8 *game_dir_file, const UTF8 *game_pag_file,
    int nCachePages)
{
    if (cache_initted)
    {
        return HF_OPEN_STATUS_ERROR;
    }

    int cc = hfAttributeFile.Open(game_dir_file, game_pag_file, nCachePages);
    if (cc != HF_OPEN_STATUS_ERROR)
    {
        // Mark caching system live
        //
        cache_initted = true;
        cs_ltime.GetUTC();
    }
    return cc;
}

void cache_redirect(void)
{
    for (int i = 0; i < N_TEMP_FILES; i++)
    {
        UTF8 TempFileName[20];
        mux_sprintf(TempFileName, sizeof(TempFileName), T("$convtemp.%d"), i);
        mux_assert(mux_fopen(&TempFiles[i], TempFileName, T("wb+")));
        mux_assert(TempFiles[i]);
        setvbuf(TempFiles[i], nullptr, _IOFBF, 16384);
    }
    cache_redirected = true;
}

void cache_pass2(void)
{
    ATTR_RECORD Record;
    cache_redirected = false;
    mux_fprintf(stderr, T("2nd Pass:\n"));
    for (int i = 0; i < N_TEMP_FILES; i++)
    {
        mux_fprintf(stderr, T("File %d: "), i);
        long int li = fseek(TempFiles[i], 0, SEEK_SET);
        mux_assert(0L == li);

        int cnt = 1000;
        size_t nSize;
        for (;;)
        {
            size_t cc = fread(&nSize, 1, sizeof(nSize), TempFiles[i]);
            if (cc != sizeof(nSize))
            {
                break;
            }
            cc = fread(&Record, 1, nSize, TempFiles[i]);
            mux_assert(cc == nSize);
            cache_put(&Record.attrKey, Record.attrText, nSize - sizeof(Aname));
            if (cnt-- == 0)
            {
                fputc('.', stderr);
                fflush(stderr);
                cnt = 1000;
            }
        }
        fclose(TempFiles[i]);
        UTF8 TempFileName[20];
        mux_sprintf(TempFileName, sizeof(TempFileName), T("$convtemp.%d"), i);
        RemoveFile(TempFileName);
        mux_fprintf(stderr, T(ENDLINE));
    }
}

void cache_cleanup(void)
{
    for (int i = 0; i < N_TEMP_FILES; i++)
    {
        fclose(TempFiles[i]);
        UTF8 TempFileName[20];
        mux_sprintf(TempFileName, sizeof(TempFileName), T("$convtemp.%d"), i);
        RemoveFile(TempFileName);
    }
}

void cache_close(void)
{
    hfAttributeFile.CloseAll();
    cache_initted = false;
}

void cache_tick(void)
{
    hfAttributeFile.Tick();
}

static void REMOVE_ENTRY(PCENT_HDR pEntry)
{
    // How is X positioned?
    //
    if (pEntry == pCacheHead)
    {
        if (pEntry == pCacheTail)
        {
            // HEAD --> X --> 0
            //    0 <--  <-- TAIL
            //
            // ASSERT: pEntry->pNextEntry == 0;
            // ASSERT: pEntry->pPrevEntry == 0;
            //
            pCacheHead = pCacheTail = 0;
        }
        else
        {
            // HEAD  --> X --> Y --> 0
            //    0 <--   <--   <--  TAIL
            //
            // ASSERT: pEntry->pNextEntry != 0;
            // ASSERT: pEntry->pPrevEntry == 0;
            //
            pCacheHead = pEntry->pNextEntry;
            pCacheHead->pPrevEntry = 0;
            pEntry->pNextEntry = 0;
        }
    }
    else if (pEntry == pCacheTail)
    {
        // HEAD  --> Y --> X --> 0
        //    0 <--   <--   <-- TAIL
        //
        // ASSERT: pEntry->pNextEntry == 0;
        // ASSERT: pEntry->pPrevEntry != 0;
        //
        pCacheTail = pEntry->pPrevEntry;
        pCacheTail->pNextEntry = 0;
        pEntry->pPrevEntry = 0;
    }
    else
    {
        // HEAD  --> Y --> X --> Z --> 0
        //    0 <--   <--   <--   <-- TAIL
        //
        // ASSERT: pEntry->pNextEntry != 0;
        // ASSERT: pEntry->pNextEntry != 0;
        //
        pEntry->pNextEntry->pPrevEntry = pEntry->pPrevEntry;
        pEntry->pPrevEntry->pNextEntry = pEntry->pNextEntry;
        pEntry->pNextEntry = 0;
        pEntry->pPrevEntry = 0;
    }
}

static void ADD_ENTRY(PCENT_HDR pEntry)
{
    if (pCacheHead)
    {
        pCacheHead->pPrevEntry = pEntry;
    }
    pEntry->pNextEntry = pCacheHead;
    pEntry->pPrevEntry = 0;
    pCacheHead = pEntry;
    if (!pCacheTail)
    {
        pCacheTail = pCacheHead;
    }
}

static void TrimCache(void)
{
    // Check to see if the cache needs to be trimmed.
    //
    while (CacheSize > mudconf.max_cache_size)
    {
        // Blow something away.
        //
        PCENT_HDR pCacheEntry = pCacheTail;
        if (!pCacheEntry)
        {
            CacheSize = 0;
            break;
        }

        REMOVE_ENTRY(pCacheEntry);
        CacheSize -= pCacheEntry->nSize;
        hashdeleteLEN(&(pCacheEntry->attrKey), sizeof(Aname),
            &mudstate.acache_htab);
        MEMFREE(pCacheEntry);
        pCacheEntry = nullptr;
    }
}

const UTF8 *cache_get(Aname *nam, size_t *pLen)
{
    if (  nam == (Aname *) 0
       || !cache_initted)
    {
        *pLen = 0;
        return nullptr;
    }

    PCENT_HDR pCacheEntry = nullptr;
    if (!mudstate.bStandAlone)
    {
        // Check the cache, first.
        //
        pCacheEntry = (PCENT_HDR)hashfindLEN(nam, sizeof(Aname),
            &mudstate.acache_htab);
        if (pCacheEntry)
        {
            // It was in the cache, so move this entry to the head of the queue.
            // and return a pointer to it.
            //
            REMOVE_ENTRY(pCacheEntry);
            ADD_ENTRY(pCacheEntry);
            if (sizeof(CENT_HDR) < pCacheEntry->nSize)
            {
                *pLen = pCacheEntry->nSize - sizeof(CENT_HDR);
                return (UTF8 *)(pCacheEntry+1);
            }
            else
            {
                *pLen = 0;
                return nullptr;
            }
        }
    }

    UINT32 nHash = CRC32_ProcessInteger2(nam->object, nam->attrnum);
    UINT32 iDir = hfAttributeFile.FindFirstKey(nHash);

    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        hfAttributeFile.Copy(iDir, &nRecord, &TempRecord);

        if (  TempRecord.attrKey.attrnum == nam->attrnum
           && TempRecord.attrKey.object == nam->object)
        {
            int nLength = nRecord - sizeof(Aname);
            *pLen = nLength;
            if (!mudstate.bStandAlone)
            {
                // Add this information to the cache.
                //
                pCacheEntry = (PCENT_HDR)MEMALLOC(sizeof(CENT_HDR)+nLength);
                if (pCacheEntry)
                {
                    pCacheEntry->attrKey = *nam;
                    pCacheEntry->nSize = nLength + sizeof(CENT_HDR);
                    CacheSize += pCacheEntry->nSize;
                    memcpy((char *)(pCacheEntry+1), TempRecord.attrText, nLength);
                    ADD_ENTRY(pCacheEntry);
                    hashaddLEN(nam, sizeof(Aname), pCacheEntry,
                        &mudstate.acache_htab);

                    TrimCache();
                }
            }
            return TempRecord.attrText;
        }
        iDir = hfAttributeFile.FindNextKey(iDir, nHash);
    }

    // We didn't find that one.
    //
    if (!mudstate.bStandAlone)
    {
        // Add this information to the cache.
        //
        pCacheEntry = (PCENT_HDR)MEMALLOC(sizeof(CENT_HDR));
        if (pCacheEntry)
        {
            pCacheEntry->attrKey = *nam;
            pCacheEntry->nSize = sizeof(CENT_HDR);
            CacheSize += pCacheEntry->nSize;
            ADD_ENTRY(pCacheEntry);
            hashaddLEN(nam, sizeof(Aname), pCacheEntry,
                &mudstate.acache_htab);

            TrimCache();
        }
    }

    *pLen = 0;
    return nullptr;
}


// cache_put no longer frees the pointer.
//
bool cache_put(Aname *nam, const UTF8 *value, size_t len)
{
    if (  !value
       || !nam
       || !cache_initted
       || len == 0)
    {
        return false;
    }
#if defined(HAVE_WORKING_FORK)
    if (mudstate.write_protect)
    {
        Log.tinyprintf(T("cache_put((%d,%d), \xE2\x80\x98%s\xE2\x80\x99, %u) while database is write-protected" ENDLINE),
            nam->object, nam->attrnum, value, len);
        return false;
    }
#endif // HAVE_WORKING_FORK

    if (len > sizeof(TempRecord.attrText))
    {
        len = sizeof(TempRecord.attrText);
    }

    // Removal from DB.
    //
    UINT32 nHash = CRC32_ProcessInteger2(nam->object, nam->attrnum);

    if (cache_redirected)
    {
        TempRecord.attrKey = *nam;
        memcpy(TempRecord.attrText, value, len);
        TempRecord.attrText[len-1] = '\0';

        int iFile = (N_TEMP_FILES-1) & (nHash >> 29);
        size_t nSize = len+sizeof(Aname);
        fwrite(&nSize, 1, sizeof(nSize), TempFiles[iFile]);
        fwrite(&TempRecord, 1, nSize, TempFiles[iFile]);
        return true;
    }

    UINT32 iDir = hfAttributeFile.FindFirstKey(nHash);
    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        hfAttributeFile.Copy(iDir, &nRecord, &TempRecord);

        if (  TempRecord.attrKey.attrnum == nam->attrnum
           && TempRecord.attrKey.object  == nam->object)
        {
            hfAttributeFile.Remove(iDir);
        }
        iDir = hfAttributeFile.FindNextKey(iDir, nHash);
    }

    TempRecord.attrKey = *nam;
    memcpy(TempRecord.attrText, value, len);
    TempRecord.attrText[len-1] = '\0';

    // Insertion into DB.
    //
    if (!hfAttributeFile.Insert((HP_HEAPLENGTH)(len+sizeof(Aname)), nHash, &TempRecord))
    {
        Log.tinyprintf(T("cache_put((%d,%d), \xE2\x80\x98%s\xE2\x80\x99, %u) failed" ENDLINE),
            nam->object, nam->attrnum, value, len);
    }

    if (!mudstate.bStandAlone)
    {
        // Update cache.
        //
        PCENT_HDR pCacheEntry = (PCENT_HDR)hashfindLEN(nam, sizeof(Aname),
            &mudstate.acache_htab);
        if (pCacheEntry)
        {
            // It was in the cache, so delete it.
            //
            REMOVE_ENTRY(pCacheEntry);
            CacheSize -= pCacheEntry->nSize;
            hashdeleteLEN((char *)nam, sizeof(Aname), &mudstate.acache_htab);
            MEMFREE(pCacheEntry);
            pCacheEntry = nullptr;
        }

        // Add information about the new entry back into the cache.
        //
        size_t nSizeOfEntry = sizeof(CENT_HDR) + len;
        pCacheEntry = (PCENT_HDR)MEMALLOC(nSizeOfEntry);
        if (pCacheEntry)
        {
            pCacheEntry->attrKey = *nam;
            pCacheEntry->nSize = nSizeOfEntry;
            CacheSize += pCacheEntry->nSize;
            memcpy((char *)(pCacheEntry+1), TempRecord.attrText, len);
            ADD_ENTRY(pCacheEntry);
            hashaddLEN(nam, sizeof(Aname), pCacheEntry,
                &mudstate.acache_htab);

            TrimCache();
        }
    }
    return true;
}

bool cache_sync(void)
{
    hfAttributeFile.Sync();
    return true;
}

// Delete this attribute from the database.
//
void cache_del(Aname *nam)
{
    if (  !nam
       || !cache_initted)
    {
        return;
    }

#if defined(HAVE_WORKING_FORK)
    if (mudstate.write_protect)
    {
        Log.tinyprintf(T("cache_del((%d,%d)) while database is write-protected" ENDLINE),
            nam->object, nam->attrnum);
        return;
    }
#endif // HAVE_WORKING_FORK

    UINT32 nHash = CRC32_ProcessInteger2(nam->object, nam->attrnum);
    UINT32 iDir = hfAttributeFile.FindFirstKey(nHash);

    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        hfAttributeFile.Copy(iDir, &nRecord, &TempRecord);

        if (  TempRecord.attrKey.attrnum == nam->attrnum
           && TempRecord.attrKey.object == nam->object)
        {
            hfAttributeFile.Remove(iDir);
        }
        iDir = hfAttributeFile.FindNextKey(iDir, nHash);
    }

    if (!mudstate.bStandAlone)
    {
        // Update cache.
        //
        PCENT_HDR pCacheEntry = (PCENT_HDR)hashfindLEN(nam, sizeof(Aname),
            &mudstate.acache_htab);
        if (pCacheEntry)
        {
            // It was in the cache, so delete it.
            //
            REMOVE_ENTRY(pCacheEntry);
            CacheSize -= pCacheEntry->nSize;;
            hashdeleteLEN((char *)nam, sizeof(Aname), &mudstate.acache_htab);
            MEMFREE(pCacheEntry);
            pCacheEntry = nullptr;
        }
    }
}

#endif // MEMORY_BASED
