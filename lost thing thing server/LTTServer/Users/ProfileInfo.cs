using GHDataFile;
using LTTServer.Database;
using SkiaSharp;
using System.Diagnostics.CodeAnalysis;


namespace LTTServer.Users;


internal class ProfileInfo
{
    // Internal static fields.
    internal static char[] AllowedUsernameChars => _allowedUsernameChars.ToArray();

    internal static char[] AllowedLowercaseUsernameChars => _allowedLowerUsernameChars;

    internal static char[] AllowedEmailChars => _allowedEmailChars.ToArray();


    // Internal fields.
    internal ulong ID { get; set; }

    [MemberNotNull(nameof(_name))]
    internal string Name
    {
        get => _name;
        private set
        {
            _name = value;
            LowercaseName = value.ToLower();
        }
    }

    internal string LowercaseName { get; private set; }

    [MemberNotNull(nameof(_surname))]
    internal string Surname
    {
        get => _surname;
        private set
        {
            _surname= value.Trim();
            LowercaseSurname = value.ToLower();
        }
    }

    internal string LowercaseSurname { get; private set; }

    internal TextHash Password { get; set; }

    [MemberNotNull(nameof(_email))]
    internal string Email
    {
        get => _email;
        private set 
        {
            _email = value.Trim();
        }
    }

    internal int[] Posts { get; set; } = Array.Empty<int>();

    internal int[] Comments { get; set; } = Array.Empty<int>();


    // Private static fields.
    private static HashSet<char> _allowedUsernameChars;
    private static HashSet<char> _allowedEmailChars;
    private static char[] _allowedLowerUsernameChars = new char[] 
    {
        'a', 'ā', 'b', 'c', 'č', 'd', 'e', 'ē', 'f', 'g', 'ģ',
        'h', 'i', 'ī', 'j', 'k', 'ķ', 'l', 'ļ', 'm', 'n', 'ņ',
        'o', 'q', 'p', 'r', 's', 'š', 't', 'u', 'ū', 'v', 'w', 'x', 'y', 'z', 'ž' 
    };
    private static readonly Dictionary<ulong, SKBitmap> _cachedProfileIcons = new();


    // Private fields.
    private string _name;
    private string _surname;
    private string _email;


    /* Data compound. */
    private const int ID_ID = 1;
    private const int ID_NAME = 2;
    private const int ID_SURNAME = 3;
    private const int ID_EMAIL = 4;
    private const int ID_PASSHASH1 = 5;
    private const int ID_PASSHASH2 = 6;
    private const int ID_POSTS = 7;
    private const int ID_COMMENTS = 8;


    // Static constructors.
    static ProfileInfo()
    {
        _allowedUsernameChars = new()
        {
            'a', 'A', 'b', 'B', 'c', 'C', 'd', 'D', 'e', 'E', 'f', 'F', 'g', 'G', 'h', 'H',
            'i', 'I', 'j', 'J', 'k', 'K', 'l', 'L', 'm', 'M', 'n', 'N', 'o', 'O', 'p', 'P',
            'q', 'Q', 'r', 'R', 's', 'S', 't', 'T', 'u', 'U', 'v', 'V', 'w', 'W', 'x', 'X',
            'y', 'Y', 'z', 'Z', 'ā', 'Ā', 'č', 'Č', 'ē', 'Ē', 'ģ', 'Ģ', 'ī', 'Ī', 'ķ', 'Ķ',
            'ļ', 'Ļ', 'ņ', 'Ņ', 'š', 'Š', 'ū', 'Ū', 'ž', 'Ž'
        };

        _allowedEmailChars = new(_allowedUsernameChars)
        {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '_', '-', '@'
        };
    }


    // Constructors.
    private ProfileInfo(ulong id,
        string name,
        string surname,
        TextHash password,
        string email,
        int[]? posts,
        int[]? comments)
    {
        ID = id;
        Name = name;
        Surname = surname;
        Password = password;    
        Email = email;
        Posts = posts ?? Array.Empty<int>();
        Comments = comments ?? Array.Empty<int>();
    }


    // Internal static methods.
    internal static ProfileInfo? TryCreateUnverifiedProfile(
        string name,
        string surname,
        string password,
        string email,
        string[]? allowedEmailSuffixes = null)
    {
        if (!VerifyName(name) || !VerifyName(surname) || !VerifyEmail(email, allowedEmailSuffixes) || !VerifyPassword(password))
        {
            return null;
        }

        return new ProfileInfo(0, name, surname, TextHash.GetHash(password), email, null, null);
    }
        internal static ProfileInfo? TryCreateFromCompound(DataFileCompound compound, string[]? allowedEmailSuffixes = null)
    {
        ulong ID = 0L;
        string? Name = null, Surname = null, Email = null;
        TextHash Password = new();
        int[]? Posts = null, Comments = null;

        try
        {
            ID = compound.Get<ulong>(ID_ID);
            Name = compound.Get<string>(ID_NAME);
            Surname = compound.Get<string>(ID_SURNAME);
            Password = new TextHash(compound.Get<long>(ID_PASSHASH1)!, compound.Get<long>(ID_PASSHASH2)!);
            Email = compound.Get<string>(ID_EMAIL);
            Posts = compound.Get<int[]>(ID_POSTS);
            Comments = compound.Get<int[]>(ID_COMMENTS);
        }
        catch (Exception e)
        {
            Server.OutputError($"Failed to read user profile! {e}");
            return null;
        }

        if (!VerifyName(Name) || !VerifyName(Surname) || !VerifyEmail(Email, allowedEmailSuffixes))
        {
            return null;
        }

        return new ProfileInfo(ID, Name!, Surname!, Password, Email!, Posts, Comments!);
    }

    internal static bool VerifyName(string? username)
    {
        if (string.IsNullOrWhiteSpace(username))
        {
            return false;
        }

        username = username.Trim();

        const int MAX_USERNAME_LENGTH = 50;
        if (username.Length > MAX_USERNAME_LENGTH)
        {
            return false;
        }

        foreach (char Letter in username)
        {
            if (!_allowedUsernameChars.Contains(Letter))
            {
                return false;
            }
        }

        return true;
    }

    internal static bool VerifyEmail(string? email, string[]? allowedSuffixes = null)
    {
        // Structure.
        if (string.IsNullOrWhiteSpace(email))
        {
            return false;
        }

        email = email.Trim();

        // Length.
        const int MAX_LENGTH = 100;
        if ((email.Length > MAX_LENGTH) || (email.Length == 0))
        {
            return false;
        }

        // Suffix.
        if  (allowedSuffixes != null)
        {
            bool HasSuffix = false;
            foreach (string Suffix in allowedSuffixes)
            {
                if (email.EndsWith(Suffix))
                {
                    HasSuffix = true;
                    break;
                }
            }

            if (!HasSuffix)
            {
                return false;
            }
        }

        // Characters.
        bool? CurrentLetterSpecial = false;
        bool ContainsAt = false;

        foreach (char Letter in email)
        {
            if (!_allowedEmailChars.Contains(Letter))
            {
                return false;
            }

            if (Letter is '@' or '.' or '_' or '-')
            {
                if (Letter  == '@')
                {
                    ContainsAt = true;
                }

                if (CurrentLetterSpecial == null || CurrentLetterSpecial.Value)
                {
                    return false;
                }
                CurrentLetterSpecial = true;
            }
            else
            {
                CurrentLetterSpecial = false;
            }
        }

        return ContainsAt;
    }

    internal static bool VerifyPassword(string? password)
    {
        if (string.IsNullOrWhiteSpace(password))
        {
            return false;
        }

        const int MAX_LENGTH = 100;
        const int MIN_LENGTH = 5;
        if ((password.Length > MAX_LENGTH) || (password.Length < MIN_LENGTH))
        {
            return false;
        }

        return true;
    }


    // Internal methods.
    internal DataFileCompound GetAsCompound()
    {
        DataFileCompound Compound = new();

        Compound.Add(ID_ID, ID)
            .Add(ID_NAME, Name)
            .Add(ID_SURNAME, Surname)
            .Add(ID_EMAIL, Email)
            .Add(ID_PASSHASH1, Password.Part1)
            .Add(ID_PASSHASH2, Password.Part2)
            .Add(ID_POSTS, Posts)
            .Add(ID_COMMENTS, Comments);

        return Compound;
    }

    internal bool TrySetName(string name)
    {
        if (VerifyName(name))
        {
            Name = name.Trim();
            return true;
        }
        return false;
    }

    internal bool TrySetSurname(string surname)
    {
        if (VerifyName(surname))
        {
            Surname = surname.Trim();
            return true;
        }
        return false;
    }

    internal bool TrySetEmail(string email, string[]? allowedSuffixes = null)
    {
        if (VerifyEmail(email, allowedSuffixes))
        {
            Email = email.Trim();
            return true;
        }
        return false;
    }

    internal bool TrySetPassword(string password)
    {
        if (VerifyPassword(password))
        {
            Password = TextHash.GetHash(password);
            return true;
        }
        return false;
    }


    // Inherited methods.
    public override string ToString()
    {
        return $"{Name} {Surname} {Email}";
    }
}