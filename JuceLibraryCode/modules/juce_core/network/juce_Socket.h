/*
  ==============================================================================

   This file is part of the juce_core module of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission to use, copy, modify, and/or distribute this software for any purpose with
   or without fee is hereby granted, provided that the above copyright notice and this
   permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
   NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
   IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ------------------------------------------------------------------------------

   NOTE! This permissive ISC license applies ONLY to files within the juce_core module!
   All other JUCE modules are covered by a dual GPL/commercial license, so if you are
   using any other modules, be sure to check that you also comply with their license.

   For more details, visit www.juce.com

  ==============================================================================
*/

#ifndef JUCE_SOCKET_H_INCLUDED
#define JUCE_SOCKET_H_INCLUDED


//==============================================================================
/**
    A wrapper for a streaming (TCP) socket.

    This allows low-level use of sockets; for an easier-to-use messaging layer on top of
    sockets, you could also try the InterprocessConnection class.

    @see DatagramSocket, InterprocessConnection, InterprocessConnectionServer
*/
class JUCE_API  StreamingSocket
{
public:
    //==============================================================================
    /** Creates an uninitialised socket.

        To connect it, use the connect() method, after which you can read() or write()
        to it.

        To wait for other sockets to connect to this one, the createListener() method
        enters "listener" mode, and can be used to spawn new sockets for each connection
        that comes along.
    */
    StreamingSocket();

    /** Destructor. */
    ~StreamingSocket();

    //==============================================================================
    /** Binds the socket to the specified local port.

        @return    true on success; false may indicate that another socket is already bound
                    on the same port
    */
    bool bindToPort (int localPortNumber);

    /** Binds the socket to the specified local port and local address.

        If localAddress is not an empty string then the socket will be bound to localAddress
        as well. This is useful if you would like to bind your socket to a specific network
        adapter. Note that localAddress must be an IP address assigned to one of your
        network address otherwise this function will fail.
        @return    true on success; false may indicate that another socket is already bound
                    on the same port
        @see bindToPort(int localPortNumber), IPAddress::findAllAddresses
    */
    bool bindToPort (int localPortNumber, const String& localAddress);

    /** Returns the local port number to which this socket is currently bound.

        This is useful if you need to know to which port the OS has actually bound your
        socket when calling the constructor or bindToPort with zero as the
        localPortNumber argument. Returns -1 if the function fails. */
    int getBoundPort() const noexcept;

    /** Tries to connect the socket to hostname:port.

        If timeOutMillisecs is 0, then this method will block until the operating system
        rejects the connection (which could take a long time).

        @return true if it succeeds.
        @see isConnected
    */
    bool connect (const String& remoteHostname,
                  int remotePortNumber,
                  int timeOutMillisecs = 3000);

    /** True if the socket is currently connected. */
    bool isConnected() const noexcept                           { return connected; }

    /** Closes the connection. */
    void close();

    /** Returns the name of the currently connected host. */
    const String& getHostName() const noexcept                  { return hostName; }

    /** Returns the port number that's currently open. */
    int getPort() const noexcept                                { return portNumber; }

    /** True if the socket is connected to this machine rather than over the network. */
    bool isLocal() const noexcept;

    /** Returns the OS's socket handle that's currently open. */
    int getRawSocketHandle() const noexcept                     { return handle; }

    //==============================================================================
    /** Waits until the socket is ready for reading or writing.

        If readyForReading is true, it will wait until the socket is ready for
        reading; if false, it will wait until it's ready for writing.

        If the timeout is < 0, it will wait forever, or else will give up after
        the specified time.

        If the socket is ready on return, this returns 1. If it times-out before
        the socket becomes ready, it returns 0. If an error occurs, it returns -1.
    */
    int waitUntilReady (bool readyForReading,
                        int timeoutMsecs) const;

    /** Reads bytes from the socket.

        If blockUntilSpecifiedAmountHasArrived is true, the method will block until
        maxBytesToRead bytes have been read, (or until an error occurs). If this
        flag is false, the method will return as much data as is currently available
        without blocking.

        @return the number of bytes read, or -1 if there was an error.
        @see waitUntilReady
    */
    int read (void* destBuffer, int maxBytesToRead,
              bool blockUntilSpecifiedAmountHasArrived);

    /** Writes bytes to the socket from a buffer.

        Note that this method will block unless you have checked the socket is ready
        for writing before calling it (see the waitUntilReady() method).

        @return the number of bytes written, or -1 if there was an error.
    */
    int write (const void* sourceBuffer, int numBytesToWrite);

    //==============================================================================
    /** Puts this socket into "listener" mode.

        When in this mode, your thread can call waitForNextConnection() repeatedly,
        which will spawn new sockets for each new connection, so that these can
        be handled in parallel by other threads.

        @param portNumber       the port number to listen on
        @param localHostName    the interface address to listen on - pass an empty
                                string to listen on all addresses
        @return    true if it manages to open the socket successfully.

        @see waitForNextConnection
    */
    bool createListener (int portNumber, const String& localHostName = String());

    /** When in "listener" mode, this waits for a connection and spawns it as a new
        socket.

        The object that gets returned will be owned by the caller.

        This method can only be called after using createListener().

        @see createListener
    */
    StreamingSocket* waitForNextConnection() const;

private:
    //==============================================================================
    String hostName;
    int volatile portNumber, handle;
    bool connected, isListener;
    mutable CriticalSection readLock;

    StreamingSocket (const String& hostname, int portNumber, int handle);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StreamingSocket)
};


//==============================================================================
/**
    A wrapper for a datagram (UDP) socket.

    This allows low-level use of sockets; for an easier-to-use messaging layer on top of
    sockets, you could also try the InterprocessConnection class.

    @see StreamingSocket, InterprocessConnection, InterprocessConnectionServer
*/
class JUCE_API  DatagramSocket
{
public:
    //==============================================================================
    /**
        Creates a datagram socket.

        You first need to bind this socket to a port with bindToPort if you intend to read
        from this socket.

        If enableBroadcasting is true, the socket will be allowed to send broadcast messages
        (may require extra privileges on linux)
    */
    DatagramSocket (bool enableBroadcasting = false);


    /** Destructor. */
    ~DatagramSocket();

    //==============================================================================
    /** Binds the socket to the specified local port.

        The localPortNumber is the port on which to bind this socket. If this value is 0,
        the port number is assigned by the operating system.

        @return    true on success; false may indicate that another socket is already bound
                    on the same port
    */
    bool bindToPort (int localPortNumber);

    /** Binds the socket to the specified local port and local address.

        If localAddress is not an empty string then the socket will be bound to localAddress
        as well. This is useful if you would like to bind your socket to a specific network
        adapter. Note that localAddress must be an IP address assigned to one of your
        network address otherwise this function will fail.
        @return    true on success; false may indicate that another socket is already bound
                    on the same port
        @see bindToPort(int localPortNumber), IPAddress::findAllAddresses
    */
    bool bindToPort (int localPortNumber, const String& localAddress);

    /** Returns the local port number to which this socket is currently bound.

        This is useful if you need to know to which port the OS has actually bound your
        socket when bindToPort was called with zero.

        Returns -1 if the socket didn't bind to any port yet or an error occured. */
    int getBoundPort() const noexcept;

    /** Returns the OS's socket handle that's currently open. */
    int getRawSocketHandle() const noexcept                     { return handle; }

    //==============================================================================
    /** Waits until the socket is ready for reading or writing.

        If readyForReading is true, it will wait until the socket is ready for
        reading; if false, it will wait until it's ready for writing.

        If the timeout is < 0, it will wait forever, or else will give up after
        the specified time.

        If the socket is ready on return, this returns 1. If it times-out before
        the socket becomes ready, it returns 0. If an error occurs, it returns -1.
    */
    int waitUntilReady (bool readyForReading,
                        int timeoutMsecs) const;

    /** Reads bytes from the socket.

        If blockUntilSpecifiedAmountHasArrived is true, the method will block until
        maxBytesToRead bytes have been read, (or until an error occurs). If this
        flag is false, the method will return as much data as is currently available
        without blocking.

        @return the number of bytes read, or -1 if there was an error.
        @see waitUntilReady
    */
    int read (void* destBuffer, int maxBytesToRead,
              bool blockUntilSpecifiedAmountHasArrived);

    /** Reads bytes from the socket and return the IP address of the sender.

        If blockUntilSpecifiedAmountHasArrived is true, the method will block until
        maxBytesToRead bytes have been read, (or until an error occurs). If this
        flag is false, the method will return as much data as is currently available
        without blocking.

        @return the number of bytes read, or -1 if there was an error. On a successful
                 result, the senderIPAddress value will be set to the IP of the sender.
        @see waitUntilReady
    */
    int read (void* destBuffer, int maxBytesToRead,
              bool blockUntilSpecifiedAmountHasArrived,
              String& senderIPAddress, int& senderPortNumber);

    /** Writes bytes to the socket from a buffer.

        Note that this method will block unless you have checked the socket is ready
        for writing before calling it (see the waitUntilReady() method).

        @return the number of bytes written, or -1 if there was an error.
    */
    int write (const String& remoteHostname, int remotePortNumber,
               const void* sourceBuffer, int numBytesToWrite);

    /** Closes the underlying socket object.

        Closes the underlying socket object and aborts any read or write operations.
        Note that all other methods will return an error after this call. This
        method is useful if another thread is blocking in a read/write call and you
        would like to abort the read/write thread. Simply deleting the socket
        object without calling shutdown may cause a race-condition where the read/write
        returns just before the socket is deleted and the subsequent read/write would
        try to read from an invalid pointer. By calling shutdown first, the socket
        object remains valid but all current and subsequent calls to read/write will
        return immediately.
    */
    void shutdown();

    //==============================================================================
    /** Join a multicast group

        @return true if it succeeds.
    */
    bool joinMulticast (const String& multicastIPAddress);

    /** Leave a multicast group

        @return true if it succeeds.
    */
    bool leaveMulticast (const String& multicastIPAddress);

    //==============================================================================
    /** Allow other applications to re-use the port.

        Allow any other application currently running to bind to the same port.
        Do not use this if your socket handles sensitive data as it could be
        read by any, possibly malicious, third-party apps.

        Returns true on success.
    */
    bool setEnablePortReuse (bool enabled);

private:
    //==============================================================================
    int handle;
    bool isBound;
    String lastBindAddress, lastServerHost;
    int lastServerPort;
    void* lastServerAddress;
    mutable CriticalSection readLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DatagramSocket)
};


#endif   // JUCE_SOCKET_H_INCLUDED
