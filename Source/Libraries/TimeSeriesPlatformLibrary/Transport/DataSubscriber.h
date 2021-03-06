//******************************************************************************************************
//  DataSubscriber.h - Gbtc
//
//  Copyright � 2010, Grid Protection Alliance.  All Rights Reserved.
//
//  Licensed to the Grid Protection Alliance (GPA) under one or more contributor license agreements. See
//  the NOTICE file distributed with this work for additional information regarding copyright ownership.
//  The GPA licenses this file to you under the Eclipse Public License -v 1.0 (the "License"); you may
//  not use this file except in compliance with the License. You may obtain a copy of the License at:
//
//      http://www.opensource.org/licenses/eclipse-1.0.php
//
//  Unless agreed to in writing, the subject software distributed under the License is distributed on an
//  "AS-IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. Refer to the
//  License for the specific language governing permissions and limitations.
//
//  Code Modification History:
//  ----------------------------------------------------------------------------------------------------
//  03/26/2012 - Stephen C. Wills
//       Generated original version of source code.
//  03/22/2018 - J. Ritchie Carroll
//		 Updated DataSubscriber callback function signatures to always include instance reference.
//
//******************************************************************************************************

#ifndef __DATA_SUBSCRIBER_H
#define __DATA_SUBSCRIBER_H

#include <string>
#include <vector>

#include "../Common/Types.h"
#include "../Common/EndianConverter.h"
#include "../Common/ThreadSafeQueue.h"
#include "Types.h"
#include "SignalIndexCache.h"

using namespace boost::asio;

namespace GSF {
namespace TimeSeries {
namespace Transport
{
    class DataSubscriber;

    // Simple exception type thrown by the data subscriber.
    class SubscriberException : public Exception
    {
    private:
        string m_message;

    public:
        SubscriberException(const string& message) noexcept
        {
            m_message = message;
        }

        const char* what() const noexcept
        {
            return &m_message[0];
        }
    };

    // Info structure used to configure subscriptions.
    struct SubscriptionInfo
    {
        string FilterExpression;
        void(*NewMeasurementsCallback)(DataSubscriber*, const vector<MeasurementPtr>&);

        bool RemotelySynchronized;
        bool Throttled;

        bool UdpDataChannel;
        uint16_t DataChannelLocalPort;

        bool IncludeTime;
        float64_t LagTime;
        float64_t LeadTime;
        bool UseLocalClockAsRealTime;
        bool UseMillisecondResolution;

        string StartTime;
        string StopTime;
        string ConstraintParameters;
        int32_t ProcessingInterval;

        string ExtraConnectionStringParameters;

        SubscriptionInfo()
            : NewMeasurementsCallback(nullptr),
            RemotelySynchronized(false),
            Throttled(false),
            UdpDataChannel(false),
            DataChannelLocalPort(9500),
            IncludeTime(true),
            LagTime(10.0),
            LeadTime(5.0),
            UseLocalClockAsRealTime(false),
            UseMillisecondResolution(false),
            ProcessingInterval(-1)
        {
        }
    };

    // Helper class to provide retry and auto-reconnect functionality to the subscriber.
    class SubscriberConnector
    {
    private:
        typedef void(*ErrorMessageCallback)(DataSubscriber*, const string&);
        typedef void(*ReconnectCallback)(DataSubscriber*);

        ErrorMessageCallback m_errorMessageCallback;
        ReconnectCallback m_reconnectCallback;

        string m_hostname;
        uint16_t m_port;

        int m_maxRetries;
        int m_retryInterval;
        bool m_autoReconnect;

        bool m_cancel;

        // Auto-reconnect handler.
        static void AutoReconnect(DataSubscriber* source);

    public:
        // Creates a new instance.
        SubscriberConnector();

        // Registers a callback to provide error messages each time
        // the subscriber fails to connect during a connection sequence.
        void RegisterErrorMessageCallback(ErrorMessageCallback errorMessageCallback);

        // Registers a callback to notify after an automatic reconnection attempt has been made.
        // This callback will be called whether the connection was successful or not, so it is
        // recommended to check the connected state of the subscriber using the IsConnected() method.
        void RegisterReconnectCallback(ReconnectCallback reconnectCallback);

        // Begin connection sequence.
        bool Connect(DataSubscriber& subscriber);

        // Cancel all current and
        // future connection sequences.
        void Cancel();

        // Set the hostname of the publisher to connect to.
        void SetHostname(const string& hostname);

        // Set the port that the publisher is listening on.
        void SetPort(uint16_t port);

        // Set the maximum number of retries during a connection sequence.
        void SetMaxRetries(int maxRetries);

        // Sets the interval of idle time (in milliseconds) between connection attempts.
        void SetRetryInterval(int retryInterval);

        // Sets flag that determines whether the subscriber should
        // automatically attempt to reconnect when the connection is terminated.
        void SetAutoReconnect(bool autoReconnect);

        // Getters for configurable settings.
        string GetHostname() const;
        uint16_t GetPort() const;
        int GetMaxRetries() const;
        int GetRetryInterval() const;
        bool GetAutoReconnect() const;
    };

    class DataSubscriber
    {
    private:
        static const int MaxPacketSize = 32768;
        static const size_t PayloadHeaderSize = 8;

        // Function pointer types
        typedef void(*DispatcherFunction)(DataSubscriber*, const vector<uint8_t>&);
        typedef void(*MessageCallback)(DataSubscriber*, const string&);
        typedef void(*DataStartTimeCallback)(DataSubscriber*, int64_t);
        typedef void(*MetadataCallback)(DataSubscriber*, const vector<uint8_t>&);
        typedef void(*NewMeasurementsCallback)(DataSubscriber*, const vector<MeasurementPtr>&);
        typedef void(*ConfigurationChangedCallback)(DataSubscriber*);
        typedef void(*ConnectionTerminatedCallback)(DataSubscriber*);

        // Structure used to dispatch
        // callbacks on the callback thread.
        struct CallbackDispatcher
        {
            DataSubscriber* Source;
            vector<uint8_t> Data;
            DispatcherFunction Function;
        };

        SubscriberConnector m_connector;
        SubscriptionInfo m_currentSubscription;
        EndianConverter m_endianConverter;
        IPAddress m_hostAddress;
        bool m_compressMetadata;
        bool m_disconnecting;
        void* m_userData;

        // Statistics counters
        long m_totalCommandChannelBytesReceived;
        long m_totalDataChannelBytesReceived;
        long m_totalMeasurementsReceived;
        bool m_connected;
        bool m_subscribed;

        // Measurement parsing
        SignalIndexCache m_signalIndexCache;
        size_t m_timeIndex;
        int64_t m_baseTimeOffsets[2];

        // Callback thread members
        Thread m_callbackThread;
        ThreadSafeQueue<CallbackDispatcher> m_callbackQueue;

        // Command channel
        Thread m_commandChannelResponseThread;
        io_service m_commandChannelService;
        TcpSocket m_commandChannelSocket;
        vector<uint8_t> m_readBuffer;
        vector<uint8_t> m_writeBuffer;

        // Data channel
        Thread m_dataChannelResponseThread;
        io_service m_dataChannelService;
        UdpSocket m_dataChannelSocket;

        // Callbacks
        MessageCallback m_statusMessageCallback;
        MessageCallback m_errorMessageCallback;
        DataStartTimeCallback m_dataStartTimeCallback;
        MetadataCallback m_metadataCallback;
        NewMeasurementsCallback m_newMeasurementsCallback;
        MessageCallback m_processingCompleteCallback;
        ConfigurationChangedCallback m_configurationChangedCallback;
        ConnectionTerminatedCallback m_connectionTerminatedCallback;
        ConnectionTerminatedCallback m_autoReconnectCallback;

        // Threads
        void RunCallbackThread();
        void RunCommandChannelResponseThread();
        void RunDataChannelResponseThread();

        // Command channel callbacks
        void ReadPayloadHeader(const ErrorCode& error, size_t bytesTransferred);
        void ReadPacket(const ErrorCode& error, size_t bytesTransferred);

        // Server response handlers
        void HandleSucceeded(uint8_t commandCode, uint8_t* data, size_t offset, size_t length);
        void HandleFailed(uint8_t commandCode, uint8_t* data, size_t offset, size_t length);
        void HandleMetadataRefresh(uint8_t* data, size_t offset, size_t length);
        void HandleDataPacket(uint8_t* data, size_t offset, size_t length);
        void HandleDataStartTime(uint8_t* data, size_t offset, size_t length);
        void HandleProcessingComplete(uint8_t* data, size_t offset, size_t length);
        void HandleUpdateSignalIndexCache(uint8_t* data, size_t offset, size_t length);
        void HandleUpdateBaseTimes(uint8_t* data, size_t offset, size_t length);
        void HandleConfigurationChanged(uint8_t* data, size_t offset, size_t length);
        void ProcessServerResponse(uint8_t* buffer, size_t offset, size_t length);

        // Dispatchers
        void Dispatch(DispatcherFunction function);
        void Dispatch(DispatcherFunction function, const uint8_t* data, size_t offset, size_t length);
        void DispatchStatusMessage(const string& message);
        void DispatchErrorMessage(const string& message);

        static void StatusMessageDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void ErrorMessageDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void DataStartTimeDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void MetadataDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void NewMeasurementsDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void ProcessingCompleteDispatcher(DataSubscriber* source, const vector<uint8_t>& data);
        static void ConfigurationChangedDispatcher(DataSubscriber* source, const vector<uint8_t>& data);

        // The connection terminated callback is a special case that
        // must be called on its own separate thread so that it can
        // safely close all sockets and stop all subscriber threads
        // (including the callback thread) before executing the callback.
        void ConnectionTerminatedDispatcher();
        
        void Disconnect(bool autoReconnect);

    public:
        // Creates a new instance of the data subscriber.
        DataSubscriber(bool compressMetadata = true);

        // Releases all threads and sockets
        // tied up by the subscriber.
        ~DataSubscriber();

        // Callback registration
        //
        // Callback functions are defined with the following signatures:
        //   void ProcessStatusMessage(string message)
        //   void ProcessErrorMessage(string message)
        //   void ProcessDataStartTime(TimeSeriesFramework::int64_t startTime)
        //   void ProcessMetadata(vector<TimeSeriesFramework::uint8_t> metadata)
        //   void ProcessNewMeasurements(vector<TimeSeriesFramework::Measurement> newMeasurements)
        //   void ProcessProcessingComplete(string message)
        //   void ProcessConnectionTerminated()
        //
        // Metadata is provided to the user as zlib-compressed XML,
        // and must be decompressed and interpreted before it can be used.
        void RegisterStatusMessageCallback(MessageCallback statusMessageCallback);
        void RegisterErrorMessageCallback(MessageCallback errorMessageCallback);
        void RegisterDataStartTimeCallback(DataStartTimeCallback dataStartTimeCallback);
        void RegisterMetadataCallback(MetadataCallback metadataCallback);
        void RegisterNewMeasurementsCallback(NewMeasurementsCallback newMeasurementsCallback);
        void RegisterProcessingCompleteCallback(MessageCallback processingCompleteCallback);
        void RegisterConfigurationChangedCallback(ConfigurationChangedCallback configurationChangedCallback);
        void RegisterConnectionTerminatedCallback(ConnectionTerminatedCallback connectionTerminatedCallback);
        void RegisterAutoReconnectCallback(ConnectionTerminatedCallback autoReconnectCallback);

        // Gets or sets value that determines whether
        // the metadata transfer is compressed.
        bool IsMetadataCompressed() const;
        void SetMetadataCompressed(bool compressed);

        // Gets or sets user defined data reference
        void* GetUserData() const;
        void SetUserData(void* userData);

        SubscriberConnector& GetSubscriberConnector();

        // Synchronously connects to publisher.
        void Connect(string hostname, uint16_t port);

        // Disconnects from the publisher.
        //
        // The method does not return until all connections have been
        // closed and all threads spawned by the subscriber have shut
        // down gracefully (with the exception of the thread that
        // executes the connection terminated callback).
        void Disconnect();

        // Subscribe to measurements to start receiving data.
        void Subscribe(SubscriptionInfo info);
        SubscriptionInfo GetCurrentSubscription() const;

        // Cancel the current subscription to stop receiving data.
        void Unsubscribe();

        // Send a command to the server.
        //
        // Command codes can be found in the "Constants.h" header file.
        // They are defined as:
        //   ServerCommand::Authenticate
        //   ServerCommand::MetadataRefresh
        //   ServerCommand::Subscribe
        //   ServerCommand::Unsubscribe
        //   ServerCommand::RotateCipherKeys
        //   ServerCommand::UpdateProcessingInterval
        //   ServerCommand::DefineOperationalModes
        //   ServerCommand::ConfirmNotification
        //   ServerCommand::ConfirmBufferBlock
        //   ServerCommand::PublishCommandMeasurements
        void SendServerCommand(uint8_t commandCode);
        void SendServerCommand(uint8_t commandCode, string message);
        void SendServerCommand(uint8_t commandCode, const uint8_t* data, size_t offset, size_t length);

        // Convenience method to send the currently defined and/or supported
        // operational modes to the server. Supported operational modes are
        // UTF-8 encoding, common serialization format, and optional metadata compression.
        void SendOperationalModes();

        // Functions for statistics gathering
        long GetTotalCommandChannelBytesReceived() const;
        long GetTotalDataChannelBytesReceived() const;
        long GetTotalMeasurementsReceived() const;
        bool IsConnected() const;
        bool IsSubscribed() const;
    };
}}}

#endif