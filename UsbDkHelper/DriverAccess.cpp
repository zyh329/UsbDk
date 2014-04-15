#include "stdafx.h"
#include "DriverAccess.h"
#include "Public.h"

//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::GetDevicesList(PUSB_DK_DEVICE_INFO &DevicesArray, ULONG &NumberDevice)
{
    DevicesArray = nullptr;
    for (;;)
    {
        DWORD   bytesReturned;

        // get number of devices
        if (!DeviceIoControl(m_hDriver,
            IOCTL_USBDK_COUNT_DEVICES,
            nullptr,
            0,
            &NumberDevice,
            sizeof(NumberDevice),
            &bytesReturned,
            nullptr))
        {
            throw UsbDkDriverAccessException(TEXT("Counting devices failed"));
        }

        // get list of devices
        delete DevicesArray;
        DevicesArray = new USB_DK_DEVICE_INFO[NumberDevice];

        if (!DeviceIoControl(m_hDriver,
            IOCTL_USBDK_ENUM_DEVICES,
            nullptr,
            0,
            DevicesArray,
            NumberDevice * sizeof(USB_DK_DEVICE_INFO),
            &bytesReturned,
            nullptr))
        {
            DWORD err = GetLastError();
            if (err == ERROR_INSUFFICIENT_BUFFER || err == ERROR_MORE_DATA)
            {
                continue;
            }
            else
            {
                UsbDkDriverAccessException(TEXT("Enumeration failed in IOCTL_USBDK_ENUM_DEVICES."), err);
            }
        }
        NumberDevice = bytesReturned / sizeof(USB_DK_DEVICE_INFO);
        break;
    }
}
//------------------------------------------------------------------------------------------------
void UsbDkDriverAccess::ReleaseDeviceList(PUSB_DK_DEVICE_INFO DevicesArray)
{
    delete[] DevicesArray;
}
//------------------------------------------------------------------------------------------------

ULONG UsbDkDriverAccess::AddRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    ULONG RedirectorID;
    SendIoctlWithDeviceId(IOCTL_USBDK_ADD_REDIRECT, DeviceID, &RedirectorID);

    return RedirectorID;
}
//------------------------------------------------------------------------------------------------

void UsbDkDriverAccess::RemoveRedirect(USB_DK_DEVICE_ID &DeviceID)
{
    SendIoctlWithDeviceId(IOCTL_USBDK_REMOVE_REDIRECT, DeviceID);
}
//------------------------------------------------------------------------------------------------
