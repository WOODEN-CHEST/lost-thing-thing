using GHDataFile;
using LTTServer.Search;
using LTTServer.Users.SMTS;
using System.Text;


namespace LTTServer.Users;


internal class ProfileManager : IDisposable
{
    // Internal static fields.
    internal const int VERIFICATION_CODE_LENGTH = 10;

    internal static readonly TimeSpan MaxVerificationTime = new(0, 10, 0); // 10 minutes.


    // Internal fields.
    internal string UsersDir { get; private init; }

    internal string UserEntriesPath { get; private init; }

    internal string GlobalUserDataPath { get; private init; }


    // Private fields.
    private Dictionary<string, (ProfileInfo ProfileInfo, DateTime CreationTime)> _unverifiedProfiles = new();
    private string[]? _allowedEmailSuffixes;

    /* Components. */
    private SMTServer _emailServer;
    private ProfileIconManager _iconManager;


    /* Users. */
    private ulong _availableSessionID;
    private ulong _availableUserID;
    private ulong _availableProfileIcon;

    private readonly Dictionary<char, HashSet<ProfileInfo>> _profileBuckets = new();
    private readonly List<ProfileInfo> _profiles = new();

    /* Global data file. */
    private const int ID_SESSION_ID = 1;
    private const int DEFAULT_SESSION_ID = 0;
    private const int ID_USER_ID = 2;
    private const int DEFAULT_USER_ID = 0;

    /* Users file */
    private const int ID_USERS_ARRAY = 1;


    // Constructors.
    internal ProfileManager(string defaultIconName, string[]? allowedEmailSuffixes)
    {
        UsersDir = Path.Combine(Server.DatabaseManager.DatabasePath, "users");
        UserEntriesPath = Path.Combine(UsersDir, "entries" + IDataFile.FileExtension);
        GlobalUserDataPath = Path.Combine(UsersDir, "global" + IDataFile.FileExtension);
        _iconManager = new(UsersDir, defaultIconName);
        _allowedEmailSuffixes = allowedEmailSuffixes;

        if (!Directory.Exists(UsersDir))
        {
            Directory.CreateDirectory(UsersDir);
        }

        foreach (char Character in ProfileInfo.AllowedLowercaseUsernameChars)
        {
            _profileBuckets.Add(Character, new HashSet<ProfileInfo>());
        }

        _emailServer = new();
        ReadGlobalData();
        ReadProfiles();
    }


    // Internal static methods.
    internal static string GenerateVerificationCode()
    {
        StringBuilder VerificationString = new(VERIFICATION_CODE_LENGTH);

        for (int i = 0; i < VERIFICATION_CODE_LENGTH; i++)
        {
            switch (Random.Shared.Next(3))
            {
                case 1:
                    VerificationString.Append((char)Random.Shared.Next('a', 'z' + 1));
                    break;
                case 2:
                    VerificationString.Append((char)Random.Shared.Next('A', 'Z' + 1));
                    break;
                default:
                    VerificationString.Append((char)Random.Shared.Next('0', '9' + 1));
                    break;
            }
        }

        return VerificationString.ToString();
    }


    // Internal methods.
    internal bool CreateUnverifiedProfile(string name, string surname, string password, string email)
    {
        ProfileInfo? Profile = ProfileInfo.TryCreateUnverifiedProfile(name, surname, password, email, _allowedEmailSuffixes);

        if (Profile == null)
        {
            Server.OutputError("Invalid user credentials found, this should have been handled client-side." +
                $"Name: \"{name}\", Surname:\"{surname}\", Email:\"{email}\"");
            return false;
        }

        foreach (ProfileInfo ExistingProfile in _profiles)
        {
            if (ExistingProfile.Email == Profile.Email)
            {
                return false;
            }
        }

        string VerificationCode = GenerateVerificationCode();
        AddUnverifiedProfile(VerificationCode, Profile);
        //SendVerificationEmail(email, VerificationCode);

        return true;
    }

    internal bool VerifyProfile(string email, string code)
    {
        RemoveExpiredUnverifiedProfiles();

        if ((email == null) || (code == null) || (code.Length != VERIFICATION_CODE_LENGTH))
        {
            return false;
        }

        if (!_unverifiedProfiles.TryGetValue(code, out var UnverifiedProfile))
        {
            return false;
        }

        if (UnverifiedProfile.ProfileInfo.Email != email.Trim())
        {
            return false;
        }

        _unverifiedProfiles.Remove(code);
        CreateProfile(UnverifiedProfile.ProfileInfo);
        return true;
    }

#if DEBUG
    internal void ForceVerifyProfiles()
    {
        foreach (var UnverifiedProfile in _unverifiedProfiles)
        {
            CreateProfile(UnverifiedProfile.Value.ProfileInfo);
        }

        _unverifiedProfiles.Clear();
    }
#endif

    internal ProfileInfo[] GetProfiles(string name)
    {
        if (name == null || name.Length == 0)
        {
            throw new ArgumentException("Name is null or length of 0", nameof(name));
        }

        name = name.ToLower();

        // Filter by containing letters.
        HashSet<ProfileInfo> FilteredProfiles = new();

        FilteredProfiles.UnionWith(_profileBuckets[name[0]]);
        foreach (char Letter in name)
        {
            FilteredProfiles.IntersectWith(_profileBuckets[Letter]);
        }

        // Filter by containing string.
        foreach (ProfileInfo Profile in FilteredProfiles)
        {
            if (!Searcher.SearchStringSpaced(Profile.LowercaseName + Profile.LowercaseSurname, name, true))
            {
                FilteredProfiles.Remove(Profile);
            }
        }

        return FilteredProfiles.ToArray();
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

        using DataFileReader Reader = DataFileReader.GetReader(GlobalUserDataPath);
        DataFileCompound UserDataCompound = Reader.Read();

        _availableSessionID = UserDataCompound.GetOrDefault<ulong>(ID_SESSION_ID, DEFAULT_SESSION_ID);
        _availableUserID = UserDataCompound.GetOrDefault<ulong>(ID_USER_ID, DEFAULT_USER_ID);
        Server.OutputInfo("Read global user data.");
    }

    private void WriteGlobalData()
    {
        DataFileCompound UserDataCompound = new();
        UserDataCompound.Add(ID_SESSION_ID, _availableSessionID);
        UserDataCompound.Add(ID_USER_ID, _availableUserID);

        DataFileWriter.GetWriter().Write(GlobalUserDataPath, UserDataCompound);

        Server.OutputInfo("Saved global user data.");
    }

    private void CreateGlobalData()
    { 
        _availableSessionID = DEFAULT_SESSION_ID;
        _availableUserID = DEFAULT_USER_ID;
        Server.OutputWarning("Created new global user data.");
    }


    /* User profile file. */
    private void ReadProfiles()
    {
        Server.OutputInfo("Reading user profiles.");
        if (!File.Exists(UserEntriesPath))
        {
            Server.OutputWarning("Read no user profiles since the user profile file does not exist.");
            return;
        }

        using DataFileReader Reader = DataFileReader.GetReader(UserEntriesPath);
        DataFileCompound AllProfilesCompound = Reader.Read();

        foreach (DataFileCompound ProfileCompound in AllProfilesCompound.Get<DataFileCompound[]>(ID_USERS_ARRAY)!)
        {
            ProfileInfo? Profile = ProfileInfo.TryCreateFromCompound(ProfileCompound, _allowedEmailSuffixes);

            if (Profile != null)
            {
                _profiles.Add(Profile);
                AddProfileToLetterBuckets(Profile);
            }
        }

        Server.OutputInfo($"Read {_profiles.Count} user profiles.");
    }

    private void WriteProfiles()
    {
        var ProfileCompounds = new DataFileCompound[_profiles.Count];

        for (int i = 0; i <  ProfileCompounds.Length; i++)
        {
            ProfileCompounds[i] = _profiles[i].GetAsCompound();
        }

        DataFileCompound BaseCompound = new();
        BaseCompound.Add(ID_USERS_ARRAY, ProfileCompounds);
        DataFileWriter.GetWriter().Write(UserEntriesPath, BaseCompound);

        Server.OutputInfo($"Saved {ProfileCompounds.Length} user profiles.");
    }


    /* Creating user profile. */
    private void SendVerificationEmail(string email, string code)
    {
        string Message = "Šis e-pasts ir nosūtīts, jo jūsu e-pasta adrese tika izmantota, lai veidotu kontu mājaslapā LostThingThing." +
            $"\nJūsu apstiprināšanas kods ir \"{code}\"." +
            $"\nJa šī ir bijusi kļūda, varat e-pastu ignorēt. AAAAAAA";

        _emailServer.SendEmail(email, "LostThingThing konta izveides apstiprināšana.", Message);
    }

    private void AddUnverifiedProfile(string code, ProfileInfo profile)
    {
        _unverifiedProfiles[code] = (profile, DateTime.Now);
        RemoveExpiredUnverifiedProfiles();
    }

    private void RemoveExpiredUnverifiedProfiles()
    {
        List<string> CodesToRemove = new();

        foreach (var Profile in _unverifiedProfiles)
        {
            if (DateTime.Now - Profile.Value.CreationTime > MaxVerificationTime)
            {
                CodesToRemove.Add(Profile.Key);
            }
        }

        foreach (string Code in CodesToRemove)
        {
            _unverifiedProfiles.Remove(Code);
        }
    }

    /* Profile */
    private void CreateProfile(ProfileInfo profile)
    {
        profile.ID = _availableUserID;
        _availableUserID++;

        _profiles.Add(profile);
        AddProfileToLetterBuckets(profile);
        Server.OutputInfo($"Created profile with the e-mail \"{profile.Email}\"");
    }

    private void AddProfileToLetterBuckets(ProfileInfo profile)
    {
        foreach (char Letter in profile.LowercaseName + profile.LowercaseSurname)
        {
            _profileBuckets[Letter].Add(profile);
        }
    }

    private void RemoveProfileFromLetterBuckets(ProfileInfo profile)
    {
        foreach (char Letter in profile.Name)
        {
            _profileBuckets[Letter].Remove(profile);
        }
    }


    // Inherited methods.
    public void Dispose()
    {
        WriteGlobalData();
        WriteProfiles();
        _unverifiedProfiles.Clear();
    }
}