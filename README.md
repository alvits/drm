### Dynamic Reconfiguration Management Framework

**`Makefile` is written to generate the following executables.**

`DOMSERVER=dom0` - This is the server daemon for the hypervisor that listens for connections and accepts messages from domUs.

`TESTDOMU=domU` - This is a test client for domUs to send a test message t dom0.

`DOMCLIENT=dom_relay` - This is a domU client that relays messages received from JVMs thru domain socket and sends them to dom0.

`TESTSERVER=threadserver` - This is a domU test server to test domain sockets without relaying to domU. This is basically dom_relay without connectivity to dom0.

`TESTCLIENT=testclient` - This is a domU test client to send test messages.

`TESTPE=testpe` - This is the client code to send message to Policy Engine.

**The following macros are defined on Makefile.**

`__USE_POSIX` - To use posix source code compilation directive.

**The following set of macros will be disabled for production executables.**

`DEBUG_DRM` - To print debug messages.

`TWO_WAY` - To return received messages back to sender.
