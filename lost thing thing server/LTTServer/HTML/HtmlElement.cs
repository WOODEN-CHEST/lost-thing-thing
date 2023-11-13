using System.Text;

namespace LTTServer.HTML;

internal class HtmlElement : ICloneable
{
    // Internal static fields.
    internal const string ATTRIBUTE_NAME_ID = "id";

    // Internal fields.
    internal string TagName { get; private init; }

    internal string? Content { get; set; }

    internal HtmlElement[] SubElements => _subElements.ToArray();

    internal Dictionary<string, string?> Attributes { get; private init; } = new();

    internal bool IsVoidElement => IsTagVoid(TagName);


    // Private fields.
    private readonly List<HtmlElement> _subElements = new();


    // Constructors.
    public HtmlElement(string tagName) : this(tagName, null) { }

    public HtmlElement(string tagName, string? content) : this(tagName, content, null) { }

    public HtmlElement(string tagName, string? content, string? id)
    {
        if (tagName == null)
        {
            throw new ArgumentNullException(nameof(tagName));
        }
        if (string.IsNullOrWhiteSpace(tagName))
        {
            throw new ArgumentException($"Invalid tag name: \"{tagName}\"", nameof(tagName));
        }
        TagName = tagName;

        Content = content;

        if (id != null)
        {
            AddAttribute(ATTRIBUTE_NAME_ID, id);
        }
    }

    
    // Internal static methods.
    internal static bool IsTagVoid(string tagName)
    {
        if (tagName == null)
        {
            throw new ArgumentNullException(nameof(tagName));
        }

        return tagName.ToLower() is "area" or "base" or "br" or "col" or "embed"
                or "hr" or "img" or "input" or "link" or "meta" or "param"
                or "source" or "track" or "wbr" or "!doctype";
    }


    // Internal methods.
    internal HtmlElement AddAttribute(string name, string? value)
    {
        if (string.IsNullOrWhiteSpace(name))
        {
            throw new ArgumentException($"Invalid attribute name: \"{name}\"", nameof(name));
        }

        string FormattedName = name.Trim();
        Attributes[FormattedName] = value ?? string.Empty;

        return this;
    }

    internal HtmlElement RemoveAttribute(string name)
    {
        if (name == null)
        {
            throw new ArgumentNullException(nameof(name));
        }

        Attributes.Remove(name);

        return this;
    }

    internal HtmlElement ClearAttributes()
    {
        Attributes.Clear();
        return this;
    }

    internal HtmlElement AddSubElement(HtmlElement element)
    {
        if (element == null)
        {
            throw new ArgumentNullException(nameof(element));
        }

        _subElements.Add(element);

        return element;
    }

    internal HtmlElement AddSubElement(string tag) => AddSubElement(new HtmlElement(tag));

    internal HtmlElement AddSubElement(string tag, string? content) => AddSubElement(new HtmlElement(tag, content));

    internal HtmlElement AddSubElement(string tag, string? content, string? id) => AddSubElement(new HtmlElement(tag, content, id));


    internal HtmlElement RemoveSubElement(HtmlElement element)
    {
        if (element == null)
        {
            throw new ArgumentNullException(nameof(element));
        }

        _subElements.Remove(element);

        return element;
    }

    internal HtmlElement ClearSubElements()
    {
        _subElements.Clear();
        return this;
    }

    internal HtmlElement? GetFirstSubElementOfTag(string tag)
    {
        foreach (HtmlElement Element in _subElements)
        {
            if (Element.TagName == tag)
            {
                return Element; 
            }
        }

        return null;
    }

    internal HtmlElement? GetElementByID(string id)
    {
        HtmlElement? SearchElement(HtmlElement element)
        {
            element.Attributes.TryGetValue(ATTRIBUTE_NAME_ID, out string? ID);

            if ((ID != null) && (ID == id))
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

        return SearchElement(this);
    }


    // Inherited methods.
    public override string ToString()
    {
        StringBuilder HTMLString = new($"<");

        // Unified for all tag types.
        HTMLString.Append(TagName);
        foreach (var Attribute in Attributes)
        {
            HTMLString.Append(' ');

            HTMLString.Append(Attribute.Key);
            if (Attribute.Value != null)
            {
                HTMLString.Append($"=\"{Attribute.Value}\"");
            }
        }
        HTMLString.Append('>');

        // Void elements.
        if (IsVoidElement)
        {
            return HTMLString.ToString();
        }

        // Non-void elements.
        HTMLString.Append(Content);
        foreach (HtmlElement Element in _subElements)
        {
            HTMLString.Append(Element.ToString());
        }
        HTMLString.Append($"</{TagName}>");

        return HTMLString.ToString();
    }

    public object Clone()
    {
        HtmlElement NewElement = new(TagName, Content);

        foreach (var Attribute in Attributes)
        {
            NewElement.AddAttribute(Attribute.Key, Attribute.Value);    
        }

        foreach (HtmlElement SubElement in SubElements)
        {
            NewElement.AddSubElement((HtmlElement)SubElement.Clone());
        }

        return NewElement;
    }
}