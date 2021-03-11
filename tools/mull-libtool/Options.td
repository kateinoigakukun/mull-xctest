include "llvm/Option/OptParser.td"

def o : Separate<["-"], "o">;
def arch_only : Separate<["-"], "arch_only">;
def static : Flag<["-"], "static">;
def dynamic : Flag<["-"], "dynamic">;
def D : Flag<["-"], "D">;
def U : Flag<["-"], "U">;
def filelist : Separate<["-"], "filelist">;
def l : Joined<["-"], "l">;
def L : Joined<["-"], "L">;
def framework : Separate<["-"], "framework">;
def V : Flag<["-"], "V">;
def no_warning_for_no_symbols : Flag<["-"], "no_warning_for_no_symbols">;
def dependency_info : Separate<["-"], "dependency_info">;
def syslibroot: Separate<["-"], "syslibroot">;
def install_name: Separate<["-"], "install_name">;
def compatibility_version: Separate<["-"], "compatibility_version">;
def current_version: Separate<["-"], "current_version">;
def noall_load: Separate<["-"], "noall_load">;
