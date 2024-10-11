import ctypes
from ctypes import Structure, c_uint32, c_uint16, c_uint8, c_void_p, c_char_p, POINTER, CFUNCTYPE

class NFResult:
    NF_MISS     = 0
    NF_MATCH	= 1

class NFInetHook:
    NF_INET_PRE_ROUTING     = 0
    NF_INET_LOCAL_IN        = 1
    NF_INET_FORWARD         = 2
    NF_INET_LOCAL_OUT       = 3
    NF_INET_POST_ROUTING    = 4
    NF_INET_NUMHOOKS        = 5

class NFProto:
    NFPROTO_UNSPEC      = 0
    NFPROTO_INET        = 1
    NFPROTO_IPV4        = 2
    NFPROTO_ARP         = 3
    NFPROTO_NETDEV      = 5
    NFPROTO_BRIDGE      = 7
    NFPROTO_IPV6        = 10
    NFPROTO_DECNET      = 12
    NFPROTO_NUMPROTO    = 13

# Define the Ethernet header structure
class EthHdr(Structure):
    _fields_ = [
        ("h_dest", c_uint8 * 6),
        ("h_source", c_uint8 * 6),
        ("h_proto", c_uint16)
    ]

# Define the IP header structure
class IpHdr(Structure):
    _fields_ = [
        ("ihl", c_uint8, 4),
        ("version", c_uint8, 4),
        ("tos", c_uint8),
        ("tot_len", c_uint16),
        ("id", c_uint16),
        ("frag_off", c_uint16),
        ("ttl", c_uint8),
        ("protocol", c_uint8),
        ("check", c_uint16),
        ("saddr", c_uint32),
        ("daddr", c_uint32)
    ]

# Define the UDP header structure
class UdpHdr(Structure):
    _fields_ = [
        ("source", c_uint16),
        ("dest", c_uint16),
        ("len", c_uint16),
        ("check", c_uint16)
    ]

class SkBuff(Structure):
    _fields_ = [
        ("list", c_void_p),
        ("sk", c_void_p),
        ("m", c_void_p),
        ("cb", ctypes.c_char * 64),
        ("len", ctypes.c_int),
        ("buf", c_void_p)  # Placeholder for the flexible array member
    ]

class Net(Structure):
    _fields_ = [
        ("list", c_void_p),
        ("nf", c_void_p)
    ]

class NetHookPythonOps(Structure):
    _fields_ = [
        ("hook", c_char_p),
        ("dev", c_void_p),
        ("priv", c_void_p),
        ("cond", c_char_p),
        ("pf", ctypes.c_uint8),
        ("hooknum", ctypes.c_uint32),
        ("priority", ctypes.c_int)
    ]

class LibNF:
    def __init__(self, lib_path='/home/ubuntu/Nutcracker/lib/build/lib/libnftnl.so', path=__file__):
        # Load the shared library
        self.lib = ctypes.CDLL(lib_path)
        self.path = path
        self.lib.nf_register_net_hook_python.argtypes = [c_char_p, POINTER(Net), POINTER(NetHookPythonOps)]
        self.lib.nf_register_net_hook_python.restype = ctypes.c_int

    def nf_register_net_hook_python(self, net, ops):
        # Convert name to bytes and call the new function
        path_bytes = self.path.encode('utf-8')
        return self.lib.nf_register_net_hook_python(path_bytes, ctypes.byref(net), ctypes.byref(ops))


# Example usage:
if __name__ == "__main__":
    lib = LibNF()
    net = Net()
    nf_hook_ops = NetHookOps()

    # Define names for the functions
    hook_func_name = "hook_function_name"
    cond_func_name = "cond_function_name"

    # Initialize nf_hook_ops fields with appropriate values
    nf_hook_ops.hook = hook_func_name.encode('utf-8')  # Store the function name as a byte string
    nf_hook_ops.dev = c_void_p()  # Set appropriately
    nf_hook_ops.priv = c_void_p()  # Set appropriately
    nf_hook_ops.cond = cond_func_name.encode('utf-8')  # Store the function name as a byte string
    nf_hook_ops.pf = NFProto.NFPROTO_INET
    nf_hook_ops.hooknum = NFInetHook.NF_INET_PRE_ROUTING
    nf_hook_ops.priority = 0

    result = lib.nf_register_net_hook_python("example_script.py", net, nf_hook_ops)
    print(f"Result: {result}")