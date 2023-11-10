using LTTServer.HTML;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Listening;

internal class HTMLProvider : IPageDataProvider
{
    // Fields.
    public string SearchPattern { get; private set; }

    public string[] Paths => _pages.Keys.ToArray();


    // Private fields.
    private Dictionary<string, HtmlDocument> _pages = new();


    // Constructors.
    internal HTMLProvider(string fileExtension)
    {
        if (fileExtension == null)
        {
            throw new ArgumentNullException(nameof(fileExtension));
        }
        SearchPattern = '*' + fileExtension;
        LoadData();
    }

    

    public byte[]? GetData(string path)
    {
        _pages.TryGetValue(path, out HtmlDocument? Document);

        if (Document != null)
        {
            return Server.Encoding.GetBytes(Document.ToString());
        }

        return null;
    }

    public string? GetDataAsText(string path)
    {
        _pages.TryGetValue(path, out HtmlDocument? Document);
        return Document?.ToString();
    }

    public void LoadData()
    {
        _pages.Clear();
        string CurrentPath = "Not Set";

        try
        {
            foreach (string FilePath in Directory.GetFiles(Server.RootPath, SearchPattern, SearchOption.AllDirectories))
            {
                CurrentPath = FilePath;
                string RelativePath = Path.GetRelativePath(Server.RootPath, CurrentPath);

                _pages.Add(('/' + RelativePath).Replace('\\', '/'), HtmlDocument.FromFile(FilePath));
            }
        }
        catch (IOException e)
        {
            throw new Exception($"Failed to read HTML file \"{CurrentPath}\". {e}");
        }
    }
}