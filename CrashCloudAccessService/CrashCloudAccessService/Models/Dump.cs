
namespace CrashCloudAccessService.Models
{
    public class Dump
    {
        public int Id { get; set; }
        public string DumpHash { get; set; }
        public string Hardware { get; set; }
        public int Crashes { get; set; }
    }
}
