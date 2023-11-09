using LTTServer.CMDControl;
using LTTServer.HTML;
using LTTServer.Listening;
using LTTServer.Logging;
using System.Net;
using System.Text;

namespace LTTServer;

internal static class Server
{
    // Internal static fields.
    internal static int RequestCount => s_serverListener.RequestCount;

    internal static DateTime StartupTime { get; private set; }

    internal static Encoding Encoding { get; private set; } = Encoding.UTF8;

    internal static string RootPath { get; private set; }

    internal static bool IsServerStopped
    {
        get
        {
            lock (s_status)
            {
                return s_status.IsStopped;
            }
        }
    }


    // Private static fields.
    private static Listener s_serverListener;
    private static CommandReader s_commandReader;
    private readonly static ServerStatus s_status = new() { IsStopped = false };
    private static Logger s_logger;


    // Internal static methods.
    internal static void Main(string[] args)
    {
        StartupTime = DateTime.Now;
        RootPath = @"D:/Workstations/Programming/Projects/lost thing thing";

        s_logger = new();

        if (!HttpListener.IsSupported)
        {
            OutputCritical("HttpListener is not supported.");
            return;
        }

        try
        {
            s_serverListener = new("http://localhost/", "http://127.0.0.1/");
        }
        catch (Exception e)
        {
            OutputError($"Failed to create server listener: {e}");
            return;
        }

        s_commandReader = new();
        ListenForMessages();
    }

    internal static void ReloadPageData() => s_serverListener.ReloadPageData();
    internal static void ShutDown()
    {
        OutputInfo("Server stopped");
        s_logger.Dispose();
        lock (s_status)
        {
            s_status.IsStopped = true;
        }
    }


    /* Output. */
    internal static void OutputInfo(string text) => s_logger.Info(text);

    internal static void OutputWarning(string text) => s_logger.Warning(text);

    internal static void OutputError(string text) => s_logger.Error(text);

    internal static void OutputCritical(string text) => s_logger.Critical(text);


    // Private static methods.
    private static void ListenForMessages()
    {
        OutputInfo("Server started");

        try
        {
            s_serverListener.Listen();
            s_commandReader.ReadCommand();
        }
        catch (Exception e)
        {
            OutputCritical(e.ToString());
        }

        if (!s_status.IsStopped)
        {
            ShutDown();
        }
        
    }
}