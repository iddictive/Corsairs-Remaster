# Read-only reproduction of the game counters retained in rusage.json.
import ctypes,sys,time,json,subprocess
class Usage(ctypes.Structure):
    _fields_=[("uuid",ctypes.c_ubyte*16)]+[(name,ctypes.c_uint64) for name in ("user","system","pkg_idle","interrupt","pageins","wired","resident","footprint","start","exit","child_user","child_system","child_idle","child_interrupt","child_pageins","child_elapsed","read","written")]
lib=ctypes.CDLL("/usr/lib/libproc.dylib",use_errno=True)
lib.proc_pid_rusage.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_void_p]
lib.proc_pid_rusage.restype=ctypes.c_int
pid=int(sys.argv[1])
def sample():
    value=Usage()
    if lib.proc_pid_rusage(pid,2,ctypes.byref(value))!=0:raise OSError(ctypes.get_errno(),"proc_pid_rusage")
    return {key:getattr(value,key) for key in ("read","written","pageins","resident","footprint")}
a=sample();start=time.monotonic();vm0=subprocess.check_output(["vm_stat"],text=True);time.sleep(6);b=sample();dt=time.monotonic()-start;vm1=subprocess.check_output(["vm_stat"],text=True)
print(json.dumps({"pid":pid,"seconds":dt,"before":a,"after":b,"read_MiB_s":(b["read"]-a["read"])/dt/1048576,"write_MiB_s":(b["written"]-a["written"])/dt/1048576,"vm_before":vm0,"vm_after":vm1},indent=2))
