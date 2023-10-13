#include "hv.h"

BOOLEAN nrot_hv_init() {
	//allocate structures
	if (!nrot_ept_init()) return FALSE;
	if (!nrot_vmx_init()) return FALSE;
	return TRUE;
}