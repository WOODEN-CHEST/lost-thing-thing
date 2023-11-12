using GHDataFile;
using LTTServer.Database;
using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static System.Net.Mime.MediaTypeNames;

namespace LTTServer.Users;

internal class ProfileInfo
{
    // Internal static fields.
    internal static char[] AllowedUsernameChars => _allowedUsernameChars.ToArray();

    internal static char[] AllowedEmailChars => _allowedEmailChars.ToArray();


    // Internal fields.
    internal ulong ID { get; set; }

    [MemberNotNull(nameof(_name))]
    internal string Name
    {
        get => _name;
        set
        {
            if (!VerifyName(value))
            {
                throw new ArgumentException($"Invalid name \"{value}\"", nameof(value));
            }
            _name = value.Trim();
        }
    }

    [MemberNotNull(nameof(_surname))]
    internal string Surname
    {
        get => _surname;
        set
        {
            if (!VerifyName(value))
            {
                throw new ArgumentException($"Invalid surname \"{value}\"", nameof(value));
            }
            _surname= value.Trim();
        }
    }

    internal TextHash Password { get; set; }

    [MemberNotNull(nameof(_email))]
    internal string Email
    {
        get => _email;
        set 
        {
            if (!VerifyEmail(value))
            {
                throw new ArgumentException($"Invalid email \"{value}\"", nameof(value));
            }
            _email = value.Trim();
        }
    }

    internal int[] Posts { get; set; } = Array.Empty<int>();

    internal int[] Comments { get; set; } = Array.Empty<int>();


    // Private static fields.
    private static HashSet<char> _allowedUsernameChars;

    private static HashSet<char> _allowedEmailChars;


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
    private ProfileInfo(
        ulong id,
        string name, 
        string surname, 
        TextHash password, 
        string email,
        int[] posts,
        int[] comments)
    {
        ID = id;
        _name = name;
        _surname = surname;
        Password = password;
        _email = email;
        Posts = posts;
        Comments = comments;
    }


    // Internal static methods.
    internal static ProfileInfo? TryCreateProfile(
        ulong id,
        string name,
        string surname,
        string password,
        string email,
        int[]? posts,
        int[]? comments)
    {
        if (!VerifyName(name) || !VerifyName(surname) || !VerifyEmail(email) || !VerifyPassword(password))
        {
            return null;
        }

        posts ??= Array.Empty<int>();
        comments ??= Array.Empty<int>();

        return new ProfileInfo(id, name.Trim(), surname.Trim(), TextHash.GetHash(password), email.Trim(), posts, comments);
    }

    internal static ProfileInfo? TryCreateFromCompound(DataFileCompound compound)
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

        if (!VerifyName(Name) || !VerifyName(Surname) || !VerifyEmail(Email))
        {
            return null;
        }

        Posts ??= Array.Empty<int>();
        Comments ??= Array.Empty<int>();

        return new ProfileInfo(ID, Name!.Trim(), Surname!.Trim(), Password, Email!.Trim(), Posts, Comments);
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

    internal static bool VerifyEmail(string? email)
    {
        if (string.IsNullOrWhiteSpace(email))
        {
            return false;
        }

        email = email.Trim();

        const int MAX_LENGTH = 100;
        if ((email.Length > MAX_LENGTH) || (email.Length == 0))
        {
            return false;
        }

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
}