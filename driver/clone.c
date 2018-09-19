// Clone.c
//
// The mainline filter device
// 
// File created on 9/22/2005
//
//;***********************************************************
//;  Copyright 2005 Greg Honsa
//;
//;***********************************************************

///////////////////////////////////////////////////////////////////////////////////////////////////
//  SerialClonePnpDispatch
//      Dispatch routine to handle PnP requests
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the PnP IRP
//
//  Return Value:
//      NT status code
//

#include "pch.h"
#ifdef SERIALCLONE_WMI_TRACE
#include "SerialClone.tmh"
#endif


NTSTATUS ClonePnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;
    PIO_STACK_LOCATION              irpStack;
    PDEVICE_CAPABILITIES            deviceCapabilities;

    SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);
    // Get our current IRP stack location
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    SerialCloneDumpIrp(Irp);
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if(deviceExtension->TypeFlag != ISCLONE)
	{
		SerialCloneDebugPrint(DBG_PNP, DBG_ERR, __FUNCTION__" !! IRP:%p Not Clone Device", Irp);
		FailRequest(DeviceObject,Irp,STATUS_NO_SUCH_DEVICE);
	}
	// if we cant get the lock bail on the irp
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
        return status;
    }
    switch (irpStack->MinorFunction) 
    {
		case IRP_MN_START_DEVICE:
			return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_QUERY_STOP_DEVICE:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_CANCEL_STOP_DEVICE:
			return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_STOP_DEVICE:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_QUERY_REMOVE_DEVICE:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_CANCEL_REMOVE_DEVICE:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_SURPRISE_REMOVAL:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_REMOVE_DEVICE:
	        return SucceedRequest(DeviceObject,Irp);

		case IRP_MN_DEVICE_USAGE_NOTIFICATION:
			return RepeatRequest(DeviceObject,Irp);
	
		case IRP_MN_QUERY_ID:
		{
			PWCHAR idstring;
			ULONG nchars; 
			ULONG size; 
			PWCHAR id;
			switch (irpStack->Parameters.QueryId.IdType)
			{						// select based on id type
				case BusQueryInstanceID:
					idstring = L"0000";
					break;
				// For the device ID, we need to supply an enumerator name plus a device identifer.
				// The enumerator name is something you should choose to be unique, which is why
				// I used the name of the driver in this instance.

				case BusQueryDeviceID:
					idstring = (deviceExtension->TypeFlag & ISCLONE) ? LDRIVERNAME L"\\*GCH4133" : LDRIVERNAME L"\\*GCH4134";
					break;

				case BusQueryHardwareIDs:
					idstring = (deviceExtension->TypeFlag & ISCLONE) ? L"*GCH4133" : L"*GCH4134";
					break;

				case BusQueryCompatibleIDs:

				default:
					Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
					Irp->IoStatus.Information = 0;
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
					return STATUS_SUCCESS;
			}						// select based on id type
			nchars = wcslen(idstring);
			size = (nchars + 2) * sizeof(WCHAR);
			id = (PWCHAR) ExAllocatePool(PagedPool, size);
			if (!id)
			{
				Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				Irp->IoStatus.Information = 0;
				break;
				//IoCompleteRequest(Irp, IO_NO_INCREMENT);
				//return STATUS_INSUFFICIENT_RESOURCES;
			}

			//wcscpy(id, idstring);
			RtlStringCbCopyW(id,size,idstring);
			id[nchars+1] = 0;			// extra null terminator

			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = (ULONG_PTR) id;
		}
		break;
	case IRP_MN_QUERY_CAPABILITIES:	
		{    // assume Clone device
			PDEVICE_OBJECT pdo;
			PSERIALCLONE_DEVICE_EXTENSION pdx;

			pdo = deviceExtension->	FDeviceObject;
			pdx = (PSERIALCLONE_DEVICE_EXTENSION) pdo->DeviceExtension;
			irpStack->Parameters.DeviceCapabilities.Capabilities = &pdx->devcaps;
			Irp->IoStatus.Status = STATUS_SUCCESS;

			break;
		}

	case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
		
		status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		// Adjust the active I/O count
		SerialCloneReleaseRemoveLock(deviceExtension);
		SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
		return STATUS_SUCCESS;

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		{
			PDEVICE_RELATIONS oldrel = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
			PDEVICE_RELATIONS newrel = NULL;
			NTSTATUS status = Irp->IoStatus.Status;

			if (irpStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
			{							// query for PDO address
				ASSERT(!oldrel);			// no-one has any business with this but us!
				newrel = (PDEVICE_RELATIONS) ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
				if (newrel)
				{
					newrel->Count = 1;
					newrel->Objects[0] = DeviceObject;
					ObReferenceObject(DeviceObject);
					status = STATUS_SUCCESS;
				}
				else
					status = STATUS_INSUFFICIENT_RESOURCES;
			}		// query for PDO address

			if (newrel)
			{							// install new relation list
				if (oldrel)
					ExFreePool(oldrel);
				Irp->IoStatus.Information = (ULONG_PTR) newrel;
			}				// install new relation list

			Irp->IoStatus.Status = status;
			break;
		}
		
    default:
        // Not suppored is whats left.
		Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    }

    status = Irp->IoStatus.Status ;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    // Adjust the active I/O count
    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

NTSTATUS ClonePowerDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_POWER, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if(deviceExtension->TypeFlag!=ISCLONE)
	{
		SerialCloneDebugPrint(DBG_PNP, DBG_ERR, __FUNCTION__" !! IRP:%p Not Clone Device", Irp);
		FailRequest(DeviceObject,Irp,STATUS_NO_SUCH_DEVICE);
	}
    
	if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        SerialCloneDebugPrint(DBG_POWER, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
        return status;
    }
        PoStartNextPowerIrp(Irp);

    SerialCloneReleaseRemoveLock(deviceExtension);
    SerialCloneDebugPrint(DBG_POWER, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, STATUS_SUCCESS);
    return SucceedRequest(DeviceObject,Irp);

}


NTSTATUS CloneSystemControlDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
        return status;
    }

    //IoSkipCurrentIrpStackLocation(Irp);
    //status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);
    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, STATUS_SUCCESS);

    return SucceedRequest(DeviceObject,Irp);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneCloseDispatch
//      Dispatch routine to handle IRP_MJ_CLOSE
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_CLOSE IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS CloneCloseDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);

    // Get our device extension from the device object
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneCleanupDispatch
//      Dispatch routine to handle IRP_MJ_CLEANUP
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_CLEANUP IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS CloneCleanupDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);

    // Get our device extension from the device object
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneWriteDispatch
//      Dispatch routine to handle IRP_MJ_WRITE
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_WRITE IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS CloneWriteDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);

    // Get our device extension from the device object
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneDeviceIoControlDispatch
//      Dispatch routine to handle IRP_MJ_DEVICE_CONTROL
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_DEVICE_CONTROL IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS CloneDeviceIoControlDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);

    // Get our device extension from the device object
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneInternalDeviceIoControlDispatch
//      Dispatch routine to handle IRP_MJ_INTERNAL_DEVICE_CONTROL
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_INTERNAL_DEVICE_CONTROL IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS CloneInternalDeviceIoControlDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);
    // Get our device extension from the device object
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Make sure we can accept IRPs
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        SerialCloneDebugPrint(DBG_GENERAL, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
        return status;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);
    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}
