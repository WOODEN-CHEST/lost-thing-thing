using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection.Metadata;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.HTML;

internal class HtmlDocument : ICloneable
{

    // Internal fields.
    internal HtmlElement? HTMLRoot { get; set; }

    internal HtmlElement? Head { get; set; }

    internal HtmlElement? Body { get; set; }


    // Constructors.
    internal HtmlDocument(bool createEmpty = false)
    {
        if (createEmpty)
        {
            return;
        }

        HTMLRoot = new("html");
        HTMLRoot.AddAttribute("lang", "lv");

        Head = new("head");
        Body = new("body");

        HtmlElement MetaTag = new("meta");
        MetaTag.AddAttribute("charset", "utf-8");

        HTMLRoot.AddSubElement(MetaTag);
        HTMLRoot.AddSubElement(Head);
        HTMLRoot.AddSubElement(Body);
    }


    // Internal static methods.
    internal static HtmlDocument FromFile(string path) => new HTMLReader().ReadFile(path);


    // Internal methods.
    internal HtmlElement? GetElementByID(string id)
    {
        return HTMLRoot?.GetElementByID(id);
    }

    internal HtmlElement? GetFirstSubElementOfTag(string tag)
    {
        return HTMLRoot?.GetFirstSubElementOfTag(tag);
    }


    // Inherited methods.
    public override string ToString()
    {
        return $"<!DOCTYPE html>{HTMLRoot}";
    }

    public object Clone()
    {
        HtmlDocument NewDocument = new(true);
        NewDocument.HTMLRoot = (HtmlElement?)HTMLRoot?.Clone();
        NewDocument.Head = NewDocument.HTMLRoot?.GetFirstSubElementOfTag("head");
        NewDocument.Body = NewDocument.HTMLRoot?.GetFirstSubElementOfTag("body");

        return NewDocument;
    }
}