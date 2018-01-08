using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using UnityEngine;

internal static class RabbitNativeMethods
{
    [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
    internal static extern IntPtr LoadLibrary(
        string lpFileName
    );

    [DllImport("user32.dll", EntryPoint = "MessageBox", CharSet = CharSet.Unicode)]
    internal static extern Int32 MessageBox(Int32 hWnd, String lpText, String lpCaption,
                      UInt32 uType);

    [DllImport("EasyRabbitWrap", EntryPoint = "CreateConnection")]
    public static extern IntPtr CreateConnection(string host, int port);

    [DllImport("EasyRabbitWrap", EntryPoint = "Login")]
    public static extern bool Login(IntPtr conn, string user, string pass);

    [DllImport("EasyRabbitWrap", EntryPoint = "SendString")]
    public static extern bool SendString(IntPtr conn, string exchange, string routingkey, string messagebody);

    [DllImport("EasyRabbitWrap", EntryPoint = "ConsumeMessage")]
    public static extern bool ConsumeMessage(IntPtr conn, ref UInt32 headerSize, [In, Out] byte[] headerData, ref UInt32 ReadSize, [In, Out] byte[] ReadData);

    [DllImport("EasyRabbitWrap", EntryPoint = "SendRPC")]
    public static extern bool SendRPC(IntPtr conn, string exchange, string routingkey, out UInt32 ReadSize, byte[] ReadData);

    [DllImport("EasyRabbitWrap", EntryPoint = "ConnectToExchange")]
    public static extern IntPtr ConnectToExchange(IntPtr conn, string exchange, string routingkey);

    [DllImport("StealthLinkCWrap", EntryPoint = "CreateServer")]
    public static extern IntPtr CreateServer(string host, string port);

}