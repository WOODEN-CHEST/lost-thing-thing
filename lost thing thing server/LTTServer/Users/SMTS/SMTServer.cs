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
    private readonly Dictionary<string, (ProfileInfo profileInfo, DateTime timeSent)> _verifications = new();


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
        try
        {
            Client.SendAsync(Message, null);
        }
        catch (Exception e)
        {
            Server.OutputError($"Failed to send an email to address \"{targetEmail}\" with the content \"{content}\". {e}");
        }
    }
}