//--------------------------------------------------------------------------------------------------
//
//  File:       m+mManagerApplication.cpp
//
//  Project:    m+m
//
//  Contains:   The class definition for the application object of the m+m manager application.
//
//  Written by: Norman Jaffe
//
//  Copyright:  (c) 2014 by H Plus Technologies Ltd. and Simon Fraser University.
//
//              All rights reserved. Redistribution and use in source and binary forms, with or
//              without modification, are permitted provided that the following conditions are met:
//                * Redistributions of source code must retain the above copyright notice, this list
//                  of conditions and the following disclaimer.
//                * Redistributions in binary form must reproduce the above copyright notice, this
//                  list of conditions and the following disclaimer in the documentation and/or
//                  other materials provided with the distribution.
//                * Neither the name of the copyright holders nor the names of its contributors may
//                  be used to endorse or promote products derived from this software without
//                  specific prior written permission.
//
//              THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//              EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//              OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//              SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//              INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//              TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//              BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//              CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//              ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//              DAMAGE.
//
//  Created:    2014-07-14
//
//--------------------------------------------------------------------------------------------------

#include "m+mManagerApplication.h"

#include "m+mEntitiesPanel.h"
#include "m+mPeekInputHandler.h"
#include "m+mRegistryLaunchThread.h"
#include "m+mScannerThread.h"
#include "m+mServiceLaunchThread.h"
#include "m+mSettingsWindow.h"
#include "m+mYarpLaunchThread.h"

#include <m+m/m+mEndpoint.h>
#include <m+m/m+mRequests.h>

#if (! MAC_OR_LINUX_)
# include <io.h>
#endif // ! MAC_OR_LINUX_

//#include <odl/ODEnableLogging.h>
#include <odl/ODLogging.h>

#if defined(__APPLE__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wc++11-extensions"
# pragma clang diagnostic ignored "-Wconversion"
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
# pragma clang diagnostic ignored "-Wdocumentation"
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
# pragma clang diagnostic ignored "-Wextern-c-compat"
# pragma clang diagnostic ignored "-Wextra-semi"
# pragma clang diagnostic ignored "-Wpadded"
# pragma clang diagnostic ignored "-Wshadow"
# pragma clang diagnostic ignored "-Wsign-conversion"
# pragma clang diagnostic ignored "-Wunused-parameter"
# pragma clang diagnostic ignored "-Wweak-vtables"
#endif // defined(__APPLE__)
#if (! MAC_OR_LINUX_)
# pragma warning(push)
# pragma warning(disable: 4996)
# pragma warning(disable: 4458)
#endif // ! MAC_OR_LINUX_
#include <yarp/os/impl/SystemInfo.h>
#if (! MAC_OR_LINUX_)
# pragma warning(pop)
#endif // ! MAC_OR_LINUX_
#if defined(__APPLE__)
# pragma clang diagnostic pop
#endif // defined(__APPLE__)
#if defined(__APPLE__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif // defined(__APPLE__)
/*! @file
 
 @brief The class definition for the application object of the m+m manager application. */
#if defined(__APPLE__)
# pragma clang diagnostic pop
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Namespace references
#endif // defined(__APPLE__)

using namespace MplusM;
using namespace MPlusM_Manager;
using namespace std;

#if defined(__APPLE__)
# pragma mark Private structures, constants and variables
#endif // defined(__APPLE__)

/*! @brief @c true if an exit has been requested and @c false otherwise. */
static bool lExitRequested = false;

/*! @brief The number of milliseconds before a thread is force-killed. */
static const int kThreadKillTime = 3000;

#if defined(__APPLE__)
# pragma mark Global constants and variables
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Local functions
#endif // defined(__APPLE__)

bool caseInsensitiveMatch(const char * string1,
                          const char * string2)
{
	OD_LOG_ENTER(); //####
	OD_LOG_S2("string1 = ", string1, "string2 = ", string2); //####
	bool matched = true;

	if (! string1)
	{
        ManagerWindow * mainWindow = ManagerApplication::getMainWindow();
        
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Bad first string pointer",
                                    String::empty, String::empty, mainWindow);
        mainWindow->toFront(true);
	}
	else if (! string2)
	{
        ManagerWindow * mainWindow = ManagerApplication::getMainWindow();
        
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Bad second string pointer",
                                    String::empty, String::empty, mainWindow);
        mainWindow->toFront(true);
	}
    else
    {
        for ( ; matched; ++string1, ++string2)
        {
            if (*string1)
            {
                if (*string2)
                {
                    matched = (toupper(*string1) == toupper(*string2));
                }
                else
                {
                    // First string is longer.
                }
            }
            else if (*string2)
            {
                // Second string is longer.
                matched = false;
            }
            else
            {
                // Reached end-of-string on both strings.
                break;
            }
            
        }
    }
	OD_LOG_EXIT_B(matched); //####
	return matched;
} // caseInsensitiveMatch

#if defined(__APPLE__)
# pragma mark Class methods
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Constructors and Destructors
#endif // defined(__APPLE__)

ManagerApplication::ManagerApplication(void) :
    inherited(), _mainWindow(nullptr), _yarp(nullptr), _scanner(nullptr),
    _registryLauncher(nullptr), _yarpLauncher(nullptr), _peeker(nullptr),
    _peekHandler(nullptr), _registryServiceCanBeLaunched(false)
{
#if defined(MpM_ServicesLogToStandardError)
    OD_LOG_INIT(ProjectInfo::projectName, kODLoggingOptionIncludeProcessID | //####
                kODLoggingOptionIncludeThreadID | kODLoggingOptionWriteToStderr | //####
                kODLoggingOptionEnableThreadSupport); //####
#else // ! defined(MpM_ServicesLogToStandardError)
    OD_LOG_INIT(ProjectInfo::projectName, kODLoggingOptionIncludeProcessID | //####
                kODLoggingOptionIncludeThreadID | kODLoggingOptionEnableThreadSupport); //####
#endif // ! defined(MpM_ServicesLogToStandardError)
    OD_LOG_ENTER(); //####
    OD_LOG_EXIT_P(this); //####
} // ManagerApplication::ManagerApplication

ManagerApplication::~ManagerApplication(void)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::~ManagerApplication

#if defined(__APPLE__)
# pragma mark Actions and Accessors
#endif // defined(__APPLE__)

#if (! MAC_OR_LINUX_)
# pragma warning(push)
# pragma warning(disable: 4100)
#endif // ! MAC_OR_LINUX_
void ManagerApplication::anotherInstanceStarted(const String & commandLine)
{
#if (! defined(OD_ENABLE_LOGGING))
# if MAC_OR_LINUX_
#  pragma unused(commandLine)
# endif // MAC_OR_LINUX_
#endif // ! defined(OD_ENABLE_LOGGING)
    OD_LOG_OBJENTER(); //####
    OD_LOG_S1s("commandLine = ", commandLine.toStdString()); //####
    // When another instance of the app is launched while this one is running,
    // this method is invoked, and the commandLine parameter tells you what
    // the other instance's command-line arguments were.
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::anotherInstanceStarted
#if (! MAC_OR_LINUX_)
# pragma warning(pop)
#endif // ! MAC_OR_LINUX_

bool ManagerApplication::checkForRegistryServiceAndLaunchIfDesired(void)
{
    OD_LOG_OBJENTER(); //####
    bool                     didLaunch = false;
#if MAC_OR_LINUX_
    yarp::os::impl::Logger & theLogger = Common::GetLogger();
#endif // MAC_OR_LINUX_

    if (0 < _registryServicePath.length())
    {
        // If the Registry Service is installed, give the option of running it.
        if (validateRegistryService())
        {
            didLaunch = doLaunchRegistry();
        }
    }
    else
    {
        // If YARP isn't installed, say so and leave.
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, getApplicationName(),
                                    "A Registry Service executable could not be found in the PATH "
                                    "system environment variable. "
                                    "Launching the Registry Service is not possible.",
                                    String::empty, _mainWindow);
        _mainWindow->toFront(true);
    }
    OD_LOG_OBJEXIT_B(didLaunch); //####
    return didLaunch;
} // ManagerApplication::checkForRegistryServiceAndLaunchIfDesired

yarp::os::Network * ManagerApplication::checkForYarpAndLaunchIfDesired(void)
{
    OD_LOG_OBJENTER(); //####
    yarp::os::Network *      result = nullptr;
#if MAC_OR_LINUX_
    yarp::os::impl::Logger & theLogger = Common::GetLogger();
#endif // MAC_OR_LINUX_
    
    // No running YARP server was detected - first check if YARP is actually available:
    if (0 < _yarpPath.length())
    {
        // If YARP is installed, give the option of running a private copy.
        if (validateYarp())
        {
            char             ipBuffer[INET_ADDRSTRLEN + 1];
            const char *     ipAddressAsString = nullptr;
            struct in_addr   serverAddress;
            int              serverPort = 10000;
            String           selectedIpAddress;
            String           serverPortAsString;
            YarpStringVector ipAddressVector;
            
#if MAC_OR_LINUX_
            theLogger.warning("Private YARP network being launched.");
#endif // MAC_OR_LINUX_
            if (Utilities::GetCurrentYarpConfiguration(serverAddress, serverPort))
            {
                if (INADDR_NONE != serverAddress.s_addr)
                {
                    serverPortAsString = String(serverPort);
                    ipAddressAsString = inet_ntop(AF_INET, &serverAddress.s_addr, ipBuffer,
                                                  sizeof(ipBuffer));
                    _configuredYarpAddress = ipAddressAsString;
                    _configuredYarpPort = serverPort;
                }
            }
			Utilities::GetMachineIPs(ipAddressVector);
            int         initialSelection = 0;
            int         portChoice = serverPort;
            int         res = 0;
            StringArray ipAddressArray;
            String      appName(JUCEApplication::getInstance()->getApplicationName());
            AlertWindow ww(appName, "Please select the IP address and port to be used to start the "
                           "private YARP network:", AlertWindow::QuestionIcon, _mainWindow);
            
            for (unsigned ii = 0; ipAddressVector.size() > ii; ++ii)
            {
                YarpString anAddress(ipAddressVector[ii]);
                
                ipAddressArray.add(anAddress.c_str());
                if (ipAddressAsString && (anAddress == ipAddressAsString))
                {
                    initialSelection = ii;
                }
            }
            ww.addComboBox("IPAddress", ipAddressArray, "IP address:");
            ComboBox * cBox = ww.getComboBoxComponent("IPAddress");
            
            cBox->setSelectedItemIndex(initialSelection);
            ww.addTextEditor("NetworkPort", serverPortAsString, "Network port:");
            ww.addButton("OK", 1, KeyPress(KeyPress::returnKey, 0, 0));
            ww.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
            for (bool keepGoing = true; keepGoing; )
            {
                res = ww.runModalLoop();
                if (1 == res)
                {
                    int    addressIndexChosen = cBox->getSelectedItemIndex();
                    String portText = ww.getTextEditorContents("NetworkPort");

                    selectedIpAddress = ipAddressArray[addressIndexChosen];
                    portChoice = portText.getIntValue();
                    if (Utilities::ValidPortNumber(portChoice))
                    {
                        keepGoing = false;
                    }
                    else
                    {
                        AlertWindow::showMessageBox(AlertWindow::WarningIcon, getApplicationName(),
                                                    "The port number is invalid. "
                                                    "Please enter a value between 1024 and 65535",
                                                    String::empty, _mainWindow);
                    }
                }
                else
                {
                    keepGoing = false;
                }
            }
            _mainWindow->toFront(true);
            if (1 == res)
            {
                _yarpLauncher = new YarpLaunchThread(_yarpPath, selectedIpAddress, portChoice);
                if (_yarpLauncher)
                {
                    _yarpLauncher->startThread();
                    // Sleep for a little while and recheck if YARP is active.
                    for ( ; ! result; )
                    {
                        Thread::sleep(100);
                        if (_yarpLauncher->isThreadRunning())
                        {
                            result = new yarp::os::Network;
                        }
                        else
                        {
                            _yarpLauncher = nullptr;
                            break;
                        }
                        
                    }
                }
            }
        }
    }
    else
    {
        // If YARP isn't installed, say so and leave.
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, getApplicationName(),
                                    "No YARP network was detected and a YARP executable could not "
                                    "be found in the PATH system environment variable. "
                                    "Execution is not possible.", String::empty, _mainWindow);
        _mainWindow->toFront(true);
    }
    OD_LOG_OBJEXIT_P(result); //####
    return result;
} // ManagerApplication::checkForYarpAndLaunchIfDesired

void ManagerApplication::connectPeekChannel(void)
{
    OD_LOG_OBJENTER(); //####
    if (_peeker)
    {
        if (! Utilities::NetworkConnectWithRetries(MpM_REGISTRY_STATUS_NAME_, _peeker->name(),
                                                   STANDARD_WAIT_TIME_))
        {
            OD_LOG("(! Utilities::NetworkConnectWithRetries(MpM_REGISTRY_STATUS_NAME, " //####
                   "_peeker->name(), STANDARD_WAIT_TIME_))"); //####
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::connectPeekChannel

void ManagerApplication::doCleanupSoon(void)
{
    OD_LOG_OBJENTER(); //####
    if (_scanner)
    {
        _scanner->doCleanupSoon();
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::doCleanupSoon

void ManagerApplication::doLaunchAService(const ApplicationInfo & appInfo)
{
    OD_LOG_OBJENTER(); //####
    bool                     okSoFar = false;
    String                   caption("Launching the ");
    String                   execType;
    YarpStringVector         services;
#if MAC_OR_LINUX_
    yarp::os::impl::Logger & theLogger = Common::GetLogger();
#endif // MAC_OR_LINUX_
    
    caption += appInfo._description;
    if (0 < appInfo._criteria.length())
    {
        execType = "adapter";
        if (Utilities::GetServiceNamesFromCriteria(appInfo._criteria.toStdString(), services))
        {
            if (0 < services.size())
            {
                okSoFar = true;
            }
            else
            {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, caption,
                                            "The service to which this adapter must attach is not "
                                            "running on this YARP network. "
                                            "It will not be possible to launch the adapter.",
                                            String::empty, _mainWindow);
                _mainWindow->toFront(true);
            }
        }
    }
    else
    {
        execType = "service";
        okSoFar = true;
    }
    if (okSoFar)
    {
#if MAC_OR_LINUX_
        theLogger.warning((appInfo._description + " being launched.").toStdString());
#endif // MAC_OR_LINUX_
        String                        endpointToUse;
        String                        portToUse;
        String                        tagToUse;
        StringArray                   argsToUse;
        ScopedPointer<SettingsWindow> settings(new SettingsWindow(caption, execType, appInfo,
                                                                  endpointToUse, tagToUse,
                                                                  portToUse, argsToUse));
        
        if (SettingsWindow::kSettingsOK == settings->runModalLoop())
        {
            ServiceLaunchThread * aLauncher = new ServiceLaunchThread(appInfo._applicationPath,
                                                                      endpointToUse, tagToUse,
                                                                      portToUse, argsToUse,
                                                                  appInfo._options.contains("g"));
            
            if (aLauncher)
            {
                _serviceLaunchers.add(aLauncher);
                aLauncher->startThread();
                doScanSoon();
            }
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::doLaunchAService

void ManagerApplication::doLaunchOtherApplication(void)
{
    OD_LOG_OBJENTER(); //####
    _applicationMenu.setLookAndFeel(&_mainWindow->getLookAndFeel());
    int res = _applicationMenu.show();

    if (0 < res)
    {
        const ApplicationInfo & appInfo = _applicationList.at(res - 1);

        doLaunchAService(appInfo);
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::doLaunchOtherApplication

bool ManagerApplication::doLaunchRegistry(void)
{
    OD_LOG_OBJENTER(); //####
    bool                     result = false;
#if MAC_OR_LINUX_
    yarp::os::impl::Logger & theLogger = Common::GetLogger();
#endif // MAC_OR_LINUX_
    int                      portChoice = 0;
    int                      res = 0;
    String                   appName(JUCEApplication::getInstance()->getApplicationName());
    AlertWindow              ww("Launching the Registry Service", "Please select the network port "
                                "to be used to start the Registry Service\n(enter 0 to use the "
                                "default port):",
                                AlertWindow::QuestionIcon, _mainWindow);
    
#if MAC_OR_LINUX_
    theLogger.warning("Registry Service being launched.");
#endif // MAC_OR_LINUX_
    ww.addTextEditor("NetworkPort", "0", "Network port [0 for the default port]:");
    ww.addButton("OK", 1, KeyPress(KeyPress::returnKey, 0, 0));
    ww.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));
    for (bool keepGoing = true; keepGoing; )
    {
        res = ww.runModalLoop();
        if (1 == res)
        {
            String portText = ww.getTextEditorContents("NetworkPort");
            
            portChoice = portText.getIntValue();
            if ((! portChoice) || Utilities::ValidPortNumber(portChoice))
            {
                keepGoing = false;
            }
            else
            {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon,
                                            "Launching the Registry Service",
                                            "The port number is invalid. "
                                            "Please enter a value between 1024 and 65535",
                                            String::empty, _mainWindow);
            }
        }
        else
        {
            keepGoing = false;
        }
    }
    if (1 == res)
    {
        _registryLauncher = new RegistryLaunchThread(_registryServicePath, portChoice);
        if (_registryLauncher)
        {
            _registryLauncher->startThread();
            _registryServiceCanBeLaunched = false;
            result = true;
        }
    }
    _mainWindow->toFront(true);
    OD_LOG_OBJEXIT_B(result); //####
    return result;
} // ManagerApplication::doLaunchRegistry

void ManagerApplication::doScanSoon(void)
{
    OD_LOG_OBJENTER(); //####
    if (_scanner)
    {
        _scanner->doScanSoon();
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::doScanSoon

String ManagerApplication::findPathToExecutable(const String & execName)
{
    OD_LOG_ENTER(); //####
    OD_LOG_S1s("execName = ", execName.toStdString()); //####
    String result;
    
    if (juce::File::isAbsolutePath(execName))
    {
        bool       doCheck = false;
        juce::File aFile(juce::File::createFileWithoutCheckingPath(execName));
        
        if (aFile.existsAsFile())
        {
            doCheck = true;
        }
#if (! MAC_OR_LINUX_)
        else
        {
			String temp(aFile.getFullPathName() + ".exe");
            
            aFile = juce::File::createFileWithoutCheckingPath(temp);
            if (aFile.existsAsFile())
            {
                doCheck = true;
            }
        }
#endif // ! MAC_OR_LINUX_
        if (doCheck)
        {
            String temp(aFile.getFullPathName());
            
#if MAC_OR_LINUX_
            if (! access(temp.toStdString().c_str(), X_OK))
#else // ! MAC_OR_LINUX_
            if (! _access(temp.toStdString().c_str(), 0)) // Note that there's no explicit check for
                                                          // execute permission in Windows
#endif // ! MAC_OR_LINUX_
            {
                // We've found an executable that we can use!
                result = temp;
            }
        }
    }
    else
    {
        String pathVar(getEnvironmentVar("PATH"));
        
        if (pathVar.isNotEmpty())
        {
            for ( ; pathVar.isNotEmpty(); )
            {
                bool       doCheck = false;
                juce::File aFile;
                String     pathToCheck;
#if MAC_OR_LINUX_
                int        indx = pathVar.indexOfChar(':');
#else // ! MAC_OR_LINUX_
                int        indx = pathVar.indexOfChar(';');
#endif // ! MAC_OR_LINUX_
                
                if (-1 == indx)
                {
                    pathToCheck = pathVar;
                    pathVar.clear();
                }
                else
                {
                    pathToCheck = pathVar.substring(0, indx);
                    pathVar = pathVar.substring(indx + 1);
                }
                pathToCheck = juce::File::addTrailingSeparator(pathToCheck);
                aFile = juce::File::createFileWithoutCheckingPath(pathToCheck + execName);
                if (aFile.existsAsFile())
                {
                    doCheck = true;
                }
#if (! MAC_OR_LINUX_)
                else
                {
                    String temp(aFile.getFullPathName() + ".exe");

                    aFile = juce::File::createFileWithoutCheckingPath(temp);
                    if (aFile.existsAsFile())
                    {
                        doCheck = true;
                    }
                }
#endif // ! MAC_OR_LINUX_
                if (doCheck)
                {
                    String temp(aFile.getFullPathName());
                    
#if MAC_OR_LINUX_
                    if (! access(temp.toStdString().c_str(), X_OK))
#else // ! MAC_OR_LINUX_
                    if (! _access(temp.toStdString().c_str(), 0)) // Note that there's no explicit
                                                                  // check for execute permission in
                                                                  // Windows
#endif // ! MAC_OR_LINUX_
                    {
                        // We've found an executable that we can use!
                        result = temp;
						pathVar = "";
                    }
                }
            }
        }
    }
    OD_LOG_EXIT_s(result.toStdString()); //####
    return result;
} // ManagerApplication::findPathToExecutable

ManagerApplication * ManagerApplication::getApp(void)
{
    OD_LOG_ENTER(); //####
    ManagerApplication * result = static_cast<ManagerApplication *>(JUCEApplication::getInstance());

    OD_LOG_EXIT_P(result); //####
    return result;
} // ManagerApplication::getApp

const String ManagerApplication::getApplicationName(void)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_OBJEXIT_S(ProjectInfo::projectName); //####
    return ProjectInfo::projectName;
} // ManagerApplication::getApplicationName

const String ManagerApplication::getApplicationVersion(void)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_OBJEXIT_S(ProjectInfo::versionString); //####
    return ProjectInfo::versionString;
} // ManagerApplication::getApplicationVersion

bool ManagerApplication::getArgumentsForApplication(ApplicationInfo & theInfo)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("theInfo = ", &theInfo); //####
    bool         okSoFar = false;
    ChildProcess runApplication;
    StringArray  nameAndArgs(theInfo._applicationPath);

    nameAndArgs.add("--args");
    if (runApplication.start(nameAndArgs))
    {
        const String childOutput(runApplication.readAllProcessOutput());

        runApplication.waitForProcessToFinish(kThreadKillTime);
        if (0 < childOutput.length())
        {
            StringArray aRecord(StringArray::fromTokens(childOutput, "\t", ""));

            // The input lines should be composed of argument descriptions separated by the
            // ARGUMENT_SEPARATOR_ string.
            for (int ii = 0, mm = aRecord.size(); mm > ii; ++ii)
            {
				String                              trimmedRecord(aRecord[ii].trim());
				YarpString                          argString(trimmedRecord.toStdString());
                Utilities::BaseArgumentDescriptor * argDesc =
                                                    Utilities::ConvertStringToArgument(argString);

                if (argDesc)
                {
                    theInfo._argDescriptions.push_back(argDesc);
                }
            }
            okSoFar = true;
        }
    }
    OD_LOG_OBJEXIT_B(okSoFar); //####
    return okSoFar;
} // ManagerApplication::getArgumentsForApplication

String ManagerApplication::getEnvironmentVar(const char * varName)
{
    OD_LOG_ENTER(); //####
    OD_LOG_S1("varName = ", varName); //####
    String           result;
    YarpStringVector keys;
    YarpStringVector values;
    YarpString       match(varName);
    
    getEnvironmentVars(keys, values);
    for (size_t ii = 0, mm = keys.size(); mm > ii; ++ii)
    {
		const char * keysAsChars = keys[ii].c_str();

		if (caseInsensitiveMatch(varName, keysAsChars))
		{
			result = values[ii].c_str();
			break;
		}
        
    }
    OD_LOG_EXIT_s(result.toStdString()); //####
    return result;
} // ManagerApplication::getEnvironmentVar

void ManagerApplication::getEnvironmentVars(YarpStringVector & keys,
                                            YarpStringVector & values)
{
    OD_LOG_ENTER(); //####
    OD_LOG_P1("result = ", result);
#if (! defined(__APPLE__))
    yarp::os::Property vars(yarp::os::impl::SystemInfo::getPlatformInfo().environmentVars);
    YarpString         varsAsString(vars.toString());
    yarp::os::Bottle   varsAsBottle(varsAsString);
#endif // ! defined(__APPLE__)

    keys.clear();
    values.clear();
#if defined(__APPLE__)
    // Since neither the 'environ' global variable nor the 'getenv()' function contain a useable
    // set of environment variables, we will cheat - launch a command shell, indicating that it is
    // an interactive, login shell, with a single command 'set'; this will cause various files to
    // be loaded that set the shell environment... giving us a useable set of environment variables.
    String       execPath("/bin/sh");
    ChildProcess runApplication;
    StringArray  nameAndArgs(execPath);
    
    nameAndArgs.add("-i");
    nameAndArgs.add("-l");
    nameAndArgs.add("-c");
    nameAndArgs.add("set");
    if (runApplication.start(nameAndArgs))
    {
        const String childOutput(runApplication.readAllProcessOutput());
        
        runApplication.waitForProcessToFinish(kThreadKillTime);
        if (0 < childOutput.length())
        {
            StringArray aRecord(StringArray::fromTokens(childOutput, "\n", ""));
            
            for (int ii = 0, mm = aRecord.size(); mm > ii; ++ii)
            {
                std::string tmpVariable(aRecord[ii].toStdString());
                size_t      equalsSign = tmpVariable.find("=");
                
                if (std::string::npos != equalsSign)
                {
                    keys.push_back(tmpVariable.substr(0, equalsSign));
                    values.push_back(tmpVariable.substr(equalsSign + 1));
                }
            }
        }
    }
#else // ! defined(__APPLE__)
    for (int ii = 0, numVars = varsAsBottle.size(); numVars > ii; ++ii)
    {
        yarp::os::Value & aValue = varsAsBottle.get(ii);
        
        if (aValue.isList())
        {
            yarp::os::Bottle * asList = aValue.asList();
            
            if (asList && (2 == asList->size()))
            {
                yarp::os::Value & keyValue = asList->get(0);
                yarp::os::Value & valueValue = asList->get(1);
                
                if (keyValue.isString() && valueValue.isString())
                {
                    YarpString keyString = keyValue.asString();
                    YarpString valueString = valueValue.asString();
                    
                    if ((0 < keyString.length()) && (0 < valueString.length()))
                    {
                        keys.push_back(keyString);
                        values.push_back(valueString);
                    }
                }
            }
        }
    }
#endif // ! defined(__APPLE__)
    OD_LOG_EXIT(); //####
} // ManagerApplication::getEnvironmentVars

String ManagerApplication::getHomeDir(void)
{
    OD_LOG_ENTER(); //####
    juce::File homeDir(juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    String     result(homeDir.getFullPathName());
    
    OD_LOG_EXIT_s(result.toStdString()); //####
    return result;
} // ManagerApplication::getHomeDir

ManagerWindow * ManagerApplication::getMainWindow(void)
{
    OD_LOG_ENTER(); //####
    ManagerWindow * result = getApp()->_mainWindow;
    
    OD_LOG_EXIT_P(result);
    return result;
} // ManagerApplication::getMainWindow

bool ManagerApplication::getParametersForApplication(const String &    execName,
                                                     ApplicationInfo & theInfo)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_S1s("execName = ", execName.toStdString()); //####
    OD_LOG_P1("theInfo = ", &theInfo); //####
    bool   okSoFar = false;
    String execPath(findPathToExecutable(execName));
    
    if (0 < execPath.length())
    {
        ChildProcess runApplication;
        StringArray  nameAndArgs(execPath);
        
        nameAndArgs.add("--info");
        if (runApplication.start(nameAndArgs))
        {
            const String childOutput(runApplication.readAllProcessOutput());
            
            runApplication.waitForProcessToFinish(kThreadKillTime);
            if (0 < childOutput.length())
            {
                StringArray aRecord(StringArray::fromTokens(childOutput, "\t", ""));
                
                // The input lines should be composed of three tab-separated items:
                // 0) Type ('Service' or 'Adapter')
                // 1) Allowed options
                // 2) Matching criteria
                // 3) Description
                if (4 <= aRecord.size())
                {
                    String execKind(aRecord[0]);
                    
                    if (execKind == "Adapter")
                    {
                        theInfo._kind = kApplicationAdapter;
                        okSoFar = (4 <= aRecord.size());
                    }
                    else if (execKind == "Service")
                    {
                        theInfo._kind = kApplicationService;
                        okSoFar = true;
                    }
                    if (okSoFar)
                    {
                        theInfo._applicationPath = nameAndArgs[0];
                        theInfo._options = aRecord[1];
                        theInfo._criteria = aRecord[2];
                        theInfo._description = aRecord[3].trim();
                        theInfo._shortName = execName;
                    }
                }
            }
        }
    }
    OD_LOG_OBJEXIT_B(okSoFar); //####
    return okSoFar;
} // ManagerApplication::getParametersForApplication

bool ManagerApplication::getPrimaryChannelForService(const ApplicationInfo & appInfo,
                                                     const String &          endpointName,
                                                     const String &          tag,
                                                     const String &          portNumber,
                                                     const StringArray &     arguments,
                                                     String &                channelName)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P3("appInfo = ", &appInfo, "arguments = ", &arguments, "channelName = ", //####
              &channelName); //####
    OD_LOG_S3s("endpointName = ", endpointName, "tag = ", tag, "portNumber = ", portNumber); //####
    bool         okSoFar = false;
    ChildProcess runApplication;
    StringArray  nameAndArgs(appInfo._applicationPath);

    nameAndArgs.add("--channel");
    if (0 < portNumber.length())
    {
        OD_LOG("(0 < portNumber())"); //####
        nameAndArgs.add("--port");
        nameAndArgs.add(portNumber);
    }
    if (0 < tag.length())
    {
        OD_LOG("(0 < tag())"); //####
        nameAndArgs.add("--tag");
        nameAndArgs.add(tag);
    }
    if (0 < endpointName.length())
    {
        OD_LOG("(0 < endpointName())"); //####
        nameAndArgs.add("--endpoint");
        nameAndArgs.add(endpointName);
    }
    if (0 < arguments.size())
    {
        nameAndArgs.addArray(arguments);
    }
    if (runApplication.start(nameAndArgs))
    {
        const String childOutput(runApplication.readAllProcessOutput());

        runApplication.waitForProcessToFinish(kThreadKillTime);
        if (0 < childOutput.length())
        {
            StringArray aRecord(StringArray::fromTokens(childOutput, "\t", ""));

            if (1 <= aRecord.size())
            {
                channelName = aRecord[0];
                okSoFar = true;
            }
        }
    }
    OD_LOG_OBJEXIT_B(result); //####
    return okSoFar;
} // ManagerApplication::getPrimaryChannelForService

String ManagerApplication::getRealName(void)
{
    OD_LOG_ENTER(); //####
    String          result;
#if defined(__APPLE__)
    struct passwd   pwd;
    struct passwd * pwdPtr = nullptr;
    char *          buf;
    long            bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
#endif // defined(__APPLE__)
    
    // Note that yarp::os::impl::SystemInfo::getUserInfo() does nothing in Mac OS X!
#if defined(__APPLE__)
    if (-1 == bufSize)
    {
        // Value was indeterminate.
        bufSize = 16384; // Should be more than enough.
    }
    buf = static_cast<char *>(malloc(bufSize));
    if (buf)
    {
        getpwuid_r(getuid(), &pwd, buf, bufSize, &pwdPtr);
        if (pwdPtr)
        {
            result = pwd.pw_gecos;
        }
        free(buf);
    }
#else // ! defined(__APPLE__)
    result = yarp::os::impl::SystemInfo::getUserInfo().realName.c_str();
#endif // ! defined(__APPLE__)
    OD_LOG_EXIT_s(result.toStdString()); //####
    return result;
} // ManagerApplication::getRealName

String ManagerApplication::getUserName(void)
{
    OD_LOG_ENTER(); //####
    String          result;
#if defined(__APPLE__)
    struct passwd   pwd;
    struct passwd * pwdPtr = nullptr;
    char *          buf;
    long            bufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
#endif // defined(__APPLE__)
    
    // Note that yarp::os::impl::SystemInfo::getUserInfo() does nothing in Mac OS X!
#if defined(__APPLE__)
    if (-1 == bufSize)
    {
        // Value was indeterminate.
        bufSize = 16384; // Should be more than enough.
    }
    buf = static_cast<char *>(malloc(bufSize));
    if (buf)
    {
        getpwuid_r(getuid(), &pwd, buf, bufSize, &pwdPtr);
        if (pwdPtr)
        {
            result = pwd.pw_name;
        }
        free(buf);
    }
#else // ! defined(__APPLE__)
    result = yarp::os::impl::SystemInfo::getUserInfo().userName.c_str();
#endif // ! defined(__APPLE__)
    OD_LOG_EXIT_s(result.toStdString()); //####
    return result;
} // ManagerApplication::getUserName

#if (! MAC_OR_LINUX_)
# pragma warning(push)
# pragma warning(disable: 4100)
#endif // ! MAC_OR_LINUX_
void ManagerApplication::initialise(const String & commandLine)
{
#if (! defined(OD_ENABLE_LOGGING))
# if MAC_OR_LINUX_
#  pragma unused(commandLine)
# endif // MAC_OR_LINUX_
#endif // ! defined(OD_ENABLE_LOGGING)
    OD_LOG_OBJENTER(); //####
    OD_LOG_S1s("commandLine = ", commandLine.toStdString()); //####
    bool launchedRegistry = false;
    
#if MAC_OR_LINUX_
    Common::SetUpLogger(ProjectInfo::projectName);
#endif // MAC_OR_LINUX_
    Common::Initialize(ProjectInfo::projectName);
    Utilities::SetUpGlobalStatusReporter();
#if defined(MpM_ReportOnConnections)
    ChannelStatusReporter * reporter = Utilities::GetGlobalStatusReporter();
#endif // defined(MpM_ReportOnConnections)

    Utilities::CheckForNameServerReporter();
    loadApplicationLists();
    _mainWindow = new ManagerWindow(ProjectInfo::projectName);
    if (Utilities::CheckForValidNetwork(true))
    {
        _yarp = new yarp::os::Network; // This is necessary to establish any connections to the YARP
                                       // infrastructure
    }
    else
    {
        OD_LOG("! (yarp::os::Network::checkNetwork())"); //####
#if MAC_OR_LINUX_
        yarp::os::impl::Logger & theLogger = Common::GetLogger();
#endif // MAC_OR_LINUX_

#if MAC_OR_LINUX_
        theLogger.warning("YARP network not running.");
#endif // MAC_OR_LINUX_
        _yarpPath = findPathToExecutable("yarp");
        _yarp = checkForYarpAndLaunchIfDesired();
    }
    if (_yarp)
    {
        if (! Utilities::CheckForRegistryService())
        {
            _registryServicePath = findPathToExecutable(MpM_REGISTRY_EXECUTABLE_NAME_);
            launchedRegistry = checkForRegistryServiceAndLaunchIfDesired();
        }
        EntitiesPanel & entities = _mainWindow->getEntitiesPanel();
        
        entities.recallPositions();
        _peeker = new Common::GeneralChannel(false);
        _peekHandler = new PeekInputHandler;
        if (_peeker && _peekHandler)
        {
#if defined(MpM_ReportOnConnections)
            _peeker->setReporter(reporter);
            _peeker->getReport(reporter);
#endif // defined(MpM_ReportOnConnections)
            YarpString peekName = Common::GetRandomChannelName(HIDDEN_CHANNEL_PREFIX_ "peek_/"
                                                               DEFAULT_CHANNEL_ROOT_);
            
            if (_peeker->openWithRetries(peekName, STANDARD_WAIT_TIME_))
            {
                _peeker->setReader(*_peekHandler);
                _scanner = new ScannerThread(*_mainWindow, launchedRegistry);
                _scanner->startThread();
            }
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::initialise
#if (! MAC_OR_LINUX_)
# pragma warning(pop)
#endif // ! MAC_OR_LINUX_

void ManagerApplication::loadApplicationLists(void)
{
    OD_LOG_OBJENTER(); //####
    juce::File  commonDir(juce::File::getSpecialLocation(juce::File::commonDocumentsDirectory));
    String      commonPath(juce::File::addTrailingSeparator(commonDir.getFullPathName()) + "m+m");
    String      pathToStdList(juce::File::addTrailingSeparator(commonPath) +
                              "standardApplications.txt");
    String      pathToCustomList(juce::File::addTrailingSeparator(commonPath) +
                                 "customApplications.txt");
    juce::File  stdListFile(pathToStdList);
    juce::File  customListFile(pathToCustomList);
    StringArray lines;
    
    if (stdListFile.existsAsFile())
    {
        stdListFile.readLines(lines);
    }
    if (customListFile.existsAsFile())
    {
        customListFile.readLines(lines);
    }
    _applicationMenu.addSectionHeader("Applications available:");
    _applicationMenu.addSeparator();
    for (int ii = 0, idx = 0, mm = lines.size(); mm > ii; ++ii)
    {
        String aLine(lines[ii]);
        
        if (0 < aLine.length())
        {
            if ('#' == aLine[0])
            {
                if ((1 < aLine.length()) && ('-' == aLine[1]))
                {
                    _applicationMenu.addSeparator();
                }
            }
            else
            {
                ApplicationInfo theInfo;
                
                // Strip off any trailing newlines.
                aLine = aLine.trim();
                if (getParametersForApplication(aLine, theInfo) &&
                    getArgumentsForApplication(theInfo))
                {
                    _applicationList.push_back(theInfo);
                    _applicationMenu.addItem(++idx, theInfo._description);
                }
            }
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::loadApplicationLists

bool ManagerApplication::moreThanOneInstanceAllowed(void)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_OBJEXIT_B(true); //####
    return true;
} // ManagerApplication::moreThanOneInstanceAllowed

void ManagerApplication::shutdown(void)
{
    OD_LOG_OBJENTER(); //####
    SetExitRequest();
	if (_scanner)
	{
		_scanner->signalThreadShouldExit();
	}
    for (int ii = 0, mm = _serviceLaunchers.size(); mm > ii; ++ii)
    {
        ServiceLaunchThread * aLauncher = _serviceLaunchers[ii];

        aLauncher->killChildProcess();
    }
    if (_registryLauncher)
    {
        _registryLauncher->killChildProcess();
    }
	if (_yarpLauncher)
	{
		_yarpLauncher->killChildProcess();
	}
	if (_scanner)
	{
        _scanner->stopThread(kThreadKillTime);
		_scanner = nullptr; // shuts down thread
	}
    for (int ii = 0, mm = _serviceLaunchers.size(); mm > ii; ++ii)
    {
        ServiceLaunchThread * aLauncher = _serviceLaunchers[ii];

        aLauncher->stopThread(kThreadKillTime);
    }
    if (_registryLauncher)
    {
        _registryLauncher->stopThread(kThreadKillTime);
        _registryLauncher = nullptr; // shuts down thread
    }
	if (_yarpLauncher)
    {
        _yarpLauncher->stopThread(kThreadKillTime);
		_yarpLauncher = nullptr; // shuts down thread
        restoreYarpConfiguration();
	}
	EntitiesPanel & entities = _mainWindow->getEntitiesPanel();

    entities.rememberPositions();
#if defined(MpM_DoExplicitClose)
    _peeker->close();
#endif // defined(MpM_DoExplicitClose)
    Common::GeneralChannel::RelinquishChannel(_peeker);
    _mainWindow = nullptr; // (deletes our window)
    yarp::os::Network::fini();
    _yarp = nullptr;
    Utilities::ShutDownGlobalStatusReporter();
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::shutdown

void ManagerApplication::systemRequestedQuit(void)
{
    OD_LOG_OBJENTER(); //####
    // This is called when the app is being asked to quit: you can ignore this
    // request and let the app carry on running, or call quit() to allow the app to close.
    quit();
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::systemRequestedQuit

void ManagerApplication::restoreYarpConfiguration(void)
{
    OD_LOG_OBJENTER(); //####
    ChildProcess runYarp;
    String       appName(JUCEApplication::getInstance()->getApplicationName());
    StringArray  nameAndArgs(_yarpPath);
    
    nameAndArgs.add("conf");
    nameAndArgs.add(_configuredYarpAddress);
    nameAndArgs.add(String(_configuredYarpPort));
    if (runYarp.start(nameAndArgs))
    {
        const String childOutput(runYarp.readAllProcessOutput());
        
        runYarp.waitForProcessToFinish(kThreadKillTime);
    }
    OD_LOG_OBJEXIT(); //####
} // ManagerApplication::restoreYarpConfiguration

bool ManagerApplication::validateRegistryService(void)
{
    OD_LOG_OBJENTER(); //####
    bool         doLaunch = false;
    ChildProcess runRegistryService;
    String       appName(JUCEApplication::getInstance()->getApplicationName());
    StringArray  nameAndArgs(_registryServicePath);
    
    nameAndArgs.add("--vers");
    if (runRegistryService.start(nameAndArgs))
    {
        const String childOutput(runRegistryService.readAllProcessOutput());
        
        runRegistryService.waitForProcessToFinish(kThreadKillTime);
        if (0 < childOutput.length())
        {
            // We have a useable Registry Service executable - ask what the user wants to do.
            _registryServiceCanBeLaunched = true;
            doLaunch = (1 == AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,
                                                          "No running Registry Service was "
                                                          "detected - do you wish to launch the "
                                                          "Registry Service?",
                                                          "If you do, it may take a few moments to "
                                                          "start, depending on network traffic and "
                                                          "system activity. "
                                                          "Also, the Registry Service will be "
                                                          "shut down when this application exits, "
                                                          "resulting in a potential loss of "
                                                          "connectivity to any m+m services that "
                                                          "were started after the Registry Service "
                                                          "was launched.", "Yes", "No",
                                                          _mainWindow, nullptr));
        }
        else
        {
            // The Registry Service executable can't actually be launched!
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, appName,
                                        "The Registry Service executable found in the PATH system "
                                        "environment variable did not return valid data. "
                                        "Launching the Registry Service is not possible.",
                                        String::empty, _mainWindow);
        }
    }
    else
    {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, appName,
                                    "The Registry Service executable found in the PATH system "
                                    "environment variable could not be started. "
                                    "Launching the Registry Service is not possible.",
                                    String::empty, _mainWindow);
    }
    _mainWindow->toFront(true);
    OD_LOG_OBJEXIT_B(doLaunch); //####
    return doLaunch;
} // ManagerApplication::validateRegistryService

bool ManagerApplication::validateYarp(void)
{
    OD_LOG_OBJENTER(); //####
    bool         doLaunch = false;
    ChildProcess runYarp;
    String       appName(JUCEApplication::getInstance()->getApplicationName());
    StringArray  nameAndArgs(_yarpPath);
    
    nameAndArgs.add("version");
    if (runYarp.start(nameAndArgs))
    {
        const String childOutput(runYarp.readAllProcessOutput());
        
        runYarp.waitForProcessToFinish(kThreadKillTime);
        if (0 < childOutput.length())
        {
            // We have a useable YARP executable - ask what the user wants to do.
            doLaunch = (1 == AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon,
                                                          "No YARP network was detected - do you "
                                                          "wish to launch a private YARP network?",
                                                          "If you do, it may take a few moments to "
                                                          "start, depending on network traffic and "
                                                          "system activity. "
                                                          "Also, the private YARP network will be "
                                                          "shut down when this application exits, "
                                                          "resulting in a potential loss of "
                                                          "connectivity to any m+m services that "
                                                          "were started after the private YARP "
                                                          "network was launched.", "Yes", "No",
                                                          _mainWindow, nullptr));
        }
        else
        {
            // The YARP executable can't actually be launched!
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, appName,
                                        "No YARP network was detected and the YARP executable "
                                        "found in the PATH system environment variable did not "
                                        "return valid data. "
                                        "Execution is not possible.", String::empty, _mainWindow);
        }
    }
    else
    {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, appName,
                                    "No YARP network was detected and the YARP executable found in "
                                    "the PATH system environment variable could not be started. "
                                    "Execution is not possible.", String::empty, _mainWindow);
    }
    _mainWindow->toFront(true);
    OD_LOG_OBJEXIT_B(doLaunch); //####
    return doLaunch;
} // validateYarp

#if defined(__APPLE__)
# pragma mark Global functions
#endif // defined(__APPLE__)

void MPlusM_Manager::CalculateTextArea(Point<int> &   dimensions,
                                       const Font &   aFont,
                                       const String & aString)
{
    OD_LOG_ENTER(); //####
    OD_LOG_P2("dimensions = ", &dimensions, "aFont = ", &aFont); //####
    OD_LOG_S1s("aString = ", aString.toStdString()); //####
    float       maxWidth = 0;
    StringArray asLines;
    
    asLines.addLines(aString);
    int numRows = asLines.size();
    
    for (int ii = 0; ii < numRows; ++ii)
    {
        const String & aRow = asLines[ii];
        float          aWidth = aFont.getStringWidthFloat(aRow);
        
        if (maxWidth < aWidth)
        {
            maxWidth = aWidth;
        }
    }
    dimensions = Point<int>(static_cast<int>(maxWidth + 0.5),
                            static_cast<int>((numRows * aFont.getHeight()) + 0.5));
    OD_LOG_EXIT(); //####
} // MPlusM_Manager::CalculateTextArea

#if (! MAC_OR_LINUX_)
# pragma warning(push)
# pragma warning(disable: 4100)
#endif // ! MAC_OR_LINUX_
bool CheckForExit(void * stuff)
{
#if (! defined(OD_ENABLE_LOGGING))
# if MAC_OR_LINUX_
#  pragma unused(stuff)
# endif // MAC_OR_LINUX_
#endif // ! defined(OD_ENABLE_LOGGING)
    OD_LOG_ENTER(); //####
    OD_LOG_P1("stuff = ", stuff); //####
    OD_LOG_EXIT_B(lExitRequested); //####
    return lExitRequested;
} // CheckForExit
#if (! MAC_OR_LINUX_)
# pragma warning(pop)
#endif // ! MAC_OR_LINUX_

void SetExitRequest(void)
{
    OD_LOG_ENTER(); //####
    lExitRequested = true;
    OD_LOG_EXIT(); //####
} // SetExitRequest