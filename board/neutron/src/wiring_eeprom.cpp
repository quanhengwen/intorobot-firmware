/**
 ******************************************************************************
 * @file     : wiring_eerprom.cpp
 * @author   : robin
 * @version	 : V1.0.0
 * @date     : 6-December-2014
 * @brief    :
 ******************************************************************************
 Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "wiring_eeprom.h"

uint32_t SECTORError = 0;
/*
   TypeErase
   Banks
   Sector
   NbSectors
   VoltageRange
   */
static FLASH_EraseInitTypeDef EraseInit={FLASH_TYPEERASE_SECTORS,0,ID_ErasePAGE0,1,FLASH_VOLTAGE_RANGE_3};



/* Global variable used to store variable value in read sequence */
uint16_t EepromDataVar = 0;

/* Virtual address defined by the user: 0xFFFF value is prohibited */
static uint16_t EepromAddressTab[EEPROM_SIZE];

static FLASH_Status EEPROM_Format(void);
static uint16_t EEPROM_FindValidPage(uint8_t Operation);
static uint16_t EEPROM_VerifyPageFullWriteVariable(uint16_t EepromAddress, uint16_t EepromData);
static uint16_t EEPROM_PageTransfer(uint16_t EepromAddress, uint16_t EepromData);


/*********************************************************************************
 *Function		: uint16_t EEPROM_Init(void)
 *Description	: Restore the pages to a known good state in case of page's status corruption after a power loss.
 *Input		: none
 *Output		: none
 *Return		: none
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
uint16_t EEPROM_Init(void)
{
    uint16_t PageStatus0 = 6, PageStatus1 = 6;
    uint16_t EepromAddressIdx = 0;
    uint16_t EepromStatus = 0, ReadStatus = 0;
    int16_t x = -1;
    uint16_t  FlashStatus;

    /* Unlock the Flash Program Erase controller */
    HAL_FLASH_Unlock();
    //FLASH_Unlock();


    /* Get Page0 status */
    PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
    /* Get Page1 status */
    PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

    /* Check for invalid header states and repair if necessary */
    switch (PageStatus0)
    {
        case ERASED:
            if (PageStatus1 == VALID_PAGE) /* Page0 erased, Page1 valid */
            {
                /* Erase Page0 */
                EraseInit.Sector=ID_ErasePAGE0;
                EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;

                HAL_FLASHEx_Erase(&EraseInit,&SECTORError);
                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus = FLASH_EraseSector(ID_ErasePAGE0,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif
            }
            else if (PageStatus1 == RECEIVE_DATA) /* Page0 erased, Page1 receive */
            {
                /* Erase Page0 */
                EraseInit.Sector=ID_ErasePAGE0;
                EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
                HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus = FLASH_EraseSector(ID_ErasePAGE0,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

                /* Mark Page1 as valid */

                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,PAGE1_BASE_ADDRESS,VALID_PAGE);
                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif
            }
            else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            break;

        case RECEIVE_DATA:
            if (PageStatus1 == VALID_PAGE) /* Page0 receive, Page1 valid */
            {
                /* Transfer data from Page1 to Page0 */
                for (EepromAddressIdx = 0; EepromAddressIdx < EEPROM_SIZE; EepromAddressIdx++)
                {
                    if (( *(__IO uint16_t*)(PAGE0_BASE_ADDRESS + 6)) == EepromAddressTab[EepromAddressIdx])
                    {
                        x = EepromAddressIdx;
                    }
                    if (EepromAddressIdx != x)
                    {
                        /* Read the last variables' updates */
                        ReadStatus = EEPROM_ReadVariable(EepromAddressTab[EepromAddressIdx], &EepromDataVar);
                        /* In case variable corresponding to the virtual address was found */
                        if (ReadStatus != 0x1)
                        {
                            /* Transfer the variable to the Page0 */
                            EepromStatus = EEPROM_VerifyPageFullWriteVariable(EepromAddressTab[EepromAddressIdx], EepromDataVar);
                            /* If program operation was failed, a Flash error code is returned */
                            if (EepromStatus != FLASH_COMPLETE)
                            {
                                return EepromStatus;
                            }
                        }
                    }
                }
                /* Mark Page0 as valid */
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS,VALID_PAGE);
                FlashStatus=FLASH_COMPLETE;

#if 0
                FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

                FLASH_Erase_Sector(ID_ErasePAGE1,FLASH_VOLTAGE_RANGE_3);
                FlashStatus=FLASH_COMPLETE;
#if 0
                /* Erase Page1 */
                FlashStatus = FLASH_EraseSector(ID_ErasePAGE1,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

            }
            else if (PageStatus1 == ERASED) /* Page0 receive, Page1 erased */
            {
                /* Erase Page1 */
                EraseInit.Sector=ID_ErasePAGE1;
                EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
                HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus =FLASH_EraseSector(ID_ErasePAGE1,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

                /* Mark Page0 as valid */
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS,VALID_PAGE);
                FlashStatus=FLASH_COMPLETE;

#if 0
                FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

            }
            else /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            break;
        case VALID_PAGE:
            if (PageStatus1 == VALID_PAGE) /* Invalid state -> format eeprom */
            {
                /* Erase both Page0 and Page1 and set Page0 as valid page */
                FlashStatus = EEPROM_Format();
                /* If erase/program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
            }
            else if (PageStatus1 == ERASED) /* Page0 valid, Page1 erased */
            {
                /* Erase Page1 */
                EraseInit.Sector=ID_ErasePAGE1;
                EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
                HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus =FLASH_EraseSector(ID_ErasePAGE1,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif
            }
            else /* Page0 valid, Page1 receive */
            {
                /* Transfer data from Page0 to Page1 */
                for (EepromAddressIdx = 0; EepromAddressIdx < EEPROM_SIZE; EepromAddressIdx++)
                {
                    if ((*(__IO uint16_t*)(PAGE1_BASE_ADDRESS + 6)) == EepromAddressTab[EepromAddressIdx])
                    {
                        x = EepromAddressIdx;
                    }
                    if (EepromAddressIdx != x)
                    {
                        /* Read the last variables' updates */
                        ReadStatus = EEPROM_ReadVariable(EepromAddressTab[EepromAddressIdx], &EepromDataVar);
                        /* In case variable corresponding to the virtual address was found */
                        if (ReadStatus != 0x1)
                        {
                            /* Transfer the variable to the Page1 */
                            EepromStatus = EEPROM_VerifyPageFullWriteVariable(EepromAddressTab[EepromAddressIdx], EepromDataVar);
                            /* If program operation was failed, a Flash error code is returned */
                            if (EepromStatus != FLASH_COMPLETE)
                            {
                                return EepromStatus;
                            }
                        }
                    }
                }


                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,PAGE1_BASE_ADDRESS,VALID_PAGE);
                FlashStatus=FLASH_COMPLETE;


#if 0
                /* Mark Page1 as valid */
                FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
                /* If program operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif

                /* Erase Page0 */
                EraseInit.Sector=ID_ErasePAGE0;
                EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
                HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

                FlashStatus=FLASH_COMPLETE;
#if 0
                FlashStatus =FLASH_EraseSector(ID_ErasePAGE0,VoltageRange_3);
                /* If erase operation was failed, a Flash error code is returned */
                if (FlashStatus != FLASH_COMPLETE)
                {
                    return FlashStatus;
                }
#endif
            }
            break;
        default:  /* Any other state -> format eeprom */
            /* Erase both Page0 and Page1 and set Page0 as valid page */
            FlashStatus = EEPROM_Format();
            /* If erase/program operation was failed, a Flash error code is returned */
            if (FlashStatus != FLASH_COMPLETE)
            {
                return FlashStatus;
            }
            break;
    }
    return FLASH_COMPLETE;
}

/*********************************************************************************
 *Function		: uint16_t EEPROM_ReadVariable(uint16_t EepromAddress, uint16_t *EepromData)
 *Description	: Returns the last stored variable data, if found, which correspond to the passed virtual address
 *Input		: EepromAddress: Variable virtual address
EepromData: Global variable contains the read variable value
 *Output		: none
 *Return		:  Success or error status:
 - 0: if variable was found
 - 1: if the variable was not found
 - NO_VALID_PAGE: if no valid page was found.
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
uint16_t EEPROM_ReadVariable(uint16_t EepromAddress, uint16_t *EepromData)
{
    uint16_t ValidPage = PAGE0;
    uint16_t AddressValue = 0, ReadStatus = 1;
    uint32_t Address = PAGE0_BASE_ADDRESS, PageStartAddress = PAGE0_BASE_ADDRESS;

    /* Get active Page for read operation */
    ValidPage = EEPROM_FindValidPage(READ_FROM_VALID_PAGE);

    /* Check if there is no valid page */
    if (ValidPage == NO_VALID_PAGE)
    {
        return  NO_VALID_PAGE;
    }

    /* Get the valid Page start Address */
    PageStartAddress = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

    /* Get the valid Page end Address */
    Address = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

    /* Check each active page address starting from end */
    while (Address > (PageStartAddress + 2))
    {
        /* Get the current location content to be compared with virtual address */
        AddressValue = (*(__IO uint16_t*)Address);
        /* Compare the read address with the virtual address */
        if (AddressValue == EepromAddress)
        {
            /* Get content of Address-2 which is variable value */
            *EepromData = (*(__IO uint16_t*)(Address - 2));
            /* In case variable value is read, reset ReadStatus flag */
            ReadStatus = 0;
            break;
        }
        else
        {
            /* Next address location */
            Address = Address - 4;
        }
    }
    /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
    return ReadStatus;
}

/*********************************************************************************
 *Function		: uint16_t EEPROM_WriteVariable(uint16_t EepromAddress, uint16_t EepromData)
 *Description	: Writes/upadtes variable data in EEPROM.
 *Input		: EepromAddress: Variable virtual address
EepromData: 16 bit data to be written
 *Output		: none
 *Return		: Success or error status:
 *                 - FLASH_COMPLETE: on success
 *                 - PAGE_FULL: if valid page is full
 *                 - NO_VALID_PAGE: if no valid page was found
 *                 - Flash error code: on write Flash error
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
uint16_t EEPROM_WriteVariable(uint16_t EepromAddress, uint16_t EepromData)
{
    uint16_t Status = 0;

    /* Unlock the Flash Program Erase controller */
    //FLASH_Unlock();
    HAL_FLASH_Unlock();

    /* Write the variable virtual address and value in the EEPROM */
    Status = EEPROM_VerifyPageFullWriteVariable(EepromAddress, EepromData);

    /* In case the EEPROM active page is full */
    if (Status == PAGE_FULL)
    {
        /* Perform Page transfer */
        Status = EEPROM_PageTransfer(EepromAddress, EepromData);
    }

    /* Return last operation status */
    return Status;
}

/*********************************************************************************
 *Function		: static FLASH_Status EEPROM_Format(void)
 *Description	: Erases PAGE0 and PAGE1 and writes VALID_PAGE header to PAGE0
 *Input		: none
 *Output		: none
 *Return		: Status of the last operation (Flash write or erase) done during
 *                 EEPROM formating
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
static FLASH_Status EEPROM_Format(void)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;

    /* Erase Page0 */
    EraseInit.Sector=ID_ErasePAGE0;
    EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

    FlashStatus=FLASH_COMPLETE;
#if 0
    FlashStatus = FLASH_EraseSector(ID_ErasePAGE0,VoltageRange_3);

    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }
#endif



    /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,PAGE0_BASE_ADDRESS,VALID_PAGE);
    FlashStatus=FLASH_COMPLETE;

#if 0
    FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);

    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }
#endif

    /* Erase Page1 */

    EraseInit.Sector=ID_ErasePAGE1;
    EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

    FlashStatus=FLASH_COMPLETE;
#if 0
    FlashStatus =FLASH_EraseSector(ID_ErasePAGE1,VoltageRange_3);
#endif

    /* Return Page1 erase operation status */
    return FlashStatus;

}


/*********************************************************************************
 *Function		: static uint16_t EEPROM_FindValidPage(uint8_t Operation)
 *Description	: Find valid Page for write or read operation
 *Input		: Operation: operation to achieve on the valid page.
 *Output		: none
 *Return		: This parameter can be one of the following values:
 *               @arg READ_FROM_VALID_PAGE: read operation from valid page
 *               @arg WRITE_IN_VALID_PAGE: write operation from valid page
 *               @retval Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
 *                 of no valid page was found
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
static uint16_t EEPROM_FindValidPage(uint8_t Operation)
{
    uint16_t PageStatus0 = 6, PageStatus1 = 6;

    /* Get Page0 actual status */
    PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);

    /* Get Page1 actual status */
    PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

    /* Write or read operation */
    switch (Operation)
    {
        case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
            if (PageStatus1 == VALID_PAGE)
            {
                /* Page0 receiving data */
                if (PageStatus0 == RECEIVE_DATA)
                {
                    return PAGE0;         /* Page0 valid */
                }
                else
                {
                    return PAGE1;         /* Page1 valid */
                }
            }
            else if (PageStatus0 == VALID_PAGE)
            {
                /* Page1 receiving data */
                if (PageStatus1 == RECEIVE_DATA)
                {
                    return PAGE1;         /* Page1 valid */
                }
                else
                {
                    return PAGE0;         /* Page0 valid */
                }
            }
            else
            {
                return NO_VALID_PAGE;   /* No valid Page */
            }

        case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
            if (PageStatus0 == VALID_PAGE)
            {
                return PAGE0;           /* Page0 valid */
            }
            else if (PageStatus1 == VALID_PAGE)
            {
                return PAGE1;           /* Page1 valid */
            }
            else
            {
                return NO_VALID_PAGE ;  /* No valid Page */
            }

        default:
            return PAGE0;             /* Page0 valid */
    }
}


/*********************************************************************************
 *Function		: static uint16_t EEPROM_VerifyPageFullWriteVariable(uint16_t EepromAddress, uint16_t EepromData)
 *Description	: Verify if active page is full and Writes variable in EEPROM.
 *Input		: EepromAddress: 16 bit virtual address of the variable
EepromData: 16 bit data to be written as variable value
 *Output		: none
 *Return		:   * @retval Success or error status:
 *                 - FLASH_COMPLETE: on success
 *                 - PAGE_FULL: if valid page is full
 *                 - NO_VALID_PAGE: if no valid page was found
 *                 - Flash error code: on write Flash error
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
static uint16_t EEPROM_VerifyPageFullWriteVariable(uint16_t EepromAddress, uint16_t EepromData)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;
    uint16_t ValidPage = PAGE0;
    uint32_t Address = PAGE0_BASE_ADDRESS, PageEndAddress = PAGE1_END_ADDRESS;

    /* Get valid Page for write operation */
    ValidPage = EEPROM_FindValidPage(WRITE_IN_VALID_PAGE);

    /* Check if there is no valid page */
    if (ValidPage == NO_VALID_PAGE)
    {
        return  NO_VALID_PAGE;
    }

    /* Get the valid Page start Address */
    Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

    /* Get the valid Page end Address */
    PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

    /* Check each active page address starting from begining */
    while (Address < PageEndAddress)
    {
        /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
        if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
        {
            /* Set variable data */
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,Address,EepromData);
            FlashStatus=FLASH_COMPLETE;


#if 0
            FlashStatus = FLASH_ProgramHalfWord(Address, EepromData);
            /* If program operation was failed, a Flash error code is returned */
            if (FlashStatus != FLASH_COMPLETE)
            {
                return FlashStatus;
            }
#endif
            /* Set variable virtual address */

            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,Address + 2,EepromAddress);
            FlashStatus=FLASH_COMPLETE;



            // FlashStatus = FLASH_ProgramHalfWord(Address + 2, EepromAddress);



            /* Return program operation status */
            return FlashStatus;
        }
        else
        {
            /* Next address location */
            Address = Address + 4;
        }
    }
    /* Return PAGE_FULL in case the valid page is full */
    return PAGE_FULL;
}

/*********************************************************************************
 *Function		: static uint16_t EEPROM_PageTransfer(uint16_t EepromAddress, uint16_t EepromData)
 *Description	: Transfers last updated variables data from the full Page to an empty one.
 *Input		: EepromAddress: 16 bit virtual address of the variable
EepromData: 16 bit data to be written as variable value
 *Output		: none
 *Return		:   Success or error status:
 *                 - FLASH_COMPLETE: on success
 *                 - PAGE_FULL: if valid page is full
 *                 - NO_VALID_PAGE: if no valid page was found
 *                 - Flash error code: on write Flash error
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
static uint16_t EEPROM_PageTransfer(uint16_t EepromAddress, uint16_t EepromData)
{
    FLASH_Status FlashStatus = FLASH_COMPLETE;
    uint32_t OldPageAddress = PAGE0_BASE_ADDRESS, NewPageAddress = PAGE1_BASE_ADDRESS;
    uint16_t ValidPage = PAGE0, EepromAddressIdx = 0;
    uint16_t EepromStatus = 0, ReadStatus = 0;

    /* Get active Page for read operation */
    ValidPage = EEPROM_FindValidPage(READ_FROM_VALID_PAGE);

    if (ValidPage == PAGE1)       /* Page1 valid */
    {
        /* New page address where variable will be moved to */
        NewPageAddress = PAGE0_BASE_ADDRESS;

        /* Old page address where variable will be taken from */
        OldPageAddress = PAGE1_BASE_ADDRESS;
    }
    else if (ValidPage == PAGE0)  /* Page0 valid */
    {
        /* New page address where variable will be moved to */
        NewPageAddress = PAGE1_BASE_ADDRESS;

        /* Old page address where variable will be taken from */
        OldPageAddress = PAGE0_BASE_ADDRESS;
    }
    else
    {
        return NO_VALID_PAGE;       /* No valid Page */
    }

    /* Set the new Page status to RECEIVE_DATA status */

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,NewPageAddress,RECEIVE_DATA);
    FlashStatus=FLASH_COMPLETE;

#if 0
    FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }
#endif

    /* Write the variable passed as parameter in the new active page */
    EepromStatus = EEPROM_VerifyPageFullWriteVariable(EepromAddress, EepromData);
    /* If program operation was failed, a Flash error code is returned */
    if (EepromStatus != FLASH_COMPLETE)
    {
        return EepromStatus;
    }

    /* Transfer process: transfer variables from old to the new active page */
    for (EepromAddressIdx = 0; EepromAddressIdx < EEPROM_SIZE; EepromAddressIdx++)
    {
        if (EepromAddressTab[EepromAddressIdx] != EepromAddress)  /* Check each variable except the one passed as parameter */
        {
            /* Read the other last variable updates */
            ReadStatus = EEPROM_ReadVariable(EepromAddressTab[EepromAddressIdx], &EepromDataVar);
            /* In case variable corresponding to the virtual address was found */
            if (ReadStatus != 0x1)
            {
                /* Transfer the variable to the new active page */
                EepromStatus = EEPROM_VerifyPageFullWriteVariable(EepromAddressTab[EepromAddressIdx], EepromDataVar);
                /* If program operation was failed, a Flash error code is returned */
                if (EepromStatus != FLASH_COMPLETE)
                {
                    return EepromStatus;
                }
            }
        }
    }

    /* Erase the old Page: Set old Page status to ERASED status */

    //aad
    uint16_t temp_erase_sector;
    if(OldPageAddress==PAGE0_BASE_ADDRESS)
    {
        temp_erase_sector=ID_ErasePAGE0;
    }
    else if(OldPageAddress==PAGE1_BASE_ADDRESS)
    {
        temp_erase_sector=ID_ErasePAGE1;
    }
    else
    {
        return NO_VALID_PAGE;       /* No valid Page */
    }

    EraseInit.Sector=temp_erase_sector;
    EraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&EraseInit,&SECTORError);

    FlashStatus=FLASH_COMPLETE;
#if 0
    FlashStatus = FLASH_EraseSector(temp_erase_sector,VoltageRange_3);
    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }
#endif

    /* Set new Page status to VALID_PAGE status */
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,NewPageAddress,VALID_PAGE);
    FlashStatus=FLASH_COMPLETE;

#if 0
    FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
    /* If program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
        return FlashStatus;
    }
#endif

    /* Return last operation flash status */
    return FlashStatus;
}

/*********************************************************************************
 *Function		: EEPROMClass::EEPROMClass()
 *Description	: Arduino Compatibility EEPROM methods
 *Input		: none
 *Output		: none
 *Return		: none
 *author		: lz
 *date			: 2015-2-1
 *Others		: none
 **********************************************************************************/
EEPROMClass::EEPROMClass()
{
    EEPROM_Init();
	for (uint16_t i = 0 ; i < EEPROM_SIZE ; i++)
	{
		EepromAddressTab[i] = i;
	}
}

uint8_t EEPROMClass::read(int address)
{
    uint16_t data;

    if ((address < EEPROM_SIZE) && (EEPROM_ReadVariable(EepromAddressTab[address], &data) == 0))
    {
        return data;
    }
    return 0xFF;
}

/*

说明:
	用户使用0--Param_Eeprom_ADDR 地址

*/
void EEPROMClass::write(int address, uint8_t value)
{
    if (address < Param_Eeprom_ADDR)
    {
        EEPROM_WriteVariable(EepromAddressTab[address], value);
    }
}

/*

说明:
	用户使用Param_Eeprom_ADDR--EEPROM_SIZE 地址

*/

void EEPROMClass::write_system(int address, uint8_t value)
{
    if (address < EEPROM_SIZE)
    {
        EEPROM_WriteVariable(EepromAddressTab[address], value);
    }
}


EEPROMClass EEPROM;

