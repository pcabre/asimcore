/*
 * Copyright (c) 2014, Intel Corporation
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of the Intel Corporation nor the names of its 
 *   contributors may be used to endorse or promote products derived from 
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @author Roger Espasa
 * @brief
 */

#ifndef _CACHE_H
#define _CACHE_H

// generic
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>

// ASIM core
#include "asim/syntax.h"
#include "asim/mesg.h"
#include "asim/utils.h"
#include "asim/ioformat.h"
#include "asim/trace.h"
#include "asim/cache_manager.h"
#include "asim/atoi.h"

namespace iof = IoFormat;
using namespace iof;
using namespace std;

const UINT64 shiftable_1_64bit = 1;

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
////
////
//// WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! :
////
////
//// This cache implementation DOES NOT KNOW/ASSUME anything about the size of
//// the objects that you are storing in the cache. In PARTICULAR, it DOES NOT
//// ASSUME that the objects being stored are QUADWORDS. This is necessary in
//// order to support the directory cache, that holds 27-bit objects (directory
//// entries). MOREOVER, this cache implementation does not assume any
//// particular mapping from your addresses into cache locations. That is, this
//// implementation DOES NOT assume the classical-typical-canonical mapping
//// where low order bits indicate Byte-In-Quadword, then the next bits indicate
//// Q-In-Line, then the next bits are the Index into the cache and whatever is
//// left makes up the tag of the cache line.
////
//// HOWEVER, and before you decide NOT to use this data type :-), functions are
//// provided that will help you do all the above very easily (see below).
////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
////
////
////
//// PURPOSE AND GOALS OF cache_mesi.h
//// ---------------------------------
////
//// The purpose of this data type is to handle for you all the nasty details of
//// managing a general cache. It offers a generic implementation of a cache
//// with the following major parameters:
//// 
////     NumWays: 1 for direct mapped, NumWays > 1 for a set associative cache
////     NumLinesPerWay: Indicates the number of cache lines in each way.
////     NumObjectsPerLine: Indicates the number of ``objects'' (i.e.,
////     quadwords) in each cache line.
////
//// Although most likely you will be using this template for a fairly
//// conventional cache that holds lines of quadwords (64bit objects), no
//// assumption is made on what
////
////
//// INDEXING INTO THE CACHE
//// -----------------------
//// 
//// One of the major design decisions behind this data type is that it must be
//// generic enough to support reasonable indexing combinations into the cache
//// such as:
//// 
////        Virtual-Index/Virtual-Tag
////        Virtual-Index/Physical-Tag
////        Physical-Index/Physical-Tag
//// 
//// Furthermore, this code must also make NO ASSUMPTIONS on how the bits of an
//// address are used/mangled in order to construct the <index,tag> pair.
//// 
//// The consequences of these two goals are that
//// 
////  1) Functions provided here always take as input parameters an <index,tag>
////  pair and use them directly (no masking/shifting) to access the data inside
////  the cache. For example, the function used to get the state of a cache line
////  is:
//// 
////                line_state<NumObjectsPerLine> *GetLineState(UINT64 index,
////                UINT64 tag);
//// 
////     The user of this function (i.e., you) must compute the index and tag he
////     wishes based on the characteristics of his cache indexing and mapping
////     functions.
//// 
////  2) Several groups of functions are provided to easily compute <index,tag>
////  pairs based on the most common mapping algorithms. In particular, we offer
////  two sets of functions, called 'Classical' and 'Shifted' to save you from
////  the pains of doing masking and shifting to index the cache. Thus, if you
////  are using a typical cache configuration, you would access the information
////  in the cache using something like:
////
////       mycache.GetLineStatus(mycache.Index(ea),mycache.Tag(ea));
////
////
//// MAPPING FUNCTIONS PROVIDED
//// --------------------------
////
//// As already mentioned above, the interface provided by 'GetLineState' is a
//// RAW interface in that it does not process at all the <index,tag> pair
//// passed to it. The user is responsible for computing the Index/Tag according
//// to his cache characteristics.
////
//// To avoid having each of our ASIM users re-thinking the shifting and masking
//// necessary to compute the Index/Tag pairs (always a buggy process), we
//// provide two sets of functions for the begginner user of this data type.
////
//// The two sets are:
//// 
////   Classical: Assumes a liner mapping of addresses where the 3 low order
////   bits indicate the 'Byte-In-Quadword', the next Q bits select the
////   'Quadword-In-Line', the next I bits select the index within a way and
////   whatever is left is considered part of the tag:
//// 
////                                                     I        Q    3
////                          +----------------------+----------+----+----+
////                          |      Tag             |  Index   |QinL|BinQ|
////                          +----------------------+----------+----+----+
//// 
////               where I = log2(NumLinesPerWay)
////                     Q = log2(NumObjectsPerLine)
//// 
//// 
//// 
////   Shifted: Used when a cache is NOT holding quadwords (for example, in the
////   directory cache).  Assumes that the user has already shifted the input
////   address according to the size of the objects stored in the cache and,
////   therefore, the functions provided should not be shifting the addresses by
////   '3' -- to account for quadwords. As the figure shows, there are no bits
////   corresponding to 'ByteInQuadword'
////        
////                                                     I        O   
////                          +----------------------+----------+----+
////                          |      Tag             |  Index   |OinL|
////                          +----------------------+----------+----+
//// 
////               where I = log2(NumLinesPerWay)
////                     O = log2(NumObjectsPerLine)
//// 
//// 
////
//// Note that you ARE NOT FORCED TO USE THE MAPPING FUNCTIONS PROVIDED. In
//// fact, you can use any other sophisticated mapping/indexing function you
//// need, since the code provided here always takes as parameters an <idx,tag>
//// pair, and uses them directly (no masking/shifting at all) to access the
//// data within the cache.
////
//// VIRTUAL VS. PHYSICAL INDEXING AND TAGGING
//// -----------------------------------------
////
//// This cache code is generic enough to support reasonable combinations of
//// indexing into the cache:
////
////   Virtual-Index/Virtual-Tag
////   Virtual-Index/Physical-Tag
////   Physical-Index/Physical-Tag
////
//// Thus, when you are asking whether a given address will hit or miss in the
//// cache, you have to provide both the index and the tag that you expect to
//// find (i.e., the tag that must be in the cache in order to declare HIT). If
//// you are using the cache as a Virtually-Index/Virtually-Tagged cache, then
//// you supply the same address to the first two parameters (of course, same
//// thing for Physically-Indexed/Physically-Tagged):
////
////     p = mycache->GetLineState(mycache->Index(vea),mycache->Tag(vea));
////
//// On the other hand, if you have a Virtually-Indexed/Physically-Tagged cache,
//// then this code assumes that you have translated your virtual address (vea)
//// into a physical address (pea) using some sort of DTB (either in parallel
//// with the cache lookup or in a previous cycle) and then you invoke
//// GetLineState with the following parameters:
////
////     p = mycache->GetLineState(mycache->Index(vea),mycache->Tag(pea));
////
//// Note that what you get in return is not hit/miss (that would be too simple
//// minded, wouldn't it? :-)... you get back a pointer to the cache line STATE.
//// With that, you can compute yourself whether it is a hit or a miss and you
//// can also generate the appropriate coherency message request (given the
//// current state of the cache).
////
////
//// THE 'WithData' PARAMETER
//// ------------------------
////
////
////
//// USAGE DIRECTIONS FOR CACHE_MESI.H
//// ---------------------------------
////
//// The idea behind this data type is that you always follow a given sequence
//// of calls to do reads/writes into the data cache.
////
//// 1) First, you always invoke 'GetLineState' with the <index,tag> you are
//// looking for.  If you 'hit' in the cache, you'll get a pointer to a
//// 'line_state' data structure. Once you have the 'line_state' pointer, you
//// can truly determine whether you have a hit or not based on the status of
//// the line (Invalid, Exclusive, Shared, etc...)  and you can also find out in
//// which way of the cache the line found was stored.
////
//// 2) After you get the cache STATE, you can update it directly by using the
//// member functions of the line_state class.
////
//// 3) If you need to, you update the LRU/MRU information by invoking
//// UpdateLRU().
////
//// 4) Finally, in case you have true data stored in the cache, you can set the
//// data for every object in the cache line
////


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////
//// CACHE LINE STATE
////
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// The STATE for a cache line consists of three main parts
//
// - the physical tag of the line: a bunch of bits that identify the cache line
//
// - the status of the line: the line can be in one of a number of different
// states.  if the line is "Invalid" the other fields should be ignored.
//
// - The cache 'way' where the line is located (always 0 for a direct-map
// cache).
//
// - a bit vector indicating validty of each object in the line. Note that if
// the line is in 'Invalid' state, the contents of the bit vector are considered
// 'UNPREDICTABLE'. Moreover, if the state of the line is NOT Invalid, then at
// least 1 bit in the bit vector should be set.
//
// - a bit vector indicating 'dirtyness' of each object in the line. Note that
// if the line is in 'Invalid' state, the contents of the bit vector are
// considered 'UNPREDICTABLE'. Moreover, if the state of the line is NOT
// S_EXCLUSIVE_DIRTY, then all bits SHOULD BE ZERO. Conversly, if the state of
// the line is S_EXCLUSIVE_DIRTY then at least 1 bit in the bit vector should be
// set.
//

// WARNING -- WARNING -- WARNING -- WARNING -- WARNING -- WARNING -- WARNING
//
// 4/4/2003 - I have changed the code in order to use NumLinesPerWay parameter 
// that is not a power of 2. Now the masks are done considering the smallest
// power of 2 that is > NumLinesPerWay.
// However, as it is now this implies:
// - You are providing externally the CLASSIC and SHIFTED mapping functions 
// and not using the ones given or 
// - You use the ones given and you know that the address that you are passing to 
// them has this structure: TAG | INDEX | POS. (Note that passing sequential numbers
// as adresses will end in an assertion being raised if NumLinesPerWay is not
// a power of 2).
//
//  Oscar Rosell

#include "asim/line_status.h"

template<UINT32 NumObjectsPerLine, class T> class line_state
{
    
  private:
    PUBLIC_OBJ(UINT64, Tag);
    PUBLIC_OBJ(UINT32, OwnerId);
    PUBLIC_OBJ_ASSERT(UINT32, Way, _NewVal < 256);
    LINE_STATUS	status;
    bool		valid[NumObjectsPerLine];
    bool		dirty[NumObjectsPerLine];
    T           info;

    // Stats
    PUBLIC_OBJ_INIT(UINT32, Accesses, 0);
    PUBLIC_OBJ_INIT(UINT64, AccumDistance, 0);
    PUBLIC_OBJ_INIT(UINT64, PreviousCycle, 0);
    
  public:
    line_state() : status(S_INVALID) {}

    line_state(const LINE_STATUS new_status, const bool new_dirty, const UINT32 owner_id = UINT32_MAX)
        : status(new_status), info()
    {
        SetOwnerId(owner_id);
        for (UINT i=0; i<NumObjectsPerLine; i++)
        {
            valid[i] = true;
            dirty[i] = new_dirty;
        }
    }

    line_state(const line_state* const copy)
        : status(S_INVALID) // need this because of the "if" in SetStatus()
    {
        ASSERTX(copy);
        SetTag(copy->GetTag());
        SetWay(copy->GetWay());
        SetStatus(copy->status);
        for (UINT i=0; i<NumObjectsPerLine; i++)
        {
            valid[i] = copy->valid[i];
            dirty[i] = copy->dirty[i];
        } 
        SetOwnerId(copy->GetOwnerId());
    }


    line_state(const line_state& copy)
        : status(S_INVALID) // need this because of the "if" in SetStatus()
    {
        SetTag(copy.GetTag());
        SetWay(copy.GetWay());
        SetStatus(copy.status);
        for (UINT i=0; i<NumObjectsPerLine; i++)
        {
            valid[i] = copy.valid[i];
            dirty[i] = copy.dirty[i];
        } 
        SetOwnerId(copy.GetOwnerId());
    }

    LINE_STATUS	GetStatus()		{ return status; };
    bool	GetValidBit(UINT32 i)   
    { 
        ASSERTX(i < NumObjectsPerLine); 
        return valid[i]; 
    };
        
    bool	GetDirtyBit(UINT32 i)   
    { 
        ASSERTX(i < NumObjectsPerLine); 
        return dirty[i]; 
    };
    
    T& GetInfo()     { return info; };
    
    void	SetStatus(LINE_STATUS s){ if (status != S_PERFECT) status = s; }; 
    void	SetValidBit(UINT32 i)	
    { 
        ASSERTX(i < NumObjectsPerLine); 
        valid[i] = true; 
    };
    void	ClearValidBit(UINT32 i)   
    { 
        ASSERTX(i < NumObjectsPerLine); 
        valid[i] = false; 
    };
    
    void	SetDirtyBit(UINT32 i)	
    { 
        ASSERTX(i < NumObjectsPerLine); 
        dirty[i] = true; 
    };
    void	ClearDirtyBit(UINT32 i)   
    { 
        ASSERTX(i < NumObjectsPerLine); 
        //YARDI: why is the valid bit being cleared here instead of the
        //       dirty bit?
        //       Temporarily commenting and clearing the dirty bit instead
        //valid[i] = false; 
        dirty[i] = false;
    };

    void SetInfo(const T& new_info) { info = new_info; };

    void	Clear() 
    { 
        SetTag(0xdeadbeef);
        status = S_INVALID; 
        for (UINT32 i = 0; i < NumObjectsPerLine; i++) 
        {
            valid[i] = false;
            dirty[i] = false; 
        }
        SetOwnerId(UINT32(-1));
    }

    void	Dump()
    {
        Dump(cout);
        cout << endl;
    }
    
    std::string DumpString()
    {
        ostringstream out;
        Dump(out);
        return out.str();
    }

    void Dump(ostream& out)
    {
        out << "\ttag=0x" << fmt_x(GetTag())
             << ", way=" << (UINT32)GetWay() << ", status=" << LINE_STATUS_STRINGS[status];
        out << ", valid=0b";
        for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) 
        {
            out << (valid[i] ? 1 : 0); 
        }
        out << ", dirty=0b";
        for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) 
        {
            out << (dirty[i] ? 1 : 0); 
        }
        out << ", ownerId=" << GetOwnerId();
    }
    
    //
    // dump the cache contents
    //
    int SaveTagArrayState(UINT64 index, UINT32 way, ostream &out)
    {
        if(GetStatus() != S_INVALID) 
        {
            out << "S: " << index;
            out << " W: " << way;
            out << " ";
            out << "\ttag=0x" << fmt_x(GetTag())
                  //<< ", way=" << (UINT32)GetWay() 
                  << ", status=" << LINE_STATUS_STRINGS[status];
            out << ", valid=0b";
            for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) 
            {
                out << (valid[i] ? 1 : 0); 
            }
            out << ", dirty=0b";
            for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) 
            {
                out << (dirty[i] ? 1 : 0); 
            }
            out << ", ownerId=" << GetOwnerId();
            return true;
         }
         return false;
    }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////
////
////     LRU INFO MANAGER CLASS
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// The way we implement the lru_info for a particular group of cache lines is as
// follows We have an array used to implement a linked list. Each position in
// the array represents a particular way of the cache. I.e., position 4 in the
// array represents set 4. All sets are linked and, as the upper level (the
// gen_cache_class) touches data, it invokes the routines makeMRU() and
// makeLRU(). Most likely, in a normal cache, as the processor touches cache
// lines, you make those lines the most-recently-used of their set. At any point
// in time, the user can ask for both the first element of this list (the MRU
// line) and for the last element of the list (the LRU line).
//
template<UINT8 NumWays> class lru_info
{
  private: 
    struct {
        INT8		next;
        INT8		prev;
    } linklist[NumWays];
    
    UINT8		mru;
    UINT8		lru;
    
  public:

    // Constructor
    lru_info();
    
    // Make a particular way, the MRU (most-recently-used) way
    void	makeMRU(UINT8 way);
    
    // Make a particular way, the LRU (least-recently-used) way
    void	makeLRU(UINT8 way);
    
    // Get The MRU and LRU ways
    UINT8	getMRU() { return mru; };
    UINT8	getLRU() { return lru; };
    UINT8       getLRU(UINT64 reserved_mask) {
      UINT8 way = lru;
      while (((1<<way) & reserved_mask) != 0) {
	way = linklist[way].prev;
	ASSERT(way < NumWays, "No free ways?");
      }
      return way;
    }
    UINT8   getRandom(){ return random()%NumWays; };
    
    // Debugging
    void	Dump();
    
    // Save LRU state
    //  This replaces the Dump() function
    //  The Dump() was dumping LRU information for a particular
    //  replacement policy
    //  By making this function a member of this class, each policy
    //  will dump its own LRU state
    void SaveState(ostream &out)
    {
        INT8 p,i;
    
        out << "MRU ->";
        //out << "LRU : ";
        for (i = 0, p = mru; i < NumWays; i++, p = linklist[p].next ) 
        //for (i = 0, p = lru; i < NumWays; i++, p = linklist[p].prev ) 
        {
            ASSERTX(p != -1);
            out << " " << p << " ->";
        }
        out << endl;  
    }
    
    //
    // TODO Restore LRU state (currently implemented only for EV7)
    //
};

template<UINT8 NumWays>
class pseudo_lru_info : public lru_info<NumWays>
{
    typedef UINT16 PseudoLRUMaskType;
  private:
    PseudoLRUMaskType pseudoLRU;

    static const PseudoLRUMaskType mask1s[NumWays]; 
    static const PseudoLRUMaskType mask0s[NumWays];

    static const PseudoLRUMaskType mask1sFind[NumWays];
    static const PseudoLRUMaskType mask0sFind[NumWays];

  public:
    pseudo_lru_info() : 
        lru_info<NumWays>(),
        pseudoLRU(0)
    {
    }

    UINT8   getPseudoLRU() 
    {
        int i;

        for (i=0; i<NumWays ; ++i)
        {
            if (((pseudoLRU&mask1sFind[i])==0) && ((pseudoLRU|mask0sFind[i])==PseudoLRUMaskType(-1)))
            {
                return (i);
            }
        }
        cerr << "ERROR: correct the masks. pseudoLRU: " << UINT32(pseudoLRU) <<  endl;
        ASSERTX(false);
        return(NumWays);
    };

    // Make a particular way, the MRU (most-recently-used) way
    void	makeMRU(UINT8 way)
    {
        ASSERTX(way < NumWays);

        lru_info<NumWays>::makeMRU(way);

        pseudoLRU = pseudoLRU | mask1s[way];
        pseudoLRU = pseudoLRU & mask0s[way];
    }
    
    //TODO: 
    //  Save and restore LRU state functions for this policy
};

/* 3,2,2,2 configuration */
/*
template<>
UINT8 pseudo_lru_info<9>::mask1s[9] = {0x0B, 0x0A, 0x0C, 0x08, 0x30, 0x20, 0x40, 0x80, 0x00};
template<>
UINT8 pseudo_lru_info<9>::mask0s[9] = {0xFF, 0xFE, 0xFD, 0xF9, 0xF7, 0xE7, 0x57, 0x97, 0x17};
template<>
UINT8 pseudo_lru_info<9>::mask1sFind[9] = {0x0B, 0x0A, 0x0C, 0x08, 0x30, 0x20, 0xC0, 0x80, 0x40};
template<>
UINT8 pseudo_lru_info<9>::mask0sFind[9] = {0xFF, 0xFE, 0xFD, 0xF9, 0xF7, 0xE7, 0xD7, 0x97, 0x57};
*/

template<UINT8 NumWays>
class ev7_replacement_info : public lru_info<NumWays>
{

  private:
    
    UINT64 mask;
    
    UINT64 allSetMask;
    
  public:
  
    ev7_replacement_info() :
        lru_info<NumWays>(),
        mask(0)
    {
        
        allSetMask = (UINT64)(-1);
        allSetMask = allSetMask >> (64 - NumWays);
               
    }

    // Find the first way with its bit set to 0
    UINT8 findFirstClear(UINT64 reserved_mask) 
    {
        ASSERT((reserved_mask & allSetMask) != allSetMask, "All ways cannot be reserved!");
        
        UINT64 current_mask = reserved_mask | mask;

        if(current_mask == allSetMask)
        {
            // No "good" way within the not reserved ones
            // Do a find first 0 using the reserved mask
            for (int i=0; i<NumWays ; ++i)
            {
                if (((reserved_mask >> i) & 0x1) == 0x0)
                {
                    return (i);
                }
            }            
        }
        else
        {
            for (int i=0; i<NumWays ; ++i)
            {
                if (((current_mask >> i) & 0x1) == 0x0)
                {
                    return (i);
                }
            }
        }
        
        cerr << "ERROR: All the bits were set!!" <<  endl;
        ASSERTX(false);
        return(NumWays);
        
    };

    // Make a particular way, the MRU (most-recently-used) way
    void makeMRU(UINT8 way)
    {
        
        ASSERTX(way < NumWays);

        // Set the corresponding bit
        mask = mask | (shiftable_1_64bit << way);

        // If all the bits are set, clear them 
        if((mask & allSetMask) == allSetMask) {
            mask =  (shiftable_1_64bit << way);
	}
        
    }
  
    // Save LRU state
    void  SaveState(ostream& out)
    {
        out << "0x" << fmt_x(mask) << endl;
    }
    
    // Restore LRU state
    void  RestoreState(UINT64 currMask)
    {
        mask = currMask;
    }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////
////    REPLACEMENT POLICIES: replacement part of the generic cache class
////  
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <UINT8 NumWays, UINT32 NumLinesPerWay>
class LRUReplacement
{
  public:
  //
  // As its name indicates, contains all the LRU data structures for each set in the cache
  //
  typedef lru_info<NumWays>		lruInfo;
  UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0)
  {
    /*ASSERT(reserved_mask == 0, "LRUReplacement doesn't support reserved status.");*/
      
    UINT32 way;
    ASSERTX(index<NumLinesPerWay);
    // Get the LRU way
    way = LruArray[index].getLRU(reserved_mask);

    ASSERT(way < NumWays && (reserved_mask & (1<<way)) == 0, "Ooops, LRU picked a bogus way");

    return way;
  }

protected:
  UINT32 getLRU(int index) {
    return LruArray[index].getLRU();
  }
  UINT32 getMRU(int index) {
    return LruArray[index].getMRU();
  }
  void makeMRU(int index, int way) {
    return LruArray[index].makeMRU(way);
  }
  void makeLRU(int index, int way) {
    return LruArray[index].makeLRU(way);
  }
  void Dump(UINT64 index) {
    LruArray[index].Dump();
  }
  void SaveState(UINT64 index,ostream &out) {
      LruArray[index].SaveState(out);
  }
private:
  lruInfo LruArray[NumLinesPerWay];
};

template <UINT8 NumWays, UINT32 NumLinesPerWay>
class PseudoLRUReplacement
{
  public:
  //
  // As its name indicates, contains all the LRU data structures for each set in the cache
  //
  typedef pseudo_lru_info<NumWays>		lruInfo;

  UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0)  {
    ASSERT(reserved_mask == 0, "PseudoLRUReplacement doesn't support reserved status.");
      
    UINT32 way;
    ASSERTX(index<NumLinesPerWay);
    // Get the LRU way
    way = LruArray[index].getPseudoLRU();

    return way;
  }

  protected:
  UINT32 getLRU(int index) {
    return LruArray[index].getLRU();
  }
  UINT32 getMRU(int index) {
    return LruArray[index].getMRU();
  }
  void makeMRU(int index, int way) {
    return LruArray[index].makeMRU(way);
  }
  void makeLRU(int index, int way) {
    return LruArray[index].makeLRU(way);
  }
  void Dump(UINT64 index) {
    LruArray[index].Dump();
  }
  void SaveState(UINT64 index,ostream &out) {
      LruArray[index].SaveState(out);
  }
private:

  lruInfo LruArray[NumLinesPerWay];
};


/* This is a generalized PLRU that supports random at the top
 * and/or random at the bottom variants.  
 *
 *
 */
#include "plru_masks.h"
template<UINT8 NumWays, int rand_at_bottom> 
class PLRUTree {
private:
  UINT64 state;
  int mruWay;
  const static int NumLeafs = NumWays / rand_at_bottom;
  
public:
  PLRUTree(): state(0), mruWay(NumWays-1) {}
    UINT32 getPLRUWay(UINT64 reserved_mask) {
      /* we need to AND together the bits of the reserved mask
	 for all the ways that are grouped together
      */
      UINT64 orig_r_mask = reserved_mask;
      UINT64 temp_mask = (UINT64) (-1);
      for (int i = 0 ; i < rand_at_bottom; i++) {
	temp_mask = reserved_mask & temp_mask;
	reserved_mask >>= NumLeafs;
      }
      reserved_mask = temp_mask;
      int temp;
      int currentWay = PLRU_Mask<NumLeafs>::getLRUWayWithReservations(state,reserved_mask);
      if (rand_at_bottom!=1) {
	temp = random() % rand_at_bottom;
	int first_temp = temp;
	
	while ( ((1<< (currentWay + (temp * NumLeafs))) & orig_r_mask) != 0) {
	  temp ++;
	  temp = temp % rand_at_bottom;
	
	  ASSERT(temp != first_temp, " No free ways at bottom?");
	}
      }
      else {
	temp = 0;
      }

      return currentWay + (temp * NumLeafs);
    }
    UINT32 getMRUWay() {
      /* I dont think this actually gets used, but since
	 its here we'll make it behave reasonably sanely*/
      return mruWay;
    }
    void setMRU(UINT32 way) {
      mruWay = way;
      way = way % NumLeafs;
      state = PLRU_Mask<NumLeafs>::makeMRU(way,state);
    }
    void setLRU(UINT32 way) {
      way = way & NumLeafs;
      state = PLRU_Mask<NumLeafs>::makeLRU(way,state);
    }

  void Dump() {
    cout << state;
  }
};

template <int rand_at_top,
	  int rand_at_bottom>
class GeneralizedPseudoLRUReplacement {
public:

  template <UINT8 NumWays, 
	    UINT32 NumLinesPerWay>
  class T	  
  {
    const int treeSize;
     UINT64 masks[rand_at_top];
  public:
  T(): treeSize(NumWays/rand_at_top){
      int i;
      UINT64 mask= -1;
      mask = mask >> (64-treeSize);
      for (i = 0; i < rand_at_top; i++) {
	masks[i] = mask;
	mask = mask << treeSize;
      }
    }
    UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0)
    {
      /*ASSERT(reserved_mask == 0, "PseudoLRUReplacement doesn't support reserved status.");*/
   
      ASSERTX(index<NumLinesPerWay);
      
      UINT32 way;
      int treeNum = 0; 
      int firstNum; 
      UINT64 tempMask;

      if (rand_at_top > 1) {
	treeNum = random() % rand_at_top;
      }
      firstNum = treeNum;
      while ((masks[treeNum] & reserved_mask) == masks[treeNum]) {
	/* everything in that tree is reserved */
	treeNum++;
	treeNum = treeNum % rand_at_top;
	ASSERT(treeNum != firstNum, "All ways are reserved!");
      }
      /* get the part of the reserved bits which corresponds to this tree*/
      tempMask = (reserved_mask & masks[treeNum])   >> (UINT32)(treeSize * treeNum);

      way = trees[index][treeNum].getPLRUWay(tempMask);
     
      way = way +(treeSize * treeNum);

      ASSERT((reserved_mask & (1<<way)) == 0, "Ooops, PLRU picked a reserved way!");

      return way;
    }
  protected:
    UINT32 getLRU(UINT64 index) 
    {    
      int treeNum = 0 ;
      if (rand_at_top > 1) {
       treeNum = random() % rand_at_top;
      }
      return trees[index][treeNum].getPLRUWay(0) + (treeNum * treeSize) ;
    }
    UINT32 getMRU(UINT64 index) {
      int treeNum = 0 ;
      if (rand_at_top > 1) {
       treeNum = random() % rand_at_top;
      }
      return trees[index][treeNum].getMRUWay() + (treeNum *treeSize);
    }
    void makeMRU(UINT64 index, UINT32 wayNum) {
      int treeNum = wayNum / treeSize;
      wayNum = wayNum % treeSize;
      trees[index][treeNum].setMRU(wayNum);
    }
    void makeLRU(UINT64 index, UINT32 wayNum) {
      int treeNum = wayNum / treeSize;
      wayNum = wayNum % treeSize;
      trees[index][treeNum].setLRU(wayNum);
    }
    void Dump(UINT64 index) {
      int i;
      for (i = 0 ; i < rand_at_top; i++) {
	cout << "Tree " << i ;
	trees[index][i].Dump();
	cout << endl;
      }
    }
  private:
    PLRUTree<NumWays /rand_at_top, rand_at_bottom> trees[NumLinesPerWay][rand_at_top];
  };

};


// let's name at least one version of this replacement policy.  this is supposed
// to be the PLRU policy used in Nehalem.  if not, let's change this typedef
// when we confirm the appropriate policy.
//
// NOTE: there is a limitation in C++ where you *cannot* template typdefs.  I
// would like to write:
//
//    template <UINT8 NumWays, UINT32 NumLinesPerWay> 
//    typedef GeneralizedPseudoLRUReplacement< 1,1 >::T<NumWays, NumLinesPerWay>
//            NehalemPlruReplacement;
//
// that will not complie.  grr grumble !@#$%.  so instead, we have the lovely:
//
#define NehalemPlruReplacement GeneralizedPseudoLRUReplacement<1,1>::T
// 
// the funny thing is that we got into this whole mess because i wanted to avoid
// the confusing  GeneralizedPseudoLRUReplacement<1,1>::T expression... that
// expression exists because the author of that class ran into the same problem.
//   --slechta


template <UINT8 NumWays, UINT32 NumLinesPerWay>
class RandomReplacement
{
  public:
  //
  // As its name indicates, contains all the LRU data structures for each set in the cache
  //
  typedef lru_info<NumWays>		lruInfo;

  UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0)
  {
    ASSERT(reserved_mask == 0, "RandomReplacement doesn't support reserved status.");
      
    UINT32 way;
    ASSERTX(index<NumLinesPerWay);
    // Get a random way
    way = LruArray[index].getRandom();

    return way;
  }
  
  //YARDI: this is still a NOP
  //void RestoreLRU(istream& in);
  
  //TODO: restoreLRUstate
  //void RestoreLRU(istream& in);

  protected:
  UINT32 getLRU(int index) {
    return LruArray[index].getLRU();
  }
  UINT32 getMRU(int index) {
    return LruArray[index].getMRU();
  }
  void makeMRU(int index, int way) {
    return LruArray[index].makeMRU(way);
  }
  void makeLRU(int index, int way) {
    return LruArray[index].makeLRU(way);
  }
  void Dump(UINT64 index) {
    LruArray[index].Dump();
  }
  void SaveState(UINT64 index,ostream &out) {
      LruArray[index].SaveState(out);
  }
private:
  lruInfo LruArray[NumLinesPerWay];
};

template <UINT8 NumWays, UINT32 NumLinesPerWay>
class RandomNotMRUReplacement
{
  public:
  //
  // As its name indicates, contains all the LRU data structures for each set in the cache
  //
  typedef lru_info<NumWays>		lruInfo;

  UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0) 
  {
    ASSERT(reserved_mask == 0, "RandomNotMRUReplacement doesn't support reserved status.");
      
    ASSERTX(index<NumLinesPerWay);
    ASSERTX(NumWays>1);
    // Get the MRU
    UINT8 mruWay = LruArray[index].getMRU();
    // Get a random number between 0 and NumWays-1
    UINT8 ran = random()%(NumWays-1);
    // Get random but not MRU
    UINT8 way = (1 + mruWay + ran)%NumWays;

    return way;
  }
  
  //TODO: restoreLRUstate
  //void RestoreLRU(istream& in);

  protected:
  UINT32 getLRU(int index) {
    return LruArray[index].getLRU();
  }
  UINT32 getMRU(int index) {
    return LruArray[index].getMRU();
  }
  void makeMRU(int index, int way) {
    return LruArray[index].makeMRU(way);
  }
  void makeLRU(int index, int way) {
    return LruArray[index].makeLRU(way);
  }
  void Dump(UINT64 index) {
      LruArray[index].Dump();
  }
  void SaveState(UINT64 index,ostream &out) {
      LruArray[index].SaveState(out);
  }

private:

  lruInfo LruArray[NumLinesPerWay];
};

template <UINT8 NumWays, UINT32 NumLinesPerWay>
class EV7_scheme_replacement
{
  public:
  
  typedef ev7_replacement_info<NumWays>		ev7Info;

  UINT32 GetVictim(UINT64 index, UINT64 reserved_mask = 0) 
  {
    
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(NumWays > 1);
    
    // Get the first not set
    UINT8 way = LruArray[index].findFirstClear(reserved_mask);
    
    return way;
    
  }
  
  //restore functional LRU state
  void RestoreLRU(istream& in);
  
  protected:  
  UINT32 getLRU(int index) {
    return LruArray[index].getLRU();
  }
  UINT32 getMRU(int index) {
    return LruArray[index].getMRU();
  }
  void makeMRU(int index, int way) {
    return LruArray[index].makeMRU(way);
  }
  void makeLRU(int index, int way) {
    return LruArray[index].makeLRU(way);
  }
  void Dump(UINT64 index) {
    LruArray[index].Dump();
  }
  void SaveState(UINT64 index,ostream &out) {
      LruArray[index].SaveState(out);
  }
private:

  ev7Info LruArray[NumLinesPerWay];
};






////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////
////
////  GENERIC CACHE DATA TYPE
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<UINT8 NumWays, 
         UINT32 NumLinesPerWay, 
         UINT32 NumObjectsPerLine,
         class T = UINT64, 
         bool WithData = false, 
         template <UINT8,UINT32> class VictimPolicy = LRUReplacement, 
         class INFO = UINT32> 
class gen_cache_class : public VictimPolicy<NumWays,NumLinesPerWay>
{
  public:
  using VictimPolicy<NumWays,NumLinesPerWay>::Dump;
    typedef line_state<NumObjectsPerLine, INFO> lineState;

    // accessors to query the template parameters
    UINT32 GetNumWays () { return NumWays; }
    UINT32 GetNumLinesPerWay () { return NumLinesPerWay; }
    UINT32 GetNumObjectsPerLine () { return NumObjectsPerLine; }
    
  private:

  // WARNING! Each cache instance has its own random state to guarantee
  // that the warm-up phase produces the same result (i.e. lockstepped processors).
  #define CACHE_RANDOM_STATE_LENGTH 128
  
  static UINT32 DEFAULT_CACHE_RANDOM_SEED;
  UINT32 random_state[CACHE_RANDOM_STATE_LENGTH / 4];
  
  // What percent of this cache is warm (cold misses are turned into hits)? 0-100% are legal values.
  const UINT32 warmPercent;
  const UINT64 warmFactor;  ///< warmFactor = warmPercent*RAND_MAX
  const LINE_STATUS initialWarmedState;
  
  //
  // Tag array holding the contents of the cache and its state.
  //
  lineState	TagArray[NumLinesPerWay][NumWays];
  
  //
  // This array reduces to almost nothing if the user sets 'WithData' to
  // FALSE. Otherwise, this array is used to hold the REAL data that would be in
  // the real cache. At this point, our feeders do not still support this
  // ability, but this class is ready to accept data as soon as the feeders
  // become ready
  //
  T   DataArray[WithData ? NumLinesPerWay : 1][WithData ? NumWays : 1][NumObjectsPerLine];

  //
  // Shift Amounts useful for extracting index and tag information from an address
  //
  UINT64	IndexMask;
  UINT64	PosMask;
  UINT64	ClassicalTagMask;
  UINT32	ClassicalIndexShift;
  UINT64	ShiftedTagMask;
  UINT32	ShiftedIndexShift;

  std::string Level;
  PTR_SIZED_UINT LevelInstance;


  INT32		FindWay(UINT64 index, UINT64 tag, UINT32 warm_owner = UINT32(-1), const bool isProbe = false);

  public:
  
  // Constructor
  gen_cache_class(const UINT32 warm_percent = 0, const LINE_STATUS initial_warmed_state = S_SHARED, const INT32 random_seed = -1);
  
  // Destructor
  ~gen_cache_class();

  // Name the Level of Organization this cache pertains to
  void SetLevel(std::string level) { Level = level; };
  std::string GetLevel() const { return Level;}
  // Index in the level
  void SetLevelInstance(PTR_SIZED_UINT instance) { LevelInstance = instance; };
  PTR_SIZED_UINT GetLevelInstance() const { return LevelInstance;};

  //
  // Method that sets the state of all lines to INVALID.
  //
  void ClearAllLines();
  
  //
  // Main function to check whether a given index is present or not in 
  // the cache. This routine returns a pointer to the STATE portion of the
  // data cache. Once you have a pointer to the STATE structure, you can do whatever
  // you feel like with it (change it, set dirty bits, etc...). Also, the
  // data structure returned contains the 'way' in which the cache line is stored.
  // You will need the 'way' for a lot of the remaining functions in this class.
  // If the <idx,tag> pair can not be found in the cache, the function returns 
  // a NULL pointer.
  //
  lineState	*GetLineState(UINT64 index, UINT64 tag, UINT32 warm_owner = UINT32(-1), const bool isProbe = false);
    
  //
  // Warm up method. Stores the provided line into the cache replacing another if needed. 
  // If replWay == -1, the current replacement algorithm is used. Otherwise, the replWay
  // is replaced. Line state and initial owner can be specified.
  //
  UINT64 WarmUpFill(UINT64 index, UINT64 tag, INT32 replWay, LINE_STATUS initialState = S_SHARED, UINT32 warm_owner = UINT32(-1));
  
  //
  // Get a particular way of a particular index
  //
  lineState	*GetWayLineState(UINT64 index, UINT32 way);
  
  
  //
  // Functions used to find a Victim within a given index to be replaced
  //
  lineState	*GetLRUState(UINT64 index);
  lineState	*GetMRUState(UINT64 index);
  lineState *GetVictimState(UINT64 index, bool invalidFirst=true);
  UINT32 GetVictimWayNum(UINT64 index, UINT64 reserved_mask, bool invalidFirst) ;
  //
  // Functions to update the LRU information according to your favorite scheme. 
  //
  void	MakeMRU(UINT64 index, UINT32 way);
  void	MakeLRU(UINT64 index, UINT32 way);
  

  //
  // CLASSIC Mapping Functions used to compute an <index,tag> pair to be used
  // with the previous functions. Index returns the index portion according to
  // the definition at the beginning of this file. Likewise for Tag. 'Pos'
  // returns the 'Quadword-In-Line' bits so you can determine the exact position
  // of a quadword within a line.
  // 
  UINT64		Index(UINT64 addr) const;  
  UINT64		Tag(UINT64 addr) const;
  UINT64		Pos(UINT64 addr) const;
  UINT64		Original(UINT64 index, UINT64 tag) const; // rebuilds the original address from an <index,tag>
  
  //
  // SHIFTED Mapping Functions used to compute an <index,tag> pair to be used
  // with the previous functions. Index returns the index portion according to
  // the definition at the beginning of this file. Likewise for Tag. 'Pos'
  // returns the 'Quadword-In-Line' bits so you can determine the exact position
  // of a quadword within a line.
  // 
  UINT64		IndexShifted(UINT64 addr);  
  UINT64		TagShifted(UINT64 addr);
  UINT64		PosShifted(UINT64 addr);
  UINT64		OriginalShifted(UINT64 index, UINT64 tag); // rebuilds the original address from an <index,tag>
  
  //
  //
  // In case you are storing real data in your cache (i.e, WithData ==
  // true), you will need the following functions to read and write the
  // data information. Notice that you HAVE TO PROVIDE A SPECIFIC way
  // where the data has to be placed (you obtain the 'way' from the
  // GetLineState() call).  For your own protection, 'SetLineData' will
  // not allow overwriting a cache line that is NOT in S_EXCLUSIVE_DIRTY
  // state. So, whenever writing to a cache line, you first have to update
  // its state and then perform the physical write.
  // 
  // The last 'SetLineData' will only activite the dirty bit for the particular object you are writing.
  //
  void	GetLineData(UINT64 index, UINT32 way, T data[NumObjectsPerLine]);
  void	GetLineData(UINT64 index, UINT32 way, UINT32 ObjectInLine, T *data);
  void	SetLineData(UINT64 index, UINT32 way, T data[NumObjectsPerLine]);
  void	SetLineData(UINT64 index, UINT32 way, UINT32 ObjectInLine, T data);
  
	
  //
  // Debugging routines
  //
  void				Dump(UINT64 index, UINT32 way);
  void				DumpLRU(UINT64 index);
  void				tester();
  
  //
  // functional save/restore support
  //
  void        SaveLRUState(UINT64 index, ostream &out);
  void        SaveCacheState(UINT64 index, UINT32 way, ostream& out);
  void        RestoreLRUState(istream &in);
  void        RestoreCacheState(istream &in);

};

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template<UINT8,UINT32> class VictimPolicy, class INFO>
UINT32 gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::DEFAULT_CACHE_RANDOM_SEED = 0;

////////////////////////////////////////////////////
//
// Constructor
//
////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine, 
         class T, bool WithData, template<UINT8,UINT32> class VictimPolicy, class INFO>
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::gen_cache_class(const UINT32 warm_percent,
                                                                                      const LINE_STATUS initial_warmed_state,
                                                                                      const INT32 random_seed)
    : warmPercent(warm_percent), 
      warmFactor(UINT64(warm_percent) * RAND_MAX), 
      initialWarmedState(initial_warmed_state), 
      Level(""), 
      LevelInstance(0)
{
    UINT32 i,j;
    // this variable is not used in case tracing is not enabled
    UINT32 KiloObjects = ( NumWays *  NumLinesPerWay * NumObjectsPerLine) / 1024;
    
    VERIFYX(isPowerOf2(NumObjectsPerLine));
    IndexMask = CEIL_POW2(NumLinesPerWay) - 1;
    PosMask = NumObjectsPerLine -1;
    ClassicalIndexShift = ilog2(NumObjectsPerLine) + 3;
    ClassicalTagMask = ~(UINT64)((CEIL_POW2(NumLinesPerWay) * NumObjectsPerLine * 8) - 1);
    ShiftedIndexShift = ilog2(NumObjectsPerLine);
    ShiftedTagMask = ~(UINT64)((CEIL_POW2(NumLinesPerWay) * NumObjectsPerLine) - 1);
    
    // Initialize the random state
    if(random_seed < 0)
    {
        initstate(DEFAULT_CACHE_RANDOM_SEED, (char*)random_state, CACHE_RANDOM_STATE_LENGTH);
        DEFAULT_CACHE_RANDOM_SEED++;
    }
    else
        initstate(random_seed, (char*)random_state, CACHE_RANDOM_STATE_LENGTH);
    
    for ( i = 0; i < NumLinesPerWay; i++ ) {
        for (j = 0; j < NumWays; j++ ) {
            TagArray[i][j].Clear();
            TagArray[i][j].SetWay(j);

            if (warmPercent > 0)
            {
                TagArray[i][j].SetStatus(S_WARM);
            }
        }
    }
    TRACE(Trace_Sys, 
          cout << "You just created a "
          << warmPercent << "% warm cache with "
          << (UINT32)NumWays << " ways, "
          << NumLinesPerWay << " lines per way, "
          << NumObjectsPerLine << " objects per line = "
          << KiloObjects << " simulated KiloObjects (software variable = "
          << int(sizeof(*this)/1024) << " kilobytes)" << endl);
    
    TRACE(Trace_Sys,
          cout << "IndexMask           = " << fmt_x(IndexMask) << endl); 
    TRACE(Trace_Sys,
          cout << "PosMask             = " << fmt_x(PosMask) << endl); 
    TRACE(Trace_Sys,
          cout << "ClassicalIndexShift = " << ClassicalIndexShift << endl); 
    TRACE(Trace_Sys,
          cout << "ClassicalTagMask    = " << fmt_x(ClassicalTagMask) << endl); 
    TRACE(Trace_Sys,
          cout << "ShiftedIndexShift   = " << ShiftedIndexShift << endl); 
    TRACE(Trace_Sys,
          cout << "ShiftedTagMask      = " << fmt_x(ShiftedTagMask) << endl); 
}

/////////////////////////////////////////////////////////////
//
// Destructor
//
/////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine, 
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::~gen_cache_class()
{
}

/////////////////////////////////////////////////////////////
//
// Method to set all the lines to S_INVALID
//
/////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine, 
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::ClearAllLines()
{
    
    for (UINT64 i = 0; i < NumLinesPerWay; i++ )
    {
        for (UINT64 j = 0; j < NumWays; j++ )
        {            
            TagArray[i][j].SetStatus(S_INVALID);            
        }
    }    
    
}

/////////////////////////////////////////////////////////////
//
// This is the basic function that looks for a cache line. The ones exported to
// the user are simply wrappers around this one that allow getting access to the
// Tag data structure or to the data itself
//
/////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
inline INT32
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::FindWay(UINT64 index, UINT64 tag, UINT32 warm_owner, const bool isProbe)
{
    UINT32 i;
    vector<UINT32> warm;
    bool inv=false;
    INT32 return_way = -1;
    INT32 return_way_reserved = -1;
    
    warm.clear();

     //cout << "FindWay on index " << index
       //  << " tag " << tag << "..." << endl;

    for ( i = 0; i < NumWays; i++ ) 
    {
         //cout << "\tTagArray[" << index
          //   << "][" << i
           //  << "].GetTag " << TagArray[index][i].GetTag() << endl;
        if ( TagArray[index][i].GetTag() == tag ) 
        {
            if(TagArray[index][i].GetStatus() == S_INVALID)
            {
            //    cout<<" tag matched and status Invalid"<<endl;
                inv=true;
            }
            else if(TagArray[index][i].GetStatus() == S_RESERVED)
            {
                ASSERT(return_way_reserved == -1, "Index: 0x" << fmt_x(index) << " Tag: 0x" << fmt_x(tag) << 
                                                  " Status_1: " << LINE_STATUS_STRINGS[TagArray[index][i].GetStatus()] <<
                                                  " Status_2: " <<  LINE_STATUS_STRINGS[TagArray[index][return_way_reserved].GetStatus()] );
                return_way_reserved = i;
             //   cout<<" tag matched and status reserved"<<endl;
            }
            else
            {
                ASSERT(return_way == -1, "Index: 0x" << fmt_x(index) << " Tag: 0x" << fmt_x(tag) << 
                                         " Status_1: " << LINE_STATUS_STRINGS[TagArray[index][i].GetStatus()] <<
                                         " Status_2: " <<  LINE_STATUS_STRINGS[TagArray[index][return_way].GetStatus()] );
                return_way = i;
              //  cout<<" tag matched and returning way "<<return_way<<endl;
            }
        }
        else if ( TagArray[index][i].GetStatus() == S_WARM  && !isProbe)
        {
            warm.push_back(i);
        }
    }
    if (return_way != -1)
    {
        return return_way;
    }
    else if(return_way_reserved != -1)
    {
        return return_way_reserved;
    }

    if(!inv)
    {
        if ( warm.size() > 0 )
	    {
            
            // Set the current cache random state and save the existing one
            char *tmp_state = setstate((char*)random_state);
            
	        UINT64 rand_factor = (warmPercent == 100) ? 0 : UINT64(random()) * 100;
        //  cout << "warmFactor=" << warmFactor << ", rand_factor=" << rand_factor;

            // Randomize the selected warmed way
            UINT64 warm_way = warm[UINT64(random()) % warm.size()];            
            ASSERTX(TagArray[index][warm_way].GetStatus() == S_WARM);

	        if ( (warmFactor > rand_factor) &&
                 (CACHE_MANAGER::GetInstance().GetStatus(Level, index, tag) == S_INVALID) )
	        {
//              cout << " -- WARM!" << endl;                                

                TagArray[index][warm_way].SetTag(tag);
                TagArray[index][warm_way].SetStatus(initialWarmedState);
                TagArray[index][warm_way].SetOwnerId(warm_owner);
                CACHE_MANAGER::GetInstance().SetStatus(Level, LevelInstance, index, tag, initialWarmedState);
                for (UINT32 j=0; j<NumObjectsPerLine; j++)
                {
                    TagArray[index][warm_way].SetValidBit(j);
                }
                return warm_way;
            }
            else
            {
//              cout << " -- COLD!" << endl;

                TagArray[index][warm_way].SetTag(tag);
                TagArray[index][warm_way].SetStatus(S_INVALID);
                return -1;
            }
            
            // Restore the previous random state
            setstate(tmp_state);
            
        }
    }

//    cout << "Miss at index=0x" << fmt_x(index) << ", tag=0x" << fmt_x(tag) << ". :-(" << endl;
    return -1;
}

//////////////////////////////////////////////////////////////////
//
//
// GET a line_state POINTER
//
// Function returns a pointer to the STATE of a given line. All ways of the
// cache are searched to find a match against parameter 'tag'.
//////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
line_state<NumObjectsPerLine,INFO> *
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetLineState(UINT64 index, UINT64 tag, UINT32 warm_owner, const bool isProbe)
{
    INT32 way;
   
    ASSERTX(index < NumLinesPerWay);
    
    // Do an associative search on the several ways of the cache.
    way = FindWay(index,tag,warm_owner, isProbe);
   
//    cout<<" find way returned way "<<way<<endl; 
    return ( way == -1 ) ? NULL : (&(TagArray[index][way]));
}

//////////////////////////////////////////////////////////////////
//
//
// FILL the cache with the provided line 
//
//////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::WarmUpFill(UINT64 index, UINT64 tag, INT32 replWay, LINE_STATUS initialState, UINT32 warm_owner)
{
    TRACE(Trace_Sys, cout << "Doing warmup fill!\n");

    if (CACHE_MANAGER::GetInstance().GetStatus(Level, index, tag) != S_INVALID)
    {
        // The line already exists in a peer cache. We return the MRU 
        return GetMRUState(index)->GetWay();
    }
       
    // Do an associative search on the several ways of the cache.
    INT32 way = FindWay(index, tag, warm_owner);
    
    if(way == -1) // Miss, store the line
    {              
        TRACE(Trace_Sys, cout << "Filling " << index << "tag " << tag << "\n");
        lineState * victimLine;
        
        // Select replacement candidate
        if(replWay == -1)
        {
            victimLine = GetVictimState(index);
        }
        else
        {
            victimLine = GetWayLineState(index, replWay);
        }
       
        CACHE_MANAGER::GetInstance().SetStatus(Level, LevelInstance, index, victimLine->GetTag(), S_INVALID);
        // Do fill
        victimLine->SetTag(tag);
        victimLine->SetStatus(initialState);
        victimLine->SetOwnerId(warm_owner);
        CACHE_MANAGER::GetInstance().SetStatus(Level, LevelInstance, index, tag, initialState);

        for (UINT32 j=0; j<NumObjectsPerLine; j++)
        {
            victimLine->SetValidBit(j);
        }
        way = victimLine->GetWay();
        
    }

    return way;        
    
}


///////////////////////////////////////////////////////////////////
//
// Get a specific way of a specific cache index 
//
///////////////////////////////////////////////////////////////////

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
line_state<NumObjectsPerLine,INFO> *
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetWayLineState(UINT64 index, UINT32 way)
{

  ASSERTX(index < NumLinesPerWay);
  if(way >= NumWays) {
    printf("Way: %d, NumWays: %d\n", way, NumWays);
  }
  ASSERTX(way < NumWays);
    
  return (&(TagArray[index][way]));
}

/////////////////////////////////////////////////////////////////////
//
// Get the TAG POINTER of the LEAST RECENTLY USED WAY IN A GIVEN SET
//
/////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
line_state<NumObjectsPerLine,INFO> *
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetLRUState(UINT64 index)
{
    UINT32 way;
    // Get the LRU way
    ASSERTX(index < NumLinesPerWay);
    way = this->getLRU(index);
    // Return pointer to it.
    return &(TagArray[index][way]);
}

////////////////////////////////////////////////////////////////////
//
// Get the TAG POINTER of the MOST RECENTLY USED WAY IN A GIVEN SET
//
////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
line_state<NumObjectsPerLine,INFO> *
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetMRUState(UINT64 index)
{
    UINT32 way;
    
    // Get the MRU way
    ASSERTX(index < NumLinesPerWay);
    way = this->getMRU(index);
    // Return pointer to it.
    return &(TagArray[index][way]);
}

////////////////////////////////////////////////////////////////////
//
// Get the TAG POINTER of the NEXT LINE TO VICTIMIZE according to the current policy
//
////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
line_state<NumObjectsPerLine,INFO> *
  gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetVictimState(UINT64 index, bool invalidFirst)
{
    UINT32 way;
    UINT64 reserved_mask = 0;

    // Search for an invalid way that should be the first to be
    // victimized (only if invalidFirst is set!)
    for (UINT16 i = 0; i < NumWays; ++i)
    {
        if(invalidFirst)
	      {
            if (TagArray[index][i].GetStatus()==S_INVALID)
            {
                return &(TagArray[index][i]);
            }
	      }
        
	      if (TagArray[index][i].GetStatus()==S_RESERVED)
        {
            reserved_mask = reserved_mask | (shiftable_1_64bit << i);
        }
    }
    
    // No invalid line -> get the victim according to the selected replacement algorithm
    way = this->GetVictim(index, reserved_mask);
    
    // Return pointer to it.
    return &(TagArray[index][way]);
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT32
  gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetVictimWayNum(UINT64 index, UINT64 reserved_mask, bool invalidFirst) 
{
    // Search for an invalid way that should be the first to be
    // victimized (only if invalidFirst is set!)
    if(invalidFirst) {
      for (UINT16 i = 0; i < NumWays; ++i) {
	if (TagArray[index][i].GetStatus()==S_INVALID) {
	  return i;
	}
      }
    }
    // No invalid line -> get the victim according to the selected replacement algorithm

    return this->GetVictim(index, reserved_mask);

}


////////////////////////////////////////////////////////////////////
//
// UPDATE LRU
//
////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::MakeMRU(UINT64 index, UINT32 way)
{
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    /*cout << Level << " is making " << way << " mru in " << index << endl;*/
    this->makeMRU(index,way);
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::MakeLRU(UINT64 index, UINT32 way)
{
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    this->makeLRU(index,way);
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
// CLASSICAL MAPPING FUNCTIONS
//
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
inline UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::Index(UINT64 addr) const
{ 
    addr = (addr >> ClassicalIndexShift) & IndexMask;
    ASSERTX(addr < NumLinesPerWay);
    
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::Tag(UINT64 addr) const
{
    addr = addr & ClassicalTagMask;
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::Pos(UINT64 addr) const
{
    addr = (addr >> 3) & PosMask;
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::Original(UINT64 index, UINT64 tag) const
{
    ASSERTX((tag == 0xdeadbeef) || (tag & (index << ClassicalIndexShift)) == 0);
    return (tag | (index << ClassicalIndexShift));
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// SHIFTED MAPPING FUNCTIONS
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
inline UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::IndexShifted(UINT64 addr)
{
    addr = (addr >> ShiftedIndexShift) & IndexMask;
    ASSERTX(addr < NumLinesPerWay);
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::TagShifted(UINT64 addr)
{
    addr = addr & ShiftedTagMask;
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::PosShifted(UINT64 addr)
{
    addr = addr & PosMask;
    return addr;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
UINT64
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::OriginalShifted(UINT64 index, UINT64 tag)
{
    ASSERTX ((tag == 0xdeadbeef) || (tag & (index << ShiftedIndexShift)) == 0);
    return (tag | (index << ShiftedIndexShift));
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
//
// THE FOLLOWING SET OF FUNCTIONS ONLY MAKE SENSE IF YOU HAVE INSTANTIATED THE
// TEMPLATE WITH PAREMETER 'WithData' SET TO true.
//
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
// Copies all data stored in a cache line into the array 'data'
//
/////////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetLineData(UINT64 index, UINT32 way, T data[NumObjectsPerLine])
{
    if ( WithData == false ) return;
    
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    
    for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) {
        data[i] =  DataArray[index][way][i];
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Copies a single data item stored in a cache line into the variable 'data'
//
/////////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::GetLineData(UINT64 index, UINT32 way, UINT32 ObjectInLine, T *data)
{
    if ( WithData == false ) return;
    
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    ASSERTX(ObjectInLine < NumObjectsPerLine);
    
    *data = DataArray[index][way][ObjectInLine];
}

//////////////////////////////////////////////////////////////////////////
//
// This function Updates a FULL cache line of the DATA ARRAY.
//
//////////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::SetLineData(UINT64 index, UINT32 way, T data[NumObjectsPerLine])
{
    if ( WithData == false ) return;
    
    //
    // Compute index
    //
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    
    //
    // Check we are in the appropriate state
    //
// ASSERT((TagArray[index][way].GetStatus() == S_EXCLUSIVE_DIRTY)||(TagArray[index][way].GetStatus() == S_EXCLUSIVE_CLEAN),"You can not use SetLineData on a line that is not S_EXCLUSIVE_*\n");

    //
    // copy data into the cache
    //
    for (UINT32 i = 0; i < NumObjectsPerLine; i++ ) {
        ASSERT(TagArray[index][way].GetValidBit(i) == true,"ValidBit(i) must be true before writing data into the cache!\n");
        TagArray[index][way].SetDirtyBit(i);
        DataArray[index][way][i] = data[i];
    }
}

///////////////////////////////////////////////////////////////////////////
//
// This function Updates a single OBJECT within a cache line of the DATA ARRAY.
//
///////////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::SetLineData(UINT64 index, UINT32 way, UINT32 ObjectInLine, T data)
{
    if ( WithData == false ) return;
    
    //
    // Compute index
    //
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);
    ASSERTX(ObjectInLine < NumObjectsPerLine);
    
    //
    // Check we are in the appropriate state
 //
// ASSERT((TagArray[index][way].GetStatus() == S_EXCLUSIVE_DIRTY)||(TagArray[index][way].GetStatus() == S_EXCLUSIVE_CLEAN),"You can not use SetLineData on a line that is not S_EXCLUSIVE_*\n");
    
    //
    // copy data item into the cache
    //
    ASSERT(TagArray[index][way].GetValidBit(ObjectInLine) == true,"ValidBit(i) must be true before writing data into the cache!\n");
    TagArray[index][way].SetDirtyBit(ObjectInLine);
    DataArray[index][way][ObjectInLine] = data;
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::Dump(UINT64 index, UINT32 way)
{
    //NOTE: DO NOT USE THIS FUNCTION. THIS SHOULD BE REPLACED BY
    //      A CALL TO SaveState(). 
    //      FOR NOW, WE ASSERT SO NOONE USES IT
    ASSERT(false,"This function has been replaced by a SaveState in cache.h");
    
    //rest of the original function follows
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);

    //cout << "\tDump for cache line at <" << fmt_x(index)
      //   << "x,--> way " << way << ":";
    TagArray[index][way].Dump();
}

template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::DumpLRU(UINT64 index)
{
     //NOTE: DO NOT USE THIS FUNCTION. THIS SHOULD BE REPLACED BY
     //      A CALL TO SaveLRUState(). 
     //      FOR NOW, WE ASSERT SO NOONE USES IT
     ASSERT(false,"This function has been replaced by SaveLRUState in cache.h");
 
     //rest of the original function follows    
     ASSERTX(index < NumLinesPerWay);
     //cout << "Dump for index 0x" << fmt_x(index) << " :";
     //
     
     // NOTE: 
     //    this should really call the Dump routine for each replacment policy
     //  instead if calling a generic Dump()
     //    Some replacement policies do not use the default linked-list to 
     //  model LruArray. In this case, Dump() will dump bogus information
     //    Can fix this by having each replacement policy implement its own
     //  dump routine and replace this call by something like this->DumpLRUState(index)   
     //
     //this->LruArray[index].Dump();
     this->Dump(index);
}
//////////////////////////////////////////////////////////////////////////
//
// Tester routine
//
//////////////////////////////////////////////////////////////////////////
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::tester(void)
{
    this->LruArray[4].Dump();
    
    this->LruArray[4].makeMRU(5);
    ASSERTX(this->LruArray[4].GetLRU() == 4);
    ASSERTX(this->LruArray[4].GetMRU() == 5);
    this->LruArray[4].Dump();
    
    this->LruArray[4].makeLRU(2);
    ASSERTX(this->LruArray[4].GetLRU() == 2);
    ASSERTX(this->LruArray[4].GetMRU() == 5);
    this->LruArray[4].Dump();
    
    this->LruArray[4].makeLRU(5);
    ASSERTX(this->LruArray[4].GetLRU() == 5);
    ASSERTX(this->LruArray[4].GetMRU() == 0);
    this->LruArray[4].Dump();
    
    this->LruArray[4].makeMRU(3);
    ASSERTX(this->LruArray[4].GetLRU() == 5);
    ASSERTX(this->LruArray[4].GetMRU() == 3);
    this->LruArray[4].Dump();
    
    this->LruArray[4].makeMRU(2);
    ASSERTX(this->LruArray[4].GetLRU() == 5);
    ASSERTX(this->LruArray[4].GetMRU() == 2);
    this->LruArray[4].Dump();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
////
////
////  IMPLEMENTATION for LRU manager class
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<UINT8 NumWays>
lru_info<NumWays>::lru_info()
{
    UINT32 i;
    
    //
    // We set a doubly linked list of all elements in the chain array
    //
    for (i = 0; i < NumWays; i++ ) {
        linklist[i].next = (i == 0)            ? 0xff : (i-1);
        linklist[i].prev = (i == (NumWays -1)) ? 0xff : (i+1);
    }
    
    // Now, by pure convention, we declare the MRU set, the one in position 0
    // and we declare the LRU set the one in position NumWays -1.
    // ** Actually, we'll do it the other way around- 0 = LRU,
    // ** NumWays -1 is MRU to match Tracs 
    lru = 0;
    mru = NumWays - 1;
}

template<UINT8 NumWays>
void
lru_info<NumWays>::makeMRU(UINT8 w)
{
    ASSERTX(w < NumWays);
    
    //
    // Fast check: if 'w' is already the MRU, nothing to be done
    //
    if ( mru == w ) 
    {
        return;
    }

    //
    // Step number one: unlink element 'w' from its current position in the doubly
    // linked list (and connect its neighbours together)
    //
    if ( linklist[w].prev != -1 ) 
    {
        linklist[linklist[w].prev].next = linklist[w].next;
    }

    if ( linklist[w].next != -1 ) 
    {
        linklist[linklist[w].next].prev = linklist[w].prev;
    }
    
    //
    // Step two; if element 'w' happened to be the LRU element, then update LRU
    //
    if ( lru == w ) 
    {
        lru = linklist[w].prev;
    }

    //
    // Step three: move element 'w' to the head of the list (he is MRU).
    //
    linklist[w].prev = -1;
    linklist[w].next = mru;
    linklist[mru].prev = w;
    
    // Make 'w' the MRU element.
    mru = w;
}

template<UINT8 NumWays>
void
lru_info<NumWays>::makeLRU(UINT8 w)
{
    ASSERTX(w < NumWays);
    
    //
    // Fast check: if 'w' is already the LRU, nothing to be done
    //
    if ( lru == w ) 
    {
        return;
    }
    
    //
    // Step number one: unlink element 'w' from its current position in the doubly
    // linked list (and connect its neighbours together)
    //
    if ( linklist[w].prev != -1 ) 
    {
        linklist[linklist[w].prev].next = linklist[w].next;
    }

    if ( linklist[w].next != -1 ) 
    {
        linklist[linklist[w].next].prev = linklist[w].prev;
    }
    
    //
    // Step two; if element 'w' happened to be the MRU element, then update MRU
    //
    if ( mru == w ) 
    {
        mru = linklist[w].next;
    }

    //
    // Step three: move element 'w' to the tail of the list (he is LRU).
    //
    linklist[w].next = -1;
    linklist[w].prev = lru;
    linklist[lru].next = w;
    
    // Make 'w' the LRU element.
    lru = w;
}

template<UINT8 NumWays>
void
lru_info<NumWays>::Dump()
{
    INT8 p,i;

    //NOTE: This function is now implemented to be part of the 
    //      active replacement policy
    //See the class definitions of each policy for the new function
    
    for (i = 0, p = mru; i < NumWays; i++, p = linklist[p].next ) 
    {
        ASSERTX(p != -1);
        cout << " " << p << " ->";
    }
}

//////////////////////////////////////////////////////////////
//// 
//// Save/restore cache contents
////
//////////////////////////////////////////////////////////////

//
// save contents
//
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::SaveCacheState(UINT64 index, UINT32 way, ostream& out)
{
    UINT64 pa;
    
    ASSERTX(index < NumLinesPerWay);
    ASSERTX(way < NumWays);

    //we have to add an 'if' here becuase this function returns a false
    //if the line we are dumping is invalid    
    if(TagArray[index][way].SaveTagArrayState(index,way,out)) 
    {
        pa = Original(index,TagArray[index][way].GetTag());
        out << ", PA=0x" << fmt_x(pa) << endl;
    }
}

//
// restore contents
//
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::RestoreCacheState(istream& in)
{
    //YARDI:
    // right now this is a hacked parser for the BMP LLC.
    //
    
    int MAX_CHARS_PER_LINE=256;
    char *line, *token, *prevToken;
    UINT64 index;
    UINT32 way;
    UINT64 tag;
    LINE_STATUS currStatus;
    UINT32 OwnerID;
    UINT8 validLineInfo,done;
    
    //init all lines to invalid
    //the ones that are not will be assigned the correct status below
    for(UINT32 i=0; i<NumLinesPerWay; i++)
    {
        for(UINT8 j=0; j<NumWays; j++)
        {
            TagArray[i][j].Clear();            
            TagArray[i][j].SetWay(j);
        }
    }
    
    //init parser
    line = new char[MAX_CHARS_PER_LINE];
    prevToken = new char[MAX_CHARS_PER_LINE];
    token = new char[MAX_CHARS_PER_LINE];    
    validLineInfo = false;
    done=false;
    while(!done)
    {
        //read a line
        in.getline(line,256);
        //cout << "Original line : " << endl;
        //cout << line << endl;
        validLineInfo = false;    //filter out everything else other than contents
        if(in.gcount() > 1)
        {
            //tokenize
            token = strtok(line, " ,:=\t");
            while(token != NULL)
            {
                //cout << "Curr token: " << token << endl;
                if(!strcmp(prevToken,"DONE"))
                {
                    //done parsing cache contents
                    done = true;
                    validLineInfo = false;
                    break;
                }
                if(!strcmp(prevToken,"S"))
                {
                    //rest of this line contains valid info
                    validLineInfo = true;
                    index = atoi_general_unsigned(token);
                    //cout << "S: " << index << endl;
                    ASSERTX(index < NumLinesPerWay);
                }
                if(!strcmp(prevToken,"W"))
                {
                    way = atoi_general_unsigned(token);
                    //cout << "W: " << way << endl;
                    ASSERTX(way < NumWays);
                    TagArray[index][way].SetWay(way);
                    //cout << "Set way = " << TagArray[index][way].GetWay();
                }
                if(!strcmp(prevToken,"tag"))
                {
                    tag = atoi_general_unsigned(token);
                    //cout << "tag=0x" << hex << tag << endl;
                    TagArray[index][way].SetTag(tag);
                    //cout << "Set tag = " << TagArray[index][way].GetTag();   
                }
                if(!strcmp(prevToken,"status"))
                {
                    for(UINT8 i=0; i < S_MAX_LINE_STATUS; i++)
                    {
                        if(!strcmp(LINE_STATUS_STRINGS[i],token))
                        {
                            currStatus = (LINE_STATUS)i;
                            //cout << "status=" << LINE_STATUS_STRINGS[i] << " - " << token << endl;
                        }
                    }
                    //when restoring, we make a reserved line invalid
                    if(currStatus == S_RESERVED)
                    {
                        TagArray[index][way].Clear();
                        TagArray[index][way].SetWay(way);
                    }
                    else
                        TagArray[index][way].SetStatus(currStatus);
                    //cout << "Set status = " << TagArray[index][way].GetStatus();   
                }
                if(!strcmp(prevToken,"valid"))
                {
                    //valid bit info is of the form 0b11111111
                    //so ignore the first two characters when assigning valid bits
                    ASSERTX(strlen(token) == (NumObjectsPerLine+2));
                    //cout << "valid=";
                    for(UINT8 i=0,j=2; i < NumObjectsPerLine; i++)
                    {
                        //i is used to index the objects on a line
                        //j is used to index the "token"
                        //this avoid any assertions in Set/Clear ValidBit
                        //cout << token[j];
                        if(token[j]=='1')
                            TagArray[index][way].SetValidBit(i);
                        else
                            TagArray[index][way].ClearValidBit(i);
                    }
                }
                if(!strcmp(prevToken,"dirty"))
                {
                    //dirty bit info is of the form 0b11111111
                    //so ignore the first two characters when assigning valid bits
                    ASSERTX(strlen(token) == (NumObjectsPerLine+2));
                    //cout << "dirty=";
                    for(UINT8 i=0,j=2; i < NumObjectsPerLine; i++)
                    {
                        //i is used to index the objects on a line
                        //j is used to index the "token"
                        //this avoid any assertions in Set/Clear DirtyBit
                        //cout << token[j];
                        if(token[j]=='1')
                            TagArray[index][way].SetDirtyBit(i);
                        else
                            TagArray[index][way].ClearDirtyBit(i);
                    }
                }
                if(!strcmp(prevToken,"ownerId"))
                {
                    //cout << "owenerId token = " << token << endl;
                    OwnerID = atoi_general_unsigned(token);
                    //cout << "ownerID=" << OwnerID << endl;
                    TagArray[index][way].SetOwnerId(OwnerID);
                }
                //remember prev token
                strcpy(prevToken,token);
                //get next token
                token = strtok(NULL, " ,:=\t");
            }//end while(token!=NULL)
            
            //debug: check if we filled in everything correctly
            /*
            if(validLineInfo)
            {
                //cout << "Line at S=" << index << " W=" << way << "   done" << endl;
                if(TagArray[index][way].SaveTagArrayState(index, way, out)) 
                {
                    pa = Original(index,TagArray[index][way].GetTag());
                    out << " PA=0x" << fmt_x(pa) << endl;
                }
            }
            */
        }//end (gcount>1)
    }//end while
}

//////////////////////////////////////////////////////////////
//// 
//// Save/restore LRU state
////
//////////////////////////////////////////////////////////////
//
// Save LRU state
//
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::SaveLRUState(UINT64 index, ostream &out)
{
    ASSERTX(index < NumLinesPerWay);
    //cout << "Dump for index 0x" << fmt_x(index) << " :";
    out << "S: " << index << " ";
    //each replacement algorithm now has its own SaveState function
    this->SaveState(index,out);
    //the old Dump() function is no longer required
    //this->Dump(index,out);
}

//
// Restore LRU state
//
template<UINT8 NumWays, UINT32 NumLinesPerWay, UINT32 NumObjectsPerLine,
         class T, bool WithData, template <UINT8,UINT32> class VictimPolicy, class INFO>
void
gen_cache_class<NumWays,NumLinesPerWay,NumObjectsPerLine,T,WithData,VictimPolicy,INFO>::RestoreLRUState(istream &in)
{
    //parse the istream to get an index into LruArray
    //parsing should be VictimPolicy-specific, so call the
    //restoreLRU function of the current victim policy
    this->RestoreLRU(in);
}

//there will be one such function for each replacement policy
template<UINT8 NumWays, UINT32 NumLinesPerWay>
void
EV7_scheme_replacement<NumWays,NumLinesPerWay>::RestoreLRU(istream& in)
{
    //YARDI:
    // right now this is a hacked parser for the BMP LLC.
    //

    //parse and restore LRU state from input
    int MAX_CHARS_PER_LINE=256;
    char *line, *token, *prevToken;
    UINT32 numIndex = 0;
    UINT64 currMask;
    UINT32 currIndex;
    UINT8 foundIndex;
    
    line = new char[MAX_CHARS_PER_LINE];
    prevToken = new char[MAX_CHARS_PER_LINE];
    token = new char[MAX_CHARS_PER_LINE];    
    
    //there are at most NumLinesPerWay indices for the LruArray
    //so we have to read these many lines from the dump file
    foundIndex = false;
    currIndex = 0;
    currMask = 0;
    while(numIndex < NumLinesPerWay)
    {
        ++numIndex;
        //read a line
        in.getline(line,256);
        //cout << line << "   ";
        //tokenize
        token = strtok(line, " ,:\t");
        while(token != NULL) 
        {
            if(foundIndex)
            {
                //the prev token gave the index
                //so this token must be the mask value
                currMask = atoi_general_unsigned(token);
                //out << "S: " << currIndex << " ";
                //out << "0x" << currMask << endl;
                
                //call the restore function of the specific structure
                //  used by this policy
                this->LruArray[currIndex].RestoreState(currMask);
                foundIndex = false;
            }
            if(!strcmp(prevToken,"S"))
            {
                //then current token must be the index
                currIndex = atoi_general_unsigned(token);
                ASSERTX(currIndex < NumLinesPerWay);
                foundIndex = true;
            }
            strcpy(prevToken,token);
            //next token                  
            token = strtok(NULL, " ,:\t");
        }
    }
}

#endif // CACHE_H
