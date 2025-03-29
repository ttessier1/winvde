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
# * Implement getopt and getopt_long linux functions - Done preliminary!
# * Implement windows log level constants to linux log level constants mapping - Done preliminary!
# * Change all int socket file descriptors to SOCKET - Done preliminary
# * Change all _read and _write to send and recv functions where SOCKET is used - Done preliminary
# * Implement stat functions supporting links and get_real_path to find the actual link path
#   - readlink, custom struct to override 64 bit windows stat, - Done preliminary
# * Implement group and user id functions
#  - make a user id lookup and group id lookup ensuring that administrator is 0 and administrators group is 0 - Done preliminary
#   - make a user and group listing function so that users of the program may look up the information - Done preliminary
#   - the user and group lists may need to be cached to be able to list the snapshot of when the lists were captured in case changes occur - not done
# * Implement readv and writev
#     - readv and writev needs to simulate the linux functions - Done preliminary
# * Implement modules - Done preliminary
* Implement plugins
# * Implement mod_support - Done prelminary
# * Implement Memory Stream - Done preliminary
# * Implement DoIt function - Done preliminary
# * Fix artifacts in the console routines to clear line before output or otherwise clear the screen before output - Done preliminary
# * Fix backspace to work properly and not allow backspace past prompt - Done preliminary
# * Fix up/down characters to traverse history - Done preliminary
# * Todo Implement plugin dump - Done preliminary
* Todo Implement plugin iplog
* Todo Implement plugin pdump
* Todo Implement module Tap

NOTE: initial working management connecion is possible now but buggy 2025-03-27
The problem is that some of the details from linux use a memory stream with a file pointer,
so all this needs to be converted to use the memorystream that we have implemented until a
group of stream functions can be made to work with socket, file, memorystream with its own
handle - open, close, write, read, printf, vprint, scanf functions would need to be made to support all of these.

* Implement memory stream functions for mgmt functions
Management functions are as follows:
ds                 ============    DATA SOCKET MENU
ds/showinfo                        show ds info
help               [arg]           Help (limited to arg when specified)
logout                             logout from this mgmt terminal
shutdown                           shutdown of the switch
showinfo                           show switch version and info
load               path            load a configuration script
debug              ============    DEBUG MENU
debug/list                         list debug categories
debug/add          dbgpath         enable debug info for a given category
debug/del          dbgpath         disable debug info for a given category
plugin             ============    PLUGINS MENU
plugin/list                        list plugins
plugin/add         library         load a plugin
plugin/del         name            unload a plugin
hash               ============    HASH TABLE MENU
hash/showinfo                      show hash info
hash/setsize       N               change hash size
hash/setgcint      N               change garbage collector interval
hash/setexpire     N               change hash entries expire time
hash/setminper     N               minimum persistence time
hash/print                         print the hash table
hash/find          MAC [VLAN]      MAC lookup
port               ============    PORT STATUS MENU
port/showinfo                      show port info
port/setnumports   N               set the number of ports
port/sethub        0/1             1=HUB 0=switch
port/setvlan       N VLAN          set port VLAN (untagged)
port/createauto                    create a port with an automatically allocated id (inactive|notallocatable)
port/create        N               create the port N (inactive|notallocatable)
port/remove        N               remove the port N
port/allocatable   N 0/1           Is the port allocatable as unnamed? 1=Y 0=N
port/setuser       N user          access control: set user
port/setgroup      N user          access control: set group
port/epclose       N ID            remove the endpoint port N/id ID
port/defqlen       LEN             set the default queue length for new ports
port/epqlen        N ID LEN        set the lenth of the queue for port N/id IP
port/print         [N]             print the port/endpoint table
port/allprint      [N]             print the port/endpoint table (including inactive port)
vlan               ============    VLAN MANAGEMENT MENU
vlan/create        N               create the VLAN with tag N
vlan/remove        N               remove the VLAN with tag N
vlan/addport       N PORT          add port to the vlan N (tagged)
vlan/delport       N PORT          delete port from the vlan N (tagged)
vlan/print         [N]             print the list of defined vlan
vlan/allprint      [N]             print the list of defined vlan (including inactive port)

Test from inside the root winvde folder with
# Server - start a 16 port hub with hash size of 32 and management socket at C:\windows\temp\mgmt.sock and data sockets at C:\windows\temp\data
x64\Debug\winvde_switch.exe --numports 16 --hub --hashsize 32 -M C:\windows\temp\mgmt.sock -s C:\windows\temp\data
# Client
x64\debug\winvde_term.exe c:\windows\temp\mgmt.sock

If success - vde[c:\windows\temp\mgmt.sock]: is the prompt and the "help"  returns the above list of management functions, which is also retreived on first run
along with 

WINVDE switch V.2.3.3
(C) Based on Virtual Square Team (coord. R. Davoli) 2005,2006,2007 - GPLv2
WinPort started Tim Tessier

NOTE: Socket connecting error: No error 10061 if server not running or otherwise not permitted

# Server rc file can be in %USERPROFILE%\.winvde2\winvde_switch.rc    
#Sample RC file
port/create 1
port/create 2
port/create 3
port/create 4
port/create 5
port/create 6
port/create 7
port/create 8
port/create 9
port/create 10
port/create 11
port/create 12
port/create 13
port/create 14
port/create 15

This will at least make is so that when you run port/allprint 
something similar to the following should return unless something is terribly wrong

Port 0001 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0002 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0003 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0004 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0005 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0006 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0007 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0008 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0009 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0010 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0011 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0012 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0013 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0014 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Port 0015 untagged_vlan=0000 INACTIVE - NOT Unnamed Allocatable
 Current User: NONE Access Control: (User: NONE - Group: NONE)
Success

NOTE: there are issues with mixing int file descriptor and socket and now memory stream
this will be an issue when calling functions that need to handle all three ( console output, socket output, memory stream output)
a union struct combination may be able to handle this situation with minimal overhead.

Commands 
# ds - currently not tied to a resource function

returns 
No Error


# ds/showinfo - currently tied to datasocket showinfo function and returns

returns
No Error
ctl dir \windows\temp\data      ds/showinfo
std mode 0600
Success

# help - currently tied to help function and returns

returns 
ds                 ============    DATA SOCKET MENU
ds/showinfo                        show ds info
help               [arg]           Help (limited to arg when specified)
logout                             logout from this mgmt terminal
shutdown                           shutdown of the switch
showinfo                           show switch version and info
load               path            load a configuration script
debug              ============    DEBUG MENU
debug/list                         list debug categories
debug/add          dbgpath         enable debug info for a given category
debug/del          dbgpath         disable debug info for a given category
plugin             ============    PLUGINS MENU
plugin/list                        list plugins
plugin/add         library         load a plugin
plugin/del         name            unload a plugin
hash               ============    HASH TABLE MENU
hash/showinfo                      show hash info
hash/setsize       N               change hash size
hash/setgcint      N               change garbage collector interval
hash/setexpire     N               change hash entries expire time
hash/setminper     N               minimum persistence time
hash/print                         print the hash table
hash/find          MAC [VLAN]      MAC lookup
port               ============    PORT STATUS MENU
port/showinfo                      show port info
port/setnumports   N               set the number of ports
port/sethub        0/1             1=HUB 0=switch
port/setvlan       N VLAN          set port VLAN (untagged)
port/createauto                    create a port with an automatically allocated id (inactive|notallocatable)
port/create        N               create the port N (inactive|notallocatable)
port/remove        N               remove the port N
port/allocatable   N 0/1           Is the port allocatable as unnamed? 1=Y 0=N
port/setuser       N user          access control: set user
port/setgroup      N user          access control: set group
port/epclose       N ID            remove the endpoint port N/id ID
port/defqlen       LEN             set the default queue length for new ports
port/epqlen        N ID LEN        set the lenth of the queue for port N/id IP
port/print         [N]             print the port/endpoint table
port/allprint      [N]             print the port/endpoint table (including inactive port)
vlan               ============    VLAN MANAGEMENT MENU
vlan/create        N               create the VLAN with tag N
vlan/remove        N               remove the VLAN with tag N
vlan/addport       N PORT          add port to the vlan N (tagged)
vlan/delport       N PORT          delete port from the vlan N (tagged)
vlan/print         [N]             print the list of defined vlan
vlan/allprint      [N]             print the list of defined vlan (including inactive port)

# logout - currently tied to mgmt vde_logout

returns 
No Error and
closes the management connection

# shutdown - currently tied to mgmt vde_shutdown

returns 
No Error and
closes the management connection after shutting down the switch

# showinfo - currently tied to mgmt showinfo

returns
WINVDE switch V.2.3.3           showinfo
(C) Based on Virtual Square Team (coord. R. Davoli) 2005,2006,2007 - GPLv2
WinPort started Tim Tessier
pid 18856 MAC 00:ff:94:4c:2c:d9 uptime 471
mgmt C:\windows\temp\mgmt.sock perm 0600
Success

# load - To Be Implemented - NOTE: should at least return not implemented

# debug - currently not tied to a function

returns
No Error

# debug/list - currently tied to debugopt list function

returns
No error                        debug
CATEGORY               TAG STATUS HELPlist
------------           --- ------ ----
hash/+                 011 OFF    hash: new element
hash/-                 012 OFF    hash: discarded element
port/+                 021 OFF    new port
port/-                 022 OFF    closed port
port/descr             023 OFF    set port description
port/ep/+              031 OFF    new endpoint
port/ep/-              032 OFF    closed endpoint
Success

If the debug features are enabled or desabled with debug/add [path] or debug/del [path]
The status field should reflect the changes

# debug/add hash/+ or <path> currently tied to debugopt debugadd function

returns 
Success
No Error

# debug/del hash/+ or <path> currently tied to debugopt debugdel function

returns
Success
No Error

# plugin - currently not tied to a function

returns
No Error

# plugin/list - currently tied to plugin pluginlist function, dependant on compiled plugin dlls ( none compiled )

returns
Success
NAME                   HELP     plugin/list
------------           ----
No error

# plugin/add test.dll or <path>- Not implemented - should return Not Implemented

returns
No Error

# plugin/del test.dll or <path> - Not implemented - should return Not Implemented

returns 
No Error

# hash  - currently not tied to a function

returns
No Error

# TODO:
# hash/showinfo - currently tied to hash showinfo function

returns
Server Closese Connection( Crash or otherwise error close)

# TODO:
# hash/setsize - unknown
# hash/setgcint - unknown
# hash/setexpire - unknown
# hash/setminper  - unknown
# hash/print  - unknown
# hash/find  - unknown
# port  - unknown
# port/showinfo  - unknown
# port/setnumports  - unknown
# port/sethub  - unknown
# port/setvlan  - unknown
# port/createauto - unknown
# port/create - unknown
# port/remove  - unknown
# port/allocatable - unknown
# port/setuser - unknown
# port/setgroup  - unknown
# port/epclose  - unknown
# port/defqlen  - unknown
# port/epqlen  - unknown
# port/print  - unknown
# port/allprint  - unknown
# vlan  - unknown
# vlan/create  - unknown
# vlan/remove  - unknown
# vlan/addport  - unknown
# vlan/delport  - unknown
# vlan/print  - unknown
# vlan/allprint  - unknown

