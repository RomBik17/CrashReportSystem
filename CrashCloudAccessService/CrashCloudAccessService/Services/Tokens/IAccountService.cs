
using System.Collections.Generic;
using System.Threading.Tasks;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.Services.Tokens
{
    public interface IAccountService
    {
        Task<List<Person>> GetAsync();
        Task<Person> GetByIdAsync(string hash);
        Task<Person> UpdateAccountHashAsync(string hash);
        Task RemoveAsync(string hash);
        Task<Person> AddAsync(Person account);
    }
}
