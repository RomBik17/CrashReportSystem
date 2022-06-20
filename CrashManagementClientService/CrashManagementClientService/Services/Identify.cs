using System;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;

namespace CorsairWorkerService.Services
{
    public class Identify
    {
        public static ulong ParseDumpKey(string txtFile)
        {
            string keyword = "Dump Key Value: ";
            using (var sr = new StreamReader(txtFile))
            {
                while (!sr.EndOfStream)
                {
                    string line = sr.ReadLine();
                    if (String.IsNullOrEmpty(line)) continue;
                    if (line.IndexOf(keyword, StringComparison.CurrentCultureIgnoreCase) >= 0)
                    {
                        line = line.Replace(keyword, "");
                        return Convert.ToUInt64(line);
                    }
                }
            }

            return 0;
        }

        public static string ParseHardwareInfo(string txtFile)
        {
            string keyword = "Hardware Info:";
            string hardwareInfo = "";
            using (var sr = new StreamReader(txtFile))
            {
                bool isHardwareInfo = false;
                while (!sr.EndOfStream)
                {
                    string line = sr.ReadLine();

                    if (String.IsNullOrEmpty(line))
                    {
                        isHardwareInfo = false;
                        continue;
                    }

                    if (line.IndexOf(keyword, StringComparison.CurrentCultureIgnoreCase) >= 0) isHardwareInfo = true;

                    if (isHardwareInfo) hardwareInfo += line;
                }
            }

            return hardwareInfo;
        }

        public static string CreateUserId()
        {
            string id = "";

            //cpu id
            ManagementObjectSearcher cpu = new ManagementObjectSearcher("Select * From Win32_processor");
            ManagementObjectCollection cpuList = cpu.Get();
            foreach (ManagementObject mo in cpuList)
            {
                id += mo["ProcessorID"].ToString();
            }

            //hard drive id
            ManagementObject dsk = new ManagementObject(@"win32_logicaldisk.deviceid=""c:""");
            dsk.Get();
            id += dsk["VolumeSerialNumber"].ToString();

            //motherboard id
            ManagementObjectSearcher motherboard = new ManagementObjectSearcher("SELECT * FROM Win32_BaseBoard");
            ManagementObjectCollection motherboardList = motherboard.Get();
            foreach (ManagementObject mo in motherboardList)
            {
                id += (string)mo["SerialNumber"];
            }

            return id;
        }

        //ToDo change dll directory
        //Build in x86
        [DllImport(@"C:\Users\rrafa\Desktop\CrashManagementClientService\Debug\IdentifyDll.dll", CharSet = CharSet.Auto)]
        public static extern bool Identify_Dump(ulong Dump_Key_Value, [MarshalAs(UnmanagedType.BStr)] string Path_For_Dumps);
    }
}
