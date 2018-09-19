#include "stdafx.h"
#include "SCDevEnum.h"


typedef struct {
    LPCTSTR String;     // string looking for
    LPCTSTR Wild;       // first wild character if any
    BOOL    InstanceId;
}IdEntry ;

// PORTs class
DEFINE_GUID(GUID_CLASS_PORT, 0x4d36e978, 0x0e325, 0x11ce, 0x08, 0xe4, 0x08,
            0x00, 0x2b, 0x0e1, 0x03, 0x18);


//******************************************************************
//
//	GetDeviceStringProperty - Return a string property for a device
//
//	Input:	HDEVINFO Devs  - Handle to the Device Info
//			PSP_DEVINFO_DATA DevInfo - Device Info structurn
//			DWORD Prop - Id of properity to be retrieved
//
//	Return Value: string containing description
//******************************************************************
LPTSTR GetDeviceStringProperty(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop)
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize; 
    DWORD dataType;
    DWORD szChars;

    size = 1024; // initial guess
    buffer = new TCHAR[(size/sizeof(TCHAR))+1];
    if(!buffer) 
	{
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) 
	{
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		{
            goto failed;
        }
        if(dataType != REG_SZ) 
		{
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+1];
        if(!buffer) 
		{
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    return buffer;
failed:
    if(buffer) 
	{
        delete [] buffer;
    }
    return NULL;
}

//******************************************************************
//
//	GetDeviceDescription - Return a string description for a device
//
//	Input:	HDEVINFO Devs  - Handle to the Device Info
//			PSP_DEVINFO_DATA DevInfo - Device Info structurn
//
//	Return Value: string containing description
//******************************************************************

LPTSTR GetDeviceDescription(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo)
{
    LPTSTR desc;
    desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_FRIENDLYNAME);
    if(!desc) 
	{
        desc = GetDeviceStringProperty(Devs,DevInfo,SPDRP_DEVICEDESC);
    }
    return desc;
}

//******************************************************************
//
//	GetIdType - Determine if this is instance id or hardware id and 
//				if there's any wildcards instance ID is prefixed by 
//				'@'wildcards are '*'
//
//	Input:	ID string
//
//	Return Value: IdEntry
//******************************************************************

IdEntry GetIdType(LPCTSTR Id)
{
    IdEntry Entry;

    Entry.InstanceId = FALSE;
    Entry.Wild = NULL;
    Entry.String = Id;

    if(Entry.String[0] == INSTANCEID_PREFIX_CHAR) 
	{
        Entry.InstanceId = TRUE;
        Entry.String = CharNext(Entry.String);
    }
    if(Entry.String[0] == QUOTE_PREFIX_CHAR) 
	{
        //
        // prefix to treat rest of string literally
        //
        Entry.String = CharNext(Entry.String);
    } 
	else 
	{
        //
        // see if any wild characters exist
        //
        Entry.Wild = _tcschr(Entry.String,WILD_CHAR);
    }
    return Entry;
}

//******************************************************************
//
//	GetMultiSzIndexArray - Creates a string aray from a MultiSz 
//
//	Input:	Multi String
//
//	Return Value: array of strings
//******************************************************************

LPTSTR * GetMultiSzIndexArray(LPTSTR MultiSz)

{
    LPTSTR scan;
    LPTSTR * array;
    int elements;

    for(scan = MultiSz, elements = 0; scan[0] ;elements++) 
	{
        scan += lstrlen(scan)+1;
    }
    array = new LPTSTR[elements+2];
    if(!array) 
	{
        return NULL;
    }
    array[0] = MultiSz;
    array++;
    if(elements) 
	{
        for(scan = MultiSz, elements = 0; scan[0]; elements++) 
		{
            array[elements] = scan;
            scan += lstrlen(scan)+1;
        }
    }
    array[elements] = NULL;
    return array;
}

//******************************************************************
//
//	DelMultiSz -     Deletes the string array allocated by 
//					GetDevMultiSz/GetRegMultiSz/GetMultiSzIndexArray
//
//	Input:	string array
//
//	Return Value: void
//******************************************************************
void DelMultiSz(LPTSTR * Array)
{
    if(Array) 
	{
        Array--;
        if(Array[0]) 
		{
            delete [] Array[0];
        }
        delete [] Array;
    }
}

//******************************************************************
//
//	GetDevMultiSz -     Get a multi-sz device property
//					    and return as an array of strings
//	Input:	HDEVINFO Devs  - Handle to the Device Info
//			PSP_DEVINFO_DATA DevInfo - Device Info structurn
//			DWORD Prop - Id of properity to be retrieved
//
//	Return Value: String array
//******************************************************************
LPTSTR * GetDevMultiSz(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Prop)

{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer) 
	{
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(Devs,DevInfo,Prop,&dataType,(LPBYTE)buffer,size,&reqSize)) 
	{
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		{
            goto failed;
        }
        if(dataType != REG_MULTI_SZ) 
		{
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer) 
		{
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');
    array = GetMultiSzIndexArray(buffer);
    if(array) 
	{
        return array;
    }

failed:
    if(buffer) 
	{
        delete [] buffer;
    }
    return NULL;
}

//******************************************************************
//
//	GetRegMultiSz -     Get a multi-sz from registry
//					    and return as an array of strings
//
//	Input:	HKEY registry key
//			Property id string
//
//	Return Value: string array
//******************************************************************
LPTSTR * GetRegMultiSz(HKEY hKey,LPCTSTR Val)
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR * array;
    DWORD szChars;
    LONG regErr;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size/sizeof(TCHAR))+2];
    if(!buffer) 
	{
        return NULL;
    }
    reqSize = size;
    while((regErr = RegQueryValueEx(hKey,Val,NULL,&dataType,(PBYTE)buffer,&reqSize) != NO_ERROR)) 
	{
        if(GetLastError() != ERROR_MORE_DATA) 
		{
            goto failed;
        }
        if(dataType != REG_MULTI_SZ) 
		{
            goto failed;
        }
        size = reqSize;
        delete [] buffer;
        buffer = new TCHAR[(size/sizeof(TCHAR))+2];
        if(!buffer) 
		{
            goto failed;
        }
    }
    szChars = reqSize/sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars+1] = TEXT('\0');

    array = GetMultiSzIndexArray(buffer);
    if(array) 
	{
        return array;
    }

failed:
    if(buffer) 
	{
        delete [] buffer;
    }
    return NULL;
}

//******************************************************************
//
//	WildCardMatch -    Compare a single item against wildcard
//					I'm sure there's better ways of implementing this
//					Other than a command-line management tools
//					it's a bad idea to use wildcards as it implies
//					assumptions about the hardware/instance ID
//					eg, it might be tempting to enumerate root\* to
//					find all root devices, however there is a CfgMgr
//					API to query status and determine if a device is
//					root enumerated, which doesn't rely on implementation
//					details.
//
//	Input:	    Item - item to find match for eg a\abcd\c
//              MatchEntry - eg *\*bc*\*
//
//Return Value:
//
//    TRUE if any match, otherwise FALSE
//******************************************************************
BOOL WildCardMatch(LPCTSTR Item,const IdEntry & MatchEntry)

{
    LPCTSTR scanItem;
    LPCTSTR wildMark;
    LPCTSTR nextWild;
    size_t matchlen;

    //
    // before attempting anything else
    // try and compare everything up to first wild
    //
    if(!MatchEntry.Wild) 
	{
        return _tcsicmp(Item,MatchEntry.String) ? FALSE : TRUE;
    }
    if(_tcsnicmp(Item,MatchEntry.String,MatchEntry.Wild-MatchEntry.String) != 0) 
	{
        return FALSE;
    }
    wildMark = MatchEntry.Wild;
    scanItem = Item + (MatchEntry.Wild-MatchEntry.String);

    for(;wildMark[0];) 
	{
        //
        // if we get here, we're either at or past a wildcard
        //
        if(wildMark[0] == WILD_CHAR) 
		{
            //
            // so skip wild chars
            //
            wildMark = CharNext(wildMark);
            continue;
        }
        //
        // find next wild-card
        //
        nextWild = _tcschr(wildMark,WILD_CHAR);
        if(nextWild) 
		{
            //
            // substring
            //
            matchlen = nextWild-wildMark;
        } else 
		{
            //
            // last portion of match
            //
            size_t scanlen = lstrlen(scanItem);
            matchlen = lstrlen(wildMark);
            if(scanlen < matchlen) {
                return FALSE;
            }
            return _tcsicmp(scanItem+scanlen-matchlen,wildMark) ? FALSE : TRUE;
        }
        if(_istalpha(wildMark[0])) 
		{
            //
            // scan for either lower or uppercase version of first character
            //
            TCHAR u = _totupper(wildMark[0]);
            TCHAR l = _totlower(wildMark[0]);
            while(scanItem[0] && scanItem[0]!=u && scanItem[0]!=l) 
			{
                scanItem = CharNext(scanItem);
            }
            if(!scanItem[0]) 
			{
                //
                // ran out of string
                //
                return FALSE;
            }
        } 
		else 
		{
            //
            // scan for first character (no case)
            //
            scanItem = _tcschr(scanItem,wildMark[0]);
            if(!scanItem) 
			{
                //
                // ran out of string
                //
                return FALSE;
            }
        }
        //
        // try and match the sub-string at wildMark against scanItem
        //
        if(_tcsnicmp(scanItem,wildMark,matchlen)!=0) 
		{
            //
            // nope, try again
            //
            scanItem = CharNext(scanItem);
            continue;
        }
        //
        // substring matched
        //
        scanItem += matchlen;
        wildMark += matchlen;
    }
    return (wildMark[0] ? FALSE : TRUE);
}

//******************************************************************
//
//	WildCompareHwIds -     Compares all strings in Array against Id
//						    Use WildCardMatch to do real compare
//
//
//	Input:	    Array - pointer returned by GetDevMultiSz
//			    MatchEntry - string to compare against
//
//	Return Value:     TRUE if any match, otherwise FALSE
//******************************************************************
BOOL WildCompareHwIds(LPTSTR * Array,const IdEntry & MatchEntry)
{
    if(Array) {
        while(Array[0]) {
            if(WildCardMatch(Array[0],MatchEntry)) {
                return TRUE;
            }
            Array++;
        }
    }
    return FALSE;
}
//******************************************************************
//
//	FindDevices - Enumerates PNP devices looking for requested type
//
//	Input:	Flags for search
//			ID String to search for
//			Callback routine to handle results
//	Return Value: IdEntry
//******************************************************************

HDEVINFO FindDevices(DWORD Flags,LPTSTR DeviceName,CallbackFunc Callback,LPVOID Context)
{
    HDEVINFO devs = INVALID_HANDLE_VALUE;
    HDEVINFO rsltdevs = INVALID_HANDLE_VALUE;
    IdEntry * templ = NULL;
    DWORD err;
    int failcode = EXIT_FAIL;
    int argIndex;
    DWORD devIndex;
    SP_DEVINFO_DATA devInfo;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    BOOL doSearch = FALSE;
    BOOL match= FALSE;
    BOOL all = FALSE;
    GUID cls ; //= GUID_CLASS_PORT;
    DWORD numClass = 0;
    int skip = 0;

    templ = new IdEntry[1];

    if(!templ) 
	{
        goto final;
    }
	templ[0] = GetIdType(DeviceName);
    doSearch = TRUE;
    if(doSearch || all) 
	{
        //
        // add all id's to list
        // if there's a class, filter on specified class
        //
        devs = SetupDiGetClassDevsEx(NULL,
                                     NULL,
									 NULL,
                                     DIGCF_ALLCLASSES | Flags,
									 NULL,
                                     0,
                                     NULL);
       // devs = SetupDiGetClassDevsEx(NULL,
       //                              NULL,
		//							 NULL,
         //                            DIGCF_ALLCLASSES | Flags,
		//							 NULL,
         //                            NULL);

    } 
	else 
	{
        //
        // blank list, we'll add instance id's by hand
        //
        devs = SetupDiCreateDeviceInfoListEx( &cls ,
                                             NULL,
                                             0,
                                             NULL);
    }
    if(devs == INVALID_HANDLE_VALUE) 
	{
        goto final;
    }

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(devs,&devInfoListDetail)) 
	{
        goto final;
    }

    //
    // now enumerate them
    //
    if(all) 
	{
        doSearch = FALSE;
    }

    devInfo.cbSize = sizeof(devInfo);
    for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) 
	{
        LPTSTR *hwIds = NULL;
        LPTSTR *compatIds = NULL;
        if(doSearch) 
		{

			hwIds = GetDevMultiSz(devs,&devInfo,SPDRP_HARDWAREID);
            compatIds = GetDevMultiSz(devs,&devInfo,SPDRP_COMPATIBLEIDS);
			if(WildCompareHwIds(hwIds,templ[0]) ||
                        WildCompareHwIds(compatIds,templ[0])) 
			{
				

				match = TRUE;
            }
         }
         DelMultiSz(hwIds);
         DelMultiSz(compatIds);
        
        if(match) 
		{

			// now we want to add in our stuff 
			rsltdevs = devs;
			match = FALSE;
		}
    }

    failcode = EXIT_OK;

final:
    if(templ) {
        delete [] templ;
    }
    if(devs != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(devs);
    }
    return rsltdevs;

}




