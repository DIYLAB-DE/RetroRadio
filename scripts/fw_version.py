# Erzeugt FW_VERSION und FW_DATE bei jedem Build neu aus dem aktuellen Datum.
# FW_VERSION: v<JJ>.<M>.<T>   zweistelliges Jahr, Monat und Tag ohne führende Null
# FW_DATE:    <TT>.<MM>.<JJJJ>

Import("env")
from datetime import datetime

now = datetime.now()

fw_version = "v{:02d}.{}.{}".format(now.year % 100, now.month, now.day)
fw_date = "{:02d}.{:02d}.{:04d}".format(now.day, now.month, now.year)

env.Append(CPPDEFINES=[
    ("FW_VERSION", env.StringifyMacro(fw_version)),
    ("FW_DATE", env.StringifyMacro(fw_date)),
])

print("FW_VERSION = {}".format(fw_version))
print("FW_DATE    = {}".format(fw_date))