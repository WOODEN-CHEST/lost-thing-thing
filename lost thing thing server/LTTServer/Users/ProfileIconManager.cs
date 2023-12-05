using SkiaSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Users;

internal class ProfileIconManager : IDisposable
{
    // Internal static fields.
    internal const string ICON_DIR_NAME = "profile_icons";
    internal const string THUMBNAIL_DIR_NAME = "profile_thumbnails";
    internal const string IMAGE_EXTENSION = ".png";


    // Internal fields.
    internal string IconDir { get; private init; }

    internal string ThumbnailDir { get; private init; }

    internal string DefaultIconPath { get; private init; }

    internal string DefaultThumbnailPath { get; private init; }


    // Private fields.
    private const int CACHED_ICON_COUNT = 50;

    private Dictionary<ulong, byte[]> _cachedThumbnails = new(CACHED_ICON_COUNT);
    private Dictionary<ulong, byte[]> _cachedIcons = new(CACHED_ICON_COUNT);


    // Constructors.
    internal ProfileIconManager(string usersPath, string defaultFileName)
    {
        if (string.IsNullOrWhiteSpace(usersPath))
        {
            throw new ArgumentException(nameof(usersPath));
        }
        if (string.IsNullOrWhiteSpace(defaultFileName))
        {
            throw new ArgumentException(nameof(usersPath));
        }

        IconDir = Path.Combine(Server.RootPath, ICON_DIR_NAME);
        ThumbnailDir = Path.Combine(Server.RootPath, THUMBNAIL_DIR_NAME);
        DefaultIconPath = Path.Combine(Server.RootPath, defaultFileName);
        DefaultThumbnailPath = Path.Combine(Server.RootPath, defaultFileName);
    }


    // Internal methods.
    internal byte[]? GetIcon(ulong id)
    {
        if (!_cachedIcons.TryGetValue(id, out byte[]? Data))
        {
            return LoadIcon(id);
        }
        if (Data == null)
        {
            Data = File.ReadAllBytes(DefaultIconPath);
        }

        return Data;
    }

    internal byte[]? GetThumbnail(ulong id)
    {
        if (!_cachedThumbnails.TryGetValue(id, out byte[]? Data))
        {
            return LoadThumbnail(id);
        }
        if (Data == null)
        {
            Data = File.ReadAllBytes(DefaultThumbnailPath);
        }

        return Data;
    }



    // Private methods.
    private byte[]? LoadIcon(ulong id) => LoadImage(id, IconDir, _cachedIcons);

    private byte[]? LoadThumbnail(ulong id) => LoadImage(id, ThumbnailDir, _cachedThumbnails);

    private byte[]? LoadImage(ulong id, string directoryPath, Dictionary<ulong, byte[]> targetContainer)
    {
        // Verify args.
        if (directoryPath == null)
        {
            throw new ArgumentNullException(nameof(directoryPath));
        }
        if (targetContainer == null)
        {
            throw new ArgumentNullException(nameof(targetContainer));
        }

        // Load data from file.
        string IconPath = Path.Combine(directoryPath, id.ToString() + IMAGE_EXTENSION);

        if (!File.Exists(IconPath))
        {
            return null;
        }

        byte[] Data;
        try
        {
            Data = File.ReadAllBytes(IconPath);
        }
        catch (IOException e)
        {
            Server.OutputError($"Failed to load image with the id {id} from directory \"{directoryPath}\". {e}");
            return null;
        }

        // Add to container.
        if (targetContainer.Count >= CACHED_ICON_COUNT)
        {
            targetContainer.Remove(targetContainer.Keys.First());
        }
        targetContainer.Add(id, Data);

        return Data;
    }


    // Inherited methods.
    public void Dispose()
    {
        _cachedIcons.Clear();
        _cachedThumbnails.Clear();
    }
}