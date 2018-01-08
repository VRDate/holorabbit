using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.IO;
using UnityEngine.UI;

using System.Threading;
#if WINDOWS_UWP
using System.Threading.Tasks;
#endif
using System.Linq;


public class RabbitClient : MonoBehaviour {


    public string userName = "hololens";
    public string userPassword = "hololens";

    bool isConnected = false;
    IntPtr serverConnection;
    private System.Action _callbacks;

    public bool IsConnected
    {
        get { return isConnected; }
    }

#if WINDOWS_UWP
    Task messageConsumerTask;
#endif


    string serverAddress = "192.168.0.29";
    int serverPort = 5672;

    public enum ConnectStatus
    {
        Success = 0,
        FailedToConnect,
        FailedToLogin,
    }

    public class RabbitData
    {
        public byte[] data;
        public bool isUsed;
        public string routingkey;

        public RabbitData()
        {
            data = new byte[275000];
            isUsed = false;
            routingkey = "";
        }
    };

    public ConnectStatus Connect(string serverAddress, int serverPort, string userName, string userPassword)
    {
        // Try to establish server connection
        this.serverAddress = serverAddress;
        this.serverPort = serverPort;
        this.userName = userName;
        this.userPassword = userPassword;

        if ((serverConnection = RabbitNativeMethods.CreateConnection(serverAddress, serverPort)) == IntPtr.Zero)
        {
            return ConnectStatus.FailedToConnect;
        }

        Debug.Log("Connected to Rabbit Server at " + serverAddress + ":" + serverPort.ToString());

        // Login to the rabbitmq server using the provided credentials
        if (!RabbitNativeMethods.Login(serverConnection, userName, userPassword))
        {
            return ConnectStatus.FailedToLogin;
        }

        isConnected = true;

#if WINDOWS_UWP
        // Start the thread for the message receiver
        messageConsumerTask = new Task( ConsumeMessageFromQueue );
        messageConsumerTask.Start(); 
#endif

        return ConnectStatus.Success;
    }

    public void Invoke(System.Action callback)
    {
        lock (_lock)
        {
            _callbacks = callback;
        }
    }
    private object _lock = new object();


    void ConsumeMessageFromQueue()
    {
        uint readAmt = 0;
        uint headerAmt = 0;
                
        byte[] header = new byte[500];

        RabbitData currentRabbitData = new RabbitData();

        while (isConnected)
        {
            // Find a data chunk that is available for loading the message
            bool messageReceived = false;
                
            lock (_lock)
            {
                messageReceived = RabbitNativeMethods.ConsumeMessage(serverConnection, ref headerAmt, header, ref readAmt, currentRabbitData.data);
            }
            // the code that you want to measure comes here

            if (messageReceived)
            {
                if (readAmt == 0) continue;
                currentRabbitData.isUsed = true;

                currentRabbitData.routingkey = System.Text.Encoding.UTF8.GetString(header, 0, (int)headerAmt);

                lock (_lock)
                {
                    if (!dataDict.ContainsKey(currentRabbitData.routingkey))
                    {
                        dataDict[currentRabbitData.routingkey] = new RabbitData();
                    }

                    currentRabbitData.data.CopyTo(dataDict[currentRabbitData.routingkey].data, 0);
                }
            }
        }
    }

    public void OnApplicationQuit()
    {

    }

    public void SendRabbitMessage( string routingKey, string message )
    {
        if (!isConnected) return;
        RabbitNativeMethods.SendString(serverConnection, "stealth", routingKey, message);
    }


    public delegate bool SubscribeDelegate(string routingKey, byte[] data);
    SubscribeDelegate OnSubscribeReceived { get; set; }

    Dictionary<string, SubscribeDelegate> funcDict = new Dictionary<string, SubscribeDelegate>();

    public Dictionary<string, RabbitData> dataDict = new Dictionary<string, RabbitData>();

    public bool SubscribeToRoutingKey(string routingKey, SubscribeDelegate callback)
    {
        if (!isConnected) return false;

        lock (_lock)
        {
            if (RabbitNativeMethods.ConnectToExchange(serverConnection, "stealth", routingKey) == IntPtr.Zero)
                Debug.LogWarning("Failed to connect to exchange");
            funcDict[routingKey] = callback;
        }

        return true;
    }

    #region Singleton Methods

    private static RabbitClient instance;

    private RabbitClient() { }

    public static RabbitClient Instance
    {
        get
        {
            if (instance == null)
                instance = GameObject.FindObjectOfType(typeof(RabbitClient)) as RabbitClient;
            return instance;
        }
    }

#endregion
}

