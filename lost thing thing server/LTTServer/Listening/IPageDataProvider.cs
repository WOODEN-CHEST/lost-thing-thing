using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Listening;

internal interface IPageDataProvider
{
    // Fields.
    public string SearchPattern { get; }

    public string[] Paths { get; }


    // Static methods.
    public static string FormatAsContentPath(string path) => '/' + path;


    // Methods.
    public byte[]? GetData(string path);

    public string? GetDataAsText(string path);

    public void LoadData();
}