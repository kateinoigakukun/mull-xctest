include "llvm/Option/OptParser.td"

def o : Separate<["-"], "o">;
def arch_only : Separate<["-"], "arch_only">;
def static : Flag<["-"], "static">;
def D : Flag<["-"], "D">;
def U : Flag<["-"], "U">;
def filelist : Separate<["-"], "filelist">;
def l : JoinedOrSeparate<["-"], "l">;
def L : JoinedOrSeparate<["-"], "L">;
def V : Flag<["-"], "V">;
def no_warning_for_no_symbols : Flag<["-"], "no_warning_for_no_symbols">;
