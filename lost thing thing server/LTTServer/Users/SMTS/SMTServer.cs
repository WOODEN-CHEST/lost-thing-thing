using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Mail;
using System.Text;
using System.Threading.Tasks;

namespace LTTServer.Users.SMTS;

internal class SMTServer
{
    // Private static fields.
    private static MailAddress s_sourceAddress = new("LostThingThing@gmail.com", "Lost Thing Thing");


    // Private fields.
    private readonly Dictionary<string, (string email, DateTime timeSent)> _verifications = new();


    // Constructors.
    internal SMTServer() { }


    // Internal methods.
    internal void SendEmail(string targetEmail, string subject, string content)
    {
        if (targetEmail == null)
        {
            throw new ArgumentNullException(nameof(targetEmail));
        }
        if (subject == null)
        {
            throw new ArgumentNullException(nameof(subject));
        }
        if (content == null)
        {
            throw new ArgumentNullException(nameof(content));
        }

        // Setup client.
        SmtpClient Client = new("smtp.gmail.com", 587)
        {
            EnableSsl = true,
            DeliveryMethod = SmtpDeliveryMethod.Network,
            Credentials = new NetworkCredential(s_sourceAddress.Address, "porh rrjl usmz udxh")
        };
        MailAddress TargetAddress = new(targetEmail);

        // Set message.
        MailMessage Message = new(s_sourceAddress, TargetAddress);
        Message.Body = content;
        Message.Subject = subject;

        // Send.
        Client.SendAsync(Message, null);
    }

    internal void SendVerificationEmail(string targetEmail)
    {
        string Verification = GenerateVerificationString();
        UseVerificationString(Verification, targetEmail);

        string Message = "Šis e-pasts ir nosūtīts, jo jūsu e-pasta adrese tika izmantota, lai veidotu kontu mājaslapā LostThingThing." +
            $"\nLai apstiprinātu konta izveidi, lūdzu atveriet saiti: http://{Server.SiteAddress}/{Verification}" +
            $"\nJa šī ir bijusi kļūda, varat e-pastu ignorēt.";

        SendEmail(targetEmail, "LostThingThing konta izveides apstiprināšana.", Message);
    }

    internal bool VerifyEmail(string email, string verification)
    {
        if (!_verifications.ContainsKey(email))
        {
            return false;
        }
        bool Result = _verifications[verification].email == email;

        if (Result)
        {
            _verifications.Remove(verification);
        }
        return Result;
    }


    // Private methods.
    private string GenerateVerificationString()
    {
        StringBuilder VerificationString = new(64);

        const int VERIFICATION_STRING_LENGTH = 64;
        for (int i = 0; i < VERIFICATION_STRING_LENGTH; i++)
        {
            switch  (Random.Shared.Next(3))
            {
                case 1:
                    VerificationString.Append((char)Random.Shared.Next('a', 'z'));
                    break;
                case 2:
                    VerificationString.Append((char)Random.Shared.Next('A', 'Z'));
                    break;
                default:
                    VerificationString.Append((char)Random.Shared.Next('0', '9'));
                    break;
            }
            
        }

        return VerificationString.ToString();
    }

    private void UseVerificationString(string verification, string email)
    {
        _verifications.Add(verification, (email, DateTime.Now));
    }
}