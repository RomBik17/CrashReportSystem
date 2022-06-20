
using System;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using CrashCloudAccessService.Models;
using CrashCloudAccessService.Services.Dumps;

namespace CrashCloudAccessService.Controllers
{
    [Route("api/[controller]")]
    public class DumpController : ControllerBase
    {
        private readonly IDumpService _dumpService;

        public DumpController(IDumpService dumpService)
        {
            _dumpService = dumpService;
        }
        
        //Not authorization request to check if API is working
        public IActionResult Get()
        {
            return Ok("File Upload API running...");
        }

        /*      Create shared authentication token for Azure storage.
                With shared authentication token you can upload file directly to storage, and client won't know any information
            about storage and admin.
        */
        [Authorize]
        [HttpPost]
        [DisableRequestSizeLimit]
        [Route("upload")]
        public async Task<IActionResult> Upload(string fileName)
        {
            string sasToken = await _dumpService.Upload(fileName);
            return Ok(sasToken);
        }

        //Find file in Azure storage
        [Authorize]
        [HttpGet("{fileName}")]
        [Route("search")]
        public async Task<IActionResult> Search(string fileName)
        {
            bool result = _dumpService.FindFile(fileName);
            return Ok(result);
        }

        /*  Update information about crashes.
            If earlier there were the same dumps with the same hardware system, increase crashes.
            Else create a new data about this crash.
         */
        [Authorize]
        [HttpPost]
        [Route("context")]
        public async Task<IActionResult> Context(string dumpHash, string hardware)
        {
            Dump dump;
            try
            {
                dump = await _dumpService.GetByPropertiesAsync(dumpHash, hardware);
            }
            catch (Exception ex)
            {
                Dump tempDump = new Dump();

                tempDump.DumpHash = dumpHash;
                tempDump.Hardware = hardware;
                tempDump.Crashes = 0;

                dump = await _dumpService.AddAsync(tempDump);
            }

            await _dumpService.UpdateCrashesAsync(dump.Id);

            return Ok("Context updated");
        }
    }
}
