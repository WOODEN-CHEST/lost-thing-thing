using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.HTML;

internal class HtmlFileException : IOException
{
    internal HtmlFileException(string? message, string? filePath) : base($"Exception reading file \"{filePath}\": {message}") { }
}