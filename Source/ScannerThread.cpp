//--------------------------------------------------------------------------------------------------
//
//  File:       ScannerThread.cpp
//
//  Project:    M+M
//
//  Contains:   The class declaration for the background port and service scanner.
//
//  Written by: Norman Jaffe
//
//  Copyright:  (c) 2014 by HPlus Technologies Ltd. and Simon Fraser University.
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
//  Created:    2014-07-17
//
//--------------------------------------------------------------------------------------------------

#include "ScannerThread.h"
#include "ChannelContainer.h"
#include "ChannelsWindow.h"

#include "ODEnableLogging.h"
#include "ODLogging.h"

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/energybased/FMMMLayout.h>

#include "M+MAdapterChannel.h"

#if defined(__APPLE__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif // defined(__APPLE__)
/*! @file
 
 @brief The class declaration for the background port and service scanner. */
#if defined(__APPLE__)
# pragma clang diagnostic pop
#endif // defined(__APPLE__)

using namespace ChannelManager;
using namespace std;

#if defined(__APPLE__)
# pragma mark Private structures, constants and variables
#endif // defined(__APPLE__)

/*! @brief The minimum time between background scans in milliseconds. */
static const int64 kMinScanInterval = 5000;

/*! @brief The port used to determine if a port being checked can be used as an output. */
static MplusM::Common::AdapterChannel * lInputOnlyPort = NULL;

/*! @brief The port used to determine if a port being checked can be used as an input. */
static MplusM::Common::AdapterChannel * lOutputOnlyPort = NULL;

/*! @brief The name of the port used to determine if a port being checked can be used as an
 output. */
static yarp::os::ConstString lInputOnlyPortName;

/*! @brief The name of the port used to determine if a port being checked can be used as an
 input. */
static yarp::os::ConstString lOutputOnlyPortName;

/*! @brief @c true if the port direction resources are available. */
static bool lPortsValid = false;

#if defined(__APPLE__)
# pragma mark Local functions
#endif // defined(__APPLE__)

/*! @brief Create the resources needed to determine port directions. */
static void createDirectionTestPorts(void)
{
    OD_LOG_ENTER(); //####
    lInputOnlyPortName = MplusM::Common::GetRandomChannelName(HIDDEN_CHANNEL_PREFIX
                                                              "checkdirection/channel_");
    lOutputOnlyPortName = MplusM::Common::GetRandomChannelName(HIDDEN_CHANNEL_PREFIX
                                                               "checkdirection/channel_");
    lInputOnlyPort = new MplusM::Common::AdapterChannel(false);
    if (lInputOnlyPort)
    {
        lInputOnlyPort->setInputMode(true);
        lInputOnlyPort->setOutputMode(false);
        lOutputOnlyPort = new MplusM::Common::AdapterChannel(true);
        if (lOutputOnlyPort)
        {
            lOutputOnlyPort->setInputMode(false);
            lOutputOnlyPort->setOutputMode(true);
            if (lInputOnlyPort->openWithRetries(lInputOnlyPortName, STANDARD_WAIT_TIME) &&
                lOutputOnlyPort->openWithRetries(lOutputOnlyPortName, STANDARD_WAIT_TIME))
            {
                lPortsValid = true;
            }
        }
    }
    OD_LOG_EXIT(); //####
} // createDirectionTestPorts

/*! @brief Release the resources used to determine port directions. */
static void destroyDirectionTestPorts(void)
{
    OD_LOG_ENTER(); //####
    if (lInputOnlyPort)
    {
#if defined(MpM_DoExplicitClose)
        lInputOnlyPort->close();
#endif // defined(MpM_DoExplicitClose)
        MplusM::Common::AdapterChannel::RelinquishChannel(lInputOnlyPort);
    }
    if (lOutputOnlyPort)
    {
#if defined(MpM_DoExplicitClose)
        lOutputOnlyPort->close();
#endif // defined(MpM_DoExplicitClose)
        MplusM::Common::AdapterChannel::RelinquishChannel(lOutputOnlyPort);
    }
    lPortsValid = false;
    OD_LOG_EXIT(); //####
} // destroyDirectionTestPorts

/*! @brief Determine whether a port can be used for input and/or output.
 @param oldEntry The previous record for the port, if it exists.
 @param portName The name of the port to check.
 @returns The allowed directions for the port. */
static PortDirection determineDirection(ChannelEntry *                oldEntry,
                                        const yarp::os::ConstString & portName)
{
    OD_LOG_ENTER(); //####
    OD_LOG_S1s("portName = ", portName); //####
    PortDirection result = kPortDirectionUnknown;
    
    if (oldEntry)
    {
        result = oldEntry->getDirection();
    }
    else if (lPortsValid)
    {
        bool canDoInput = false;
        bool canDoOutput = false;
        
        // First, check if we are looking at a client port - because of how they are
        // constructed, attempting to connect to them will result in a hang, so we just
        // treat them as I/O.
        switch (MplusM::Utilities::GetPortKind(portName))
        {
            case MplusM::Utilities::kPortKindClient :
                canDoInput = canDoOutput = true;
                break;
                
            case MplusM::Utilities::kPortKindService :
            case MplusM::Utilities::kPortKindServiceRegistry :
                canDoInput = true;
                break;
                
            default :
                // Determine by doing a test connection.
                if (MplusM::Common::NetworkConnectWithRetries(lOutputOnlyPortName, portName,
                                                              STANDARD_WAIT_TIME, false))
                {
                    canDoInput = true;
                    if (! MplusM::Common::NetworkDisconnectWithRetries(lOutputOnlyPortName,
                                                                       portName,
                                                                       STANDARD_WAIT_TIME))
                    {
                        OD_LOG("(! MplusM::Common::NetworkDisconnectWithRetries(" //####
                               "lOutputOnlyPortName, portName, STANDARD_WAIT_TIME))"); //####
                    }
                }
                if (MplusM::Common::NetworkConnectWithRetries(portName, lInputOnlyPortName,
                                                              STANDARD_WAIT_TIME, false))
                {
                    canDoOutput = true;
                    if (! MplusM::Common::NetworkDisconnectWithRetries(portName, lInputOnlyPortName,
                                                                       STANDARD_WAIT_TIME))
                    {
                        OD_LOG("(! MplusM::Common::NetworkDisconnectWithRetries(portName, " //####
                               "lInputOnlyPortName, STANDARD_WAIT_TIME))"); //####
                    }
                }
                break;
                
        }
        if (canDoInput)
        {
            result = (canDoOutput ? kPortDirectionInputOutput : kPortDirectionInput);
        }
        else if (canDoOutput)
        {
            result = kPortDirectionOutput;
        }
        else
        {
            result = kPortDirectionUnknown;
        }
    }
    else
    {
        result = kPortDirectionUnknown;
    }
    OD_LOG_EXIT_L(static_cast<long> (result)); //####
    return result;
} // determineDirection

/*! @brief Record a port in a port map.
 @param portsSeen The map to be updated.
 @param aPort The port to be recorded. */
static void recordPort(PortEntryMap & portsSeen,
                       ChannelEntry * aPort)
{
    OD_LOG_ENTER(); //####
    OD_LOG_P2("portsSeen = ", &portsSeen, "aPort = ", aPort); //####
    
    if (aPort)
    {
        portsSeen.insert(PortEntryMap::value_type(aPort->getPortName(), aPort));
    }
    OD_LOG_EXIT(); //####
} // recordPort

/*! @brief Find a port in the recorded list by name.
 @param portsSeen The map to be searched.
 @param name The name of the port.
 @returns @c NULL if the port cannot be found and non-@c NULL if it is found. */
static ChannelEntry * findRecordedPort(PortEntryMap & portsSeen,
                                       const String & name)
{
    OD_LOG_ENTER(); //####
    OD_LOG_S1s("name = ", name.toStdString()); //####
    ChannelEntry *               result;
    PortEntryMap::const_iterator match(portsSeen.find(name));
    
    if (portsSeen.end() == match)
    {
        result = NULL;
    }
    else
    {
        result = match->second;
    }
    OD_LOG_EXIT_P(result); //####
    return result;
} // findRecordedPort

#if defined(__APPLE__)
# pragma mark Class methods
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Constructors and destructors
#endif // defined(__APPLE__)

ScannerThread::ScannerThread(const String &   name,
                             ChannelsWindow * window) :
    inherited(name), _window(window), _rememberedPorts(), _detectedServices(), _associatedPorts(),
    _standalonePorts(), _connections(), _entitiesSeen()
{
    OD_LOG_ENTER(); //####
    OD_LOG_S1s("name = ", name.toStdString()); //####
    OD_LOG_P1("window = ", window); //####
    createDirectionTestPorts();
    OD_LOG_EXIT(); //####
} // ScannerThread::ScannerThread

ScannerThread::~ScannerThread(void)
{
    OD_LOG_OBJENTER(); //####
    stopThread(3000); // Give thread 3 seconds to shut down.
    destroyDirectionTestPorts();
    _detectedServices.clear();
    _rememberedPorts.clear();
    _associatedPorts.clear();
    _standalonePorts.clear();
    _connections.clear();
    _entitiesSeen.clear();
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::~ScannerThread

#if defined(__APPLE__)
# pragma mark Actions
#endif // defined(__APPLE__)

void ScannerThread::addPortConnections(const MplusM::Utilities::PortVector & detectedPorts)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("detectedPorts = ", &detectedPorts); //####
    for (MplusM::Utilities::PortVector::const_iterator outer(detectedPorts.begin());
         detectedPorts.end() != outer; ++outer)
    {
        String outerName(outer->_portName.c_str());
        
        if (_rememberedPorts.end() != _rememberedPorts.find(outerName))
        {
            ConnectionDetails             details;
            MplusM::Common::ChannelVector inputs;
            MplusM::Common::ChannelVector outputs;
            
            details._outPortName = outerName;
            MplusM::Utilities::GatherPortConnections(outer->_portName, inputs, outputs,
                                                     MplusM::Utilities::kInputAndOutputOutput,
                                                     true);
            for (MplusM::Common::ChannelVector::const_iterator inner(outputs.begin());
                 outputs.end() != inner; ++inner)
            {
                String innerName(inner->_portName.c_str());
                
                if (_rememberedPorts.end() != _rememberedPorts.find(innerName))
                {
                    details._inPortName = innerName;
                    details._mode = inner->_portMode;
                    _connections.push_back(details);
                }
                yield();
            }
        }
        yield();
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::addPortConnections

void ScannerThread::addPortsWithAssociates(const MplusM::Utilities::PortVector & detectedPorts)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("detectedPorts = ", &detectedPorts); //####
    _associatedPorts.clear();
    for (MplusM::Utilities::PortVector::const_iterator outer(detectedPorts.begin());
         detectedPorts.end() != outer; ++outer)
    {
        String outerName(outer->_portName.c_str());
        
        if (_rememberedPorts.end() == _rememberedPorts.find(outerName))
        {
            PortAndAssociates associates;
            
            if (MplusM::Utilities::GetAssociatedPorts(outer->_portName, associates._associates,
                                                      STANDARD_WAIT_TIME, true))
            {
                if (associates._associates._primary)
                {
                    String caption(outer->_portIpAddress + ":" + outer->_portPortNumber);
                    
                    associates._name = outerName;
                    _associatedPorts.insert(AssociatesMap::value_type(caption, associates));
                    _rememberedPorts.insert(outerName);
                    for (MplusM::Common::StringVector::const_iterator inner =
                         associates._associates._inputs.begin();
                         associates._associates._inputs.end() != inner; ++inner)
                    {
                        _rememberedPorts.insert(inner->c_str());
                        yield();
                    }
                    for (MplusM::Common::StringVector::const_iterator inner =
                         associates._associates._outputs.begin();
                         associates._associates._outputs.end() != inner; ++inner)
                    {
                        _rememberedPorts.insert(inner->c_str());
                        yield();
                    }
                }
            }
        }
        yield();
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::addPortsWithAssociates

void ScannerThread::addRegularPortEntities(const MplusM::Utilities::PortVector & detectedPorts)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("detectedPorts = ", &detectedPorts); //####
    _standalonePorts.clear();
    for (MplusM::Utilities::PortVector::const_iterator walker(detectedPorts.begin());
         detectedPorts.end() != walker; ++walker)
    {
        String walkerName(walker->_portName.c_str());
        
        if (_rememberedPorts.end() == _rememberedPorts.find(walkerName))
        {
            String           caption(walker->_portIpAddress + ":" + walker->_portPortNumber);
            NameAndDirection info;
            ChannelsPanel &  activePanel = getActivePanel();
            ChannelEntry *   oldEntry = activePanel.findKnownPort(walkerName);
            
            _rememberedPorts.insert(walkerName);
            info._name = walkerName;
            info._direction = determineDirection(oldEntry, walker->_portName);
            _standalonePorts.insert(PortMap::value_type(caption, info));
        }
        yield();
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::addRegularPortEntities

void ScannerThread::addServices(const MplusM::Common::StringVector & services)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("services = ", &services); //####
    _detectedServices.clear();
    for (MplusM::Common::StringVector::const_iterator outer(services.begin());
         services.end() != outer; ++outer)
    {
        String outerName(outer->c_str());
        
        if (_detectedServices.end() == _detectedServices.find(outerName))
        {
            MplusM::Utilities::ServiceDescriptor descriptor;
            
            if (MplusM::Utilities::GetNameAndDescriptionForService(*outer, descriptor,
                                                                   STANDARD_WAIT_TIME))
            {
                _detectedServices.insert(ServiceMap::value_type(outerName, descriptor));
                _rememberedPorts.insert(descriptor._channelName.c_str());
                for (MplusM::Common::ChannelVector::const_iterator inner =
                                                                descriptor._inputChannels.begin();
                     descriptor._inputChannels.end() != inner; ++inner)
                {
                    MplusM::Common::ChannelDescription aChannel(*inner);
                    
                    _rememberedPorts.insert(aChannel._portName.c_str());
                    yield();
                }
                for (MplusM::Common::ChannelVector::const_iterator inner =
                                                                descriptor._outputChannels.begin();
                     descriptor._outputChannels.end() != inner; ++inner)
                {
                    MplusM::Common::ChannelDescription aChannel(*inner);
                    
                    _rememberedPorts.insert(aChannel._portName.c_str());
                    yield();
                }
            }
        }
        yield();
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::addServices

void ScannerThread::gatherEntities(void)
{
    OD_LOG_OBJENTER(); //####
    MplusM::Utilities::PortVector detectedPorts;
    MplusM::Common::StringVector  services;
    
    // Mark our utility ports as known.
    _rememberedPorts.clear();
    _rememberedPorts.insert(lInputOnlyPortName.c_str());
    _rememberedPorts.insert(lOutputOnlyPortName.c_str());
    MplusM::Utilities::GetDetectedPortList(detectedPorts);
    MplusM::Utilities::GetServiceNames(services, true);
    // Record the services to be displayed.
    addServices(services);
    // Record the ports that have associates.
    if (MplusM::Utilities::CheckForRegistryService(detectedPorts))
    {
        addPortsWithAssociates(detectedPorts);
    }
    // Record the ports that are standalone.
    addRegularPortEntities(detectedPorts);
    // Record the port connections.
    addPortConnections(detectedPorts);
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::gatherEntities

ChannelsPanel & ScannerThread::getActivePanel(void)
const
{
    OD_LOG_OBJENTER(); //####
    ChannelsPanel * result = static_cast<ChannelsPanel *>(_window->getContentComponent());
    
    OD_LOG_OBJEXIT_P(result); //####
    return *result;
} // ScannerThread::getActivePanel

void ScannerThread::addEntitiesToPanel(ChannelsPanel * newPanel)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("newPanel = ", newPanel); //####
    PortEntryMap portsSeen;
    
    _entitiesSeen.clear();
    for (ServiceMap::const_iterator outer(_detectedServices.begin());
         _detectedServices.end() != outer; ++outer)
    {
        MplusM::Utilities::ServiceDescriptor descriptor(outer->second);
        ChannelContainer *                   anEntity = new ChannelContainer(kContainerKindService,
                                                                 descriptor._canonicalName.c_str(),
                                                                         descriptor._kind.c_str(),
                                                                 descriptor._description.c_str(),
                                                                             *newPanel);
        ChannelEntry *                       aPort =
                                                anEntity->addPort(descriptor._channelName.c_str(),
                                                                  "", kPortUsageService,
                                                                  kPortDirectionInput);
        
        recordPort(portsSeen, aPort);
        for (MplusM::Common::ChannelVector::const_iterator inner =
                                                                descriptor._inputChannels.begin();
             descriptor._inputChannels.end() != inner; ++inner)
        {
            MplusM::Common::ChannelDescription aChannel(*inner);
            
            aPort = anEntity->addPort(aChannel._portName.c_str(), aChannel._portProtocol.c_str(),
                                      kPortUsageService, kPortDirectionInput);
            recordPort(portsSeen, aPort);
        }
        for (MplusM::Common::ChannelVector::const_iterator inner =
                                                                descriptor._outputChannels.begin();
             descriptor._outputChannels.end() != inner; ++inner)
        {
            MplusM::Common::ChannelDescription aChannel(*inner);
            
            aPort = anEntity->addPort(aChannel._portName.c_str(), aChannel._portProtocol.c_str(),
                                      kPortUsageService, kPortDirectionOutput);
            recordPort(portsSeen, aPort);
        }
        _entitiesSeen.push_back(anEntity);
        newPanel->addEntity(anEntity);
    }
    // Convert the detected ports with associates into entities in the background list.
    for (AssociatesMap::const_iterator outer(_associatedPorts.begin());
         _associatedPorts.end() != outer; ++outer)
    {
        ChannelEntry *                             aPort;
        ChannelContainer *                         anEntity =
                                                new ChannelContainer(kContainerKindClientOrAdapter,
                                                                     outer->first, "", "",
                                                                     *newPanel);
        const MplusM::Utilities::PortAssociation & associates = outer->second._associates;
        
        for (MplusM::Common::StringVector::const_iterator inner = associates._inputs.begin();
             associates._inputs.end() != inner; ++inner)
        {
            aPort = anEntity->addPort(inner->c_str(), "", kPortUsageOther, kPortDirectionInput);
            recordPort(portsSeen, aPort);
        }
        for (MplusM::Common::StringVector::const_iterator inner = associates._outputs.begin();
             associates._outputs.end() != inner; ++inner)
        {
            aPort = anEntity->addPort(inner->c_str(), "", kPortUsageOther, kPortDirectionOutput);
            recordPort(portsSeen, aPort);
        }
        aPort = anEntity->addPort(outer->second._name, "", kPortUsageClient,
                                  kPortDirectionInputOutput);
        recordPort(portsSeen, aPort);
        _entitiesSeen.push_back(anEntity);
        newPanel->addEntity(anEntity);
    }
    // Convert the detected standalone ports into entities in the background list.
    for (PortMap::const_iterator walker(_standalonePorts.begin());
         _standalonePorts.end() != walker; ++walker)
    {
        ChannelContainer * anEntity = new ChannelContainer(kContainerKindOther, walker->first, "",
                                                           "", *newPanel);
        PortUsage          usage;
        
        switch (MplusM::Utilities::GetPortKind(walker->second._name.toStdString().c_str()))
        {
            case MplusM::Utilities::kPortKindClient :
                usage = kPortUsageClient;
                break;
                
            case MplusM::Utilities::kPortKindService :
            case MplusM::Utilities::kPortKindServiceRegistry :
                usage = kPortUsageService;
                break;
                
            default :
                usage = kPortUsageOther;
                break;
                
        }
        ChannelEntry * aPort = anEntity->addPort(walker->second._name, "", usage,
                                                 walker->second._direction);
        
        recordPort(portsSeen, aPort);
        _entitiesSeen.push_back(anEntity);
        newPanel->addEntity(anEntity);
    }
    // Convert the detected connections into connections in the background list.
    for (ConnectionList::const_iterator walker(_connections.begin()); _connections.end() != walker;
         ++walker)
    {
        ChannelEntry * thisPort = findRecordedPort(portsSeen, walker->_outPortName);
        ChannelEntry * otherPort = findRecordedPort(portsSeen, walker->_inPortName);
        
        if (thisPort && otherPort)
        {
            thisPort->addOutputConnection(otherPort, walker->_mode);
            otherPort->addInputConnection(thisPort, walker->_mode);
        }
    }
    setEntityPositions();
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::addEntitiesToPanel

void ScannerThread::run(void)
{
    OD_LOG_OBJENTER(); //####
    while (! threadShouldExit())
    {
        // Create a new panel to add entities to.
        ChannelsPanel * newPanel = new ChannelsPanel;
        int64           loopStartTime = Time::currentTimeMillis();
        
        gatherEntities();
        addEntitiesToPanel(newPanel);
        if (updateActivePanel(newPanel))
        {
            break;
        }
        
        int64 loopEndTime = Time::currentTimeMillis();
        int64 delayAmount = (loopStartTime + kMinScanInterval) - loopEndTime;
        
        if (kMinScanInterval < delayAmount)
        {
            delayAmount = kMinScanInterval;
        }
        if (0 < delayAmount)
        {
            wait(delayAmount);
        }
        else
        {
            char numBuff[30];
            
#if MAC_OR_LINUX_
            snprintf(numBuff, sizeof(numBuff), "%g", (loopEndTime - loopStartTime) / 1000.0);
#else // ! MAC_OR_LINUX_
            _snprintf(numBuff, sizeof(numBuff) - 1, "%g", (loopEndTime - loopStartTime) / 1000.0);
            // Correct for the weird behaviour of _snprintf
            numBuff[sizeof(numBuff) - 1] = '\0';
#endif // ! MAC_OR_LINUX_
            yarp::os::impl::Logger & theLogger = MplusM::Common::GetLogger();
            
            theLogger.info(yarp::os::ConstString("actual interval = ") + numBuff +
                           yarp::os::ConstString(" seconds"));
            yield();
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::run

void ScannerThread::setEntityPositions(void)
{
    OD_LOG_OBJENTER(); //####
    ChannelsPanel &       activePanel = getActivePanel();
    Random                randomizer(Time::currentTimeMillis());
    bool                  positionsNeedUpdate = false;
    float                 fullHeight = activePanel.getHeight();
    float                 fullWidth = activePanel.getWidth();
    ogdf::Graph           gg;
    ogdf::GraphAttributes ga(gg);
    ogdf::node            phantomNode = gg.newNode();
    
    ga.directed(true);
    // If nodes are not connected, OGDF will pile them all at the origin; by adding a 'phantom' node
    // that is connected to every other node, we force OGDF to spread the nodes out.
    ga.width(phantomNode) = 1;
    ga.height(phantomNode) = 1;
    ga.x(phantomNode) = (randomizer.nextFloat() * fullWidth);
    ga.y(phantomNode) = (randomizer.nextFloat() * fullHeight);
    for (ContainerList::const_iterator it(_entitiesSeen.begin()); _entitiesSeen.end() != it; ++it)
    {
        ChannelContainer * anEntity = *it;
        
        if (anEntity)
        {
            float              newX;
            float              newY;
            Rectangle<float>   entityShape(anEntity->getLocalBounds().toFloat());
            ogdf::node         aNode = gg.newNode();
            ChannelEntry *     firstPort = anEntity->getPort(0);
            ChannelContainer * olderVersion = NULL;
            
            if (firstPort)
            {
                olderVersion = activePanel.findKnownEntityForPort(firstPort->getPortName());
            }
            else
            {
                olderVersion = activePanel.findKnownEntity(anEntity->getName());
            }
            ga.width(aNode) = entityShape.getWidth();
            ga.height(aNode) = entityShape.getHeight();
            anEntity->setNode(aNode);
            if (olderVersion)
            {
                Rectangle<float> oldShape(olderVersion->getLocalBounds().toFloat());
                
                newX = oldShape.getX();
                newY = oldShape.getY();
            }
            else
            {
                newX = (randomizer.nextFloat() * (fullWidth - entityShape.getWidth()));
                newY = (randomizer.nextFloat() * (fullHeight - entityShape.getHeight()));
                positionsNeedUpdate = true;
            }
            ga.x(aNode) = newX;
            ga.y(aNode) = newY;
            anEntity->setTopLeftPosition(newX, newY);
        }
    }
    if (positionsNeedUpdate)
    {
        // Set up the edges (connections)
        for (ContainerList::const_iterator it(_entitiesSeen.begin()); _entitiesSeen.end() != it;
             ++it)
        {
            ChannelContainer * anEntity = *it;
            
            if (anEntity)
            {
                bool       wasConnected = false;
                ogdf::node thisNode = anEntity->getNode();
                
                // Add edges between entities that are connected via their entries
                for (int ii = 0, mm = anEntity->getNumPorts(); mm > ii; ++ii)
                {
                    ChannelEntry * aPort = anEntity->getPort(ii);
                    
                    if (aPort)
                    {
                        const Connections & outputs(aPort->getOutputConnections());
                        
                        for (int jj = 0, nn = outputs.size(); nn > jj; ++jj)
                        {
                            ChannelEntry * otherPort = outputs[jj]._otherPort;
                            
                            if (otherPort)
                            {
                                ChannelContainer * otherEntity = otherPort->getParent();
                                
                                if (otherEntity)
                                {
                                    ogdf::node otherNode = otherEntity->getNode();
                                    /*ogdf::edge ee =*/ gg.newEdge(thisNode, otherNode);
                                    
                                    wasConnected = true;
                                }
                            }
                        }
                        const Connections & inputs(aPort->getInputConnections());
                        
                        if (0 < inputs.size())
                        {
                            wasConnected = true;
                        }
                    }
                }
                if (! wasConnected)
                {
                    /*ogdf::edge phantomNodeToThis =*/ gg.newEdge(phantomNode, thisNode);
                    
                }
            }
        }
        // Apply an energy-based layout
        ogdf::FMMMLayout fmmm;
        
        fmmm.useHighLevelOptions(true);
        fmmm.newInitialPlacement(false); //true);
        fmmm.qualityVersusSpeed(ogdf::FMMMLayout::qvsGorgeousAndEfficient);
        fmmm.allowedPositions(ogdf::FMMMLayout::apAll);
        fmmm.initialPlacementMult(ogdf::FMMMLayout::ipmAdvanced);
        fmmm.initialPlacementForces(ogdf::FMMMLayout::ipfKeepPositions);
        fmmm.repForcesStrength(2.0);
        fmmm.call(ga);
        for (ContainerList::const_iterator it(_entitiesSeen.begin()); _entitiesSeen.end() != it;
             ++it)
        {
            ChannelContainer * anEntity = *it;
            
            if (anEntity)
            {
                ogdf::node aNode = anEntity->getNode();
                
                if (aNode)
                {
                    anEntity->setTopLeftPosition(ga.x(aNode), ga.y(aNode));
                }
            }
        }
    }
    OD_LOG_OBJEXIT(); //####
} // ScannerThread::setEntityPositions

bool ScannerThread::updateActivePanel(ChannelsPanel * newPanel)
{
    OD_LOG_OBJENTER(); //####
    OD_LOG_P1("newPanel = ", newPanel); //####
    // Because this is a background thread, we mustn't do any UI work without first grabbing a
    // MessageManagerLock.
    bool                     result = true;
    const MessageManagerLock mml(Thread::getCurrentThread());
    
    // If something is trying to kill this job, the lock will fail, in which case we'd better
    // return.
    if (mml.lockWasGained())
    {
        ChannelsPanel & activePanel = getActivePanel();
        
        activePanel.clearAllVisitedFlags();
        newPanel->clearAllVisitedFlags();
        // Retrieve each entity from our new list; if it is known already, ignore it but mark the
        // old entity as known.
        for (int ii = 0, mm = newPanel->getNumberOfEntities(); mm > ii; ++ii)
        {
            ChannelContainer * anEntity = newPanel->getEntity(ii);
            
            if (anEntity)
            {
                ChannelContainer * oldEntity = activePanel.findKnownEntity(anEntity->getName());
                
                if (oldEntity)
                {
                    oldEntity->setVisited();
                }
                else
                {
                    // Make a copy of the newly discovered entity, and add it to the active panel.
                    ChannelContainer * newEntity = new ChannelContainer(anEntity->getKind(),
                                                                        anEntity->getName(),
                                                                        anEntity->getBehaviour(),
                                                                        anEntity->getDescription(),
                                                                        activePanel);
                    
                    newEntity->setVisited();
                    newEntity->setTopLeftPosition(anEntity->getPosition());
                    // Make copies of the ports of the entity, and add them to the new entity.
                    for (int ii = 0, mm = anEntity->getNumPorts(); mm > ii; ++ii)
                    {
                        ChannelEntry * aPort = anEntity->getPort(ii);
                        
                        if (aPort)
                        {
                            ChannelEntry * newPort = newEntity->addPort(aPort->getPortName(),
                                                                        aPort->getProtocol(),
                                                                        aPort->getUsage(),
                                                                        aPort->getDirection());
                            
                            activePanel.rememberPort(newPort);
                        }
                    }
                    activePanel.addEntity(newEntity);
                    anEntity->setVisited();
                }
            }
        }
        // Now, add the connections from the 'visited' (i.e., new) entities:
        for (int ii = 0, mm = newPanel->getNumberOfEntities(); mm > ii; ++ii)
        {
            ChannelContainer * anEntity = newPanel->getEntity(ii);
            
            if (anEntity && anEntity->wasVisited())
            {
                ChannelContainer * newEntity = activePanel.findKnownEntity(anEntity->getName());
                
                if (newEntity)
                {
                    // Now, we have the connections to establish in 'anEntity' and the entity to be
                    // updated in 'newEntity'.
                    for (int ii = 0, mm = anEntity->getNumPorts(); mm > ii; ++ii)
                    {
                        ChannelEntry * aPort = anEntity->getPort(ii);
                        
                        if (aPort)
                        {
                            ChannelEntry * newPort =
                                                    activePanel.findKnownPort(aPort->getPortName());
                        
                            OD_LOG_P1("newPort <- ", newPort); //####
                        }
                    }
                }
            }
        }
        activePanel.removeUnvisitedEntities();
        result = false;
    }
    delete newPanel;
    OD_LOG_OBJEXIT_B(result); //####
    return result;
} // ScannerThread::updateActivePanel

#if defined(__APPLE__)
# pragma mark Accessors
#endif // defined(__APPLE__)

#if defined(__APPLE__)
# pragma mark Global functions
#endif // defined(__APPLE__)