// CloneIOctl.c
//
// The Clone device IO control
// 
// File created on 9/22/2005
//
//;***********************************************************
//;  Copyright 2005 Greg Honsa
//;
//;***********************************************************



#include "pch.h"
#ifdef SERIALCLONE_WMI_TRACE
#include "SerialClone.tmh"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneDeviceIoControlDispatch
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
NTSTATUS CloneDeviceIoControlDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
{
    NTSTATUS                        status;
    PSERIALCLONE_DEVICE_EXTENSION           deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    KIRQL               oldIrql;


    SerialCloneDebugPrint(DBG_IO, DBG_TRACE, __FUNCTION__"++. IRP %p", Irp);
    deviceExtension = (PSERIALCLONE_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    status = CloneSerialIoControl(deviceExtension, Irp);

    SerialCloneDebugPrint(DBG_IO, DBG_TRACE, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  CloneSerialIoControl
//      IOCTL_SERIAL_XXX handler
//
//  Arguments:
//      IN  DeviceExtension
//              our device extension
//
//      IN  Irp
//              IOCTL_SERIAL_XXX IRP
//
//  Return Value:
//      Status
//
NTSTATUS CloneSerialIoControl(
    IN  PSERIALCLONE_DEVICE_EXTENSION   DeviceExtension,
    IN  PIRP                    Irp
    )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpStack;
    KIRQL               oldIrql;

    // Get our IRP stack location
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Information = 0;

    // check the device error condition
    status = test1CheckForError(DeviceExtension, Irp);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) 
    {
    // The IOCTL_SERIAL_SET_BAUD_RATE request sets the baud rate on a COM port.
    case IOCTL_SERIAL_SET_BAUD_RATE:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_BAUD_RATE");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BAUD_RATE)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } 

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);
                return status;
            }

            DeviceExtension->BaudRate = ((PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer)->BaudRate;
            test1DecrementIoCount(&DeviceExtension->IoLock);

        }
        break;

    // The IOCTL_SERIAL_GET_BAUD_RATE request returns the baud rate that is currently set for a COM port.
    case IOCTL_SERIAL_GET_BAUD_RATE:
        {
            PSERIAL_BAUD_RATE br;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_BAUD_RATE");

            br = (PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_BAUD_RATE)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            br->BaudRate = DeviceExtension->BaudRate;
            Irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_GET_MODEM_CONTROL request returns the value of the modem control register.
    case IOCTL_SERIAL_GET_MODEM_CONTROL:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_MODEM_CONTROL");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                test1DebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = test1UartReadMCR(&DeviceExtension->Uart);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            Irp->IoStatus.Information = sizeof(ULONG);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_MODEM_CONTROL request sets the modem control register. Parameter checking is not done.
    case IOCTL_SERIAL_SET_MODEM_CONTROL:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_MODEM_CONTROL");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            test1UartWriteMCR(&DeviceExtension->Uart, *(PUCHAR)Irp->AssociatedIrp.SystemBuffer);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_INFO_CONTROL request sets the FIFO control register (FCR). 
    // Serial does not verify the specified FIFO control information.
    case IOCTL_SERIAL_SET_FIFO_CONTROL:
        SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_FIFO_CONTROL");
        break;

    // The IOCTL_SERIAL_SET_LINE_CONTROL request sets the line control register (LCR). 
    // The line control register controls the data size, the number of stop bits, and the parity. 
    case IOCTL_SERIAL_SET_LINE_CONTROL:
        {
            PSERIAL_LINE_CONTROL    lineControl;
            UCHAR                   data;
            UCHAR                   stop;
            UCHAR                   parity;
            UCHAR                   mask;

            SerialCloneDebugPrint(DBG_IO, DBG_TRACE, "IOCTL_SERIAL_SET_LINE_CONTROL");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            lineControl = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            mask = 0xff;

            switch (lineControl->WordLength) 
            {
            case 5: 
                data = TEST1_UART_LCR_5_DATA;
                mask = 0x1f;
                break;
            case 6: 
                data = TEST1_UART_LCR_6_DATA;
                mask = 0x3f;
                break;
            case 7: 
                data = TEST1_UART_LCR_7_DATA;
                mask = 0x7f;
                break;
            case 8: 
                data = TEST1_UART_LCR_8_DATA;
                break;
            default: 
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            DeviceExtension->WmiCommData.BitsPerByte = lineControl->WordLength;

            switch (lineControl->Parity) 
            {
            case NO_PARITY: 
                DeviceExtension->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
                parity = TEST1_UART_LCR_NONE_PARITY;
                break;
            case EVEN_PARITY: 
                DeviceExtension->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
                parity = TEST1_UART_LCR_EVEN_PARITY;
                break;
            case ODD_PARITY: 
                DeviceExtension->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
                parity = TEST1_UART_LCR_ODD_PARITY;
                break;
            case SPACE_PARITY: 
                DeviceExtension->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
                parity = TEST1_UART_LCR_SPACE_PARITY;
                break;
            case MARK_PARITY: 
                DeviceExtension->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
                parity = TEST1_UART_LCR_MARK_PARITY;
                break;
            default: 
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            switch (lineControl->StopBits) 
            {
            case STOP_BIT_1: 
                DeviceExtension->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
                stop = TEST1_UART_LCR_1_STOP;
                break;
            case STOP_BITS_1_5: 
                DeviceExtension->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
                stop = TEST1_UART_LCR_1_5_STOP;
                break;
            case STOP_BITS_2: 
                DeviceExtension->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
                stop = TEST1_UART_LCR_2_STOP;
                break;
            default: 
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            DeviceExtension->LineControl = (UCHAR)((DeviceExtension->LineControl & TEST1_UART_LCR_BREAK) | (data | parity | stop));
            DeviceExtension->DataMask = mask;

            test1UartWriteLCR(&DeviceExtension->Uart, DeviceExtension->LineControl);

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_GET_LINE_CONTROL request returns information about the line control set for a COM port. 
    // The line control parameters include the number of stop bits, the number of data bits, and the parity. 
    case IOCTL_SERIAL_GET_LINE_CONTROL:
        {
            PSERIAL_LINE_CONTROL lineControl;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_LINE_CONTROL");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_LINE_CONTROL)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            lineControl = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, irpStack->Parameters.DeviceIoControl.OutputBufferLength);

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if ((DeviceExtension->LineControl & TEST1_UART_LCR_DATA_MASK) == TEST1_UART_LCR_5_DATA) 
            {
                lineControl->WordLength = 5;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_DATA_MASK) == TEST1_UART_LCR_6_DATA) 
            {
                lineControl->WordLength = 6;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_DATA_MASK) == TEST1_UART_LCR_7_DATA) 
            {
                lineControl->WordLength = 7;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_DATA_MASK) == TEST1_UART_LCR_8_DATA) 
            {
                lineControl->WordLength = 8;
            }

            if ((DeviceExtension->LineControl & TEST1_UART_LCR_PARITY_MASK) == TEST1_UART_LCR_NONE_PARITY) 
            {
                lineControl->Parity = NO_PARITY;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_PARITY_MASK) == TEST1_UART_LCR_ODD_PARITY) 
            {
                lineControl->Parity = ODD_PARITY;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_PARITY_MASK) == TEST1_UART_LCR_EVEN_PARITY) 
            {
                lineControl->Parity = EVEN_PARITY;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_PARITY_MASK) == TEST1_UART_LCR_MARK_PARITY) 
            {
                lineControl->Parity = MARK_PARITY;
            } 
            else if ((DeviceExtension->LineControl & TEST1_UART_LCR_PARITY_MASK) == TEST1_UART_LCR_SPACE_PARITY) 
            {
                lineControl->Parity = SPACE_PARITY;
            }

            if (DeviceExtension->LineControl & TEST1_UART_LCR_2_STOP) 
            {
                if (lineControl->WordLength == 5) 
                {
                    lineControl->StopBits = STOP_BITS_1_5;
                } 
                else 
                {
                    lineControl->StopBits = STOP_BITS_2;
                }
            } 
            else 
            {
                lineControl->StopBits = STOP_BIT_1;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;

    // The IOCTL_SERIAL_SET_TIMEOUTS request sets the timeout values that the driver uses with read and write requests.
    case IOCTL_SERIAL_SET_TIMEOUTS:
        {
            PSERIAL_TIMEOUTS timeouts;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_TIMEOUTS");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_TIMEOUTS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            timeouts = (PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer;

            if ((timeouts->ReadIntervalTimeout == MAXULONG) &&
                (timeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
                (timeouts->ReadTotalTimeoutConstant == MAXULONG)) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            
            DeviceExtension->Timeouts.ReadIntervalTimeout = timeouts->ReadIntervalTimeout;
            DeviceExtension->Timeouts.ReadTotalTimeoutMultiplier = timeouts->ReadTotalTimeoutMultiplier;
            DeviceExtension->Timeouts.ReadTotalTimeoutConstant = timeouts->ReadTotalTimeoutConstant;
            DeviceExtension->Timeouts.WriteTotalTimeoutMultiplier = timeouts->WriteTotalTimeoutMultiplier;
            DeviceExtension->Timeouts.WriteTotalTimeoutConstant = timeouts->WriteTotalTimeoutConstant;

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;

    // The IOCTL_SERIAL_GET_TIMEOUTS request returns the timeout values that Serial uses with read and write requests.
    case IOCTL_SERIAL_GET_TIMEOUTS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_TIMEOUTS");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_TIMEOUTS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer) = DeviceExtension->Timeouts;
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
        }
        break;

    // The IOCTL_SERIAL_GET_CHARS request sets the special characters that Serial uses for handshake flow control. 
    // Serial verifies the specified special characters.
    case IOCTL_SERIAL_SET_CHARS:
        {
            PSERIAL_CHARS newChars;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_CHARS");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_CHARS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            newChars = (PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            
            if (DeviceExtension->EscapeChar) 
            {
                if ((DeviceExtension->EscapeChar == newChars->XonChar) || 
                    (DeviceExtension->EscapeChar == newChars->XoffChar)) 
                {
                    status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
                    break;
                }
            }

            DeviceExtension->SerialChars = *newChars;

            DeviceExtension->WmiCommData.XonCharacter = newChars->XonChar;
            DeviceExtension->WmiCommData.XoffCharacter = newChars->XoffChar;

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;

    // The IOCTL_SERIAL_GET_CHARS request returns the special characters that Serial uses with handshake flow control.
    case IOCTL_SERIAL_GET_CHARS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_CHARS");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_CHARS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *(PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer = DeviceExtension->SerialChars;
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            Irp->IoStatus.Information = sizeof(SERIAL_CHARS);
        }
        break;

    // The IOCTL_SERIAL_SET_DTR request sets DTR (data terminal ready). 
    case IOCTL_SERIAL_SET_DTR:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_DTR");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if ((DeviceExtension->SerialHandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) 
            {
                status = STATUS_INVALID_PARAMETER;
            } 
            else 
            {
                test1SerialSetDTR(DeviceExtension);
            }

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_CLR_DTR request clears the data terminal ready control signal (DTR). 
    case IOCTL_SERIAL_CLR_DTR:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_CLR_DTR");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if ((DeviceExtension->SerialHandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE) 
            {
                status = STATUS_INVALID_PARAMETER;
            } 
            else 
            {
                test1SerialClrDTR(DeviceExtension);
            }

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_RESET_DEVICE request resets a COM port.
    case IOCTL_SERIAL_RESET_DEVICE:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_RESET_DEVICE");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            //*****************************************************************
            //*****************************************************************
            // TODO: Reset the device
            //*****************************************************************
            //*****************************************************************

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_RTS request sets RTS (request to send). 
    case IOCTL_SERIAL_SET_RTS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_TRACE, "IOCTL_SERIAL_SET_RTS");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if (((DeviceExtension->SerialHandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) ||
                ((DeviceExtension->SerialHandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)) 
            {
                status = STATUS_INVALID_PARAMETER;
            } 
            else 
            {
                test1SerialSetRTS(DeviceExtension);
            }

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_CLR_RTS request clears the request to send control signal (RTS). 
    case IOCTL_SERIAL_CLR_RTS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_TRACE, "IOCTL_SERIAL_CLR_RTS");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if (((DeviceExtension->SerialHandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE) ||
                ((DeviceExtension->SerialHandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)) 
            {
                status = STATUS_INVALID_PARAMETER;
            } 
            else 
            {
                test1SerialClrRTS(DeviceExtension);
            }

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_XOFF request emulates the reception of an XOFF character. 
    // The request stops reception of data. If automatic XON/XOFF flow control is not set, then a client 
    // must use a subsequent IOCTL_SERIAL_SET_XON request to restart reception of data.
    case IOCTL_SERIAL_SET_XOFF:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_XOFF");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            test1SerialPretendXoff(DeviceExtension);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_XON request emulates the reception of a XON character, which restarts reception of data.
    case IOCTL_SERIAL_SET_XON:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_XON");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            test1SerialPretendXon(DeviceExtension);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_BREAK_ON request sets the line control break signal active.
    case IOCTL_SERIAL_SET_BREAK_ON:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_BREAK_ON");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            test1SerialTurnOnBreak(DeviceExtension);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_BREAK_OFF request sets the line control break signal inactive. 
    case IOCTL_SERIAL_SET_BREAK_OFF:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_BREAK_OFF");

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            test1SerialTurnOffBreak(DeviceExtension);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_SET_QUEUE_SIZE request sets the size of the internal receive buffer. 
    // If the requested size is greater than the current receive buffer size, a new receive buffer is created. 
    // Otherwise, the receive buffer is not changed. 
    case IOCTL_SERIAL_SET_QUEUE_SIZE:
        {
            PSERIAL_QUEUE_SIZE queueSize;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_QUEUE_SIZE");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_QUEUE_SIZE)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            queueSize = (PSERIAL_QUEUE_SIZE)Irp->AssociatedIrp.SystemBuffer;

            if (queueSize->InSize <= DeviceExtension->QueueSize)
            {
                status = STATUS_SUCCESS;
                break;
            }
            
            __try 
            {
                irpStack->Parameters.DeviceIoControl.Type3InputBuffer = ExAllocatePoolWithQuota(NonPagedPool, queueSize->InSize);
            } 
            __except(EXCEPTION_EXECUTE_HANDLER) 
            {
                irpStack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                status = GetExceptionCode();
            }

            if (irpStack->Parameters.DeviceIoControl.Type3InputBuffer == NULL) 
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            return test1QueueIrp(&DeviceExtension->ReadQueue, Irp);
        }
        break;

    // The IOCTL_SERIAL_GET_WAIT_MASK request returns the event wait mask that is currently set on a COM port. 
    case IOCTL_SERIAL_GET_WAIT_MASK:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_WAIT_MASK");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = DeviceExtension->WaitMask;
        }
        break;

    // The IOCTL_SERIAL_SET_WAIT_MASK request configures Serial to notify a client after 
    // the occurrence of any one of a specified set of wait events. 
    case IOCTL_SERIAL_SET_WAIT_MASK:
        {
            ULONG   mask;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_WAIT_MASK");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            } 

            mask = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
            if (mask & ~(SERIAL_EV_RXCHAR   |
                         SERIAL_EV_RXFLAG   |
                         SERIAL_EV_TXEMPTY  |
                         SERIAL_EV_CTS      |
                         SERIAL_EV_DSR      |
                         SERIAL_EV_RLSD     |
                         SERIAL_EV_BREAK    |
                         SERIAL_EV_ERR      |
                         SERIAL_EV_RING     |
                         SERIAL_EV_PERR     |
                         SERIAL_EV_RX80FULL |
                         SERIAL_EV_EVENT1   |
                         SERIAL_EV_EVENT2)) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            return test1QueueIrp(&DeviceExtension->MaskQueue, Irp);
        }
        break;

    // The IOCTL_SERIAL_WAIT_ON_MASK request is used to wait for the occurrence of any wait event specified 
    // by using an IOCTL_SERIAL_SET_WAIT_MASK request. A wait-on-mask request is completed after one of 
    // the following events occurs: 
    // 1) A wait event occurs that was specified by the most recent set-wait-mask request. 
    // 2) An IOCTL_SERIAL_SET_WAIT_MASK request is received while a wait-on-mask request is pending. 
    //    The driver completes the pending wait-on-mask request with a status of STATUS_SUCCESS and the output wait mask is set to zero. 
    case IOCTL_SERIAL_WAIT_ON_MASK:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_WAIT_ON_MASK");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            return test1QueueIrp(&DeviceExtension->MaskQueue, Irp);
        }
        break;

    // The IOCTL_SERIAL_IMMEDIATE_CHAR request causes a specified character to be transmitted as soon as possible.
    // The immediate character request completes immediately after any other write that might be in progress.
    // Only one immediate character request can be pending at a time
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_IMMEDIATE_CHAR");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            if (DeviceExtension->ImmediateCharIrp != NULL)
            {
                status = STATUS_INVALID_PARAMETER;
                KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
            }
            else
            {
                DeviceExtension->ImmediateCharIrp = Irp;
                ++DeviceExtension->PendingWriteCount;
                KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

                return test1SerialStartImmediate(DeviceExtension);
            }
        }
        break;

    // The IOCTL_SERIAL_PURGE request cancels the specified requests and deletes data from the specified buffers. 
    // The purge request can be used to cancel all read requests and write requests and to delete all data from the read buffer and the write buffer.
    // The completion of the purge request does not indicate that the requests canceled by the purge request are completed. 
    // A client must verify that the purged requests are completed before the client frees or reuses the corresponding IRPs.
    case IOCTL_SERIAL_PURGE:
        {
            ULONG mask;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_PURGE");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            mask = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

            if ((mask == 0) || (mask & ~(SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT | SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR))) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            return test1QueueIrp(&DeviceExtension->PurgeQueue, Irp);
        }
        break;

    // The IOCTL_SERIAL_GET_HANDFLOW request returns information about the configuration of the handshake flow control set for a COM port. 
    case IOCTL_SERIAL_GET_HANDFLOW:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_HANDFLOW");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_HANDFLOW)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *((PSERIAL_HANDFLOW)Irp->AssociatedIrp.SystemBuffer) = DeviceExtension->SerialHandFlow;
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;

    // The IOCTL_SERIAL_SET_HANDFLOW request sets the configuration of handshake flow control. 
    // Serial verifies the specified handshake flow control information.
    case IOCTL_SERIAL_SET_HANDFLOW:
        {
            PSERIAL_HANDFLOW handFlow;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_SET_HANDFLOW");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_HANDFLOW)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            handFlow = (PSERIAL_HANDFLOW)Irp->AssociatedIrp.SystemBuffer;

            if (handFlow->ControlHandShake & SERIAL_CONTROL_INVALID) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (handFlow->FlowReplace & SERIAL_FLOW_INVALID) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ((handFlow->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_MASK) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ((handFlow->XonLimit < 0) || ((ULONG)handFlow->XonLimit > DeviceExtension->QueueSize)) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ((handFlow->XoffLimit < 0) || ((ULONG)handFlow->XoffLimit > DeviceExtension->QueueSize)) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            if (DeviceExtension->EscapeChar) 
            {
                if (handFlow->FlowReplace & SERIAL_ERROR_CHAR) 
                {
                    status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
                    test1DecrementIoCount(&DeviceExtension->IoLock);
                    break;
                }
            }

            test1SerialSetHandFlow(DeviceExtension, handFlow);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_GET_MODEMSTATUS request updates the modem status, and returns 
    // the value of the modem status register before the update.
    case IOCTL_SERIAL_GET_MODEMSTATUS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_MODEMSTATUS");


            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            Irp->IoStatus.Information = sizeof(ULONG);

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = test1SerialHandleModemUpdate(DeviceExtension, FALSE);
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_GET_DTRRTS request returns information about the data terminal 
    // ready control signal (DTR) and the request to send control signal (RTS).
    case IOCTL_SERIAL_GET_DTRRTS:
        {
            ULONG ModemControl;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_DTRRTS");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = test1CheckIoLock(&DeviceExtension->IoLock, Irp);
            if (!NT_SUCCESS(status) || (status == STATUS_PENDING))
            {
                SerialCloneDebugPrint(DBG_IO, DBG_WARN, __FUNCTION__"--. IRP %p STATUS %x", Irp, status);

                return status;
            }

            Irp->IoStatus.Information = sizeof(ULONG);

            ModemControl = test1UartReadMCR(&DeviceExtension->Uart);

            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;

            test1DecrementIoCount(&DeviceExtension->IoLock);
        }
        break;

    // The IOCTL_SERIAL_GET_COMMSTATUS request returns information about the communication status of a COM port.
    case IOCTL_SERIAL_GET_COMMSTATUS:
        {
            PSERIAL_STATUS serialStatus;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_COMMSTATUS");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            serialStatus = (PSERIAL_STATUS)Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            serialStatus->Errors = DeviceExtension->ErrorWord;
            DeviceExtension->ErrorWord = 0;

            serialStatus->EofReceived = FALSE;
            serialStatus->AmountInInQueue = DeviceExtension->ReadCount;
            serialStatus->AmountInOutQueue = DeviceExtension->PendingWriteCount;

            if (DeviceExtension->WriteLength) 
            {
                serialStatus->AmountInOutQueue -= IoGetCurrentIrpStackLocation(DeviceExtension->WriteQueue.CurrentIrp)->Parameters.Write.Length - (DeviceExtension->WriteLength);
            }

            serialStatus->WaitForImmediate = DeviceExtension->TxImmediate;

            serialStatus->HoldReasons = 0;
            if (DeviceExtension->TxStopReason) 
            {
                if (DeviceExtension->TxStopReason & TEST1_SERIAL_TX_CTS) 
                {
                    serialStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
                }

                if (DeviceExtension->TxStopReason & TEST1_SERIAL_TX_DSR) 
                {
                    serialStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;
                }

                if (DeviceExtension->TxStopReason & TEST1_SERIAL_TX_DCD) 
                {
                    serialStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;
                }

                if (DeviceExtension->TxStopReason & TEST1_SERIAL_TX_XOFF) 
                {
                    serialStatus->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;
                }

                if (DeviceExtension->TxStopReason & TEST1_SERIAL_TX_BREAK) 
                {
                    serialStatus->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;
                }
            }

            if (DeviceExtension->RxStopReason & TEST1_SERIAL_RX_DSR) 
            {
                serialStatus->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;
            }

            if (DeviceExtension->RxStopReason & TEST1_SERIAL_RX_XOFF) 
            {
                serialStatus->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;
            }

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            Irp->IoStatus.Information = sizeof(SERIAL_STATUS);
        }
        break;

    // The IOCTL_SERIAL_GET_PROPERTIES request returns information about the capabilities of a COM port.
    case IOCTL_SERIAL_GET_PROPERTIES:
        {
            PSERIAL_COMMPROP serialProp;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_PROPERTIES");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_COMMPROP)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            serialProp = (PSERIAL_COMMPROP)Irp->AssociatedIrp.SystemBuffer;

            RtlZeroMemory(serialProp, sizeof(SERIAL_COMMPROP));

            //*****************************************************************
            //*****************************************************************
            // TODO: modify to match device properties
            //*****************************************************************
            //*****************************************************************

            serialProp->PacketLength = sizeof(SERIAL_COMMPROP);
            serialProp->PacketVersion = 2;
            serialProp->ServiceMask = SERIAL_SP_SERIALCOMM;
            serialProp->MaxTxQueue = 0;
            serialProp->MaxRxQueue = 0;

            serialProp->MaxBaud = SERIAL_BAUD_USER;
            serialProp->SettableBaud =  SERIAL_BAUD_USER |
                                        SERIAL_BAUD_075 | 
                                        SERIAL_BAUD_110 |
                                        SERIAL_BAUD_134_5 |
                                        SERIAL_BAUD_150 |
                                        SERIAL_BAUD_300 |
                                        SERIAL_BAUD_600 |
                                        SERIAL_BAUD_1200 |
                                        SERIAL_BAUD_1800 |
                                        SERIAL_BAUD_2400 |
                                        SERIAL_BAUD_4800 |
                                        SERIAL_BAUD_7200 |
                                        SERIAL_BAUD_9600 |
                                        SERIAL_BAUD_14400 |
                                        SERIAL_BAUD_19200 |
                                        SERIAL_BAUD_38400 |
                                        SERIAL_BAUD_56K |
                                        SERIAL_BAUD_57600 |
                                        SERIAL_BAUD_115200 |
                                        SERIAL_BAUD_128K;


            serialProp->ProvSubType = SERIAL_SP_RS232;
            serialProp->ProvCapabilities = SERIAL_PCF_DTRDSR |
                                           SERIAL_PCF_RTSCTS |
                                           SERIAL_PCF_CD     |
                                           SERIAL_PCF_PARITY_CHECK |
                                           SERIAL_PCF_XONXOFF |
                                           SERIAL_PCF_SETXCHAR |
                                           SERIAL_PCF_TOTALTIMEOUTS |
                                           SERIAL_PCF_INTTIMEOUTS;

            serialProp->SettableParams = SERIAL_SP_PARITY |
                                         SERIAL_SP_BAUD |
                                         SERIAL_SP_DATABITS |
                                         SERIAL_SP_STOPBITS |
                                         SERIAL_SP_HANDSHAKING |
                                         SERIAL_SP_PARITY_CHECK |
                                         SERIAL_SP_CARRIER_DETECT;

            serialProp->SettableData = SERIAL_DATABITS_5 |
                                       SERIAL_DATABITS_6 |
                                       SERIAL_DATABITS_7 |
                                       SERIAL_DATABITS_8;

            serialProp->SettableStopParity = SERIAL_STOPBITS_10 |
                                             SERIAL_STOPBITS_15 |
                                             SERIAL_STOPBITS_20 |
                                             SERIAL_PARITY_NONE |
                                             SERIAL_PARITY_ODD  |
                                             SERIAL_PARITY_EVEN |
                                             SERIAL_PARITY_MARK |
                                             SERIAL_PARITY_SPACE;
            serialProp->CurrentTxQueue = 0;
            serialProp->CurrentRxQueue = DeviceExtension->QueueSize;

            Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
        }
        break;

    // The IOCTL_SERIAL_XOFF_COUNTER request sets an XOFF counter. An XOFF counter request 
    // supports clients that use software to emulate hardware handshake flow control.
    // An XOFF counter request is synchronized with write requests. The driver sends a specified XOFF character, 
    // and completes the request after one of the following events occurs:
    // 1) A write request is received. 
    // 2) A timer expires (a timeout value is specified by the XOFF counter request). 
    // 3) Serial receives a number of characters that is greater than or equal to a count specified by the XOFF counter request. 
    case IOCTL_SERIAL_XOFF_COUNTER:
        {
            PSERIAL_XOFF_COUNTER xoffCounter;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_XOFF_COUNTER");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_XOFF_COUNTER)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            xoffCounter = (PSERIAL_XOFF_COUNTER)Irp->AssociatedIrp.SystemBuffer;

            if (xoffCounter->Counter <= 0) 
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            Irp->IoStatus.Information = 0;

            return test1QueueIrp(&DeviceExtension->WriteQueue, Irp);
        }
        break;

    // The IOCTL_SERIAL_LSRMST_INSERT request enables or disables the insertion of information about line status 
    // and modem status in the receive data stream. If LSRMST insertion is enabled, the driver inserts event 
    // information for the supported event types. The event information includes an event header followed by 
    // event-specific data. The event header contains a client-specified escape character and a flag that identifies 
    // the event. The driver supports the following event types: 
    // SERIAL_LSRMST_LSR_DATA 
    //      A change occurred in the line status. Serial inserts an event header followed by the event-specific data, 
    //      which is the value of the line status register followed by the character present in the receive hardware 
    //      when the line-status change was processed. 
    // SERIAL_LSRMST_LSR_NODATA 
    //      A line status change occurred, but no data was available in the receive buffer. Serial inserts an 
    //      event header followed by the event-specific data, which is the value of the line status register 
    //      when the line status change was processed. 
    // SERIAL_LSRMST_MST 
    //      A change occurred in the modem status. Serial inserts an event header followed by the event-specific 
    //      data, which is the value of the modem status register when the modem-status change was processed. 
    // SERIAL_LSRMST_ESCAPE 
    //      Indicates that the next character in the receive data stream, which was received from the device, 
    //      is identical to the client-specified escape character. Serial inserts an event header. There is 
    //      no event-specific data. 
    case IOCTL_SERIAL_LSRMST_INSERT:
        {
            PUCHAR escapeChar;

            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_LSRMST_INSERT");

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            escapeChar = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);

            if (*escapeChar) 
            {
                if ((*escapeChar == DeviceExtension->SerialChars.XoffChar) ||
                    (*escapeChar == DeviceExtension->SerialChars.XonChar) ||
                    (DeviceExtension->SerialHandFlow.FlowReplace & SERIAL_ERROR_CHAR)) 
                {
                    status = STATUS_INVALID_PARAMETER;

                    KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
                    break;
                }
            }

            DeviceExtension->EscapeChar = *escapeChar;

            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;

    // The IOCTL_SERIAL_CONFIG_SIZE request returns information about configuration size. 
    case IOCTL_SERIAL_CONFIG_SIZE:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_CONFIG_SIZE");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = 0;
        }
        break;

    // The IOCTL_SERIAL_GET_STATS request returns information about the performance of a COM port. 
    // The statistics include the number of characters transmitted, the number of characters received, 
    // and useful error statistics. The driver continuously increments performance values. 
    case IOCTL_SERIAL_GET_STATS:
        {
            SerialCloneDebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_GET_STATS");

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIALPERF_STATS)) 
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            *(PSERIALPERF_STATS)Irp->AssociatedIrp.SystemBuffer = DeviceExtension->SerialStats;
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);

            Irp->IoStatus.Information = sizeof(SERIALPERF_STATS);
        }
        break;

    // The IOCTL_SERIAL_CLEAR_STATS request clears the performance statistics for a COM port.
    case IOCTL_SERIAL_CLEAR_STATS:
        {
            test1DebugPrint(DBG_IO, DBG_INFO, "IOCTL_SERIAL_CLEAR_STATS");

            KeAcquireSpinLock(&DeviceExtension->SerialLock, &oldIrql);
            RtlZeroMemory(&DeviceExtension->SerialStats, sizeof(SERIALPERF_STATS));
            RtlZeroMemory(&DeviceExtension->WmiPerfData, sizeof(SERIAL_WMI_PERF_DATA));
            KeReleaseSpinLock(&DeviceExtension->SerialLock, oldIrql);
        }
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

