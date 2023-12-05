using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Config;

internal class Configuration
{
    // Private fields.
    private readonly Dictionary<string, string[]> _settings = new();


    // Constructors.
    internal Configuration() { }


    // Private methods.
    private (string Key, string value)? ReadLine(string line)
    {
        line = line.Trim();
        const char COMMENT_INDICATOR = '#';

        if (line.StartsWith(COMMENT_INDICATOR))
        {
            return null;
        }

        const char LINE_KEY_VALUE_SEPARATOR = '=';
        const int PARTS_IN_LINE = 2;
        string[] LineParts = line.Split(LINE_KEY_VALUE_SEPARATOR, StringSplitOptions.RemoveEmptyEntries);

        if (LineParts.Length != PARTS_IN_LINE)
        {
            return null;
        }

        return (LineParts[0], LineParts[1]);
    }

    private void AddContentEntry(string key, string value)
    {
        if (!_settings.ContainsKey(key))
        {
            _settings[key] = new string[] { value };
            return;
        }

        string[] ExistingContent = _settings[key];
        string[] NewContent = new string[ExistingContent.Length + 1];
        _settings[key] = NewContent;
    }



    // Internal methods.
    internal void ReadFromFile(string path)
    {
        if (path == null)
        {
            throw new ArgumentNullException(nameof(path));
        }

        _settings.Clear();
        string[] Lines = File.ReadAllLines(path);

        foreach (string Line in Lines)
        {
            (string Key, string Value)? LineContent = ReadLine(Line);
            if (LineContent == null) continue;

            AddContentEntry(LineContent.Value.Key, LineContent.Value.Value);
        }
    }

    internal string[] Get(string key)
    {
        _settings.TryGetValue(key, out string[]? Value);
        return Value ?? Array.Empty<string>();
    }

    internal string[]? GetOrDefault(string key, params string[]? defaultValues)
    {
        if (!_settings.TryGetValue(key, out string[]? Value))
        {
            return defaultValues;
        }
        return Value!;
    }
}