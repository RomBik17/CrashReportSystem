using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.WindowsAzure.Storage.Blob;
using System;
using System.IO;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading;
using System.Threading.Tasks;
using CorsairWorkerService.Services;

namespace CorsairWorkerService
{
    public class Worker : BackgroundService
    {
        private readonly ILogger<Worker> _logger;

        //ToDo change folder address
        private const string _folderAddress = @"C:\FileSystemTest";
        private static HttpClient _client;

        public Worker(ILogger<Worker> logger)
        {
            _logger = logger;
        }

        public override Task StartAsync(CancellationToken cancellationToken)
        {
            _client = new HttpClient();

            //ToDo change API Uri
            _client.BaseAddress = new Uri("https://rombikapi.azurewebsites.net");
            _client.DefaultRequestHeaders.Accept.Clear();
            _client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

            return base.StartAsync(cancellationToken);
        }

        public override Task StopAsync(CancellationToken cancellationToken)
        {
            _client.Dispose();
            return base.StopAsync(cancellationToken);
        }

        private bool CheckConnectionToApi()
        {
            bool connection = true;

            try
            {
                System.Net.WebClient client = new System.Net.WebClient();
                string result = client.DownloadString(_client.BaseAddress + "api/dump");
            }
            catch (System.Net.WebException ex)
            {
                connection = false;
            }

            return connection;
        }

        private async Task<bool> FindFileAsync(string fileName, string token)
        {
            bool result = false;
            _client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", token);
            HttpResponseMessage response = await _client.GetAsync("api/dump/" + fileName);

            if (response.IsSuccessStatusCode)
            {
                result = await response.Content.ReadAsAsync<bool>();
                if (result)
                {
                    _logger.LogInformation("SuccessStatusCode, " + fileName + " found");
                }
                else
                {
                    _logger.LogInformation("SuccessStatusCode, " + fileName + " not found");
                }
            }
            else
            {
                _logger.LogInformation("Internal Error");
            }

            return result;
        }

        private async Task SendFile(string fileDestinition, string token)
        {
            string url = "api/dump/upload?fileName=" + Path.GetFileName(fileDestinition);
            _client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", token);
            HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, _client.BaseAddress + url);
            HttpResponseMessage response = await _client.SendAsync(request);

            if (response.IsSuccessStatusCode)
            {
                string sasToken = await response.Content.ReadAsAsync<string>();
                var cloudBlockBlob = new CloudBlockBlob(new Uri(sasToken));
                await cloudBlockBlob.UploadFromFileAsync(fileDestinition);
                _logger.LogInformation("SuccessStatusCode, " + Path.GetFileName(fileDestinition) + " uploaded");
            }
            else
            {
                _logger.LogInformation("Internal Error");
            }
        }

        private async Task UpdateContext(string dumpHash, string hardwareInfo, string token)
        {
            string url = "api/dump/context?dumpHash=" + dumpHash + "&hardware=" + hardwareInfo;
            _client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", token);
            HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, _client.BaseAddress + url);
            HttpResponseMessage response = await _client.SendAsync(request);

            if (response.IsSuccessStatusCode)
            {
                _logger.LogInformation("SuccessStatusCode, Context Updated");
            }
            else
            {
                _logger.LogInformation("Internal Error, not can't update context");
            }
        }

        private async Task DumpWorker(string token)
        {
            string[] dumpFiles = Directory.GetFiles(_folderAddress, "*.dmp");

            if (dumpFiles.Length > 0)
            {
                foreach (var dump in dumpFiles)
                {
                    ulong dumpKey;

                    string[] txtFiles = Directory.GetFiles(_folderAddress, Path.GetFileNameWithoutExtension(dump) + ".txt");
                    string txtFile = "";
                    if (txtFiles.Length > 0) txtFile = txtFiles[0];

                    if (txtFile != "") dumpKey = Identify.ParseDumpKey(txtFile);
                    else
                    {
                        _logger.LogInformation("No metadata file");
                        continue;
                    }

                    string hardwareInfo = Identify.ParseHardwareInfo(txtFile);

                    if (!Identify.Identify_Dump(dumpKey, dump))
                    {
                        _logger.LogInformation("Identify Error");
                        continue;
                    }

                    string dumpName = Path.GetFileName(dump);
                    bool foundDump = await FindFileAsync(dumpName, token);
                    if (!foundDump)
                    {
                        await SendFile(dump, token);
                    }

                    bool foundTxt = await FindFileAsync(Path.GetFileName(txtFile), token);
                    if (!foundTxt)
                    {
                        await SendFile(txtFile, token);
                    }

                    UpdateContext(Path.GetFileNameWithoutExtension(dump), hardwareInfo, token);

                    File.Delete(txtFile);
                    File.Delete(dump);
                }
            }
        }

        private async Task<string> GetToken(string username)
        {
            string token = "no token";

            string url = "api/account/token?username=" + username;
            HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Get, _client.BaseAddress + url);
            HttpResponseMessage response = await _client.SendAsync(request);

            if (response.IsSuccessStatusCode)
            {
                _logger.LogInformation("SuccessStatusCode, Authorized");
                token = await response.Content.ReadAsAsync<string>();
            }
            else
            {
                _logger.LogInformation("Internal Error, not authorized");
            }

            return token;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                bool connection = CheckConnectionToApi();

                if (connection)
                {
                    _logger.LogInformation("Connected to API");

                    string userId = Identify.CreateUserId();

                    //authorize
                    string token = await GetToken(userId);
                    //run worker
                    if (token != "no token") await DumpWorker(token);
                }
                else _logger.LogInformation("No connection to API");

                await Task.Delay(10000, stoppingToken);
            }
        }
    }
}
