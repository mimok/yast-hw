/*
 * Copyright 2018 NXP
 * Copyright 2021 Michael Grand
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_iap.h"
#include "fsl_iap_ffr.h"
#include "fsl_common.h"
#include "fsl_power.h"
////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////
static void verify_status(status_t status);
static void error_trap();
////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////
#define BUFFER_LEN 512 / 4
static uint32_t s_buffer_rbc[BUFFER_LEN];
////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////
int main() {
	flash_config_t flashInstance;
	static uint32_t status;
	uint32_t destAdrss; /* Address of the target location */
	uint32_t failedAddress, failedData;
	uint32_t pflashBlockBase = 0;
	uint32_t pflashTotalSize = 0;
	uint32_t pflashSectorSize = 0;
	uint32_t PflashPageSize = 0;
	const uint32_t s_buffer[BUFFER_LEN] = { 1, 2, 3, 4 };
	uint8_t data[512];
	cmpa_cfg_info_t cmpa = { 0x00 };
	/* Init board hardware. */
	/* set BOD VBAT level to 1.65V */
	POWER_SetBodVbatLevel(kPOWER_BodVbatLevel1650mv, kPOWER_BodHystLevel50mv,
	false);
	/* attach 12 MHz clock to FLEXCOMM0 (debug console) */
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);
	/* enable clock for GPIO*/
	CLOCK_EnableClock(kCLOCK_Gpio0);
	CLOCK_EnableClock(kCLOCK_Gpio1);

	BOARD_InitPins();
	BOARD_BootClockFROHF96M();
	BOARD_InitDebugConsole();

	PRINTF("\r\nUSB0 interface enabling for LPC55Sxx ISP\r\n\r\n");

	PRINTF("Initializing flash driver...\r\n");
	status = FLASH_Init(&flashInstance);
	verify_status(status);
	if(status != kStatus_Success) {
		error_trap();
	}

	PRINTF("Initializing PFR flash driver...\r\n");
	status = FFR_Init(&flashInstance);
	verify_status(status);
	if(status != kStatus_Success) {
		error_trap();
	}

	PRINTF("Reading CMPA\r\n");
	status = FFR_GetCustomerData(&flashInstance, (uint8_t*) &cmpa, 0, sizeof(cmpa));
	verify_status(status);
	if (status != kStatus_FLASH_Success) {
		error_trap();
	}

	PRINTF("Enabling USB0 interface\r\n");
	if ((cmpa.bootCfg & (1 << 9)) == 0) {
		cmpa.bootCfg |= 1 << 9;
		status = FFR_CustFactoryPageWrite(&flashInstance, (uint8_t*) &cmpa, false);
		verify_status(status);
		if (status != kStatus_FLASH_Success) {
			error_trap();
		}
		PRINTF("Reset the board...\r\n");
	} else {
		PRINTF("Nothing to do!\r\n");
		PRINTF("USB0 interface is already enabled\r\n");
	}

	while (1) {
	}
}

void verify_status(status_t status) {
	char *tipString = "Unknown status";
	switch (status) {
	case kStatus_Success:
		tipString = "Done!";
		break;
	case kStatus_InvalidArgument:
		tipString = "Invalid argument.";
		break;
	case kStatus_FLASH_AlignmentError:
		tipString = "Alignment Error.";
		break;
	case kStatus_FLASH_AccessError:
		tipString = "Flash Access Error.";
		break;
	case kStatus_FLASH_CommandNotSupported:
		tipString = "This API is not supported in current target.";
		break;
	default:
		break;
	}
	PRINTF("%s\r\n\r\n", tipString);
}

/*
 * @brief Gets called when an error occurs.
 */
void error_trap(void) {
	PRINTF("\r\n\r\n\r\n\t---- HALTED DUE TO FLASH ERROR! ----");
	while (1) {
	}
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////