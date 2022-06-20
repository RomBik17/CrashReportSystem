
using System.Collections.Generic;
using System.Threading.Tasks;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.DB.Dumps
{
    public interface IDumpRepository
    {
        Task<List<Dump>> GetAsync();
        Task<Dump> GetByPropertiesAsync(string dumpHash, string hardware);
        Task<Dump> GetByIdAsync(int id);
        Task<Dump> UpdateCrashesAsync(int id);
        Task RemoveAsync(int id);
        Task<Dump> AddAsync(Dump dump);
    }
}
