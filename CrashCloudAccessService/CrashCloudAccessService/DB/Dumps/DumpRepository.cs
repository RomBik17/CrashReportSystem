using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using AutoMapper;
using Microsoft.EntityFrameworkCore;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.DB.Dumps
{
    public class DumpRepository : IDumpRepository
    {
        private readonly AccountDbContext _context;
        private readonly IMapper _mapper;

        public DumpRepository(IMapper mapper, AccountDbContext context)
        {
            _context = context;
            _mapper = mapper;
        }

        public async Task<Dump> AddAsync(Dump dump)
        {
            var entity = _mapper.Map<DumpDto>(dump);
            var result = await _context.Dumps.AddAsync(entity);
            await _context.SaveChangesAsync();
            return _mapper.Map<Dump>(result.Entity);
        }

        public async Task<List<Dump>> GetAsync()
        {
            var entities = await _context.Dumps.ToListAsync();
            return _mapper.Map<List<Dump>>(entities);
        }

        public async Task<Dump> GetByPropertiesAsync(string dumpHash, string hardware)
        {
            var entity = await _context.Dumps.FirstOrDefaultAsync(m => m.DumpHash == dumpHash && m.Hardware == hardware);
            if (entity == null) throw new ArgumentException();
            return _mapper.Map<Dump>(entity);
        }

        public async Task<Dump> GetByIdAsync(int id)
        {
            var entity = await _context.Dumps.FirstOrDefaultAsync(m => m.Id == id);
            if (entity == null) throw new ArgumentException();
            return _mapper.Map<Dump>(entity);
        }

        public async Task RemoveAsync(int id)
        {
            var entity = await _context.Dumps.FirstOrDefaultAsync(m => m.Id == id);
            _context.Dumps.Remove(entity);
            await _context.SaveChangesAsync();
        }

        public async Task<Dump> UpdateCrashesAsync(int id)
        {
            var entity = await _context.Dumps.FirstOrDefaultAsync(m => m.Id == id);
            ++entity.Crashes;
            await _context.SaveChangesAsync();
            return _mapper.Map<Dump>(entity);
        }
    }
}
