using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.HTML;

internal class HTMLReader
{
    // Private fields.
    private string? _path;
    private StreamReader _reader;

    // Constructors.
    internal HTMLReader() { }

    

    // Internal methods.
    internal HtmlDocument ReadFile(string path)
    {
        if (string.IsNullOrEmpty(path))
        {
            throw new ArgumentException($"Invalid HTML file path: \"{path}\"", nameof(path));
        }
        if (!Path.IsPathFullyQualified(path))
        {
            path = Path.Combine(Server.RootPath, path);
        }
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("HTML file does not exits.", path);
        }

        _path = path;
        try
        {
            using (_reader = new StreamReader(_path))
            {
                return ReadFileData();
            }
        }
        catch (IOException e)
        {
            throw new IOException($"Failed to read HTML file \"{_path}\". {e}");
        }
    }


    // Private methods.
    private HtmlDocument ReadFileData()
    {
        if (!VerifyDocType())
        {
            throw new HTMLFileException("Missing <!DOCTYPE html>", _path);
        }

        HtmlDocument Document = new(true);
        HtmlElement? RootElement = ReadElement();

        if (RootElement == null || RootElement.TagName != "html")
        {
            throw new HTMLFileException("First tag must be a <html> tag", _path);
        }
        Document.HTMLRoot = RootElement;
        Document.Body = RootElement.GetFirstSubElementOfTag("body");
        Document.Head = RootElement.GetFirstSubElementOfTag("head");
    }

    private string[] SplitByTags()
    {
        if (!ReadUntilNonWS())
        {
            return Array.Empty<string>();
        }

        List<string> Parts = new();
        StringBuilder CurrentPart = new();
        bool InsideTag = false;
        char Character;

        while (_reader.BaseStream.Position < _reader.BaseStream.Length)
        {
            Character = ReadChar();
            CurrentPart.Append(Character);
        }




        while (_reader.BaseStream.Position < _reader.BaseStream.Length)
        {
            Character = ReadChar();

            if (!InsideTag)
            {
                if (char.IsWhiteSpace(Character))

                    if (Character == '<')
                    {
                        InsideTag = true;
                        continue;
                    }

                throw new HTMLFileException($"Unexpected character \'{Character}\'", _path);
            }

            if (Character != '>')
            {
                CurrentTag.Append(Character);
                continue;
            }

            InsideTag = false;
            Tags.Add(CurrentTag.ToString());
        }

        if (InsideTag)
        {
            throw new HTMLFileException("Found unclosed tag.", _path);
        }

        return Tags.ToArray();
    }

    private bool VerifyDocType()
    {
        HtmlElement? DocElement = ReadElement();

        return DocElement != null && DocElement.TagName == "!DOCTYPE"
            && DocElement.Attributes.ContainsKey("html");
    }

    private HtmlElement? ReadElement()
    {
        SkipPastComments();

        if (!ReadUntilNonWS())
        {
            return null;
        }

        int Character;
        if ((Character = ReadChar()) != '<')
        {
            throw new HTMLFileException($"Expected start of tag, got \"{Character}\"", _path);
        }

        string TagName = ReadASCIIWord();
        if (TagName == string.Empty)
        {
            throw new HTMLFileException("Missing tag name", _path);
        }

        (string Name, string? Value)[] Attributes = ReadTagAttributes();
    }

    private bool IsTagAComment()
    {
        long CurrentPosition = _reader.BaseStream.Position;

        if (ReadChar() != '!' || ReadChar() != '-' || ReadChar() != '-')
        {
            _reader.BaseStream.Position = CurrentPosition;
            return false;
        }
        return true;
    }

    private void SkipPastComments()
    {
        while (true)
        {
            if (!ReadUntilNonWS())
            {
                return;
            }

            if (ReadChar() != '<')
            {
                _reader!.BaseStream.Position--;
                return;
            }
            if (!IsTagAComment())
            {
                _reader!.BaseStream.Position--;
                return;
            }

            StringBuilder CommentEnd = new();


        }
    }

    private (string, string?)[] ReadTagAttributes()
    {
        List<(string name, string? value)> Attributes = new();
        char Character = ReadChar();

        while (Character != '>')
        {
            Character = ReadChar();

            if (!char.IsWhiteSpace(Character))
            {
                _reader!.BaseStream.Position--;
                Attributes.Add(ReadAttribute());
            }
        }

        return Attributes.ToArray();
    }

    private (string, string?) ReadAttribute()
    {
        StringBuilder AttributeName
        char Character = ReadChar();

        while (char.IsAsciiLetter(Character))
        {

        }
    }


    /* Read helper methods. */
    private bool ReadUntilNonWS()
    {
        int IntChar;

        while ((IntChar = _reader!.Read()) != -1)
        {
            if (char.IsWhiteSpace((char)IntChar))
            {
                continue;
            }

            _reader.BaseStream.Position--;
            return true;
        }

        return false;
    }

    private char ReadChar()
    {
        int IntChar = _reader!.Read();

        if (IntChar == -1)
        {
            throw new HTMLFileException("Unexpected end of file", _path);
        }

        return (char)IntChar;
    }

    private string ReadString()
    {
        StringBuilder Result = new();

        ReadUntilNonWS();
        if ((ReadChar() is not '\'' and not '"'))
        {
            throw new HTMLFileException("Expected a string.", _path);
        }

        char Character = ReadChar();

        while (Character is not '\'' and not '"')
        {
            Result.Append(Character);
            Character = ReadChar();
        }

        return Result.ToString();
    }

    private string ReadWord()
    {
        StringBuilder Word = new();

        ReadUntilNonWS();
        char Character = ReadChar();

        while (!char.IsWhiteSpace(Character))
        {
            Word.Append(Character);
            Character = ReadChar();
        }

        _reader!.BaseStream.Position--;
        return Character.ToString();
    }

    private string ReadASCIIWord()
    {
        StringBuilder Word = new();

        ReadUntilNonWS();
        char Character = ReadChar();

        while (char.IsAsciiLetter(Character))
        {
            Word.Append(Character);
            Character = ReadChar();
        }

        _reader!.BaseStream.Position--;
        return Character.ToString();
    }

    private string ReadStringUntil(params char[] targets)
    {
        if (targets == null)
        {
            throw new ArgumentNullException(nameof(targets));
        }

        StringBuilder Result = new();

        ReadUntilNonWS();
        char Character = ReadChar();
        while (!targets.Contains(Character))
        {
            Result.Append(Character);
            Character = ReadChar();
        }

        _reader!.BaseStream.Position--;
        return Result.ToString();
    }

    private void ReadUntil(params char[] targets)
    {
        if (targets == null)
        {
            throw new ArgumentNullException(nameof(targets));
        }

        while (!targets.Contains(ReadChar()))
        {
            continue;
        }

        _reader!.BaseStream.Position--;
    }
}