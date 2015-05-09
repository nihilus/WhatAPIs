
// ****************************************************************************
// File: Core.cpp
// Desc: Function API use commenter plug-in
//
// ****************************************************************************
#include "stdafx.h"
#include <WaitBoxEx.h>
#include <list>
#include <set>

#define MYTAG "<API*> "

struct ltstr
{
	bool operator() (const std::string &a, const std::string &b) const
	{
		return stricmp(a.c_str(), b.c_str()) < 0;
	}
};
typedef std::set<std::string, ltstr>  STRINGMAP;
typedef std::list<std::string> STRLIST;

// === Function Prototypes ===
static void processFunction(func_t *f);

// === Data ===
static UINT commentCount = 0, segmentCount = 0;
static STRINGMAP apiMap;

// Import segments bounds list
struct BOUNDS
{
	ea_t startEA, endEA;
} static *segmentPtr = NULL;
typedef std::list<BOUNDS> SEGLIST;


// Main dialog
static const char mainDialog[] =
{
	"BUTTON YES* Continue\n" // "Continue" instead of "okay"
	"WhatAPIs\n"

    #ifdef _DEBUG
    "** DEBUG BUILD **\n"
    #endif
	"Adds function API usage list comments to each function that has them.\n\n"
    "Version %Aby Sirmabus\n"
    "<#Click to open site.#www.macromonkey.com:k:1:1::>\n\n"

    " \n\n\n\n\n"
};

// Initialize
void CORE_Init()
{
}

// Uninitialize
void CORE_Exit()
{
    apiMap.clear();
}

static void idaapi doHyperlink(TView *fields[], int code) { open_url("http://www.macromonkey.com/bb/"); }

// Import module name enumeration callback
static int idaapi importNameCallback(ea_t ea, const char *name, uval_t ord, void *param)
{
	// Skip the ordinal ones
	if(name)
	{
		// Insert it if it doesn't exist
		apiMap.insert(name);
		//msg(" \"%s\"\n", name);
	}

	return(1);
}

// Plug-in process
void CORE_Process(int iArg)
{
    try
    {
        char version[16];
        sprintf(version, "%u.%u", HIBYTE(MY_VERSION), LOBYTE(MY_VERSION));
        msg("\n>> WhatAPIs: v: %s, built: %s, By Sirmabus\n", version, __DATE__);
        if (!autoIsOk())
        {
            msg("** Must wait for IDA to finish processing before starting plug-in! **\n*** Aborted ***\n\n");
            return;
        }

        // Show UI
        refreshUI();
        int uiResult = AskUsingForm_c(mainDialog, version, doHyperlink);
        if (!uiResult)
        {
            msg(" - Canceled -\n");
            return;
        }

        WaitBox::show();
        TIMESTAMP startTime = getTimeStamp();

        // Build import segment bounds table
        {
            msg("Import segments:\n");
            refreshUI();
            SEGLIST segList;
            for (int i = 0; i < get_segm_qty(); i++)
            {
                if (segment_t *s = getnseg(i))
                {
                    if (s->type == SEG_XTRN)
                    {
                        char buffer[64] = { "unknown" }; buffer[SIZESTR(buffer)] = 0;
                        get_true_segm_name(s, buffer, SIZESTR(buffer));
                        msg(" [%d] \"%s\" "EAFORMAT" - "EAFORMAT"\n", segmentCount, buffer, s->startEA, s->endEA);
                        BOUNDS b = { s->startEA, s->endEA };
                        segList.push_back(b);
                        segmentCount++;
                    }
                }
            }
            refreshUI();

            // Flatten list into an array for speed
            if (segmentCount)
            {
                UINT size = (segmentCount * sizeof(BOUNDS));
                if (segmentPtr = (BOUNDS *)_aligned_malloc(size, 16))
                {
                    BOUNDS *b = segmentPtr;
                    for (SEGLIST::iterator i = segList.begin(); i != segList.end(); i++, b++)
                    {
                        b->startEA = i->startEA;
                        b->endEA   = i->endEA;
                    }
                }
                else
                {
                    msg("\n*** Allocation failure of %u bytes! ***\n", size);
                    refreshUI();
                }
            }
        }

        if (segmentCount)
        {
            // Make a list of all import names
            if (int moduleCount = get_import_module_qty())
            {
                for (int i = 0; i < moduleCount; i++)
                    enum_import_names(i, importNameCallback);

                char buffer[32];
                msg("Parsed %s module imports.\n", prettyNumberString(moduleCount, buffer));
                refreshUI();
            }

            // Iterate through all functions..
            BOOL aborted = FALSE;
            UINT functionCount = get_func_qty();
            char buffer[32];
            msg("Processing %s functions.\n", prettyNumberString(functionCount, buffer));
            refreshUI();

            for (UINT n = 0; n < functionCount; n++)
            {
                processFunction(getn_func(n));

                if (WaitBox::isUpdateTime())
                {
                    if (WaitBox::updateAndCancelCheck((int)(((float)n / (float)functionCount) * 100.0f)))
                    {
                        msg("* Aborted *\n");
                        break;
                    }
                }
            }

            refresh_idaview_anyway();
            WaitBox::hide();
            msg("\n");
            msg("Done. %s comments add/appended in %s.\n", prettyNumberString(commentCount, buffer), timeString(getTimeStamp() - startTime));
            msg("-------------------------------------------------------------\n");
        }
        else
            msg("\n*** No import segments! ***\n");

        if (segmentPtr)
        {
            _aligned_free(segmentPtr);
            segmentPtr = NULL;
        }
        apiMap.clear();
    }
    CATCH()
}


// Return TRUE if address is in an import segment
inline BOOL isInImportSeg(ea_t ea)
{
	for(UINT i = 0; i < segmentCount; i++)
		if((ea >= segmentPtr[i].startEA) && (ea < segmentPtr[i].endEA))
			return(TRUE);

	return(FALSE);
}

// Process function
void processFunction(func_t *f)
{
	// Skip tiny functions
	if(f->size() >= 5)
	{
		// Don't add comments to API wrappers
        char name[MAXNAMELEN]; name[0] = name[SIZESTR(name)] = 0;
		if(!apiMap.empty())
		{
			if(get_short_name(BADADDR, f->startEA, name, SIZESTR(name)))
			{
				if(apiMap.find(name) != apiMap.end())
					return;
			}
		}

		// Iterate function body
        STRLIST importLstTmp;
        LPSTR commentPtr = NULL;
		char comment[MAXSTR]; comment[0] = comment[SIZESTR(comment)] = 0;
        UINT commentLen = 0;

        #define ADDNM(_str) { UINT l = strlen(_str); memcpy(comment + commentLen, _str, l); commentLen += l; _ASSERT(commentLen < MAXSTR); }

        func_item_iterator_t it(f);
		do
		{
            ea_t currentEA = it.current();

			// Will be a "to" xref
			xrefblk_t xb;
			if(xb.first_from(currentEA, XREF_FAR))
			{
				BOOL isImpFunc = FALSE;
                name[0] = 0;

				// If in import segment
				// ============================================================================================
				ea_t refAdrEa = xb.to;
				if(isInImportSeg(refAdrEa))
				{
					flags_t flags = get_flags_novalue(refAdrEa);
					if(has_name(flags) && hasRef(flags) && isDwrd(flags))
					{
						if(get_short_name(BADADDR, refAdrEa, name, SIZESTR(name)))
						{
							// Nix the imp prefix if there is one
							if(strncmp(name, "__imp_", SIZESTR("__imp_")) == 0)
								memmove(name, name + SIZESTR("__imp_"), ((strlen(name) - SIZESTR("__imp_")) + 1));

							isImpFunc = TRUE;
						}
						else
							msg(EAFORMAT" *** Failed to get import name! ***\n", refAdrEa);
					}
				}
				// Else, check for import wrapper
				// ============================================================================================
				else
				if(!apiMap.empty())
				{
					// Reference is a function entry?
					flags_t flags = get_flags_novalue(refAdrEa);
					if(isCode(flags) && has_name(flags) && hasRef(flags))
					{
						if(func_t *refFuncPtr = get_func(refAdrEa))
						{
							if(refFuncPtr->startEA == refAdrEa)
							{
								if(get_short_name(BADADDR, refAdrEa, name, SIZESTR(name)))
								{
									// Skip common unwanted types "sub_.." or "unknown_libname_.."
									if(
										// not "sub_..
										/*"sub_"*/ (*((PUINT) name) != 0x5F627573) &&

										// not "unknown_libname_..
										/*"unknown_"*/ ((*((PUINT64) name) != 0x5F6E776F6E6B6E75) && (*((PUINT64) (name + 8)) != /*"libname_"*/ 0x5F656D616E62696C)) &&

										// not nullsub_..
										/*"nullsub_"*/ (*((PUINT64) name) != 0x5F6275736C6C756E)
										)
									{
										// Nix the import prefixes
										if(strncmp(name, "__imp_", SIZESTR("__imp_")) == 0)
											memmove(name, name + SIZESTR("__imp_"), ((strlen(name) - SIZESTR("__imp_")) + 1));

										// Assumed to be a wrapped import if it's in the list
										isImpFunc = (apiMap.find(name) != apiMap.end());
									}
								}
								else
									msg(EAFORMAT" *** Failed to get function name! ***\n", refAdrEa);
							}
						}
					}
				}

				// Found import function to add list
				if(isImpFunc)
				{
					// Skip those large common STL names
					if(strncmp(name, "std::", SIZESTR("std::")) != 0)
					{
						// Skip if already seen in this function
						BOOL known = FALSE;
						for(STRLIST::iterator ji = importLstTmp.begin(); ji != importLstTmp.end(); ji++)
						{
							if(strcmp(ji->c_str(), name) == 0)
							{
								known = TRUE;
								break;
							}
						}

						// Not seen
						if(!known)
						{
							importLstTmp.push_front(name);

                            // Append to existing comments w/line feed
                            if(!commentLen && !commentPtr)
                            {
                                commentPtr = get_func_cmt(f, true);
                                if(!commentPtr)
                                    get_func_cmt(f, false);

                                if(commentPtr)
                                {
                                    commentLen = strlen(commentPtr);
                                    // Bail out not enough comment space
                                    if(commentLen >= (MAXSTR - 20))
                                    {
                                        qfree(commentPtr);
                                        return;
                                    }

                                    memcpy(comment, commentPtr, commentLen);
                                    ADDNM("\n"MYTAG);
                                }
                            }

                            if(!commentLen)
                                ADDNM(MYTAG);

							// Append a "..." (continuation) and bail out if name hits max comment length
							if((commentLen + strlen(name) + SIZESTR("()") + sizeof(", ")) >= (MAXSTR - sizeof("...")))
							{
                                ADDNM(" ...");
								break;
							}
							// Append this function name
							else
							{
								if(importLstTmp.size() != 1)
                                    ADDNM(", ");
                                ADDNM(name); ADDNM("()");
							}
						}
					}
					else
					{
						//msg("%s\n", szName);
					}
				}
			}

		}while(it.next_addr());

		if(!importLstTmp.empty() && commentLen)
		{
            // Add comment
            comment[commentLen] = 0;
			set_func_cmt(f, comment, true);
			commentCount++;
		}

        if(commentPtr)
            qfree(commentPtr);
	}
}

