
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Azure.Storage.Blobs;
using Azure.Storage.Blobs.Models;
using Microsoft.Extensions.Configuration;
using Microsoft.WindowsAzure.Storage;
using Microsoft.WindowsAzure.Storage.Blob;
using CrashCloudAccessService.DB.Dumps;
using CrashCloudAccessService.Models;

namespace CrashCloudAccessService.Services.Dumps
{
    public class DumpService : IDumpService
    {
        private readonly BlobServiceClient _blobServiceClient;
        private readonly IConfiguration _configuration;
        private readonly IDumpRepository _dumpRepository;

        public DumpService(BlobServiceClient blobServiceClient, IConfiguration configuration, IDumpRepository dumpRepository)
        {
            _blobServiceClient = blobServiceClient;
            _configuration = configuration;
            _dumpRepository = dumpRepository;
        }

        public async Task<string> Upload(string fileName)
        {
            var containerName = _configuration.GetSection("Storage:ContainerName").Value;
            var connectionString = _configuration.GetSection("Storage:ConnectionString").Value;

            //Connect to Azure storage
            CloudStorageAccount account = CloudStorageAccount.Parse(connectionString);
            CloudBlobClient serviceClient = account.CreateCloudBlobClient();

            var container = serviceClient.GetContainerReference(containerName);
            container.CreateIfNotExistsAsync().Wait();
            
            //Create blob reference
            CloudBlockBlob blob = container.GetBlockBlobReference(fileName);

            //Initialize properties for shared authentication token
            SharedAccessBlobPolicy sasConstraints = new SharedAccessBlobPolicy();
            sasConstraints.SharedAccessExpiryTime = DateTime.UtcNow.AddMinutes(30);
            sasConstraints.Permissions = SharedAccessBlobPermissions.Write | SharedAccessBlobPermissions.Create;

            //Create shared authentication token for Azure storage
            return blob.Uri + blob.GetSharedAccessSignature(sasConstraints);
        }

        public bool FindFile(string fileName)
        {
            var containerName = _configuration.GetSection("Storage:ContainerName").Value;
            var containerClient = _blobServiceClient.GetBlobContainerClient(containerName);

            //Check the whole Azure storage for file
            foreach (BlobItem blobItem in containerClient.GetBlobs())
            {
                if (blobItem.Name == fileName) return true;
            }

            return false;
        }

        public async Task<Dump> AddAsync(Dump person)
        {
            return await _dumpRepository.AddAsync(person);
        }

        public async Task<List<Dump>> GetAsync()
        {
            return await _dumpRepository.GetAsync();
        }

        public async Task<Dump> GetByPropertiesAsync(string dumpHash, string hardware)
        {
            if (dumpHash == "" || hardware == "")
            {
                throw new ArgumentOutOfRangeException();
            }
            return await _dumpRepository.GetByPropertiesAsync(dumpHash, hardware);
        }

        public async Task RemoveAsync(int id)
        {
            var entity = await _dumpRepository.GetByIdAsync(id);
            if (entity == null)
            {
                throw new ArgumentNullException();
            }

            await _dumpRepository.RemoveAsync(entity.Id);
        }

        public async Task<Dump> UpdateCrashesAsync(int id)
        {
            var entity = await _dumpRepository.GetByIdAsync(id);
            if (entity == null)
            {
                throw new ArgumentNullException();
            }

            await _dumpRepository.UpdateCrashesAsync(id);
            return entity;
        }
    }
}
