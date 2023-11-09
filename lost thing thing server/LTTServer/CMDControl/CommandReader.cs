using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.CMDControl;

internal class CommandReader
{
    // Constructors.
    internal CommandReader() { }


    // Internal methods.
    internal void ReadCommand()
    {
        while (!Server.IsServerStopped)
        {
            string? Input = Console.ReadLine();

            if (Input == null)
            {
                continue;
            }

            string[] Args = Input.ToLower().Split(' ');
            if (Args.Length == 0)
            {
                continue;
            }

            Server.OutputInfo($"{{Input}} {Input}");
            ExecuteCommand(Args);
        }
    }


    // Private methods.
    private void ExecuteCommand(string[] args)
    {
        const string CMD_STOP = "stop";
        const string CMD_RELOAD = "reload";
        const string CMD_QUERY = "query";
        const string CMD_HELP = "help";

        switch (args[0])
        {
            case CMD_STOP:
                ExecuteStop(args);
                break;

            case CMD_RELOAD:
                ExecuteReload(args);
                break;

            case CMD_QUERY:
                ExecuteQuery(args);
                break;

            case CMD_HELP:
                ExecuteHelp();
                break;

            default:
                Server.OutputWarning($"Unknown command \"{args[0]}\"." +
                    $"Type \"help\" for a list of commands.");
                break;
        }
    }

    /* Commands. */
    private void ExecuteStop(string[] args)
    {
        if (args.Length > 1)
        {
            ErrorTrailingArgs(args);
            return;
        }

        Server.ShutDown();
    }

    private void ExecuteReload(string[] args)
    {
        if (args.Length > 1)
        {
            ErrorTrailingArgs(args);
            return;
        }

        Server.OutputInfo("Reloading page data...");
        Server.ReloadPageData();
        Server.OutputInfo("Reloaded");
    }

    private void ExecuteQuery(string[] args)
    {
        const string ARG_UPTIME = "uptime";
        const string ARG_REQUESTS = "requests";


        if (args.Length > 2)
        {
            ErrorTrailingArgs(args);
            return;
        }
        else if (args.Length == 1)
        {
            ErrorMissingArgs(new string[] { ARG_UPTIME, ARG_REQUESTS });
            return;
        }

        switch (args[1])
        {
            case ARG_UPTIME:
                TimeSpan Uptime = DateTime.Now - Server.StartupTime;
                Server.OutputInfo($"Uptime: {Uptime.Days} days, {Uptime.Hours} hours, " +
                    $"{Uptime.Minutes} minutes, {Uptime.Seconds} seconds");
                break;

            case ARG_REQUESTS:
                Server.OutputInfo($"Total HTTP requests received: {Server.RequestCount}");
                break;

            default:
                ErrorUnknownArg(args[1]);
                break;
        }
    }

    private void ExecuteHelp()
    {
        Server.OutputInfo($"Commands available:{Environment.NewLine}" +
            $"help (Shows this){Environment.NewLine}" +
            $"stop (stops the server){Environment.NewLine}" +
            $"reload (reloads all server data like HTML pages, CSS style sheets, JavaScript scripts and PNG images){Environment.NewLine}" +
            "query [uptime | requests] (Queries info about the server)");
    }

    /* Errors. */
    private void ErrorTrailingArgs(string[] args)
    {
        Server.OutputWarning($"Trailing command arguments -> \"{args[^1]}\"");
    }

    private void ErrorMissingArgs(string[] solutions)
    {
        Server.OutputWarning($"Missing command arguments, expected [{string.Join(" | ", solutions)}]");
    }

    private void ErrorUnknownArg(string arg)
    {
        Server.OutputWarning($"Unknown argument \"{arg}\"");
    }
}