# RansomWatch

Anti-ransomware application and minifilter driver for Windows.
RansomWatch is a solution which monitors and analyses data collected from the file system in real time in order to identify suspicious ransomware behavior on the file system. 
RansomWatch autonomously stops ransomware applications and backup files and directories to prevent data loss.

User mode gui application is responsible for handling all the data of applications based on their gid given by the driver, report to user, backup protected areas, send kill requests to the driver with gid detected as malicious and restore changed files.

RansomWatch driver collects file system usage by application, calculate entropy for read and write operations, give multi process applications unique gid number for tracking. 
The driver is also responsible for killing malicious applications based on their gid (system processes dont have gid, cant kill system process).

Applications operations are passed to user mode application based on user mode application request for irps with filterSendMessage. We reduce overhead by not reporting system applications. Our application record applications irp ops usage. After each recording of irp ops for applications we check if they are malicious.

GID is given by the driver each time a new application enter the system by registring to process creation and ending, we evaluate the image file loaded and decide if its system process, if not a GID is given and every process generated from process with GID is given the same number.

After detecting malicious application and stopping it with our driver, we try to restore files changed by ransomware using Azure storage. when selecting areas to protect our appliication backup protected areas. After detecting a ransomware we restore files from backup based on last known valid snapshot known (based on dates of first ransomware detection in system and snapshot time).

The user mode application is written in C++ cli (clr), mixture of native and managed code.

## Open issues

1. Backup service isnt complete, when selecting protected areas we backup its directories but after that no other backups are saved, complete solution should backup more.
2. Our detection model is dynamic only, static model should also be used to find patterns in code and check known signatures.
3. Detection model should consider time as a factor.
4. Complete solution should remove application for start-up folders of Windows, Windows Registry and tasks added to task scheduler.
5. Our driver save image file loaded for each pid it tracks, this data can be used to clean ransomware.
6. There is no serialization of data, protected areas and traps should be saved when application exit and loaded when starting.
7. Our solution doesn't protect itself, we use Azure dll for example, but we don't protect it.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Dependencies

1. Microsoft Visual Studio version 2019 (2017 should be ok but not tested).
2. Windows Driver Kit (WDK).
3. .NET 4.7.2 or newer.
4. Windows SDK.
5. Visual studio build tools 1.4.2 or newer.
6. C++ cli support for Visual Studio build tools 1.4.2 or newer.
7. Microsoft WindowsAzure Storage 9.3.3. 

### Building

1. Clone or Download this project and load RWatch.sln with Visual Studio.
2. Make sure that the configuration manager is set to x64.
3. Build solution, creates both driver and application. 

### Installing

1. Our driver uses test signing so before using it Windows should be set to install and run test signed drivers.

	Enable test signed drivers can be done in elevated command prompt with the command:
	```
	bcdedit /set testsigning on
	```
	After running this command restart Windows.


2. Copy the solution application (```application.exe```) and the dll next to it (```Microsoft.WindowsAzure.Storage.dll```) to the target machine. those files are generated under: \<project location>/x64/Release
3. Copy driver files: ```FsFilter.inf, FsFilter.sys, FsFilter.cer``` to the target machine and place them in the same directory. those files are generated under:  \<project location>/x64/Debug

4. Install the driver using inf file (requires elevated command prompt)
	```
	RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 \<Driver files location>\FsFilter.inf
	```

5. Start the driver using ``fltmc`` command or by ``sc`` command.
   We tested our solution with service control manager sc:
	```
	sc start FsFilter
	```

6. Stopping the driver by using the command:
	```
	sc stop FsFilter
	```

6. Removing the driver by using the command:
	```
	sc delete FsFilter
	```

7. Run the application, we recommend to run it as Admin.
 
### Prerequisites for running

For running the application visual runtime for windows is required.
The application requires the driver to run to work properly.

## Testing

Testing ransomwares requires a VM and a ransomware applications.

1. After installing the driver and running the application, add a directory to the protected areas with the application.
2. The application will generate trap files for each extension in subdirs of protected areas and snapshot files to Azure storage.
3. Run a ransomware.
4. After detection the application generates a log file under system drive with the application gid.
Detection info also written to a log viewer inside the application.

## Authors

* **Rafael Wurf**
* **Aviad Gafni**

## License

This project is licensed under the MIT License - see [MIT](https://choosealicense.com/licenses/mit/) for details