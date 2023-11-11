using LTTServer.HTML;
using System.Net;

namespace LTTServer.Listening;


internal class Listener
{
    // Internal fields.
    internal string[] Prefixes => _prefixes .ToArray();

    internal int RequestCount { get; private set; } = 0;


    // Private fields.
    private readonly HttpListener HTTPListener;
    private readonly string[] _prefixes;

    private readonly IPageDataProvider _htmlProvider;
    private readonly IFilePageDataProvider _cssProvider;
    private readonly IFilePageDataProvider _javascriptProvider;
    private readonly IFilePageDataProvider _pngProvider;
    private readonly IFilePageDataProvider _iconProvider;

    /* Constants. */
    private const string EXTENSION_CSS = ".css";
    private const string EXTENSION_JAVASCRIPT = ".js";
    private const string EXTENSION_PNG = ".png";
    private const string EXTENSION_ICON = ".ico";

    private const string METHOD_GET = "GET";
    private const string METHOD_POST = "POST";


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

        _htmlProvider = new HTMLProvider();
        _cssProvider = new BasicFilePageDataProvider(EXTENSION_CSS);
        _javascriptProvider = new BasicFilePageDataProvider(EXTENSION_JAVASCRIPT);
        _pngProvider = new BasicFilePageDataProvider(EXTENSION_PNG);
        _iconProvider = new BasicFilePageDataProvider(EXTENSION_ICON);
    }


    // Internal methods.
    internal void Listen()
    {
        try
        {
            HTTPListener.Start();
            ListenForMessagesAsync();
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

    internal void ReloadPageData()
    {
        _cssProvider.LoadData();
        _javascriptProvider.LoadData();
        _pngProvider.LoadData();
        _iconProvider.LoadData();
    }


    // Private methods.
    private async void ListenForMessagesAsync()
    {
        while (!Server.IsServerStopped)
        {
            HttpListenerContext Context = await HTTPListener.GetContextAsync();

            RequestCount++;
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
        string? ResourcePath = context.Request.RawUrl ?? IFilePageDataProvider.EmptyPath;

        byte[]? Data = null;
        string ResourcePathExtension = Path.GetExtension(ResourcePath)!;

        switch (ResourcePathExtension)
        {

            case EXTENSION_CSS:
                Data = _cssProvider.GetData(ResourcePath, context);
                break;

            case EXTENSION_JAVASCRIPT:
                Data = _javascriptProvider.GetData(ResourcePath, context);
                break;

            case EXTENSION_PNG:
                Data = _pngProvider.GetData(ResourcePath, context);
                break;

            case EXTENSION_ICON:
                Data = _iconProvider.GetData(ResourcePath, context);
                break;

            default:
                Data = _htmlProvider.GetData(ResourcePath, context);
                break;
        }

        return Data ?? Array.Empty<byte>();
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