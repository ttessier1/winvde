# ToDo
Complete Initial Project

This project is currently not complete and is based heavily on virtual square vde-2 GitHub - virtualsquare/vde-2 as a direct port to windows but is not a fork
This project also includes code from vdeplug4 https://github.com/rd235/vdeplug4.git
This projects goal is to be able to provide windows vde support which may not end up being fully complete due to complexities with porting

The goal is to be able to connect a tap device, usermode protocol driver or microsoft loopback device to the vde switch for use with kathara network simulation
The current plugin which ties into docker is at: https://github.com/KatharaFramework/NetworkPlugin/ and is linux only
A rewrite of this plugin to windows will also be necessary but only once a switch device can be created and deployed
