from re import M
import pack_v4


class Project(pack_v4.Project):
    def generate(self):
        self.prefix = "mptest"
        self.version = "0.0.1"
        self.bits = [
            "exports", 
            "cstd", 
            {"name": "hook_malloc", "requires": ["USE_DYN_ALLOC"]},
            "hook_longjmp", "hook_assert", "inttypes", 
            {"name": "ds_string", "requires": ["USE_DYN_ALLOC", "USE_SYM"]},
            {"name": "ds_string_view", "requires": ["USE_DYN_ALLOC", "USE_SYM"]},
            {"name": "ds_vector", "requires": ["USE_DYN_ALLOC", "USE_SYM"]}]
        self.apis = ["mptest_api.h"]
        self.headers = ["mptest_internal.h"]
        self.sources = [
            "mptest_aparse.c",
            "mptest_fuzz.c",
            "mptest_leakcheck.c",
            "mptest_longjmp.c",
            "mptest_state.c",
            "mptest_sym.c",
            "mptest_time.c"
        ]
        self.embeds = ["aparse"]
        self.cstd = "c89"
        self.config = {
            "USE_DYN_ALLOC": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_DYN_ALLOC} to 1 in order to allow the usage "
                    "of malloc() and free() inside mptest. Mptest can run "
                    "without these functions, but certain features are "
                    "disabled."
                ),
                "default": 1
            },
            "USE_LONGJMP": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_LONGJMP} to 1 in order to use runtime assert checking "
                    "facilities. Must be enabled to use heap profiling."
                ),
                "default": 1
            },
            "USE_SYM": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_SYM} to 1 in order to use s-expression "
                    "data structures for testing. {cfg:USE_DYN_ALLOC} must be "
                    "set. "
                ),
                "default": 1
            },
            "USE_LEAKCHECK": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_LEAKCHECK} to 1 in order to use runtime leak "
                    "checking facilities. {cfg:USE_LONGJMP} must be set."
                ),
                "default": 1,
                "requires": ["USE_LONGJMP", "USE_DYN_ALLOC"]
            },
            "USE_COLOR": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_COLOR} to 1 if you want ANSI-colored output."
                ),
                "default": 1
            },
            "USE_TIME": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_TIME} to 1 if you want to time tests or suites."
                ),
                "default": 1
            },
            "USE_APARSE": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_APARSE} to 1 if you want to build a command line "
                    "version of the test program."
                ),
                "default": 0,
                "requires": ["USE_DYN_ALLOC"]
            },
            "USE_FUZZ": {
                "type": bool,
                "help": (
                    "Set {cfg:USE_FUZZ} to 1 if you want to include fuzzing and "
                    "test shuffling support."
                ),
                "default": 1
            }
        }
