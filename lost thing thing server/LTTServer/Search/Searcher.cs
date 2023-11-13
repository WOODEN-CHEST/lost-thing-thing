using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Search;

internal static class Searcher
{
    // Internal static methods.
    internal static bool SearchStringSpaced(string text, string search, bool caseSensitive = true)
    {
        if (text == null || search == null)
        {
            return false;
        }

        if (search == string.Empty)
        {
            return true;
        }
        if (text == string.Empty)
        {
            return false;
        }

        if (!caseSensitive)
        {
            text = text.ToLower();
            search = search.ToLower();
        }

        int SearchStringIndex = 0;
        foreach (char Letter in text)
        {
            if (Letter == search[SearchStringIndex])
            {
                SearchStringIndex++;

                if (SearchStringIndex >= search.Length)
                {
                    return true;
                }
            }
        }

        return false;
    }
}