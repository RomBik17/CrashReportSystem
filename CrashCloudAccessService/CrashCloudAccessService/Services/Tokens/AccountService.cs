using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using CrashCloudAccessService.DB.Accounts;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.Services.Tokens
{
    public class AccountService : IAccountService
    {
        private readonly IAccountRepository _accountRepository;
        public AccountService(IAccountRepository accountRepository)
        {
            _accountRepository = accountRepository;
        }
        public async Task<Person> AddAsync(Person person)
        {
            return await _accountRepository.AddAsync(person);
        }

        public async Task<List<Person>> GetAsync()
        {
            return await _accountRepository.GetAsync();
        }

        public async Task<Person> GetByIdAsync(string hash)
        {
            if (hash == "")
            {
                throw new ArgumentOutOfRangeException();
            }
            return await _accountRepository.GetByIdAsync(hash);
        }

        public async Task RemoveAsync(string hash)
        {
            var entity = await _accountRepository.GetByIdAsync(hash);
            if (entity == null)
            {
                throw new ArgumentNullException();
            }

            await _accountRepository.RemoveAsync(entity.Hash);
        }

        public async Task<Person> UpdateAccountHashAsync(string hash)
        {
            var entity = await _accountRepository.GetByIdAsync(hash);
            if (entity == null)
            {
                throw new ArgumentNullException();
            }

            if (hash == "")
            {
                throw new ArgumentOutOfRangeException();
            }

            entity.Hash = hash;

            await _accountRepository.UpdateAsync(entity);
            return entity;
        }
    }
}
