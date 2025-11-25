__AUTHOR__ = "7h0m4s"
__VERSION__ = 0.2

import gdb

@register
class SessCommand(GenericCommand):
    _cmdline_ = "sess"
    _syntax_ = f"{_cmdline_:s}"

    def __init__(self):
        super().__init__(complete=gdb.COMPLETE_SYMBOL)
        return

    @only_if_gdb_running         # not required, ensures that the debug session is started
    def do_invoke(self, _):
        try:
            # 1. Use gdb.parse_and_eval() to get the variable
            # This returns a 'gdb.Value' object
            curr_sess = gdb.parse_and_eval("g_current_session")

        except gdb.error as e:
            # This will fail if the variable doesn't exist (e.g., no symbols)
            print(f"Error: Could not find global variable 'g_current_session'.")
            print(f"GDB error: {e}")
            return
        
        # 2. Check if the variable is a pointer
        if curr_sess.type.code == gdb.TYPE_CODE_PTR:
            
            # Check if it's a NULL pointer
            if curr_sess == 0:
                print("'g_current_session' is a NULL pointer. Is not initialized yet.")
                return
            
            # 3. If it's a valid pointer, dereference it to get the content
            try:
                print(f"--- Session ftp_cmd_str_queue_node list ---")
                ftp_cmd_str_queue_node = curr_sess.dereference()["q_head"]
                while ftp_cmd_str_queue_node != 0:
                    self.parse_ftp_cmd_str_queue_node(ftp_cmd_str_queue_node)
                    ftp_cmd_str_queue_node = ftp_cmd_str_queue_node.dereference()["next"]
            except gdb.error as e:
                # This happens if the pointer is invalid (e.g., points to bad memory)
                print(f"Error parsing 'ftp_cmd_str_queue_node' (address {ftp_cmd_str_queue_node}).")
                print(f"GDB error: {e}")

            try:
                print(f"--- Session active_cmd_node list ---")
                active_cmd_node = curr_sess.dereference()["active_cmds"]
                while active_cmd_node != 0:
                    self.parse_active_cmd_node(active_cmd_node)
                    active_cmd_node = active_cmd_node.dereference()["next"]
            except gdb.error as e:
                # This happens if the pointer is invalid (e.g., points to bad memory)
                print(f"Error parsing 'active_cmd_node' (address {active_cmd_node}).")
                print(f"GDB error: {e}")

    def parse_active_cmd_node(self, active_cmd_node):
        cmd = active_cmd_node.dereference()["cmd"]
        raw = cmd.dereference()["raw"]
        deinit_func = cmd.dereference()["deinit_func"]
        print(f"{active_cmd_node}:{cmd}:{raw.string()}\t{deinit_func}")

    def parse_ftp_cmd_str_queue_node(self, ftp_cmd_node):
        raw = ftp_cmd_node.dereference()["cmd_str"]
        print(f"{ftp_cmd_node}:{raw.string()}")
        return