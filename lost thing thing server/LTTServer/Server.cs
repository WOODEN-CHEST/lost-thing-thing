using GHDataFile;
using LTTServer.CMDControl;
using LTTServer.Database;
using LTTServer.HTML;
using LTTServer.Listening;
using LTTServer.Logging;
using LTTServer.Search;
using LTTServer.Users;
using System.Net;
using System.Text;

namespace LTTServer;

internal static class Server
{
    // Internal static fields.
    internal static int RequestCount => ServerListener.RequestCount;

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

    internal static string SiteAddress { get; private set; } = "127.0.0.1";


    /* Components. */
    internal static DatabaseManager DatabaseManager { get; private set; }

    internal static Listener ServerListener { get; private set; }

    internal static ProfileManager ProfileManager;


    // Private static fields.

    private static CommandReader s_commandReader;
    private readonly static ServerStatus s_status = new() { IsStopped = false };
    private static Logger s_logger;
    
    


    // Static constructors.
    static Server()
    {
        StartupTime = DateTime.Now;
        RootPath = @"D:\Workstations\Programming\Projects\lost thing thing\content";

        s_logger = new();
        try
        {
            DatabaseManager = new();
            ProfileManager = new();

            ProfileInfo[] FoundProfiles = ProfileManager.GetProfiles("Am");

            if (!HttpListener.IsSupported)
            {
                OutputCritical("HttpListener is not supported.");
                return;
            }

            ServerListener = new($"http://{SiteAddress}/");
            s_commandReader = new();
        }
        catch (Exception e)
        {
            OutputCritical($"Failed to start server! {e}");
            return;
        }
    }


    // Internal static methods.
    internal static void Main(string[] args)
    { 
        ListenForMessages();
    }

    internal static void ReloadPageData() => ServerListener.ReloadPageData();
    internal static void ShutDown()
    {
        ProfileManager.Dispose();

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
            ServerListener.Listen();
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