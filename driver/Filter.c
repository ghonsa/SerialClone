// Filter.c
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

static NTSTATUS IgnoreRequest(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS OnRepeaterComplete(PDEVICE_OBJECT tdo, PIRP subirp, PVOID needsvote);
static NTSTATUS RepeatRequest(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS SucceedRequest(PDEVICE_OBJECT pdo, PIRP Irp);

NTSTATUS FilterPnpDispatch(
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

    if(deviceExtension->TypeFlag != ISFILTER)
	{
		SerialCloneDebugPrint(DBG_PNP, DBG_ERR, __FUNCTION__" !! IRP:%p Not Filter Device", Irp);
		FailRequest(DeviceObject,Irp,STATUS_NO_SUCH_DEVICE);
	}

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

        // The device stack must be started from the bottom up.  
        // So, we send the IRP down the stack and wait so that 
        // all devices below ours have started before we process 
        // this IRP and start ourselves.
        status = SerialCloneSubmitIrpSync(deviceExtension->LowerDeviceObject, Irp);
        if (!NT_SUCCESS(status))
        {
            // Someone below us failed to start, so just complete with error
            break;
        }

        // Lower drivers have finished their start operation, so now
        // we process ours.


		// GCH* Start our clone device
		//SerialCloneCreateComName(deviceExtension);
		//IoSetDeviceInterfaceState(&deviceExtension->ntDeviceName,TRUE);

        // Update our PnP state
		deviceExtension->PnpState = PnpStateStarted;

        if (deviceExtension->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
        {
            DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
        }

        break;

    case IRP_MN_QUERY_STOP_DEVICE:

        // Update our PnP state
        deviceExtension->PreviousPnpState = deviceExtension->PnpState;
        deviceExtension->PnpState = PnpStateStopPending;

        // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
        // passing it down.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

        SerialCloneReleaseRemoveLock(deviceExtension);

        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;

        break;

   case IRP_MN_CANCEL_STOP_DEVICE:

        // Send this IRP down and wait for it to come back,

        // First check to see whether we received a query before this
        // cancel. This could happen if someone above us failed a query
        // and passed down the subsequent cancel.
        if (PnpStateStopPending == deviceExtension->PnpState) 
        {
            status = SerialCloneSubmitIrpSync(deviceExtension->LowerDeviceObject, Irp);
            if (NT_SUCCESS(status))
            {
                // restore previous pnp state
                deviceExtension->PnpState = deviceExtension->PreviousPnpState;
            } 
            else 
            {
                // Somebody below us failed the cancel
                // this is a fatal error.
                ASSERTMSG("Cancel stop failed!", FALSE);
            }
        } 
        else 
        {
            // Spurious cancel so we just forward the request.
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

            SerialCloneReleaseRemoveLock(deviceExtension);

            SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
            return status;
        }

        break;

    case IRP_MN_STOP_DEVICE:

        // Mark the device as stopped.
        deviceExtension->PnpState = PnpStateStopped;
		// GCH* Tell Clone to stop. 

        // send the request down, and we are done
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

        SerialCloneReleaseRemoveLock(deviceExtension);

        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;

    case IRP_MN_QUERY_REMOVE_DEVICE:

        // Update our PnP state
        deviceExtension->PreviousPnpState = deviceExtension->PnpState;
        deviceExtension->PnpState = PnpStateRemovePending;

        // Now just send the request down and we are done
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);

        status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

        SerialCloneReleaseRemoveLock(deviceExtension);

        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;

        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        // First check to see whether we have received a prior query
        // remove request. It could happen that we did not if
        // someone above us failed a query remove and passed down the
        // subsequent cancel remove request.
        if (PnpStateRemovePending == deviceExtension->PnpState)
        {
            status = SerialCloneSubmitIrpSync(deviceExtension->LowerDeviceObject, Irp);

            if (NT_SUCCESS(status))
            {
                // restore pnp state, since remove was canceled
                deviceExtension->PnpState = deviceExtension->PreviousPnpState;
            }
            else
            {
                // Nobody can fail this IRP. This is a fatal error.
                ASSERTMSG("Cancel remove failed. Fatal error!", FALSE);
                SerialCloneDebugPrint(DBG_PNP, DBG_ERR, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
            }
        }
        else
        {
            // Spurious cancel remove request so we just forward it
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

            SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

            SerialCloneReleaseRemoveLock(deviceExtension);

            SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

            return status;
        }

        break;

   case IRP_MN_SURPRISE_REMOVAL:

        // The device has been unexpectedly removed from the machine
        // and is no longer available for I/O.

        deviceExtension->PnpState = PnpStateSurpriseRemoved;

        // We must set Irp->IoStatus.Status to STATUS_SUCCESS before
        // passing it down.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        
        status = IoCallDriver (deviceExtension->LowerDeviceObject, Irp);

        // Adjust the active I/O count
        SerialCloneReleaseRemoveLock(deviceExtension);

		// GCH* TellCLone!
        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;

   case IRP_MN_REMOVE_DEVICE:

        // The Plug & Play system has dictated the removal of this device.  We
        // have no choice but to detach and delete the device object.

        // Update our PnP state
        deviceExtension->PnpState = PnpStateRemoved;

		// GCH* Tell clone to remove
        SerialCloneReleaseRemoveLock(deviceExtension);
        SerialCloneWaitForSafeRemove(deviceExtension);

		// Send the remove IRP down the stack.
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

	// Detach our device object from the device stack
		IoDetachDevice(deviceExtension->LowerDeviceObject);

		// attempt to delete our device object
		IoDeleteDevice(deviceExtension->FDeviceObject);

        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

        return status;

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:

        if ((DeviceObject->AttachedDevice == NULL) ||
            (DeviceObject->AttachedDevice->Flags & DO_POWER_PAGABLE))
        {
            DeviceObject->Flags |= DO_POWER_PAGABLE;
        }
        status = SerialCloneSubmitIrpSync(deviceExtension->LowerDeviceObject, Irp);
        if (!(deviceExtension->LowerDeviceObject->Flags & DO_POWER_PAGABLE))
        {
            DeviceObject->Flags &= ~DO_POWER_PAGABLE;
        }
        break;
	case IRP_MN_QUERY_ID:
	
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(deviceExtension->LowerDeviceObject, Irp);
		break;

	case IRP_MN_QUERY_CAPABILITIES:	
		{	
			PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
			PDEVICE_CAPABILITIES pdc = stack->Parameters.DeviceCapabilities.Capabilities;
			PSERIALCLONE_DEVICE_EXTENSION pdx = (PSERIALCLONE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
			// Check to be sure we know how to handle this version of the capabilities structure
			if (pdc->Version < 1)
			{
				PSERIALCLONE_DEVICE_EXTENSION pdx = (PSERIALCLONE_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
				IoSkipCurrentIrpStackLocation(Irp);
				return IoCallDriver(pdx->LowerDeviceObject, Irp);
			}

			status = ForwardAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(status))
			{						// IRP succeeded
				stack = IoGetCurrentIrpStackLocation(Irp);
				pdc = stack->Parameters.DeviceCapabilities.Capabilities;
				// GCH* Save capabilities for clone
				pdx->devcaps = *pdc;	// save capabilities for whoever needs to see them
			}						// IRP succeeded

			return CompleteRequest(Irp, status, Irp->IoStatus.Information);
		}							// HandleQueryCapabilities

	case IRP_MN_QUERY_DEVICE_RELATIONS:

		if(irpStack->Parameters.QueryDeviceRelations.Type == BusRelations)
		{
			PDEVICE_RELATIONS relations = 
				(PDEVICE_RELATIONS) ExAllocatePoolWithTag(PagedPool,sizeof(PDEVICE_RELATIONS)+
				   sizeof(PDEVICE_OBJECT),SERIALCLONE_POOL_TAG);
			if(relations != NULL)
			{
				relations->Count = 1;;
				relations->Objects[0] = deviceExtension->CDeviceObject;
				Irp->IoStatus.Information = (ULONG_PTR) relations;
				Irp->IoStatus.Status = STATUS_SUCCESS;
			}
		}
	
    default:
	
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver (deviceExtension->LowerDeviceObject, Irp);
		// Adjust our active I/O count
		SerialCloneReleaseRemoveLock(deviceExtension);
        SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
        return status;
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    // Adjust the active I/O count
    SerialCloneReleaseRemoveLock(deviceExtension);
    SerialCloneDebugPrint(DBG_PNP, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
    return status;
}

NTSTATUS FilterPowerDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    PSERIALCLONE_DEVICE_EXTENSION    deviceExtension;
    NTSTATUS                        status;

    SerialCloneDebugPrint(DBG_POWER, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);

    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if (!SerialCloneAcquireRemoveLock(deviceExtension))
    {
        status = STATUS_DELETE_PENDING;

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        SerialCloneDebugPrint(DBG_POWER, DBG_WARN, __FUNCTION__"--. Delete Pending IRP %p STATUS %x", Irp, status);
        return status;
    }
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    status = PoCallDriver(deviceExtension->LowerDeviceObject, Irp);
    SerialCloneReleaseRemoveLock(deviceExtension);
    SerialCloneDebugPrint(DBG_POWER, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
    return status;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//  FilterSystemControlDispatch
//      Dispatch routine to handle IRP_MJ_SYSTEM_CONTROL
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_SYSTEM_CONTROL IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS FilterSystemControlDispatch(
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

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);
    SerialCloneReleaseRemoveLock(deviceExtension);

    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  FilterCloseDispatch
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
NTSTATUS FilterCloseDispatch(
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
//  FilterCleanupDispatch
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
NTSTATUS FilterCleanupDispatch(
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
//  SerialFilterDispatch
//      Dispatch routine to handle IRP_MJ_READ
//
//  Arguments:
//      IN  DeviceObject
//              pointer to our device object
//
//      IN  Irp
//              pointer to the IRP_MJ_READ IRP
//
//  Return Value:
//      NT status code
//
NTSTATUS ReadComplete( IN  PDEVICE_OBJECT  DeviceObject,IN  PIRP  Irp,PSERIALCLONE_DEVICE_EXTENSION pdx)
{
	NTSTATUS status;
    PIO_STACK_LOCATION    irpStack;

	SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p ", Irp);
    // Get our current IRP stack location
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	if(Irp->PendingReturned)
		IoMarkIrpPending(Irp);
	//*GCH for now dump the data buffer
    if(pdx->FDeviceObject->Flags & DO_BUFFERED_IO)
	{
		ULONG bufsiz;
		char tbuff[200];
		size_t strnglen;
		char * tmp = Irp->AssociatedIrp.SystemBuffer;
		bufsiz = irpStack->Parameters.Read.Length;
		status = RtlStringCbLengthA(tmp,bufsiz,&strnglen);
		
		RtlCopyMemory(tbuff,tmp,197);
		tbuff[197]=0;
		tbuff[198]=0;

		SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, "Buffer:: %s",tbuff);



	}
    else if(pdx->FDeviceObject->Flags & DO_DIRECT_IO)
	{


	}

	SerialCloneReleaseRemoveLock(pdx);
	SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"++. IRP %p STATUS %x", Irp, STATUS_SUCCESS);
	return STATUS_SUCCESS;
}
NTSTATUS FilterReadDispatch(
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

	IoCopyCurrentIrpStackLocationToNext(Irp); 
    //IoSkipCurrentIrpStackLocation(Irp);
	IoSetCompletionRoutine(Irp,(PIO_COMPLETION_ROUTINE)ReadComplete,deviceExtension,TRUE,TRUE,TRUE);
	status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);
		
    
	//SerialCloneReleaseRemoveLock(deviceExtension);
    SerialCloneDebugPrint(DBG_GENERAL, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//  filterWriteDispatch
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
NTSTATUS FilterWriteDispatch(
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
//  FilterDeviceIoControlDispatch
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




NTSTATUS FilterDeviceIoControlDispatch(
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
//  FilterInternalDeviceIoControlDispatch
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
NTSTATUS FilterInternalDeviceIoControlDispatch(
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
