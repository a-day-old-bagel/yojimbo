/*
    Yojimbo Client/Server Network Library.

    Copyright © 2016 - 2024, Mas Bandwidth LLC.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YOJIMBO_H
#define YOJIMBO_H

#include "serialize.h"
#include "yojimbo_config.h"
#include "yojimbo_constants.h"
#include "yojimbo_bit_array.h"
#include "yojimbo_utils.h"
#include "yojimbo_queue.h"
#include "yojimbo_sequence_buffer.h"
#include "yojimbo_address.h"
#include "yojimbo_serialize.h"
#include "yojimbo_message.h"
#include "yojimbo_channel.h"
#include "yojimbo_reliable_ordered_channel.h"
#include "yojimbo_unreliable_unordered_channel.h"
#include "yojimbo_connection.h"
#include "yojimbo_network_simulator.h"
#include "yojimbo_adaptor.h"
#include "yojimbo_network_info.h"

/** @file */

// windows =p
#ifdef SendMessage
#undef SendMessage
#endif

struct netcode_address_t;
struct netcode_server_t;
struct netcode_client_t;
struct reliable_endpoint_t;

/// The library namespace.

namespace yojimbo
{
    using namespace serialize;
}

/**
    Initialize the yojimbo library.
    Call this before calling any yojimbo library functions.
    @returns True if the library was successfully initialized, false otherwise.
 */

bool InitializeYojimbo();

/**
    Shutdown the yojimbo library.
    Call this after you finish using the library and it will run some checks for you (for example, checking for memory leaks in debug build).
 */

void ShutdownYojimbo();

// ---------------------------------

namespace yojimbo
{
    /**
        The server interface.
     */

    class ServerInterface
    {
    public:

        virtual ~ServerInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Start the server and allocate client slots.
            Each client that connects to this server occupies one of the client slots allocated by this function.
            @param maxClients The number of client slots to allocate. Must be in range [1,MaxClients]
            @see Server::Stop
         */

        virtual void Start( int maxClients ) = 0;

        /**
            Stop the server and free client slots.
            Any clients that are connected at the time you call stop will be disconnected.
            When the server is stopped, clients cannot connect to the server.
            @see Server::Start.
         */

        virtual void Stop() = 0;

        /**
            Disconnect the client at the specified client index.
            @param clientIndex The index of the client to disconnect in range [0,maxClients-1], where maxClients is the number of client slots allocated in Server::Start.
            @see Server::IsClientConnected
         */

        virtual void DisconnectClient( int clientIndex ) = 0;

        /**
            Disconnect all clients from the server.
            Client slots remain allocated as per the last call to Server::Start, they are simply made available for new clients to connect.
         */

        virtual void DisconnectAllClients() = 0;

        /**
            Send packets to connected clients.
            This function drives the sending of packets that transmit messages to clients.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from connected clients.
            This function drives the procesing of messages included in packets received from connected clients.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance server time.
            Call this at the end of each frame to advance the server time forward.
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the server running?
            The server is running after you have called Server::Start. It is not running before the first server start, and after you call Server::Stop.
            Clients can only connect to the server while it is running.
            @returns true if the server is currently running.
         */

        virtual bool IsRunning() const = 0;

        /**
            Get the maximum number of clients that can connect to the server.
            Corresponds to the maxClients parameter passed into the last call to Server::Start.
            @returns The maximum number of clients that can connect to the server. In other words, the number of client slots.
         */

        virtual int GetMaxClients() const = 0;

        /**
            Is a client connected to a client slot?
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns True if the client is connected.
         */

        virtual bool IsClientConnected( int clientIndex ) const = 0;

        /**
            Get the unique id of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The unique id of the client.
         */

        virtual uint64_t GetClientId( int clientIndex ) const = 0;

        /**
            Get the address of the client
            @param clientIndex the index of the client slot in [0,maxClients-1], where maxClients corresponds to the value passed into the last call to Server::Start.
            @returns The address of the client.
         */

        virtual netcode_address_t * GetClientAddress( int clientIndex ) const = 0;

        /**
            Get the number of clients that are currently connected to the server.
            @returns the number of connected clients.
         */

        virtual int GetNumConnectedClients() const = 0;

        /**
            Gets the current server time.
            @see Server::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type for a specific client.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the message.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on the server.
         */

        virtual Message * CreateMessage( int clientIndex, int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param clientIndex The index of the client this message belongs to. Determines which client heap is used to allocate the data.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int clientIndex, int bytes ) = 0;

        /**
            Attach data block to message.
            @param clientIndex The index of the client this block belongs to.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param clientIndex The index of the client this block belongs to.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( int clientIndex, uint8_t * block ) = 0;

        /**
            Can we send a message to a particular client on a channel?
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int clientIndex, int channelIndex ) const = 0;

        /**
            Send a message to a client over a channel.
            @param clientIndex The index of the client to send a message to.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int clientIndex, int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a client over a channel.
            @param clientIndex The index of the client to receive messages from.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Server::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int clientIndex, int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Server::ReceiveMessage.
            @param clientIndex The index of the client that the message belongs to.
            @param message The message to release.
         */

        virtual void ReleaseMessage( int clientIndex, Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection, eg. round trip time, packet loss %, # of packets sent and so on.
            @param clientIndex The index of the client.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const = 0;

        /**
            Connect a loopback client.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param userData User data for this client. Optional. Pass NULL if not needed.
         */

        virtual void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData ) = 0;

        /**
            Disconnect a loopback client.
            Loopback clients are not disconnected by regular Disconnect or DisconnectAllClient calls. You need to call this function instead.
            @param clientIndex The index of the client to disconnect. Must already be a connected loopback client.
         */

        virtual void DisconnectLoopbackClient( int clientIndex ) = 0;

        /**
            Is this client a loopback client?
            @param clientIndex The client index.
            @returns true if the client is a connected loopback client, false otherwise.
         */

        virtual bool IsLoopbackClient( int clientIndex ) const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a client directly to the loopback client slot on the server.
            @param clientIndex The client index. Must be an already connected loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Common functionality across all server implementations.
     */

    class BaseServer : public ServerInterface
    {
    public:

        BaseServer( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseServer();

        void SetContext( void * context );

        void Start( int maxClients );

        void Stop();

        void AdvanceTime( double time );

        bool IsRunning() const { return m_running; }

        int GetMaxClients() const { return m_maxClients; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int clientIndex, int type );

        uint8_t * AllocateBlock( int clientIndex, int bytes );

        void AttachBlockToMessage( int clientIndex, Message * message, uint8_t * block, int bytes );

        void FreeBlock( int clientIndex, uint8_t * block );

        bool CanSendMessage( int clientIndex, int channelIndex ) const;

        bool HasMessagesToSend( int clientIndex, int channelIndex ) const;

        void SendMessage( int clientIndex, int channelIndex, Message * message );

        Message * ReceiveMessage( int clientIndex, int channelIndex );

        void ReleaseMessage( int clientIndex, Message * message );

        void GetNetworkInfo( int clientIndex, NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        Allocator & GetGlobalAllocator() { yojimbo_assert( m_globalAllocator ); return *m_globalAllocator; }

        MessageFactory & GetClientMessageFactory( int clientIndex );

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetClientEndpoint( int clientIndex );

        Connection & GetClientConnection( int clientIndex );

        virtual void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static int StaticProcessPacketFunction( void * context,int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );

        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                ///< Base client/server config.
        Allocator * m_allocator;                                    ///< Allocator passed in to constructor.
        Adapter * m_adapter;                                        ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                           ///< Optional serialization context.
        int m_maxClients;                                           ///< Maximum number of clients supported.
        bool m_running;                                             ///< True if server is currently running, eg. after "Start" is called, before "Stop".
        double m_time;                                              ///< Current server time in seconds.
        uint8_t * m_globalMemory;                                   ///< The block of memory backing the global allocator. Allocated with m_allocator.
        uint8_t * m_clientMemory[MaxClients];                       ///< The block of memory backing the per-client allocators. Allocated with m_allocator.
        Allocator * m_globalAllocator;                              ///< The global allocator. Used for allocations that don't belong to a specific client.
        Allocator * m_clientAllocator[MaxClients];                  ///< Array of per-client allocator. These are used for allocations related to connected clients.
        MessageFactory * m_clientMessageFactory[MaxClients];        ///< Array of per-client message factories. This silos message allocations per-client slot.
        Connection * m_clientConnection[MaxClients];                ///< Array of per-client connection classes. This is how messages are exchanged with clients.
        reliable_endpoint_t * m_clientEndpoint[MaxClients];         ///< Array of per-client reliable endpoints.
        NetworkSimulator * m_networkSimulator;                      ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        uint8_t * m_packetBuffer;                                   ///< Buffer used when writing packets.
    };

    /**
        Dedicated server implementation.
     */

    class Server : public BaseServer
    {
    public:

        Server( Allocator & allocator, const uint8_t privateKey[], const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Server();

        void Start( int maxClients );

        void Stop();

        void DisconnectClient( int clientIndex );

        void DisconnectAllClients();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        bool IsClientConnected( int clientIndex ) const;

        uint64_t GetClientId( int clientIndex ) const;

        netcode_address_t * GetClientAddress( int clientIndex ) const;

        int GetNumConnectedClients() const;

        void ConnectLoopbackClient( int clientIndex, uint64_t clientId, const uint8_t * userData );

        void DisconnectLoopbackClient( int clientIndex );

        bool IsLoopbackClient( int clientIndex ) const;

        void ProcessLoopbackPacket( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        void TransmitPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( int clientIndex, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void ConnectDisconnectCallbackFunction( int clientIndex, int connected );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticConnectDisconnectCallbackFunction( void * context, int clientIndex, int connected );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;
        netcode_server_t * m_server;
        Address m_address;                                  // original address passed to ctor
        Address m_boundAddress;                             // address after socket bind, eg. valid port
        uint8_t m_privateKey[KeyBytes];
    };

    /**
        The set of client states.
     */

    enum ClientState
    {
        CLIENT_STATE_ERROR = -1,
        CLIENT_STATE_DISCONNECTED = 0,
        CLIENT_STATE_CONNECTING,
        CLIENT_STATE_CONNECTED,
    };

    /**
        The common interface for all clients.
     */

    class ClientInterface
    {
    public:

        virtual ~ClientInterface() {}

        /**
            Set the context for reading and writing packets.
            This is optional. It lets you pass in a pointer to some structure that you want to have available when reading and writing packets via Stream::GetContext.
            Typical use case is to pass in an array of min/max ranges for values determined by some data that is loaded from a toolchain vs. being known at compile time.
            If you do use a context, make sure the same context data is set on client and server, and include a checksum of the context data in the protocol id.
         */

        virtual void SetContext( void * context ) = 0;

        /**
            Disconnect from the server.
         */

        virtual void Disconnect() = 0;

        /**
            Send packets to server.
         */

        virtual void SendPackets() = 0;

        /**
            Receive packets from the server.
         */

        virtual void ReceivePackets() = 0;

        /**
            Advance client time.
            Call this at the end of each frame to advance the client time forward.
            IMPORTANT: Please use a double for your time value so it maintains sufficient accuracy as time increases.
         */

        virtual void AdvanceTime( double time ) = 0;

        /**
            Is the client connecting to a server?
            This is true while the client is negotiation connection with a server.
            @returns true if the client is currently connecting to, but is not yet connected to a server.
         */

        virtual bool IsConnecting() const = 0;

        /**
            Is the client connected to a server?
            This is true once a client successfully finishes connection negotiatio, and connects to a server. It is false while connecting to a server.
            @returns true if the client is connected to a server.
         */

        virtual bool IsConnected() const = 0;

        /**
            Is the client in a disconnected state?
            A disconnected state corresponds to the client being in the disconnected, or in an error state. Both are logically "disconnected".
            @returns true if the client is disconnected.
         */

        virtual bool IsDisconnected() const = 0;

        /**
            Is the client in an error state?
            When the client disconnects because of an error, it enters into this error state.
            @returns true if the client is in an error state.
         */

        virtual bool ConnectionFailed() const = 0;

        /**
            Get the current client state.
         */

        virtual ClientState GetClientState() const = 0;

        /**
            Get the client index.
            The client index is the slot number that the client is occupying on the server.
            @returns The client index in [0,maxClients-1], where maxClients is the number of client slots allocated on the server in Server::Start.
         */

        virtual int GetClientIndex() const = 0;

        /**
            Get the client id.
            The client id is a unique identifier of this client.
            @returns The client id.
         */

        virtual uint64_t GetClientId() const = 0;

        /**
            Get the current client time.
            @see Client::AdvanceTime
         */

        virtual double GetTime() const = 0;

        /**
            Create a message of the specified type.
            @param type The type of the message to create. The message types corresponds to the message factory created by the adaptor set on this client.
         */

        virtual Message * CreateMessage( int type ) = 0;

        /**
            Helper function to allocate a data block.
            This is typically used to create blocks of data to attach to block messages. See BlockMessage for details.
            @param bytes The number of bytes to allocate.
            @returns The pointer to the data block. This must be attached to a message via Client::AttachBlockToMessage, or freed via Client::FreeBlock.
         */

        virtual uint8_t * AllocateBlock( int bytes ) = 0;

        /**
            Attach data block to message.
            @param message The message to attach the block to. This message must be derived from BlockMessage.
            @param block Pointer to the block of data to attach. Must be created via Client::AllocateBlock.
            @param bytes Length of the block of data in bytes.
         */

        virtual void AttachBlockToMessage( Message * message, uint8_t * block, int bytes ) = 0;

        /**
            Free a block of memory.
            @param block The block of memory created by Client::AllocateBlock.
         */

        virtual void FreeBlock( uint8_t * block ) = 0;

        /**
            Can we send a message on a channel?
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns True if a message can be sent over the channel, false otherwise.
         */

        virtual bool CanSendMessage( int channelIndex ) const = 0;

        /**
            Send a message on a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @param message The message to send.
         */

        virtual void SendMessage( int channelIndex, Message * message ) = 0;

        /**
            Receive a message from a channel.
            @param channelIndex The channel index in range [0,numChannels-1].
            @returns The message received, or NULL if no message is available. Make sure to release this message by calling Client::ReleaseMessage.
         */

        virtual Message * ReceiveMessage( int channelIndex ) = 0;

        /**
            Release a message.
            Call this for messages received by Client::ReceiveMessage.
            @param message The message to release.
         */

        virtual void ReleaseMessage( Message * message ) = 0;

        /**
            Get client network info.
            Call this to receive information about the client network connection to the server, eg. round trip time, packet loss %, # of packets sent and so on.
            @param info The struct to be filled with network info [out].
         */

        virtual void GetNetworkInfo( NetworkInfo & info ) const = 0;

        /**
            Connect to server over loopback.
            This allows you to have local clients connected to a server, for example for integrated server or singleplayer.
            @param clientIndex The index of the client.
            @param clientId The unique client id.
            @param maxClients The maximum number of clients supported by the server.
         */

        virtual void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients ) = 0;

        /**
            Disconnect from server over loopback.
         */

        virtual void DisconnectLoopback() = 0;

        /**
            Is this a loopback client?
            @returns true if the client is a loopback client, false otherwise.
         */

        virtual bool IsLoopback() const = 0;

        /**
            Process loopback packet.
            Use this to pass packets from a server directly to the loopback client.
            @param packetData The packet data to process.
            @param packetBytes The number of bytes of packet data.
            @param packetSequence The packet sequence number.
         */

        virtual void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence ) = 0;
    };

    /**
        Functionality that is common across all client implementations.
     */

    class BaseClient : public ClientInterface
    {
    public:

        /**
            Base client constructor.
            @param allocator The allocator for all memory used by the client.
            @param config The base client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
            @param allocator The adapter to the game program. Specifies allocators, message factory to use etc.
         */

        explicit BaseClient( Allocator & allocator, const ClientServerConfig & config, Adapter & adapter, double time );

        ~BaseClient();

        void SetContext( void * context ) { yojimbo_assert( IsDisconnected() ); m_context = context; }

        void Disconnect();

        void AdvanceTime( double time );

        bool IsConnecting() const { return m_clientState == CLIENT_STATE_CONNECTING; }

        bool IsConnected() const { return m_clientState == CLIENT_STATE_CONNECTED; }

        bool IsDisconnected() const { return m_clientState <= CLIENT_STATE_DISCONNECTED; }

        bool ConnectionFailed() const { return m_clientState == CLIENT_STATE_ERROR; }

        ClientState GetClientState() const { return m_clientState; }

        int GetClientIndex() const { return m_clientIndex; }

        double GetTime() const { return m_time; }

        void SetLatency( float milliseconds );

        void SetJitter( float milliseconds );

        void SetPacketLoss( float percent );

        void SetDuplicates( float percent );

        Message * CreateMessage( int type );

        uint8_t * AllocateBlock( int bytes );

        void AttachBlockToMessage( Message * message, uint8_t * block, int bytes );

        void FreeBlock( uint8_t * block );

        bool CanSendMessage( int channelIndex ) const;

        bool HasMessagesToSend( int channelIndex ) const;

        void SendMessage( int channelIndex, Message * message );

        Message * ReceiveMessage( int channelIndex );

        void ReleaseMessage( Message * message );

        void GetNetworkInfo( NetworkInfo & info ) const;

    protected:

        uint8_t * GetPacketBuffer() { return m_packetBuffer; }

        void * GetContext() { return m_context; }

        Adapter & GetAdapter() { yojimbo_assert( m_adapter ); return *m_adapter; }

        void CreateInternal();

        void DestroyInternal();

        void SetClientState( ClientState clientState );

        Allocator & GetClientAllocator() { yojimbo_assert( m_clientAllocator ); return *m_clientAllocator; }

        MessageFactory & GetMessageFactory() { yojimbo_assert( m_messageFactory ); return *m_messageFactory; }

        NetworkSimulator * GetNetworkSimulator() { return m_networkSimulator; }

        reliable_endpoint_t * GetEndpoint() { return m_endpoint; }

        Connection & GetConnection() { yojimbo_assert( m_connection ); return *m_connection; }

        virtual void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        virtual int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes ) = 0;

        static void StaticTransmitPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static int StaticProcessPacketFunction( void * context, int index, uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        static void * StaticAllocateFunction( void * context, uint64_t bytes );

        static void StaticFreeFunction( void * context, void * pointer );

    private:

        ClientServerConfig m_config;                                        ///< The client/server configuration.
        Allocator * m_allocator;                                            ///< The allocator passed to the client on creation.
        Adapter * m_adapter;                                                ///< The adapter specifies the allocator to use, and the message factory class.
        void * m_context;                                                   ///< Context lets the user pass information to packet serialize functions.
        uint8_t * m_clientMemory;                                           ///< The memory backing the client allocator. Allocated from m_allocator.
        Allocator * m_clientAllocator;                                      ///< The client allocator. Everything allocated between connect and disconnected is allocated and freed via this allocator.
        reliable_endpoint_t * m_endpoint;                                   ///< reliable endpoint.
        MessageFactory * m_messageFactory;                                  ///< The client message factory. Created and destroyed on each connection attempt.
        Connection * m_connection;                                          ///< The client connection for exchanging messages with the server.
        NetworkSimulator * m_networkSimulator;                              ///< The network simulator used to simulate packet loss, latency, jitter etc. Optional.
        ClientState m_clientState;                                          ///< The current client state. See ClientInterface::GetClientState
        int m_clientIndex;                                                  ///< The client slot index on the server [0,maxClients-1]. -1 if not connected.
        double m_time;                                                      ///< The current client time. See ClientInterface::AdvanceTime
        uint8_t * m_packetBuffer;                                           ///< Buffer used to read and write packets.

    private:

        BaseClient( const BaseClient & other );

        const BaseClient & operator = ( const BaseClient & other );
    };

    /**
        Implementation of client for dedicated servers.
     */

    class Client : public BaseClient
    {
    public:

        /**
            The client constructor.
            @param allocator The allocator for all memory used by the client.
            @param address The address the client should bind to.
            @param config The client/server configuration.
            @param time The current time in seconds. See ClientInterface::AdvanceTime
         */

        explicit Client( Allocator & allocator, const Address & address, const ClientServerConfig & config, Adapter & adapter, double time );

        ~Client();

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address & address );

        void InsecureConnect( const uint8_t privateKey[], uint64_t clientId, const Address serverAddresses[], int numServerAddresses );

        void Connect( uint64_t clientId, uint8_t * connectToken );

        void Disconnect();

        void SendPackets();

        void ReceivePackets();

        void AdvanceTime( double time );

        int GetClientIndex() const;

        uint64_t GetClientId() const { return m_clientId; }

        void ConnectLoopback( int clientIndex, uint64_t clientId, int maxClients );

        void DisconnectLoopback();

        bool IsLoopback() const;

        void ProcessLoopbackPacket( const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        const Address & GetAddress() const { return m_boundAddress; }

    private:

        bool GenerateInsecureConnectToken( uint8_t * connectToken,
                                           const uint8_t privateKey[],
                                           uint64_t clientId,
                                           const Address serverAddresses[],
                                           int numServerAddresses );

        void CreateClient( const Address & address );

        void DestroyClient();

        void StateChangeCallbackFunction( int previous, int current );

        static void StaticStateChangeCallbackFunction( void * context, int previous, int current );

        void TransmitPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        int ProcessPacketFunction( uint16_t packetSequence, uint8_t * packetData, int packetBytes );

        void SendLoopbackPacketCallbackFunction( int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        static void StaticSendLoopbackPacketCallbackFunction( void * context, int clientIndex, const uint8_t * packetData, int packetBytes, uint64_t packetSequence );

        ClientServerConfig m_config;                    ///< Client/server configuration.
        netcode_client_t * m_client;                    ///< netcode client data.
        Address m_address;                              ///< Original address passed to ctor.
        Address m_boundAddress;                         ///< Address after socket bind, eg. with valid port
        uint64_t m_clientId;                            ///< The globally unique client id (set on each call to connect)
    };
}

#endif // #ifndef YOJIMBO_H
