using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Listening;

internal class PageDataProvider
{
    // Internal static fields.
    internal static string EmptyPath = "/";


    // Internal fields.
    internal string SearchPattern { get; private init; }

    internal string[] Paths => _data.Keys.ToArray();


    // Protected fields.
    private readonly Dictionary<string, byte[]> _data = new();


    // Constructors.
    internal PageDataProvider(string fileExtension)
    {
        if (fileExtension == null)
        {
            throw new ArgumentNullException(nameof(fileExtension));
        }
        SearchPattern = '*' + fileExtension;
        LoadData();
    }


    // Internal static methods.
    internal static string FormatAsContentPath(string path) => '/' + path;


    // Internal methods.
    internal byte[]? GetData(string path)
    {
        _data.TryGetValue(path, out var Data);
        return Data;
    }

    internal string? GetDataAsText(string path)
    {
        byte[]? Data = GetData(path);

        if (Data != null)
        {
            return Server.Encoding.GetString(Data);
        }
        return null;
    }

    internal void LoadData()
    {
        _data.Clear();
        string CurrentPath = "Not Set";

        try
        {
            foreach (string FilePath in Directory.GetFiles(Server.RootPath, SearchPattern, SearchOption.AllDirectories))
            {
                CurrentPath = FilePath;

                byte[] PageData = File.ReadAllBytes(CurrentPath);
                string RelativePath = Path.GetRelativePath(Server.RootPath, CurrentPath);

                _data.Add(('/' + RelativePath).Replace('\\', '/'), PageData);
            }
        }
        catch (IOException e)
        {
            throw new Exception($"Failed to read file \"{CurrentPath}\". {e}");
        }
    }
}