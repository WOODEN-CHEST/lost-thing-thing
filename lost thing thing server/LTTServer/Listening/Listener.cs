using System.Net;
using System.Text;
using static System.Runtime.InteropServices.JavaScript.JSType;

namespace LTTServer.Listening;


internal class Listener
{
    // Internal fields.
    internal string[] Prefixes => _prefixes .ToArray();


    // Private fields.
    private readonly HttpListener HTTPListener;
    private readonly string[] _prefixes;

    private readonly PageDataProvider _htmlProvider;
    private readonly PageDataProvider _cssProvider;
    private readonly PageDataProvider _javascriptProvider;
    private readonly PageDataProvider _pngProvider;

    private readonly string _pathToIndexHTML;
    private readonly string _pathTo404HTML;

    /* Constants. */
    private const string INDEX_FILE_NAME = "index";
    private const string NOT_FOUND_FILE_NAME = "404";

    private const string EXTENSION_HTML = ".html";
    private const string EXTENSION_CSS = ".css";
    private const string EXTENSION_JAVASCRIPT = ".js";
    private const string EXTENSION_PNG = ".png";

    private const string METHOD_GET = "GET";
    private const string METHOD_POST = "POST";

    private const int STATUS_CODE_NOT_FOUND = 404;


    // Constructors.
    internal Listener(params string[] prefixes)
    {
        if (prefixes == null)
        {
            throw new ArgumentNullException(nameof(prefixes));
        }
        if (prefixes.Length == 0)
        {
            throw new ArgumentException("Must have at least one prefix.");
        }

        HTTPListener = new();
        foreach (string Prefix  in prefixes)
        {
            HTTPListener.Prefixes.Add(Prefix);
        }

        _htmlProvider = new(EXTENSION_HTML);
        _cssProvider = new(EXTENSION_CSS);
        _javascriptProvider = new(EXTENSION_JAVASCRIPT);
        _pngProvider = new(EXTENSION_PNG);

        foreach (string FilePath in _htmlProvider.Paths)
        {
            string FileName = Path.GetFileNameWithoutExtension(FilePath);

            if (FileName == INDEX_FILE_NAME)
            {
                _pathToIndexHTML = FilePath;
            }
            else if (FileName == NOT_FOUND_FILE_NAME)
            {
                _pathTo404HTML = FilePath;
            }
        }
    }


    // Internal methods.
    internal void Listen()
    {
        try
        {
            HTTPListener.Start();
            ListenForMessages();
        }
        catch (Exception e)
        {
            if (HTTPListener.IsListening)
            {
                HTTPListener.Stop();
            }
            throw new Exception($"Exception during listening: {e}");
        }
    }


    // Private methods.
    private void ListenForMessages()
    {
        while (true)
        {
            HttpListenerContext Context = HTTPListener.GetContext();

            string Method = Context.Request.HttpMethod;
            byte[] Response = Array.Empty<byte>();

            switch (Method)
            {
                case METHOD_GET:
                    Response = GetResource(Context);
                    break;

                default:
                    Server.OutputWarning($"Unknown HTTP request method: \"{Method}\"");
                    break;
            }

            Respond(Context, Response);
        }
    }

    /* Getting data. */
    private byte[] GetResource(HttpListenerContext context)
    {
        string? ResourcePath = context.Request.RawUrl;

        if ((ResourcePath == null) || (ResourcePath == PageDataProvider.EmptyPath))
        {
            context.Response.Redirect(_pathToIndexHTML);
            return Array.Empty<byte>();
        }

        byte[]? Data = null;
        string ResourcePathExtension = Path.GetExtension(ResourcePath);
        switch (ResourcePathExtension)
        {
            case EXTENSION_HTML:
                Data = _htmlProvider.GetData(ResourcePath);
                break;

            case EXTENSION_CSS:
                Data = _cssProvider.GetData(ResourcePath);
                break;

            case EXTENSION_JAVASCRIPT:
                Data = _javascriptProvider.GetData(ResourcePath);
                break;

            case EXTENSION_PNG:
                Data = _pngProvider.GetData(ResourcePath);
                break;

            case "":
                Data = null;
                break;

            default:
                Server.OutputWarning($"File with unknown extension requested: \"{ResourcePathExtension}\"");
                break;
        }

        if (Data != null)
        {
            return Data;
        }

        context.Response.Redirect(_pathTo404HTML);
        return Array.Empty<byte>();
    }

    /* Responses. */
    private void Respond(HttpListenerContext context, byte[] response)
    {
        context.Response.ContentEncoding = Server.Encoding;
        context.Response.ContentLength64 = response.LongLength;
        context.Response.OutputStream.Write(response, 0, response.Length);
        context.Response.Close();
    }
}