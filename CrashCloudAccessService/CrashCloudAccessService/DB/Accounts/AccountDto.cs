
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace CrashCloudAccessService.DB.Accounts
{
    [Table("tbl_tokens")]
    public class AccountDto
    {
        [Column("Hash"), Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
        public string Hash { get; set; }
    }
}
