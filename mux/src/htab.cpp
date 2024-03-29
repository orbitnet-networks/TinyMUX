/*! \file htab.cpp
 * \brief Table hashing routines.
 *
 * $Id$
 *
 * The functions here outsource most of their work to CHashTable.  There are
 * several reasons to use the functions here instead of using CHashTable
 * directly: 1) they are briefer to use, 2) this interface predates
 * CHashTable, 3) there are many references to these functions throughout the
 * code, and 4) MUSH hardcoders are generally more familiar with this
 * interface than with the CHashTable interface.
 *
 * To replace them all would require rexamining the assumptions near every
 * reference.
 *
 * CHashTable is not aware of Keys -- only hashes of Keys. In fact, CHashTable
 * could not tell you anything about the Keys kept within its records.  It
 * will give you all the records stored under a specific hash, but it leaves
 * to its callers the small chore of looking in each record for a desired Key.
 */

#include "copyright.h"
#include "autoconf.h"
#include "config.h"
#include "externs.h"

/*! \brief Reset hash table statistics.
 *
 * Each Hash Table maintains certain statistics regarding the type and
 * number of requests they receive as well as the hash table's performance
 * in responding to those requests. The hashreset() function allows callers
 * to reset these statistics. Typically, this is done when the caller knows
 * future access patterns are of more interest than past access paterns.
 *
 * \param htab     Hash Table.
 * \return         None.
 */

void hashreset(CHashTable *htab)
{
    htab->ResetStats();
}

/*! \brief Staging area for reads and writes into CHashTable.
 *
 * The htab_rec structure is a fixed size, but only part of htab_rec is used.
 * Since requests use variable-sized Keys, the portion of htab_rec used on
 * any particular request is also variable-sized. pData is always present,
 * but aKey may occupy as little as a single byte.
 */

#pragma pack(1)
static struct
{
    void *pData;
    UTF8  aKey[LBUF_SIZE+125];
} htab_rec;
#pragma pack()

/*! \brief Look for a previously-added (Key, Data) pair in a hash table, and
 *         return its data pointer.
 *
 * Given a variable-sized Key, hashfindLEN() uses the associations previously
 * created with hashaddLEN() to find and return the corresponding 'Data' part
 * of a (Key, Data) pair, if it exists.
 *
 * nullptr is returned if the request is not valid or if the (Key, Data) pair
 * is not found.
 *
 * \param pKey     Pointer to Key to find.
 * \param nKey     Size (in bytes) of the above Key.
 * \param htab     Hash Table.
 * \return         pData or nullptr.
 */

void *hashfindLEN(const void *pKey, size_t nKey, CHashTable *htab)
{
    if (  pKey == nullptr
       || nKey <= 0)
    {
        return nullptr;
    }

    UINT32 nHash = HASH_ProcessBuffer(0, pKey, nKey);

    UINT32 iDir = HF_FIND_FIRST;
    iDir = htab->FindFirstKey(nHash);
    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        htab->Copy(iDir, &nRecord, &htab_rec);
        size_t nTarget = nRecord - sizeof(int *);

        if (  nTarget == nKey
           && memcmp(pKey, htab_rec.aKey, nKey) == 0)
        {
            return htab_rec.pData;
        }
        iDir = htab->FindNextKey(iDir, nHash);
    }
    return nullptr;
}

/*! \brief Add a new (Key, Data) pair to a hash table.
 *
 * hashaddLEN() associates a variable-sized key with a pointer using a hash
 * table. The pointer, pData, given to hashaddLEN() may be obtained again
 * later by presenting the the same key to hashfindLEN(). The data given in
 * (pKey, nKey) is saved, so the caller is free to reuse the Key buffer.
 * While the value of pData is also saved, the data that pData points to is
 * not.
 *
 * This function requires that the Key does not already exist in the hash
 * table. It may be necessary to use hashfindLEN() to insure this.
 *
 * \param pKey     Pointer to Key of (Key, Data) pair to add.
 * \param nKey     Size (in bytes) of the above Key.
 * \param pData    Pointer to Data part of (Key, Data) pair.
 * \param htab     Hash Table.
 * \return         false for failure. true for success.
 */

bool hashaddLEN(const void *pKey, size_t nKey, void *pData, CHashTable *htab)
{
    if (  pKey == nullptr
       || nKey <= 0)
    {
        return false;
    }

    UINT32 nHash = HASH_ProcessBuffer(0, pKey, nKey);

    htab_rec.pData = pData;
    memcpy(htab_rec.aKey, pKey, nKey);
    size_t nRecord = nKey + sizeof(void *);
    htab->Insert((HP_HEAPLENGTH)nRecord, nHash, &htab_rec);
    return true;
}

/*! \brief Removes a (Key, Data) pair from a hash table.
 *
 * hashdeleteLEN() disassociates a variable-sized Key from its Data pointer
 * by removing the (Key, Data) pair from the hash table and freeing any
 * related storage. However, it is the caller's responsibility to free any
 * memory that Data points to.
 *
 * \param pKey     The Key to remove.
 * \param nKey     Size (in bytes) of the above Key.
 * \param htab     Hash Table.
 * \return         None.
 */

void hashdeleteLEN(const void *pKey, size_t nKey, CHashTable *htab)
{
    if (  pKey == nullptr
       || nKey <= 0)
    {
        return;
    }

    UINT32 nHash = HASH_ProcessBuffer(0, pKey, nKey);

    UINT32 iDir = htab->FindFirstKey(nHash);
    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        htab->Copy(iDir, &nRecord, &htab_rec);
        size_t nTarget = nRecord - sizeof(int *);

        if (  nTarget == nKey
           && memcmp(pKey, htab_rec.aKey, nKey) == 0)
        {
            htab->Remove(iDir);
        }
        iDir = htab->FindNextKey(iDir, nHash);
    }
}

/*! \brief Removes all (Key, Data) entries in a hash table.
 *
 * The Hash Table is re-initialized from scratch and all storage is
 * reclaimed. The resulting Hash Table is empty.
 *
 * \param htab     Hash Table.
 * \return         None.
 */

void hashflush(CHashTable *htab)
{
    htab->Reset();
}

/*
 * ---------------------------------------------------------------------------
 * * hashreplLEN: replace the data part of a hash entry.
 */

bool hashreplLEN(const void *str, size_t nStr, void *pData, CHashTable *htab)
{
    if (  str == nullptr
       || nStr <= 0)
    {
        return false;
    }

    UINT32 nHash = HASH_ProcessBuffer(0, str, nStr);

    UINT32 iDir = htab->FindFirstKey(nHash);
    while (iDir != HF_FIND_END)
    {
        HP_HEAPLENGTH nRecord;
        htab->Copy(iDir, &nRecord, &htab_rec);
        size_t nTarget = nRecord - sizeof(int *);

        if (  nTarget == nStr
           && memcmp(str, htab_rec.aKey, nStr) == 0)
        {
            htab_rec.pData = pData;
            htab->Update(iDir, nRecord, &htab_rec);
            return true;
        }
        iDir = htab->FindNextKey(iDir, nHash);
    }
    return false;
}

void hashreplall(const void *old, void *new0, CHashTable *htab)
{
    HP_HEAPLENGTH nRecord;
    for (  UINT32 iDir = htab->FindFirst(&nRecord, &htab_rec);
           iDir != HF_FIND_END;
           iDir = htab->FindNext(&nRecord, &htab_rec))
    {
        if (htab_rec.pData == old)
        {
            htab_rec.pData = new0;
            htab->Update(iDir, nRecord, &htab_rec);
        }
    }
}

/*
 * Returns the first hash entry in 'htab'.
 */

void *hash_firstentry(CHashTable *htab)
{
    HP_HEAPLENGTH nRecord;
    UINT32 iDir = htab->FindFirst(&nRecord, &htab_rec);
    if (iDir != HF_FIND_END)
    {
        return htab_rec.pData;
    }
    return nullptr;
}

void *hash_nextentry(CHashTable *htab)
{
    HP_HEAPLENGTH nRecord;
    UINT32 iDir = htab->FindNext(&nRecord, &htab_rec);
    if (iDir != HF_FIND_END)
    {
        return htab_rec.pData;
    }
    return nullptr;
}

/*
 * Returns the key for the first hash entry in 'htab'.
 */

void *hash_firstkey(CHashTable *htab, int *nKeyLength, UTF8 **pKey)
{
    HP_HEAPLENGTH nRecord;
    UINT32 iDir = htab->FindFirst(&nRecord, &htab_rec);
    if (iDir != HF_FIND_END)
    {
        *nKeyLength = nRecord-sizeof(int *);
        *pKey = htab_rec.aKey;
        return htab_rec.pData;
    }
    *nKeyLength = 0;
    *pKey = nullptr;
    return nullptr;
}

void *hash_nextkey(CHashTable *htab, int *nKeyLength, UTF8 **pKey)
{
    HP_HEAPLENGTH nRecord;
    UINT32 iDir = htab->FindNext(&nRecord, &htab_rec);
    if (iDir != HF_FIND_END)
    {
        *nKeyLength = nRecord-sizeof(int *);
        *pKey = htab_rec.aKey;
        return htab_rec.pData;
    }
    *nKeyLength = 0;
    *pKey = nullptr;
    return nullptr;
}

/*
 * ---------------------------------------------------------------------------
 * * search_nametab: Search a name table for a match and return the flag value.
 */

bool search_nametab(dbref player, NAMETAB *ntab, const UTF8 *flagname, int *pflag)
{
    for (NAMETAB *nt = ntab; nt->name; nt++)
    {
        if (minmatch(flagname, nt->name, nt->minlen))
        {
            if (check_access(player, nt->perm))
            {
                *pflag = nt->flag;
                return true;
            }
            else
            {
                *pflag = -2;
                return false;
            }
        }
    }
    *pflag = -1;
    return false;
}

/*
 * ---------------------------------------------------------------------------
 * * find_nametab_ent: Search a name table for a match and return a pointer to it.
 */

NAMETAB *find_nametab_ent(dbref player, NAMETAB *ntab, const UTF8 *flagname)
{
    for (NAMETAB *nt = ntab; nt->name; nt++)
    {
        if (minmatch(flagname, nt->name, nt->minlen))
        {
            if (check_access(player, nt->perm))
            {
                return nt;
            }
        }
    }
    return nullptr;
}

/*
 * ---------------------------------------------------------------------------
 * * display_nametab: Print out the names of the entries in a name table.
 */

void display_nametab(dbref player, NAMETAB *ntab, const UTF8 *prefix, bool list_if_none)
{
    bool got_one = false;
    UTF8 *buf = alloc_lbuf("display_nametab");
    UTF8 *bp = buf;

    safe_str(prefix, buf, &bp);
    safe_chr(':', buf, &bp);
    for (NAMETAB *nt = ntab; nt->name; nt++)
    {
        if (  God(player)
           || check_access(player, nt->perm))
        {
            safe_chr(' ', buf, &bp);
            safe_str(nt->name, buf, &bp);
            got_one = true;
        }
    }
    *bp = '\0';
    if (  got_one
       || list_if_none)
    {
        notify(player, buf);
    }
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * interp_nametab: Print values for flags defined in name table.
 */

void interp_nametab(dbref player, NAMETAB *ntab, int flagword,
    const UTF8 *prefix, const UTF8 *true_text, const UTF8 *false_text)
{
    bool bFirst = true;
    UTF8 *buf = alloc_lbuf("interp_nametab");
    UTF8 *bp = buf;

    safe_str(prefix, buf, &bp);
    for (NAMETAB *nt = ntab; nt->name; nt++)
    {
        if (  God(player)
           || check_access(player, nt->perm))
        {
            if (!bFirst)
            {
                safe_chr(';', buf, &bp);
                bFirst = false;
            }
            safe_chr(' ', buf, &bp);
            safe_str(nt->name, buf, &bp);
            safe_str(T("..."), buf, &bp);
            if ((flagword & nt->flag) != 0)
            {
                safe_str(true_text, buf, &bp);
            }
            else
            {
                safe_str(false_text, buf, &bp);
            }
        }
    }
    *bp = '\0';
    notify(player, buf);
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * listset_nametab: Print values for flags defined in name table.
 */

void listset_nametab(dbref player, NAMETAB *ntab, int flagword, const UTF8 *prefix, bool list_if_none)
{
    UTF8 *buf = alloc_lbuf("listset_nametab");
    UTF8 *bp = buf;

    safe_str(prefix, buf, &bp);
    safe_chr(':', buf, &bp);

    bool got_one = false;
    for (NAMETAB *nt = ntab; nt->name; nt++)
    {
        if (  ((flagword & nt->flag) != 0)
           && (  God(player)
              || check_access(player, nt->perm)))
        {
            safe_chr(' ', buf, &bp);
            safe_str(nt->name, buf, &bp);
            got_one = true;
        }
    }
    *bp = '\0';
    if (  got_one
       || list_if_none)
    {
        notify(player, buf);
    }
    free_lbuf(buf);
}

/* ---------------------------------------------------------------------------
 * cf_ntab_access: Change the access on a nametab entry.
 */

CF_HAND(cf_ntab_access)
{
    NAMETAB *np;
    UTF8 *ap;

    for (ap = str; *ap && !mux_isspace(*ap); ap++)
    {
        ; // Nothing.
    }
    if (*ap)
    {
        *ap++ = '\0';
    }

    while (mux_isspace(*ap))
    {
        ap++;
    }

    for (np = (NAMETAB *) vp; np->name; np++)
    {
        if (minmatch(str, np->name, np->minlen))
        {
            return cf_modify_bits(&(np->perm), ap, pExtra, nExtra, player, cmd);
        }
    }
    cf_log_notfound(player, cmd, T("Entry"), str);
    return -1;
}
