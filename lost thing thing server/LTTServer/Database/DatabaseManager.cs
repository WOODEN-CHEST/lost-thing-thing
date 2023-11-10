using System;
using System.Collections.Generic;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Database;

internal class DatabaseManager
{
    // Internal fields.
    internal string DatabasePath { get; private set; }



    // Private fields.
    private const string DATABASE_SUBPATH = "data";


    // Constructors.
    internal DatabaseManager()
    {
        DatabasePath = Path.Combine(Server.RootPath, DATABASE_SUBPATH);
        PbeEncryptionAlgorithm a = new();
        if (!Directory.Exists(DatabasePath))
        {
            Directory.CreateDirectory(DatabasePath);
        }
    }
}