using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Database;

internal readonly struct TextHash
{
    // Fields.
    long Part1 { get; init; }

    long Part2 { get; init; }


    // Constructors.
    internal TextHash(long part1, long part2)
    {
        Part1 = part1;
        Part2 = part2;
    }


    // Inherited methods.
    public override string ToString()
    {
        return Part1.ToString() + Part2.ToString();
    }

    public override bool Equals([NotNullWhen(true)] object? obj)
    {
        if (obj == null || obj is not TextHash)
        {
            return false;
        }

        TextHash Obj2 = (TextHash)obj;
        return (Part1 == Obj2.Part1) && (Part2 == Obj2.Part2);
    }
}