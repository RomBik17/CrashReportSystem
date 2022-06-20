
using System.Collections.Generic;
using System.Threading.Tasks;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.Services.Dumps
{
    public interface IDumpService
    {
        public Task<string> Upload(string fileName);
        public bool FindFile(string fileName);
        Task<List<Dump>> GetAsync();
        Task<Dump> GetByPropertiesAsync(string dumpHash, string hardware);
        Task<Dump> UpdateCrashesAsync(int id);
        Task RemoveAsync(int id);
        Task<Dump> AddAsync(Dump dump);
    }
}
