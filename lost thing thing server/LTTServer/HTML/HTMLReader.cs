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
    private string _htmlText;
    private int _index;


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
            throw new FileNotFoundException("HTML file does not exist.", path);
        }

        _path = path;
        try
        {
            _htmlText = File.ReadAllText(path);
            return ReadFileData();
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
            throw new HtmlFileException("Missing <!DOCTYPE html>", _path);
        }

        HtmlDocument Document = new(true);
        HtmlElement? RootElement = ReadElement();

        if (RootElement == null || RootElement.TagName != "html")
        {
            throw new HtmlFileException("First tag must be a <html> tag", _path);
        }
        Document.HTMLRoot = RootElement;
        Document.Body = RootElement.GetFirstSubElementOfTag("body");
        Document.Head = RootElement.GetFirstSubElementOfTag("head");

        return Document;
    }

    private bool VerifyDocType()
    {
        HtmlElement? DocElement = ReadElement();

        return DocElement != null && DocElement.TagName == "!doctype"
            && DocElement.Attributes.ContainsKey("html");
    }

    private HtmlElement? ReadElement()
    {
        SkipPastComments();

        if (!ReadUntilNonWS())
        {
            return null;
        }

        char Character;
        if ((Character = ReadChar()) != '<')
        {
            throw new HtmlFileException($"Expected start of tag, got \"{Character}\"", _path);
        }

        string TagName = ReadLetterWord('!');
        if (TagName == string.Empty)
        {
            throw new HtmlFileException("Missing tag name", _path);
        }
        (string Name, string? Value)[] Attributes = ReadTagAttributes();

        SkipPastComments();

        HtmlElement Element = new(TagName);
        foreach (var Attribute in Attributes)
        {
            Element.AddAttribute(Attribute.Name,Attribute.Value);
        }

        if (HtmlElement.IsTagVoid(TagName.ToLower()))
        {
            return Element;
        }

        (string? TextContent, HtmlElement[] SubElements) = ReadTagContents();
        ReadTagEnd(TagName);

        foreach (var SubElement in SubElements)
        {
            Element.AddSubElement(SubElement);
        }
        Element.Content = TextContent;
        return Element;
    }

    private bool IsTagAComment()
    {
        int CurrentPosition = _index;

        if (ReadChar() != '!' || ReadChar() != '-' || ReadChar() != '-')
        {
            _index = CurrentPosition;
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
                _index--;
                return;
            }
            if (!IsTagAComment())
            {
                _index--;
                return;
            }

            StringBuilder CommentEnd = new();
            while (true)
            {
                char Character = ReadChar();

                if (Character is '-' or '>')
                {
                    CommentEnd.Append(Character);
                }
                else
                {
                    CommentEnd.Clear();
                }

                if (CommentEnd.ToString() == "-->")
                {
                    break;
                }
            }
        }
    }

    private (string, string?)[] ReadTagAttributes()
    {
        List<(string name, string? value)> Attributes = new();
        char Character = ReadChar();

        while (Character != '>')
        {
            if (!char.IsWhiteSpace(Character))
            {
                _index--;
                Attributes.Add(ReadAttribute());
            }

            Character = ReadChar();
        }

        return Attributes.ToArray();
    }

    private (string, string?) ReadAttribute()
    {
        string AttributeName = ReadLetterWord();

        if (string.IsNullOrWhiteSpace(AttributeName))
        {
            throw new HtmlFileException("Empty attribute name", _path);
        }

        ReadUntilNonWS();

        if (ReadChar() != '=')
        {
            _index--;
            return (AttributeName, null);
        }

        string AttributeValue = ReadString();
        return (AttributeName, AttributeValue);
    }

    private (string?, HtmlElement[]) ReadTagContents()
    {
        string TextValue = string.Empty;
        List<HtmlElement> SubElements = new();
        
        while (true)
        {
            SkipPastComments();
            ReadUntilNonWS();
            if (ReadChar() == '<')
            {
                if (ReadChar() == '/')
                {
                    _index -= 2;
                    return (TextValue, SubElements.ToArray());
                }

                _index -= 2;
                HtmlElement? SubElement = ReadElement();
                if (SubElement != null) SubElements.Add(SubElement);
                continue;
            }

            _index--;
            TextValue = TextValue + ReadTextUntil('<', '\r', '\n');
        }
    }

    private void ReadTagEnd(string tagName)
    {
        if (ReadChar() != '<')
        {
            throw new HtmlFileException("Expected '<'.", _path);
        }
        if (ReadChar() != '/')
        {
            throw new HtmlFileException("Expected '/'.", _path);
        }

        if (ReadTextUntil('>').Trim() != tagName)
        {
            throw new HtmlFileException("Wrong tag closed.", _path);
        }
        _index++;
    }


    /* Read helper methods. */
    private bool ReadUntilNonWS()
    {
        char Character;

        while (_index < _htmlText.Length)
        {
            Character = _htmlText[_index];
            _index++;

            if (char.IsWhiteSpace(Character))
            {
                continue;
            }
            else
            {
                _index--;
                return true;
            }
        }

        return false;
    }

    private char ReadChar()
    {
        if (_index >= _htmlText.Length)
        {
            throw new HtmlFileException("Unexpected end of file", _path);
        }

        char Character = _htmlText[_index];
        _index++;

        return Character;
    }

    private string ReadString()
    {
        StringBuilder Result = new();

        ReadUntilNonWS();
        char ChosenQuote = ReadChar();
        if (ChosenQuote is not '\'' and not '"')
        {
            throw new HtmlFileException("Expected a string.", _path);
        }

        char Character = ReadChar();

        while (Character != ChosenQuote)
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

        _index--;
        return Character.ToString();
    }

    private string ReadLetterWord(params char[] extraAllowedChars)
    {
        extraAllowedChars ??= Array.Empty<char>();
        StringBuilder Word = new();

        ReadUntilNonWS();
        char Character = ReadChar();

        while (char.IsLetter(Character) || extraAllowedChars.Contains(Character))
        {
            Word.Append(Character);
            Character = ReadChar();
        }

        _index--;
        return Word.ToString();
    }

    private string ReadTextUntil(params char[] targets)
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

        _index--;
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

        _index--;
    }
}