
using System.Collections.Generic;
using System.Threading.Tasks;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.DB.Accounts
{
    public interface IAccountRepository
    {
        Task<List<Person>> GetAsync();
        Task<Person> GetByIdAsync(string hash);
        Task<Person> UpdateAsync(Person person);
        Task RemoveAsync(string hash);
        Task<Person> AddAsync(Person person);
    }
}
