#include "stdafx.h"
#include "Installer.h"

//------------------------------------------------------------------------------------------

#define DRIVER_NAME            TEXT("UsbDk")
#define SYSTEM32_DRIVERS    TEXT("\\System32\\Drivers\\")

#define MAX_BUFFER_SIZE        1024
#define UPPER_FILTER_REGISTRY_SUBTREE TEXT("System\\CurrentControlSet\\Control\\Class\\{36FC9E60-C465-11CF-8056-444553540000}\\")
#define UPPER_FILTER_REGISTRY_KEY TEXT("UpperFilters")

using namespace std;
//------------------------------------------------------------------------------------------

UsbDkInstaller::UsbDkInstaller()
{
    m_regAccess.SetPrimaryKey(HKEY_LOCAL_MACHINE);
}
//--------------------------------------------------------------------------------

InstallResult UsbDkInstaller::Install()
{
    try
    {
        // copy driver to WIndows/System32/drivers folder
        auto driverLocation = setDriver();

        auto infFilePath = buildInfFilePath();

        if (!m_wdfCoinstaller.PreDeviceInstallEx(infFilePath))
        {
            return InstallFailureNeedReboot;
        }

        m_scManager.CreateServiceObject(DRIVER_NAME, driverLocation.c_str());

        m_wdfCoinstaller.PostDeviceInstall(infFilePath);

        addUsbDkToRegistry();
    }
    catch (exception *e)
    {
        wstring wString = __string2wstring(string(e->what()));
        OutputDebugString(wString.c_str());
        return InstallFailure;
    }

    return InstallSuccess;
}
//--------------------------------------------------------------------------------

bool UsbDkInstaller::Uninstall()
{
    try
    {
         removeUsbDkFromRegistry();

        // copy driver to WIndows/System32/drivers folder
        unsetDriver();

        auto infFilePath = buildInfFilePath();

        m_wdfCoinstaller.PreDeviceRemove(infFilePath);

        m_scManager.DeleteServiceObject(DRIVER_NAME);

        m_wdfCoinstaller.PostDeviceRemove(infFilePath);
    }
    catch (exception *e)
    {
        wstring wString = __string2wstring(string(e->what()));
        OutputDebugString(wString.c_str());
        return false;
    }

    return true;
}
//--------------------------------------------------------------------------------

tstring UsbDkInstaller::setDriver()
{
    TCHAR currDirectory[MAX_PATH];
    if (!GetCurrentDirectory(MAX_PATH, currDirectory))
    {
        throw UsbDkInstallerFailedException(TEXT("UsbDkInstaller throw the exception: GetCurrentDirectory failed!"));
    }

    tstring driverOrigLocationStr(tstring(currDirectory) + TEXT("\\") DRIVER_NAME TEXT(".sys"));

    auto driverDestLocation = buildDriverPath(DRIVER_NAME TEXT(".sys"));

    if (!CopyFile(driverOrigLocationStr.c_str(), driverDestLocation.c_str(), FALSE))
    {
        tstring errorMsg(tstring(TEXT("UsbDkInstaller throw the exception: CopyFile from")) + driverOrigLocationStr + TEXT(" to ") + driverDestLocation + TEXT(" failed."));
        throw UsbDkInstallerFailedException(errorMsg);
    }

    return driverDestLocation;
}
//-------------------------------------------------------------------------------------------

void UsbDkInstaller::unsetDriver()
{
    auto driverDestLocation = buildDriverPath(DRIVER_NAME TEXT(".sys"));

    if (!DeleteFile(driverDestLocation.c_str()))
    {
        tstring errorMsg(tstring(TEXT("UsbDkInstaller throw the exception: DeleteFile ")) + driverDestLocation + TEXT(" failed."));
        throw UsbDkInstallerFailedException(errorMsg);
    }
}
//-------------------------------------------------------------------------------------------

tstring UsbDkInstaller::buildDriverPath(const tstring &DriverFileName)
{
    TCHAR    driverDestPath[MAX_PATH];
    GetWindowsDirectory(driverDestPath, MAX_PATH);

    return tstring(driverDestPath) + SYSTEM32_DRIVERS + DriverFileName;
}
//-------------------------------------------------------------------------------------------

tstring UsbDkInstaller::buildInfFilePath()
{
    TCHAR currDir[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, currDir) == 0)
    {
        throw UsbDkInstallerFailedException(L"UsbDkInstaller throw the exception: GetCurrentDirectory failed!");
    }

    return tstring(currDir) + TEXT("\\") + DRIVER_NAME + TEXT(".inf");
}
//-------------------------------------------------------------------------------------------

void UsbDkInstaller::addUsbDkToRegistry()
{
    LPCTSTR upperFilterString = UPPER_FILTER_REGISTRY_KEY;
    LPCTSTR upperFiltersKeyStr = UPPER_FILTER_REGISTRY_SUBTREE;

    // check for value size
    DWORD valLen = m_regAccess.ReadMultiString(upperFilterString, NULL, MAX_BUFFER_SIZE, upperFiltersKeyStr);

    vector<TCHAR> valVector;
    tstringlist newfiltersList;
    if (valLen)
    {
        // get the value
        valVector.resize(valLen);
        valLen = m_regAccess.ReadMultiString(upperFilterString, &valVector[0], valLen, upperFiltersKeyStr);

        if (!valLen)
        {
            throw UsbDkInstallerFailedException(TEXT("UsbDkInstaller throw the exception: addUsbDkToRegistry failed in ReadMultiString!"));
        }

        tstringlist filtersList;
        buildStringListFromVector(filtersList, valVector);
        buildNewListWithoutEement(newfiltersList, filtersList, DRIVER_NAME);
    }

    newfiltersList.push_back(DRIVER_NAME);

    valVector.clear();
    buildMultiStringVectorFromList(valVector, newfiltersList);

    // set new value to registry
    if (!m_regAccess.WriteMultiString(upperFilterString, &valVector[0], 2*valVector.size(), upperFiltersKeyStr))
    {
        throw UsbDkInstallerFailedException(TEXT("UsbDkInstaller throw the exception: addUsbDkToRegistry failed in WriteMultiString."));
    }
}
//----------------------------------------------------------------------------

void UsbDkInstaller::removeUsbDkFromRegistry()
{
    // If key exists, and value (Multiple string) includes "UsbDk", remove it from value.

    LPCTSTR upperFilterString = UPPER_FILTER_REGISTRY_KEY;
    LPCTSTR upperFiltersKeyStr = UPPER_FILTER_REGISTRY_SUBTREE;

    // check for value size
    DWORD valLen = m_regAccess.ReadMultiString(upperFilterString, NULL, MAX_BUFFER_SIZE, upperFiltersKeyStr);

    if (valLen)
    {
        // get the value
        vector<TCHAR> valVector(valLen);
        valLen = m_regAccess.ReadMultiString(upperFilterString, &valVector[0], valLen, upperFiltersKeyStr);

        if (!valLen)
        {
            throw UsbDkInstallerFailedException(TEXT("UsbDkInstaller throw the exception: addUsbDkToRegistry failed in ReadMultiString!"));
        }

        tstringlist filtersList;
        buildStringListFromVector(filtersList, valVector);

        tstringlist newfiltersList;
        buildNewListWithoutEement(newfiltersList, filtersList, DRIVER_NAME);

        valVector.clear();
        buildMultiStringVectorFromList(valVector, newfiltersList);

        // set new value to registry
        if (!m_regAccess.WriteMultiString(upperFilterString, &valVector[0], 2 * valVector.size(), upperFiltersKeyStr))
        {
            throw UsbDkInstallerFailedException(TEXT("UsbDkInstaller throw the exception: addUsbDkToRegistry failed in WriteMultiString."));
        }
    }
}
//----------------------------------------------------------------------------

void UsbDkInstaller::buildMultiStringVectorFromList(vector<TCHAR> &valVector, tstringlist &newfiltersList)
{
    for (auto filter : newfiltersList)
    {
        std::copy(filter.begin(), filter.end(), std::back_inserter(valVector));
        valVector.push_back(TEXT('\0'));
    }
    if (valVector.empty())
    {
        valVector.push_back(TEXT('\0'));
    }
    valVector.push_back(TEXT('\0'));
}
//----------------------------------------------------------------------------

void UsbDkInstaller::buildStringListFromVector(tstringlist &filtersList, vector<TCHAR> &valVector)
{
    tstring currFilter;
    DWORD   currPos = 0;

    do
    {
        currFilter = &valVector[currPos];

        if (!currFilter.empty())
        {
            filtersList.push_back(currFilter);
        }

        currPos += currFilter.size() + 1;

    } while (!currFilter.empty());
}
//----------------------------------------------------------------------------

void UsbDkInstaller::buildNewListWithoutEement(tstringlist &newfiltersList, tstringlist &filtersList, tstring element)
{
    for (auto filter : filtersList)
    {
        if (filter != DRIVER_NAME)
        {
            newfiltersList.push_back(filter);
        }
    }
}
//----------------------------------------------------------------------------
