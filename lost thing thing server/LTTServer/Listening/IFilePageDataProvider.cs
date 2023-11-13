using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Listening;

internal interface IFilePageDataProvider : IPageDataProvider
{
    // Static fields.
    public static string EmptyPath = "/";


    // Fields.
    public string SearchPattern { get; }

    public string[] Paths { get; }


    // Static methods.
    public static string FormatAsContentPath(string path) => EmptyPath + path;


    // Methods.
    public void LoadData();
}