//--------------------------------------------------------------------------------------------------
//
//  File:       YarpLaunchThread.cpp
//
//  Project:    M+M
//
//  Contains:   The class declaration for the background YARP launcher.
//
//  Written by: Norman Jaffe
//
//  Copyright:  (c) 2015 by H Plus Technologies Ltd. and Simon Fraser University.
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
//  Created:    2015-05-05
//
//--------------------------------------------------------------------------------------------------

#include "YarpLaunchThread.h"
#include "ChannelEntry.h"
#include "ChannelManagerApplication.h"
#include "EntitiesPanel.h"
#include "EntityData.h"
#include "PortData.h"

//#include <odl/ODEnableLogging.h>
#include <odl/ODLogging.h>

#if defined(__APPLE__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-pragmas"
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif // defined(__APPLE__)

#if (! MAC_OR_LINUX_)
# include <Windows.h>
#endif //! MAC_OR_LINUX_

/*! @file
 
 @brief The class declaration for the background YARP launcher. */
#if defined(__APPLE__)
# pragma clang diagnostic pop
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Namespace references
#endif // defined(__APPLE__)

using namespace ChannelManager;
using namespace MplusM;
using namespace std;

#if defined(__APPLE__)
# pragma mark Private structures, constants and variables
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Global constants and variables
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Local functions
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Class methods
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Constructors and Destructors
#endif // defined(__APPLE__)

YarpLaunchThread::YarpLaunchThread(const String & pathToExecutable,
                                   const String & ipAddress,
                                   const int      portNumber) :
	inherited("YARP launcher"), _yarpProcess(nullptr), _yarpAddress(ipAddress),
    _yarpPath(pathToExecutable), _port(portNumber)
{
    OD_LOG_ENTER(); //####
	OD_LOG_S2s("pathToExecutable = ", pathToExecutable, "ipAddress = ", ipAddress); //####
	OD_LOG_LL1("portNumber = ", portNumber); //####
    OD_LOG_EXIT_P(this); //####
} // YarpLaunchThread::YarpLaunchThread

YarpLaunchThread::~YarpLaunchThread(void)
{
    OD_LOG_OBJENTER(); //####
	killChildProcess();
	_yarpProcess = nullptr;
    OD_LOG_OBJEXIT(); //####
} // YarpLaunchThread::~YarpLaunchThread

#if defined(__APPLE__)
# pragma mark Actions and Accessors
#endif // defined(__APPLE__)

void YarpLaunchThread::killChildProcess(void)
{
	OD_LOG_OBJENTER(); //####
	if (_yarpProcess)
	{
		_yarpProcess->kill();
	}
	OD_LOG_OBJEXIT(); //####
} // YarpLaunchThread::killChildProcess

void YarpLaunchThread::run(void)
{
    OD_LOG_OBJENTER(); //####
    _yarpProcess = new ChildProcess;
    if (_yarpProcess)
    {
        StringArray nameAndArgs(_yarpPath);

        nameAndArgs.add("server");
        nameAndArgs.add("--ip");
        nameAndArgs.add(_yarpAddress);
		nameAndArgs.add("--socket");
		nameAndArgs.add(String(_port));
        nameAndArgs.add("--write");
        if (_yarpProcess->start(nameAndArgs, 0))
        {
            const String childOutput(_yarpProcess->readAllProcessOutput());
            
            _yarpProcess->waitForProcessToFinish(10000);
        }
    }
    OD_LOG_OBJEXIT(); //####
} // YarpLaunchThread::run

#if defined(__APPLE__)
# pragma mark Global functions
#endif // defined(__APPLE__)
