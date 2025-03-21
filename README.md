# ToDo
Complete Initial Project

This project is currently not complete and is based heavily on virtual square vde-2 GitHub - virtualsquare/vde-2 as a direct port to windows but is not a fork
This project also includes code from vdeplug4 https://github.com/rd235/vdeplug4.git
This projects goal is to be able to provide windows vde support which may not end up being fully complete due to complexities with porting

The goal is to be able to connect a tap device, usermode protocol driver or microsoft loopback device to the vde switch for use with kathara network simulation
The current plugin which ties into docker is at: https://github.com/KatharaFramework/NetworkPlugin/ and is linux only
A rewrite of this plugin to windows will also be necessary but only once a switch device can be created and deployed

Current Goals:

* Implement and clean up vde_switch files -> move t winvde_switch
    - move non vde_switch functions into separate header files and accompanying c files
    - move defines into appropriate header files
    - move global variables into appropriate header files as extern and c files as storage variable
* Implement getopt and getopt_long linux functions ... Done!
* Implement windows log level constants to linux log level constants mapping ... Done!
* Change all int socket file descriptors to SOCKET
* Change all _read and _write to send and recv functions where SOCKET is used
* Implement stat functions supporting links and get_real_path to find the actual link path
  - readlink, custom struct to override 64 bit windows stat,
* Implement group and user id functions
  - make a user id lookup and group id lookup ensuring that administrator is 0 and administrators group is 0
  - make a user and group listing function so that users of the program may look up the information
  - the user and group lists may need to be cached to be able to list the snapshot of when the lists were captured in case changes occur
* Implement modules
* Implement plugins
* Implement mod_support
* Implement Memory Stream - initial
* Implement DoIt function

NOTE: there are issues with mixing int file descriptor and socket and now memory stream
this will be an issue when calling functions that need to handle all three ( console output, socket output, memory stream output)
a union struct combination may be able to handle this situation with minimal overhead.
