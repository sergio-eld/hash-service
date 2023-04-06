## Base assingment
Test Assignment - Hash Server



Develop TCP Server for hash sum calculation. The client application (e.g. telnet or netcat) should be able to open TCP connection and send strings terminated by “\n” (single byte with the value of 10). The Server should calculate a hash (of any type of your choice) and send it back in HEX terminated by “\n” as well.



The client requests should be processed in parallel to be able to utilize all of the server CPU cores. The server should be memory effective and send available data back as soon as it’s available. Incoming strings should not be limited by length.



It is allowed to use any library from standard packages of Ubuntu 20. All of the application modules should have unit test coverage. The server and tests should be able to build and run on Ubuntu 20 as well.



The code should be clean, use any coding style of your choice. There should be README with a good description of how to build and run the server and tests. The use of docker files and/or build scripts is encouraged. The code should be placed on github.com or any other open code repository.  

### Clarifications
1. Can a client send several strings (separated by '\n') within a single request?
> Yes
2. Is it safe to assume that a client can maintain a tcp connection for further requests?
> Yes
3. Are there any predictions on the size of a request string (median/max)?
> No limit, 100 TB is still accessible input
4. Would it be reasonable to provide some lightweight protocol to indicate possible run-time errors?
> Like what?
5. Am I expected to provide some stress-testing suite as a part of the Primary assignment?
> Up to you, but at least main functionality must be tested.
6. Ubuntu 20(.04?) is specified as a target system. Is this project expected to be cross-platform as a part of the Primary assignment?
> It should build and run on Ubuntu 20.04, other platforms are optional.
7. Is it safe to assume that there will be no malicious actions towards the server (DDoS, spamming requests, etc)? 
> The server should be resource efficient, no specific DoS protection is needed.