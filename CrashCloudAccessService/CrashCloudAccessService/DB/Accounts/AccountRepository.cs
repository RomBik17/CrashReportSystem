using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using AutoMapper;
using Microsoft.EntityFrameworkCore;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.DB.Accounts
{
    public class AccountRepository : IAccountRepository
    {
        private readonly AccountDbContext _context;
        private readonly IMapper _mapper;

        public AccountRepository(IMapper mapper, AccountDbContext context)
        {
            _context = context;
            _mapper = mapper;
        }

        public async Task<Person> AddAsync(Person person)
        {
            var entity = _mapper.Map<AccountDto>(person);
            var result = await _context.Accounts.AddAsync(entity);
            await _context.SaveChangesAsync();
            return _mapper.Map<Person>(result.Entity);
        }

        public async Task<List<Person>> GetAsync()
        {
            var entities = await _context.Accounts.ToListAsync();
            return _mapper.Map<List<Person>>(entities);
        }

        public async Task<Person> GetByIdAsync(string hash)
        {
            var entity = await _context.Accounts.FirstOrDefaultAsync(m => m.Hash == hash);
            if (entity == null) throw new ArgumentException();
            return _mapper.Map<Person>(entity);
        }

        public async Task RemoveAsync(string hash)
        {
            var entity = await _context.Accounts.FirstOrDefaultAsync(m => m.Hash == hash);
            _context.Accounts.Remove(entity);
            await _context.SaveChangesAsync();
        }

        public async Task<Person> UpdateAsync(Person person)
        {
            var entity = _mapper.Map<AccountDto>(person);
            var result = _context.Accounts.Update(entity);
            await _context.SaveChangesAsync();
            return _mapper.Map<Person>(result.Entity);
        }
    }
}
