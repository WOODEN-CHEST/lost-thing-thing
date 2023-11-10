using GHDataFile;
using LTTServer.Database;
using LTTServer.Logging;
using LTTServer.Users.SMTS;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Users;

internal class UserManager : IDisposable
{
    // Internal fields.
    internal string UsersDir { get; private init; }

    internal string UserEntriesDir { get; private init; }

    internal string UsernameBucketsDir { get; private init; }

    internal string GlobalUserDataPath { get; private init; }


    // Private fields.
    private SMTServer _emailServer;


    /* Global data file. */
    private ulong _availableSessionID;
    private ulong _userCount;


    /* Global data file. */
    private const int ID_SESSION_ID = 1;
    private const int DEFAULT_SESSION_ID = 0;
    private const int ID_USER_COUNT = 2;
    private const int DEFAULT_USER_COUNT = 0;


    // Constructors.
    internal UserManager()
    {
        UsersDir = Path.Combine(Server.RootPath, "users");
        UserEntriesDir = Path.Combine(UsersDir, "entries");
        UsernameBucketsDir = Path.Combine(UsersDir, "username_buckets");
        GlobalUserDataPath = Path.Combine(UsersDir, "global" + IDataFile.FileExtension);

        if (!Directory.Exists(UsersDir))
        {
            Directory.CreateDirectory(UsersDir);
        }
        if (!Directory.Exists(UserEntriesDir))
        {
            Directory.CreateDirectory(UserEntriesDir);
        }
        if (!Directory.Exists(UsernameBucketsDir))
        {
            Directory.CreateDirectory(UsernameBucketsDir);
        }

        _emailServer = new();
        ReadGlobalData();
    }


    // Internal methods.
    internal void CreateProfile(string name, string surname, string password, string email)
    {
        TextHash Password = StringHasher.GetHash(password);
        password = null!;



        _emailServer.SendVerificationEmail(email);
    }


    // Private methods.
    /* Global user data file. */
    private void ReadGlobalData()
    {
        if (!File.Exists(GlobalUserDataPath))
        {
            CreateGlobalData();
            return;
        }

        DataFileCompound UserDataCompound = DataFileReader.GetReader(GlobalUserDataPath).Read();

        _availableSessionID = UserDataCompound.GetOrDefault<ulong>(ID_SESSION_ID, DEFAULT_SESSION_ID);
        _availableSessionID = UserDataCompound.GetOrDefault<ulong>(ID_USER_COUNT, DEFAULT_USER_COUNT);
        Server.OutputInfo("Read global user data.");
    }

    private void WriteGlobalData()
    {
        DataFileCompound UserDataCompound = new();
        UserDataCompound.Add(ID_SESSION_ID, _availableSessionID);
        UserDataCompound.Add(ID_USER_COUNT, _userCount);

        DataFileWriter.GetWriter().Write(GlobalUserDataPath, UserDataCompound);

        Server.OutputInfo("Saved global user data.");
    }

    private void CreateGlobalData()
    { 
        _availableSessionID = DEFAULT_SESSION_ID;
        _userCount = DEFAULT_USER_COUNT;
        Server.OutputInfo("Created new global user data.");
    }

    // Inherited methods.
    public void Dispose()
    {
        WriteGlobalData();
    }
}