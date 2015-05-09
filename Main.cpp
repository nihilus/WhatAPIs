
// ****************************************************************************
// File: Main.cpp
// Desc: Function API usage commenter, by Sirmabus
//
// ****************************************************************************
#include "stdafx.h"

// === Function Prototypes ===
int idaapi IDAP_init();
void idaapi IDAP_term();
void idaapi IDAP_run(int arg);
extern void CORE_Init();
extern void CORE_Process(int iArg);
extern void CORE_Exit();

// === Data ===
const static char IDAP_name[] = "Function API Usage Commenter";

extern "C" ALIGN(16) plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,
	PLUGIN_UNL,
	IDAP_init,
	IDAP_term,
	IDAP_run,
    IDAP_name,
    IDAP_name,
	IDAP_name,
	NULL
};

int idaapi IDAP_init()
{
    CORE_Init();
    return(PLUGIN_OK);
}

void idaapi IDAP_term()
{
    CORE_Exit();
}

void idaapi IDAP_run(int iArg)
{
    CORE_Process(iArg);
}



