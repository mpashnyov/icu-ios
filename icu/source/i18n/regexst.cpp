//
//  regexst.h
//
//  Copyright (C) 2004-2013, International Business Machines Corporation and others.
//  All Rights Reserved.
//
//  This file contains class RegexStaticSets
//
//  This class is internal to the regular expression implementation.
//  For the public Regular Expression API, see the file "unicode/regex.h"
//
//  RegexStaticSets groups together the common UnicodeSets that are needed
//   for compiling or executing RegularExpressions.  This grouping simplifies
//   the thread safe lazy creation and sharing of these sets across
//   all instances of regular expressions.
//
#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/regex.h"
#include "uprops.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "ucln_in.h"
#include "umutex.h"

#include "regexcst.h"   // Contains state table for the regex pattern parser.
                        //   generated by a Perl script.
#include "regexst.h"



U_NAMESPACE_BEGIN


//------------------------------------------------------------------------------
//
// Unicode Set pattern strings for all of the required constant sets.
//               Initialized with hex values for portability to EBCDIC based machines.
//                Really ugly, but there's no good way to avoid it.
//
//------------------------------------------------------------------------------

// "Rule Char" Characters are those with no special meaning, and therefore do not
//    need to be escaped to appear as literals in a regexp.  Expressed
//    as the inverse of those needing escaping --  [^\*\?\+\[\(\)\{\}\^\$\|\\\.]
static const UChar gRuleSet_rule_char_pattern[]       = {
 //   [    ^      \     *     \     ?     \     +     \     [     \     (     /     )
    0x5b, 0x5e, 0x5c, 0x2a, 0x5c, 0x3f, 0x5c, 0x2b, 0x5c, 0x5b, 0x5c, 0x28, 0x5c, 0x29,
 //   \     {    \     }     \     ^     \     $     \     |     \     \     \     .     ]
    0x5c, 0x7b,0x5c, 0x7d, 0x5c, 0x5e, 0x5c, 0x24, 0x5c, 0x7c, 0x5c, 0x5c, 0x5c, 0x2e, 0x5d, 0};


static const UChar gRuleSet_digit_char_pattern[] = {
//    [    0      -    9     ]
    0x5b, 0x30, 0x2d, 0x39, 0x5d, 0};

//
//   Here are the backslash escape characters that ICU's unescape() function
//    will handle.
//
static const UChar gUnescapeCharPattern[] = {
//    [     a     c     e     f     n     r     t     u     U     x    ]
    0x5b, 0x61, 0x63, 0x65, 0x66, 0x6e, 0x72, 0x74, 0x75, 0x55, 0x78, 0x5d, 0};


//
//  Unicode Set Definitions for Regular Expression  \w
//
static const UChar gIsWordPattern[] = {
//    [     \     p     {    A     l     p     h     a     b     e     t     i      c    }
    0x5b, 0x5c, 0x70, 0x7b, 0x61, 0x6c, 0x70, 0x68, 0x61, 0x62, 0x65, 0x74, 0x69, 0x63, 0x7d,
//          \     p     {    M     }                               Mark
          0x5c, 0x70, 0x7b, 0x4d, 0x7d,
//          \     p     {    N     d     }                         Digit_Numeric
          0x5c, 0x70, 0x7b, 0x4e, 0x64, 0x7d,
//          \     p     {    P     c     }                         Connector_Punctuation
          0x5c, 0x70, 0x7b, 0x50, 0x63, 0x7d,
//          \     u     2    0     0     c      \     u     2    0     0     d     ]
          0x5c, 0x75, 0x32, 0x30, 0x30, 0x63, 0x5c, 0x75, 0x32, 0x30, 0x30, 0x64, 0x5d, 0};


//
//  Unicode Set Definitions for Regular Expression  \s
//
static const UChar gIsSpacePattern[] = {
//        [     \     p     {     W     h     i     t     e     S     p     a     c     e     }     ]
        0x5b, 0x5c, 0x70, 0x7b, 0x57, 0x68, 0x69, 0x74, 0x65, 0x53, 0x70, 0x61, 0x63, 0x65, 0x7d, 0x5d, 0};


//
//  UnicodeSets used in implementation of Grapheme Cluster detection, \X
//
static const UChar gGC_ControlPattern[] = {
//    [     [     :     Z     l     :     ]     [     :     Z     p     :     ]
    0x5b, 0x5b, 0x3a, 0x5A, 0x6c, 0x3a, 0x5d, 0x5b, 0x3a, 0x5A, 0x70, 0x3a, 0x5d,
//    [     :     C     c     :     ]     [     :     C     f     :     ]     -
    0x5b, 0x3a, 0x43, 0x63, 0x3a, 0x5d, 0x5b, 0x3a, 0x43, 0x66, 0x3a, 0x5d, 0x2d,
//    [     :     G     r     a     p     h     e     m     e     _
    0x5b, 0x3a, 0x47, 0x72, 0x61, 0x70, 0x68, 0x65, 0x6d, 0x65, 0x5f,
//    E     x     t     e     n     d     :     ]     ]
    0x45, 0x78, 0x74, 0x65, 0x6e, 0x64, 0x3a, 0x5d, 0x5d, 0};

static const UChar gGC_ExtendPattern[] = {
//    [     \     p     {     G     r     a     p     h     e     m     e     _
    0x5b, 0x5c, 0x70, 0x7b, 0x47, 0x72, 0x61, 0x70, 0x68, 0x65, 0x6d, 0x65, 0x5f,
//    E     x     t     e     n     d     }     ]
    0x45, 0x78, 0x74, 0x65, 0x6e, 0x64, 0x7d, 0x5d, 0};

static const UChar gGC_LPattern[] = {
//    [     \     p     {     H     a     n     g     u     l     _     S     y     l
    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,
//    l     a     b     l     e     _     T     y     p     e     =     L     }     ]
    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x7d,  0x5d, 0};

static const UChar gGC_VPattern[] = {
//    [     \     p     {     H     a     n     g     u     l     _     S     y     l
    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,
//    l     a     b     l     e     _     T     y     p     e     =     V     }     ]
    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x56, 0x7d,  0x5d, 0};

static const UChar gGC_TPattern[] = {
//    [     \     p     {     H     a     n     g     u     l     _     S     y     l
    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,
//    l     a     b     l     e     _     T     y     p     e     =     T     }    ]
    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x54, 0x7d, 0x5d, 0};

static const UChar gGC_LVPattern[] = {
//    [     \     p     {     H     a     n     g     u     l     _     S     y     l
    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,
//    l     a     b     l     e     _     T     y     p     e     =     L     V     }     ]
    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x56, 0x7d, 0x5d, 0};

static const UChar gGC_LVTPattern[] = {
//    [     \     p     {     H     a     n     g     u     l     _     S     y     l
    0x5b, 0x5c, 0x70, 0x7b, 0x48, 0x61, 0x6e, 0x67, 0x75, 0x6c, 0x5f, 0x53, 0x79, 0x6c,
//    l     a     b     l     e     _     T     y     p     e     =     L     V     T     }     ]
    0x6c, 0x61, 0x62, 0x6c, 0x65, 0x5f, 0x54, 0x79, 0x70, 0x65, 0x3d, 0x4c, 0x56, 0x54, 0x7d, 0x5d, 0};


RegexStaticSets *RegexStaticSets::gStaticSets = NULL;
UInitOnce gStaticSetsInitOnce = U_INITONCE_INITIALIZER;

RegexStaticSets::RegexStaticSets(UErrorCode *status)
:
fUnescapeCharSet(UnicodeString(TRUE, gUnescapeCharPattern, -1), *status),
fRuleDigitsAlias(NULL),
fEmptyText(NULL)
{
    // First zero out everything
    int i;
    for (i=0; i<URX_LAST_SET; i++) {
        fPropSets[i] = NULL;
    }
    // Then init the sets to their correct values.
    fPropSets[URX_ISWORD_SET]  = new UnicodeSet(UnicodeString(TRUE, gIsWordPattern, -1),     *status);
    fPropSets[URX_ISSPACE_SET] = new UnicodeSet(UnicodeString(TRUE, gIsSpacePattern, -1),    *status);
    fPropSets[URX_GC_EXTEND]   = new UnicodeSet(UnicodeString(TRUE, gGC_ExtendPattern, -1),  *status);
    fPropSets[URX_GC_CONTROL]  = new UnicodeSet(UnicodeString(TRUE, gGC_ControlPattern, -1), *status);
    fPropSets[URX_GC_L]        = new UnicodeSet(UnicodeString(TRUE, gGC_LPattern, -1),       *status);
    fPropSets[URX_GC_V]        = new UnicodeSet(UnicodeString(TRUE, gGC_VPattern, -1),       *status);
    fPropSets[URX_GC_T]        = new UnicodeSet(UnicodeString(TRUE, gGC_TPattern, -1),       *status);
    fPropSets[URX_GC_LV]       = new UnicodeSet(UnicodeString(TRUE, gGC_LVPattern, -1),      *status);
    fPropSets[URX_GC_LVT]      = new UnicodeSet(UnicodeString(TRUE, gGC_LVTPattern, -1),     *status);
    
    // Check for null pointers
    if (fPropSets[URX_ISWORD_SET] == NULL || fPropSets[URX_ISSPACE_SET] == NULL || fPropSets[URX_GC_EXTEND] == NULL || 
        fPropSets[URX_GC_CONTROL] == NULL || fPropSets[URX_GC_L] == NULL || fPropSets[URX_GC_V] == NULL || 
        fPropSets[URX_GC_T] == NULL || fPropSets[URX_GC_LV] == NULL || fPropSets[URX_GC_LVT] == NULL) {
        goto ExitConstrDeleteAll;
    }
    if (U_FAILURE(*status)) {
        // Bail out if we were unable to create the above sets.
        // The rest of the initialization needs them, so we cannot proceed.
        return;
    }


    //
    // The following sets  are dynamically constructed, because their
    //   initialization strings would be unreasonable.
    //


    //
    //  "Normal" is the set of characters that don't need special handling
    //            when finding grapheme cluster boundaries.
    //
    fPropSets[URX_GC_NORMAL] = new UnicodeSet(0, UnicodeSet::MAX_VALUE);
    // Null pointer check
    if (fPropSets[URX_GC_NORMAL] == NULL) {
    	goto ExitConstrDeleteAll;
    }
    fPropSets[URX_GC_NORMAL]->remove(0xac00, 0xd7a4);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_CONTROL]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_L]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_V]);
    fPropSets[URX_GC_NORMAL]->removeAll(*fPropSets[URX_GC_T]);

    // Initialize the 8-bit fast bit sets from the parallel full
    //   UnicodeSets.
    for (i=0; i<URX_LAST_SET; i++) {
        if (fPropSets[i]) {
            fPropSets[i]->compact();
            fPropSets8[i].init(fPropSets[i]);
        }
    }

    // Sets used while parsing rules, but not referenced from the parse state table
    fRuleSets[kRuleSet_rule_char-128]   = UnicodeSet(UnicodeString(TRUE, gRuleSet_rule_char_pattern, -1),   *status);
    fRuleSets[kRuleSet_digit_char-128]  = UnicodeSet(UnicodeString(TRUE, gRuleSet_digit_char_pattern, -1),  *status);
    fRuleDigitsAlias = &fRuleSets[kRuleSet_digit_char-128];
    for (i=0; i<(int32_t)(sizeof(fRuleSets)/sizeof(fRuleSets[0])); i++) {
        fRuleSets[i].compact();
    }
    
    // Finally, initialize an empty string for utility purposes
    fEmptyText = utext_openUChars(NULL, NULL, 0, status);
    
    return; // If we reached this point, everything is fine so just exit

ExitConstrDeleteAll: // Remove fPropSets and fRuleSets and return error
    for (i=0; i<URX_LAST_SET; i++) {
        delete fPropSets[i];
        fPropSets[i] = NULL;
    }
    *status = U_MEMORY_ALLOCATION_ERROR;
}


RegexStaticSets::~RegexStaticSets() {
    int32_t i;

    for (i=0; i<URX_LAST_SET; i++) {
        delete fPropSets[i];
        fPropSets[i] = NULL;
    }
    fRuleDigitsAlias = NULL;
    
    utext_close(fEmptyText);
}


//------------------------------------------------------------------------------
//
//   regex_cleanup      Memory cleanup function, free/delete all
//                      cached memory.  Called by ICU's u_cleanup() function.
//
//------------------------------------------------------------------------------
UBool
RegexStaticSets::cleanup(void) {
    delete RegexStaticSets::gStaticSets;
    RegexStaticSets::gStaticSets = NULL;
    gStaticSetsInitOnce.reset();
    return TRUE;
}

U_CDECL_BEGIN
static UBool U_CALLCONV
regex_cleanup(void) {
    return RegexStaticSets::cleanup();
}

static void U_CALLCONV initStaticSets(UErrorCode &status) {
    U_ASSERT(RegexStaticSets::gStaticSets == NULL);
    ucln_i18n_registerCleanup(UCLN_I18N_REGEX, regex_cleanup);
    RegexStaticSets::gStaticSets = new RegexStaticSets(&status);
    if (U_FAILURE(status)) {
        delete RegexStaticSets::gStaticSets;
        RegexStaticSets::gStaticSets = NULL;
    }
    if (RegexStaticSets::gStaticSets == NULL && U_SUCCESS(status)) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}
U_CDECL_END

void RegexStaticSets::initGlobals(UErrorCode *status) {
    umtx_initOnce(gStaticSetsInitOnce, &initStaticSets, *status);
}

U_NAMESPACE_END
#endif  // !UCONFIG_NO_REGULAR_EXPRESSIONS
