using GHDataFile;
using LTTServer.CMDControl;
using LTTServer.Database;
using LTTServer.HTML;
using LTTServer.Listening;
using LTTServer.Logging;
using LTTServer.Users;
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

    internal static string SiteAddress { get; private set; } = "127.0.0.1";


    // Private static fields.
    private static Listener s_serverListener;
    private static CommandReader s_commandReader;
    private readonly static ServerStatus s_status = new() { IsStopped = false };
    private static Logger s_logger;
    private static DatabaseManager _databaseManager;
    private static ProfileManager _userManager;


    // Internal static methods.
    internal static void Main(string[] args)
    { 
        StartupTime = DateTime.Now;
        RootPath = @"D:/Workstations/Programming/Projects/lost thing thing";

        s_logger = new();
        _databaseManager = new();
        _userManager = new();

        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhujibdyhukabyuyhukjkl@aaa");
        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhuajibdyhukabyuyhukjkl@aaa");
        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhugjibdyhukabyuyhukjkl@aaa");
        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhujqribdyhukabyuyhukjkl@aaa");
        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhuhjibdyhukabyuyhukjkl@aaa");
        _userManager.CreateUnverifiedProfile("name", "surname", "aaaaaa", "adbhujifgwbdyhukabyuyhukjkl@aaa");
        _userManager.ForceVerifyProfiles();

        if (!HttpListener.IsSupported)
        {
            OutputCritical("HttpListener is not supported.");
            return;
        }

        try
        {
            s_serverListener = new($"http://{SiteAddress}/");
        }
        catch (Exception e)
        {
            OutputCritical($"Failed to create server listener: {e}");
            return;
        }

        s_commandReader = new();
        ListenForMessages();
    }

    internal static void ReloadPageData() => s_serverListener.ReloadPageData();
    internal static void ShutDown()
    {
        _userManager.Dispose();

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