Data & Communication Networks
Project 1 - Socket Programming
Peter Fabinski (pnf9945)

Notes:
- Build with "make"
- Run client/server with "make run_client"/"make run_server"
- Generate test files with "make gen" (takes a minute to run, maybe reduce size)
 -> 1 and 2 are different with recognizable text
 -> 3-100 are 50MB files from /dev/zero
 -> Test files can be removed with "make cleangen"
- Test script is not changed. Run with "make test"
- Hostnames can be used as well as IP addresses in tconnect
- Port can be set in common.h (don't use parens around it, the stringify won't work)
 -> Default port is 2100
- Server will bind to local interfaces

