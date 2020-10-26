Data & Communication Networks
Project 1 - Socket Programming
Peter Fabinski (pnf9945)

Notes:
- Build with "make"
- Run client/server with "make run_client"/"make run_server"
- Test script is not changed. Run with "make test"
- Hostnames can be used as well as IP addresses (thanks to getaddrinfo)
- Port can be set in common.h (don't use parens around it, the stringify won't work)
-> Default port is 2100
- Server will bind to all local interfaces

