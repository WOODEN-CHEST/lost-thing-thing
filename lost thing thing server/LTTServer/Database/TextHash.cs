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
    internal long Part1 { get; init; }

    internal long Part2 { get; init; }


    // Constructors.
    internal TextHash(long part1, long part2)
    {
        Part1 = part1;
        Part2 = part2;
    }


    // Internal static methods.
    internal static TextHash GetHash(string text) // Shitty and easy to breach, whatever.
    {
        // Validate text.
        if (text == null)
        {
            throw new ArgumentNullException(nameof(text));
        }
        if (text.Length == 0)
        {
            throw new ArgumentException("Cannot encrypt an empty message.", nameof(text));
        }

        // Random bullshit go.
        long Part1, Part2;

        Part1 = text.Length * (-(long)Math.Sqrt(text.Length));
        Part2 = Part1 * (long)(Math.Sin((text.Length + 29384d) * text.Length) * 10d);

        int AvgValue = 0;
        foreach (char Character in text)
        {
            AvgValue += Character;
        }
        AvgValue /= text.Length;

        foreach (char Character in text)
        {
            Part1 += Character * (long)Math.Round(Part1 / 1000d);

            if (Character > AvgValue)
            {
                Part1 >>= 3;
                Part1 = Part2 + Part1 * (long)(Math.Log2(Part2) * Math.Tan(Part2)) * Math.Sign(Part2 - Part1);
            }
            else
            {
                Part2 <<= 7;
                Part2 = Part1 * (long)(Math.Log10(Part1 * Math.Abs(Part1)) * Math.Cos(Part2)) - (AvgValue / text.Length);
            }
            Part1 = Part2 & Part1;
            Part2 *= (long)Math.Cbrt(Math.Abs(Part2)) + (long)Math.Sqrt(Math.Abs(Part1));
        }

        Part2 = Part2 * Part2 * Part2 * (-Part2);

        return new(Part1, Part2);
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