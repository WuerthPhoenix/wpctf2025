__AUTHOR__ = "7h0m4s"
__VERSION__ = 0.2

# # Import the necessary GEF components
# from gef.api import GefCommand
# from gef.api.helpers import register, only_if_gdb_running
import gdb

@register
class WppoolCommand(GenericCommand):
    _cmdline_ = "wppool"
    _syntax_ = f"{_cmdline_:s}"

    def __init__(self):
        super().__init__(complete=gdb.COMPLETE_SYMBOL)
        return

    @only_if_gdb_running         # not required, ensures that the debug session is started
    def do_invoke(self, _):
        try:
            # 1. Use gdb.parse_and_eval() to get the variable
            # This returns a 'gdb.Value' object
            main_pool_val = gdb.parse_and_eval("main_pool")

        except gdb.error as e:
            # This will fail if the variable doesn't exist (e.g., no symbols)
            print(f"Error: Could not find global variable 'main_pool'.")
            print(f"GDB error: {e}")
            return
        
        # 2. Check if the variable is a pointer
        if main_pool_val.type.code == gdb.TYPE_CODE_PTR:
            
            # Check if it's a NULL pointer
            if main_pool_val == 0:
                print("'main_pool' is a NULL pointer. Is not initialized yet.")
                return
            
            # 3. If it's a valid pointer, dereference it to get the content
            try:
                # add 0x8 to the address of main_pool_val to get to the first wp_chunk
                wp_chunk = main_pool_val.dereference()["first"]
                
                while wp_chunk != 0:
                    self.parse_chunk(wp_chunk)
                    wp_chunk = wp_chunk.dereference()["next"]
                    

            except gdb.error as e:
                # This happens if the pointer is invalid (e.g., points to bad memory)
                print(f"Error parsing 'wp_chunk' (address {wp_chunk}).")
                print(f"GDB error: {e}")
        
        else:
            # 3. If it's not a pointer (e.g., 'struct Pool main_pool;'), just print it
            print(f"Content of main_pool:\n{main_pool_val}")
        
        return
        print(hexdump( gef.memory.read(parse_address("$pc"), length=0x20 )))


    def is_ascii(s):
        return all(ord(c) < 128 for c in s)




    def parse_chunk(self, wp_chunk):
        chunk_size = int( wp_chunk.dereference()["size"] )
        print_size = min(chunk_size, 64)
        chunk_data = wp_chunk.dereference()["data"]
        data = gef.memory.read(int(chunk_data), length=print_size)
        chunk_is_free = Color.redify(Color.blinkify('True')) if wp_chunk.dereference()["is_free"] else False
        title = Color.cyanify(Color.boldify("wp_chunk"))
        print(f"\n{title} at {wp_chunk} - Size: {chunk_size} - IsFree: {chunk_is_free} - Data ptr: {chunk_data}")
        if self.is_ftp_cmd_str_queue_node(data, chunk_data):
            # likely an ftp_cmd_queue_node
            self.parse_ftp_cmd_queue_node( gdb.Value(int(chunk_data)).cast( gdb.lookup_type("struct ftp_cmd_str_queue_node").pointer() ) )
        # elif int(data[0]) < 12 and all(data[1:][i] == 0x0 for i in range(1,8)):
        #     # likely a ftp_cmd
        #     self.parse_ftp_cmd( gdb.Value(int(chunk_data)).cast( gdb.lookup_type("struct ftp_cmd").pointer() ) )
        #elif chunk_data.string()[:4].isupper():
        elif self.is_cmd_str(data):
            # likely a cmd_str
            if chunk_data[4] == 0x20:
                # likely not yet parsed
                self.parse_cmd_str(chunk_data)
            elif chunk_data[4] == 0x00:
                # likely parsed already
                self.parse_cmd_str_parsed(chunk_data)
        else:
            print(hexdump( gef.memory.read(int(chunk_data), length=print_size )))
        

    def is_ftp_cmd_str_queue_node(self, data, chunk_data):
        if data[6] == 0x0 and data[7] == 0x0 and data[14] == 0x0 and data[15] == 0x0:
            try:
                v = gdb.Value(int(chunk_data)).cast( gdb.lookup_type("struct ftp_cmd_str_queue_node").pointer() )
                data = gef.memory.read(int(v.dereference()["cmd_str"]), length=5)
                if self.is_cmd_str(data):
                    next = v.dereference()["next"]
                    if next == 0 or self.is_ftp_cmd_str_queue_node( bytes( gef.memory.read(int(next), length=16 ) ), next ):
                        return True
            except gdb.error:
                print(f"Not a valid ftp_cmd_str_queue_node.")
        return False


    def is_cmd_str(self, data):
        if not all(data[i] > 0 for i in range(4)):
            return False
        try:
            cmd = data.decode('ascii')
        except UnicodeDecodeError:
            return False
        # print(type(data))
        # print(data)
        # print(chr(data[0]).isupper())
        if all(ord(chr(c)) < 128 for c in data[:4]) and chr(data[0]).isupper() and chr(data[1]).isupper() and chr(data[2]).isupper() and chr(data[3]).isupper():
            return True
        return False
    

    def parse_cmd_str_parsed(self, data):
        name = Color.boldify("cmd_str (parsed)")
        print(f"\t{name}: command:{data.string()[:4]} args:redacted")

    def parse_cmd_str(self, data):
        name = Color.boldify("cmd_str")
        print(f"\t{name}: {data.string()}")

    def parse_ftp_cmd_queue_node(self, node_ptr):
        cmd_str_ptr = node_ptr.dereference()["cmd_str"]
        next_node_ptr = node_ptr.dereference()["next"]
        name = Color.boldify("ftp_cmd_str_queue_node")
        print(f"\t{name} - CmdStr ptr: {cmd_str_ptr} - Next ptr: {next_node_ptr}")

    def parse_ftp_cmd(self, cmd_str_ptr):
        cmd_str = cmd_str_ptr['type']
        raw_str_ptr = cmd_str_ptr['raw']
        arg_str_ptr = cmd_str_ptr['arg']
        # get string from pointer
        arg_str = arg_str_ptr.string()
        name = Color.boldify("ftp_cmd")
        print(f"\t{name}: {cmd_str} raw: {raw_str_ptr} args: {arg_str}")

