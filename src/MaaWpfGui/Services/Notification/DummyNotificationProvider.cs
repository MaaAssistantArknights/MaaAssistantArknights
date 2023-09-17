using System.Threading.Tasks;

namespace MaaWpfGui.Services.Notification
{
    public class DummyNotificationProvider : IExternalNotificationProvider
    {
        public Task<bool> SendAsync(string title, string content)
        {
            return Task.FromResult(true);
        }
    }
}
