using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Listening;

internal sealed class BasicFilePageDataProvider : IFilePageDataProvider
{
    // Internal fields.
    public string SearchPattern { get; private init; }

    public string[] Paths => PageData.Keys.ToArray();


    // Protected fields.
    protected Dictionary<string, byte[]> PageData { get; set; } = new();


    // Constructors.
    internal BasicFilePageDataProvider(string fileExtension)
    {
        if (fileExtension == null)
        {
            throw new ArgumentNullException(nameof(fileExtension));
        }
        SearchPattern = '*' + fileExtension;
        LoadData();
    }


    // Internal methods.
    public byte[]? GetData(string path, HttpListenerContext context)
    {
        PageData.TryGetValue(path, out var Data);
        return Data;
    }

    public string? GetDataAsText(string path, HttpListenerContext context)
    {
        byte[]? Data = GetData(path, context);

        if (Data != null)
        {
            return Server.Encoding.GetString(Data);
        }
        return null;
    }

    public void LoadData()
    {
        PageData.Clear();
        string CurrentPath = "Not Set";

        try
        {
            foreach (string FilePath in Directory.GetFiles(Server.RootPath, SearchPattern, SearchOption.AllDirectories))
            {
                CurrentPath = FilePath;

                byte[] PageDataBytes = File.ReadAllBytes(CurrentPath);
                string RelativePath = Path.GetRelativePath(Server.RootPath, CurrentPath);

                PageData.Add(('/' + RelativePath).Replace('\\', '/'), PageDataBytes);
            }
        }
        catch (IOException e)
        {
            throw new Exception($"Failed to read file \"{CurrentPath}\". {e}");
        }
    }
}