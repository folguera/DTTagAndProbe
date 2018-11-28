import ROOT as r
from subprocess import call
import copy
# ===========================================
print "> Beginning the obtention of trigger primitives efficiencies"
# === Preliminaries
r.gROOT.SetBatch(True)


# === Importing C++ macros
print "> Importing macros..."
r.gROOT.LoadMacro("./DTAnalyzer.C+")
r.gROOT.LoadMacro("./DTTnPConfig.C+")
r.gROOT.LoadMacro("./DTTnPBaseAnalysis.C+")
r.gROOT.LoadMacro("./DTTnPLocaltrigEff.C+")
r.gROOT.LoadMacro("./DTTnPLocalTrigRes.C+");


# === Executing things that I do not understand yet
TnP = copy.deepcopy(r.DTTnPLocalTrigRes("config/config.ini"))
TnP.Loop()


print "> Execution finished!"
print "> Let's plot things now!"
call("plotter.py", "config/configPlotter_L1T_uvieu.json")