using LTTServer.Listening;
using System.Net;
using System.Text;

namespace LTTServer;

internal class Server
{
    // Internal static fields.
    internal static Listener ServerListener { get; private set; }

    internal static Encoding Encoding { get; private set; } = Encoding.UTF8;

    internal static string RootPath { get; private set; }


    // Internal static methods.
    internal static void Main(string[] args)
    {
        if (!HttpListener.IsSupported)
        {
            OutputError("HttpListener is not supported.");
            return;
        }

        RootPath = @"D:/Workstations/Programming/Projects/lost thing thing";

        try
        {
            ServerListener = new("http://localhost/", "http://127.0.0.1/");
        }
        catch (Exception e)
        {
            OutputError($"Failed to create server listener: {e}");
            return;
        }

        ListenForMessages();
    }

    internal static void OutputWarning(string text)
    {
        if (text == null)
        {
            throw new ArgumentNullException(nameof(text));
        }

        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.WriteLine(text);
        Console.ForegroundColor = ConsoleColor.White;
    }

    internal static void OutputError(string text)
    {
        if (text == null)
        {
            throw new ArgumentNullException(nameof(text));
        }

        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine(text);
        Console.ForegroundColor = ConsoleColor.White;
    }


    // Private static methods.
    private static void ListenForMessages()
    {
        while (true)
        {
            try
            {
                ServerListener.Listen();
            }
            catch (Exception e)
            {
                OutputError(e.ToString());
            }
        }
    }
}