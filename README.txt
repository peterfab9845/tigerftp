Data & Communication Networks
Project 1 - Socket Programming
Peter Fabinski (pnf9945)

Notes:
- Build with "make", this puts server binary in server/TigerS and client binary in client/TigerC
- Run client/server with "make run_client"/"make run_server", or manually from directories
- Generate test files with "make gen" (takes a minute to run, maybe reduce count var in gen.sh for smaller files)
 -> files 1 and 2 are different with recognizable text
 -> files 3-100 are 50MB files from /dev/zero
 -> Test files can be removed with "make cleangen"
- Test script is not changed. Start server, then run with "make test" or manually from client dir
- Hostnames can be used as well as IP addresses in tconnect (via getaddrinfo from netdb.h)
- Port can be set in common.h (don't use parens around it, the stringify won't work)
 -> Default port is 2100
- Server will bind to all available interfaces
- Users and passwords are set in "users.txt" in server/ dir

-- tested on same computer and multiple computers
-- threading is implemented
-- transfers work equally well for binary or ASCII files (all are done in binary mode)

