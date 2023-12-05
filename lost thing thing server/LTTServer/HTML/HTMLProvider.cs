using LTTServer.Listening;
using System.Net;


namespace LTTServer.HTML;


internal class HTMLProvider : IPageDataProvider
{
    // Private fields.
    /* Pages. */
    private HtmlDocument _templatePage; 
    private HtmlDocument _indexPage; 



    // Constructors.
    internal HTMLProvider()
    {
        SetupTemplatePage();
        SetupPageIndex();
    }


    // Internal methods.
    /* Different pages. */
    internal string GetPageIndex()
    {
        return _indexPage.ToString();
    }

    internal string GetPage404()
    {
        throw new NotImplementedException();
    }

    internal string GetPageLogin()
    {
        throw new NotImplementedException();
    }

    internal string GetPageSignUp()
    {
        throw new NotImplementedException();
    }


    // Private methods.
    /* Setup for pages. */
    private void SetupTemplatePage()
    {
        _templatePage = new();

        _templatePage.Head!.AddSubElement("link")
            .AddAttribute("href", "common.css")
            .AddAttribute("rel", "stylesheet");

        _templatePage.Head!.AddSubElement("script")
            .AddAttribute("src", "common.js");

        _templatePage.Head.AddSubElement("title", "Unset Title");

        _templatePage.Body!.AddSubElement("div", null, "TopPanel")
            .AddAttribute("onmouseenter", "OnTopPanelHoverEvent(this)")
            .AddAttribute("onmouseleave", "OnTopPanelUnHoverEvent(this)");

        _templatePage.Body.AddSubElement("div", null, "BottomInfoPanel");
    }

    private void SetupPageIndex()
    {
        _indexPage = (HtmlDocument)_templatePage.Clone();
        _indexPage.Head!.AddSubElement("link")
            .AddAttribute("href", "index/index.css")
            .AddAttribute("rel", "stylesheet");

        HtmlElement Form = new("form", null, "MyForm");
        Form.AddAttribute("method", "post");

        Form.AddSubElement("input", null)
            .AddAttribute("class", "UserInput")
            .AddAttribute("placeholder", "username");
        Form.AddSubElement("input", null)
            .AddAttribute("class", "UserInput")
            .AddAttribute("placeholder", "password");
        Form.AddSubElement("button", "click me!")
            .AddAttribute("type", "submit")
            .AddAttribute("enctype", "application/x-www-form-urlencoded");
        _indexPage.Body!.AddSubElement(Form);
    }


    /* Page modification. */
    private HtmlDocument ModifyLoggedIn(HtmlDocument document)
    {
        return document;
    }

    private HtmlDocument ModifyLoggedOut(HtmlDocument document)
    {
        HtmlElement? TopPanel = document.GetElementByID("TopPanel");

        if (TopPanel == null)
        {
            return document;
        }

        TopPanel.AddSubElement("p", "Baļķis iekšā", "TopPanelLoginCover");

        TopPanel.AddSubElement("a", "Baļķis iekšā", "LoginHyperlink")
            .AddAttribute("class", "AccountHyperLink")
            .AddAttribute("href", "login");

        TopPanel.AddSubElement("a", "Zīme augšā", "SignUpHyperlink")
            .AddAttribute("class", "AccountHyperLink")
            .AddAttribute("href", "sign_up");

        return document;
    }


    /* Getting page data. */
    private string GetPageText(string path, HttpListenerContext context)
    {
        if (path == IFilePageDataProvider.EmptyPath)
        {
            return GetPageIndex();
        }
        else
        {

        }

        context.Response.Redirect(IFilePageDataProvider.EmptyPath);
        return string.Empty;
    }

    // Inherited methods.
    public byte[]? GetData(string path, HttpListenerContext context)
    {
        return Server.Encoding.GetBytes(GetPageText(path, context));
    }

    public string? GetDataAsText(string path, HttpListenerContext context) => GetPageText(path, context);
}