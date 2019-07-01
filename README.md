# RansomWatch

RansomWatch is a solution which monitors and analyses data collected from the file system in real time in order to identify suspicious ransomware behavior on the file system. 
RansomWatch autonomously stops ransomware applications and backups data in order to prevent data loss.

User-mode GUI application is responsible for handling all data of running applications based on their GID* given by the driver, reporting to user, backup, sending kill requests to driver with GIDs detected as malicious and restoring changed files.

The driver collects file system usage, calculates entropy for read/write operations, gives multi-process applications a unique GID for tracking. 
It is also responsible for killing malicious applications based on their GID (system processes do not have GID, thus will not be killed).

Application operations are passed to RansomWatch application based on user-mode application request for IRPs (I/O Request Packet) with `filterSendMessage`. System applications are not recorded, which reduces a significant overhead. Our application records IRP operations usage. After each recording of IRP operation for application, RansomWatch application checks if the state of some applications has changed to malicious.

GID is given by the driver each time a new application is interduced to the system by registering to process creation and ending (`PsSetCreateProcessNotifyRoutine`). The driver evaluates the image file loaded and decides whether it is a system process - if not, a new GID is assigned and every process generated from this process is given the same GID.

After detecting malicious application and stopping it with our driver, RansomWatch application tries to recover files that were changed using Azure storage. The application can only recover areas that were selected to be protected from the application. The application restores based on last known valid snapshot known (based on dates of first ransomware detection in system and snapshot time).

The user-mode application is written in C++/CLI.

<br/>
*GID - Group Identifier. Not to be confused with Linux's GID, which is something completely different. It is the name of the unique identifier the driver assigns to a set of all PIDs (process identiders) that are related to one another by process creation. For example, if a process A creates a process B, A and B will share a common GID (the GID that was assigned to process A).


## Examples

<iframe width="560" height="315" src="https://www.youtube.com/embed/91QNTsN7H6Y" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

<iframe width="560" height="315" src="https://www.youtube.com/embed/awjAM1B6XCc" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>


## Open issues

1. Backup service is incomplete. Backup is done only once, when the user selects new folders to protect. A complete solution should at the very least support periodical backup.
2. Only dynamic detection is done. Static detectiong should also be used, for example in order to find patterns in exacutables and check for known malicious signatures.
3. Detection model should consider time as a factor.
4. Currently the solution does not remove the identified malicious applications from startup folder of Windows, Windows Registry and tasks added to task scheduler.
5. Our driver save image file loaded for each PID it tracks. This data can be used to clean ransomware.
6. There is no serialization of data - protected areas and traps should be saved so that RansomWatch application would be able to restore its state after shut down.
7. The solution does not protect itself. That is, a malicious application can change RansomWatch files and alter its behavior.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.


### Dependencies

1. Microsoft Visual Studio version 2019 (2017 should be OK as well but we have not tested it).
2. Windows Driver Kit (WDK).
3. .NET 4.7.2 or newer.
4. Windows SDK.
5. Visual studio build tools 1.4.2 or newer.
6. C++/CLI support for Visual Studio build tools 1.4.2 or newer.
7. Microsoft WindowsAzure Storage 9.3.3. 


### Building

1. Clone or download this project and load RWatch.sln with Visual Studio.
2. Make sure that the configuration manager is set to x64.
3. Build the solution. This builds both the driver and the application. 

### Installing

1. Our driver uses test signing so before using it Windows should be set to install and run test signed drivers.

	Enable test signed drivers can be done in elevated command prompt with the following command:
	```
	bcdedit /set testsigning on
	```
	Restart Windows for changes to take effect.


2. Copy the solution application (`application.exe`) and the DLL next to it (`Microsoft.WindowsAzure.Storage.dll`) to the target machine. those files are generated under: `\<project location>/x64/Release`
3. Copy driver files: `FsFilter.inf, FsFilter.sys, FsFilter.cer` to the target machine and place them in the same directory. Those files are generated under:  `\<project location>/x64/Debug`

4. Install the driver using .inf file (requires elevated command prompt)
	```
	RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 \<Driver files location>\FsFilter.inf
	```

5. Start the driver using `fltmc` command or by `sc` command.
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

For running the application, Visual Runtime for Windows is required.
The application requires the driver to run to work properly.


## Testing

Testing ransomwares requires a VM (well, unless you do not mind testing it on your machine) and a ransomware applications.

1. After installing the driver and running the application, add a directory to the protected areas with the application.
2. The application will generate trap files for each extension in the sub-directories of the protected areas and snapshot files to an Azure storage.
3. Run a ransomware.
4. After detection, the application generates a log file under system drive with the application GID. Detection information is also written to a log viewer inside the application.


## Authors

* **Rafael Wurf**
* **Aviad Gafni**


## License

This project is licensed under the MIT License - see [MIT](https://choosealicense.com/licenses/mit/) for details