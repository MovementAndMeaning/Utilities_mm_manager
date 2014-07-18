//--------------------------------------------------------------------------------------------------
//
//  File:       ScannerThread.h
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

#if (! defined(ScannerThread_H_))
# define ScannerThread_H_ /* Header guard */

# include "ChannelEntry.h"
# include "ChannelsPanel.h"

# if defined(__APPLE__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
# endif // defined(__APPLE__)
/*! @file
 
 @brief The class declaration for the background port and service scanner. */
# if defined(__APPLE__)
#  pragma clang diagnostic pop
# endif // defined(__APPLE__)

namespace ChannelManager
{
    class ChannelsPanel;
    class ChannelsWindow;
    
    /*! @brief A background scanner thread. */
    class ScannerThread : public Thread
    {
    public:
        
        /*! @brief The constructor.
         @param name The name to give to the thread.
         @param window The window to be updated. */
        ScannerThread(const String &   name,
                      ChannelsWindow * window);
        
        /*! @brief The destructor. */
        virtual ~ScannerThread(void);
        
        /*! @brief Perform the background scan. */
        virtual void run(void);
        
    protected:
        
    private:
        
        /*! @brief The class that this class is derived from. */
        typedef Thread inherited;
        
        /*! @brief Add the detected entities to a panel.
         @param newPanel The panel to be updated. */
        void addEntitiesToPanel(ChannelsPanel * newPanel);
        
        /*! @brief Add connections between detected ports in the to-be-displayed list.
         @param detectedPorts The set of detected YARP ports. */
        void addPortConnections(const MplusM::Utilities::PortVector & detectedPorts);
        
        /*! @brief Add ports that have associates as 'adapter' entities to the to-be-displayed list.
         @param detectedPorts The set of detected YARP ports. */
        void addPortsWithAssociates(const MplusM::Utilities::PortVector & detectedPorts);
        
        /*! @brief Add regular YARP ports as distinct entities to the to-be-displayed list.
         @param detectedPorts The set of detected YARP ports. */
        void addRegularPortEntities(const MplusM::Utilities::PortVector & detectedPorts);
        
        /*! @brief Add services as distinct entities to the list of entities.
         @param services The set of detected services. */
        void addServices(const MplusM::Common::StringVector & services);
        
        /*! @brief Identify the YARP network entities. */
        void gatherEntities(void);
        
        /*! @brief Returns the active (displayed) panel.
         @returns The active (displayed) panel. */
        ChannelsPanel & getActivePanel(void)
        const;
        
        /*! @brief Set the entity positions. */
        void setEntityPositions(void);
        
        /*! @brief Refresh the displayed entities, based on the scanned entities.
         @param newPanel The panel containing the scanned entities.
         @returns @c true if the thread should leave and @c false otherwise. */
        bool updateActivePanel(ChannelsPanel * newPanel);
        
        /*! @brief The window to be updated. */
        ChannelsWindow * _window;
        
        /*! @brief A set of known ports. */
        PortSet _rememberedPorts;
        
        /*! @brief A set of known services. */
        ServiceMap _detectedServices;
        
        /*! @brief A set of associated ports. */
        AssociatesMap _associatedPorts;
        
        /*! @brief A set of standalone ports. */
        PortMap _standalonePorts;
        
        /*! @brief A set of connections. */
        ConnectionList _connections;
        
        /*! @brief A set of entities. */
        ContainerList _entitiesSeen;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScannerThread)
        
    }; // ScannerThread
    
} // ChannelManager

#endif // ! defined(ScannerThread_H_)