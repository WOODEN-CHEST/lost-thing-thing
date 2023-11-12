using System.Collections.Concurrent;
using static System.Net.Mime.MediaTypeNames;


namespace LTTServer.Logging;


internal class Logger : IDisposable
{
    // Internal fields.
    internal string LogPath { get; private init; }

    internal const string LOG_SUBPATH = "logs";
    internal const string LOG_FILE_NAME = "latest.log";


    // Private fields.
    private readonly Task _loggerTask;
    private readonly ConcurrentQueue<string> _queuedMessages = new();
    private readonly AutoResetEvent _messageQueuedEvent = new(false);
    private bool _loggerDisposed = false;

    private readonly FileStream _logStream;


    // Constructors.
    internal Logger()
    {
        string LogDirectory = Path.Combine(Server.RootPath, LOG_SUBPATH);
        LogPath = Path.Combine(LogDirectory, LOG_FILE_NAME);

        if (!Directory.Exists(LogDirectory))
        {
            Directory.CreateDirectory(LogDirectory);
        }
        if (File.Exists(LogPath))
        {
            File.Delete(LogPath);
        }

        _logStream = File.Open(LogPath, FileMode.OpenOrCreate);
        _loggerTask = Task.Factory.StartNew(WriteMessagesTask, CancellationToken.None,
            TaskCreationOptions.LongRunning, TaskScheduler.Default);
    }




    // Internal methods.
    internal void Info(string message) => WriteMessage(LogLevel.Info, message);

    internal void Warning(string message) => WriteMessage(LogLevel.Warning, message);

    internal void Error(string message) => WriteMessage(LogLevel.Error, message);

    internal void Critical(string message) => WriteMessage(LogLevel.Critical, message);

    internal void WriteMessage(LogLevel level, string message)
    {
        if (message == null)
        {
            throw new ArgumentNullException(nameof(message));
        }

        string FormattedMessage = $"{GetFormattedDateAndTime(DateTime.Now)}[{level}] {message}{Environment.NewLine}";
        _queuedMessages.Enqueue(FormattedMessage);
        _messageQueuedEvent.Set();

        Console.ForegroundColor = level switch
        {
            LogLevel.Info => ConsoleColor.White,
            LogLevel.Warning => ConsoleColor.Yellow,
            LogLevel.Error => ConsoleColor.Red,
            LogLevel.Critical => ConsoleColor.Magenta,
            _ => ConsoleColor.White
        };
        Console.Write(FormattedMessage);
        Console.ForegroundColor = ConsoleColor.White;
    }

    // Private methods.
    private void WriteMessagesTask()
    {
        while (!_loggerDisposed)
        {
            if (_queuedMessages.Count == 0)
            {
                _messageQueuedEvent.WaitOne();
            }
            
            if (!_queuedMessages.TryDequeue(out string? Message))
            {
                continue;
            }

            _logStream.Write(Server.Encoding.GetBytes(Message));
            _logStream.Flush();
        }
    }
    

    private string GetFormattedDateAndTime(DateTime dateTime)
    {
        return $"[{(dateTime.Date.Day > 9 ? dateTime.Date.Day : $"0{dateTime.Date.Day}")}d;" +
            $"{(dateTime.Date.Month > 9 ? dateTime.Date.Month : $"0{dateTime.Date.Month}")}m;" +
            $"{(dateTime.Date.Year)}y]" +
            $"[{(dateTime.TimeOfDay.Hours > 9 ? dateTime.TimeOfDay.Hours : $"0{dateTime.TimeOfDay.Hours}")}:" +
            $"{(dateTime.TimeOfDay.Minutes > 9 ? dateTime.TimeOfDay.Minutes : $"0{dateTime.TimeOfDay.Minutes}")}:" +
            $"{(dateTime.TimeOfDay.Seconds > 9 ? dateTime.TimeOfDay.Seconds : $"0{dateTime.TimeOfDay.Seconds}")}]";
    }


    // Inherited methods.
    public void Dispose()
    {
        lock (this)
        {
            _loggerDisposed = true;
        }
        _messageQueuedEvent.Set();
        _loggerTask.Wait();

        _loggerTask.Dispose();
        _logStream?.Dispose();
    }
}