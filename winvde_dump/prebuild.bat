cd
echo Setting Path to ..\winvde_switch\x64\%1
pushd ..\winvde_switch\x64\%1
dir /b *.obj
lib /subsystem:windows /out:winvde_plugin.lib version.obj winvde_comlist.obj winvde_common.obj winvde_datasock.obj winvde_debugcl.obj winvde_debugopt.obj winvde_descriptor.obj winvde_ev.obj winvde_event.obj winvde_gc.obj winvde_getopt.obj winvde_hash.obj winvde_memorystream.obj winvde_mgmt.obj winvde_modsupport.obj winvde_module.obj winvde_output.obj winvde_packetq.obj winvde_plugin.obj winvde_port.obj winvde_printfunc.obj winvde_qtimer.obj winvde_sockutils.obj winvde_switch.obj winvde_tuntap.obj winvde_type.obj winvde_user.obj
copy winvde_plugin.lib ..\..\..\x64\%1
popd

exit /b 0