# RansomWatch

Anti-ransomware application and mini-filter driver for Windows.
RansomWatch is a solution which monitors and analyses data collected from the file system in real time in order to identify suspicious ransomware behavior on the file system. 
RansomWatch autonomously stops ransomware applications and backup files and directories to prevent data loss.

RansomWatch driver collects file system usage by application, calculate entropy for read and write operations, give multi process applications gid. file system data is passed to user mode application based on user mode application request for irpswith filterSendMessage. We reduce overhead by not reporting system applications. The driver is also responsible for killing malicious applications based on their gid (system processes dont have gid, cant kill system process).

User mode gui application is responsible for handling all the data of applications based on their gid given by the driver, report to user, backup protected areas and send kill requests to the driver with a given gid name which was detected as malicious.

The user mode application is written in C++ clr (C++ cli) with mixture of native and managed code.

## Open issues

1. create alghoritm for adding traps at startup
2. remedy plan - copy file to blob storage when write/rename/delete is done to it by untrusted process
3. complete backup azure blob service with snapshots

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

For running the application visual runtime for windows is required.
The application requires the driver to run to work properly.

### Installing

Testing of the driver requires a VM for testing, installing the driver is simple... (inf and sys file)
A step by step series of examples that tell you how to get a development env running

Say what the step will be

```
Give the example
```

And repeat

```
until finished
```

End with an example of getting some data out of the system or using it for a little demo

## Running the tests

Explain how to run the automated tests for this system

### Break down into end to end tests

Explain what these tests test and why

```
Give an example
```

### And coding style tests

Explain what these tests test and why

```
Give an example
```

## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Rafael Wurf**
* **Aviad Gafni**

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone whose code was used
* Inspiration
* etc
