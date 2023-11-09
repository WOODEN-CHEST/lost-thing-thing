using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.HTML;

internal class HtmlDocument
{

    // Internal fields.
    internal HtmlElement HTMLRoot { get; set; }

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


    // Internal methods.
    internal HtmlElement? GetElementById(string searchedId)
    {
        HtmlElement? SearchElement(HtmlElement element)
        {
            element.Attributes.TryGetValue(HtmlElement.ATTRIBUTE_NAME_ID, out string? ID);

            if ((ID != null) && (ID == searchedId))
            {
                return element;
            }

            foreach (HtmlElement SubElement in element.SubElements)
            {
                HtmlElement? FoundElement = SearchElement(SubElement);

                if (FoundElement != null)
                {
                    return FoundElement;
                }
            }

            return null;
        }

        return SearchElement(HTMLRoot);
    }


    // Inherited methods.
    public override string ToString()
    {
        return $"<!DOCTYPE html>{HTMLRoot}";
    }
}