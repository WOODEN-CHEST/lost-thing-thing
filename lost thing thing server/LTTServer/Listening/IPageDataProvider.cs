using System.Net;


namespace LTTServer.Listening;


internal interface IPageDataProvider
{
    // Methods.
    public byte[]? GetData(string path, HttpListenerContext context);

    public string? GetDataAsText(string path, HttpListenerContext context);
}