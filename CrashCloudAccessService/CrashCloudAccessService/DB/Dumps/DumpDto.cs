
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace CrashCloudAccessService.DB.Dumps
{
    [Table("tbl_dumps")]
    public class DumpDto
    {
        [Column("Id"), Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
        public int Id { get; set; }

        [Column("DumpHash")]
        public string DumpHash { get; set; }

        [Column("Hardware")]
        public string Hardware { get; set; }

        [Column("Crashes")]
        public int Crashes { get; set; }
    }
}
