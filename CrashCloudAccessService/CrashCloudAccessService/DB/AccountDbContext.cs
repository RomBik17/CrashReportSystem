
using Microsoft.EntityFrameworkCore;
using CrashCloudAccessService.DB.Accounts;
using CrashCloudAccessService.DB.Dumps;

namespace CrashCloudAccessService.DB
{
    public class AccountDbContext : DbContext
    {
        public AccountDbContext(DbContextOptions<AccountDbContext> options) : base(options)
        {
            Database.EnsureCreated();
        }

        public DbSet<AccountDto> Accounts { get; set; }
        public DbSet<DumpDto> Dumps { get; set; }
    }
}
