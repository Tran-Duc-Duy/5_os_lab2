import json
import os
import sys
import time
from collections import namedtuple

Mydstat = namedtuple("Mydstat", ["cpu", "dsk", "net", "paging", "system"])

def main():
    args = sys.argv[1:]

    if len(args) < 1:
        sys.exit("format arg not found, format can be --classic or --json")

    format = args[0]
    interval = float(args[1])
    count = int(args[2])

    stat = fill_struct()

    if format == "--classic":
        process_classic_format(stat)
    elif format == "--json":
        process_json_format(stat)
    else:
        sys.exit("invalid format specified, can be --classic or --json")

    if interval == -1:
        sys.exit(0)

    if count >= 1:
        for _ in range(count - 1):
            time.sleep(interval)
            stat = fill_struct()

            if format == "--classic":
                process_classic_format(stat)
            elif format == "--json":
                process_json_format(stat)
            else:
                sys.exit("Invalid format specified, can be --classic or --json")

def fill_struct():
    try:
        with open("/sys/kernel/debug/mydstat/info", "r") as file:
            data = file.read()
    except IOError as e:
        sys.exit(f"failed to read file: {e}, try to run tool with sudo")

    stat = Mydstat(cpu={}, dsk={}, net={}, paging={}, system={})
    lines = data.strip().split("\n")

    for line in lines:
        line = line.strip()
        if line == "------" or line == "":
            continue

        name, value = line.split(":")
        name = name.strip()
        value = int(value.strip())

        if name in ["usr", "sys", "idl", "wai", "stl"]:
            stat.cpu[name] = value
        elif name in ["read", "writ"]:
            stat.dsk[name] = value
        elif name in ["recv", "send"]:
            stat.net[name] = value
        elif name in ["in", "out"]:
            stat.paging[name] = value
        elif name in ["intr", "csw"]:
            stat.system[name] = value

    return stat

def process_classic_format(stat):
    header ="--total-cpu-usage--\t--dsk/total--\t--net/total--\t---paging--\t-system--"
    line1 = "usr sys idl wai stl\t  read   writ\t  recv   send\t   in  out \tint csw"

    cpu_line = "{usr:3d} {sys:3d} {idl:3d} {wai:3d} {stl:3d}".format(
        usr=stat.cpu.get("usr", 0),
        sys=stat.cpu.get("sys", 0),
        idl=stat.cpu.get("idl", 0),
        wai=stat.cpu.get("wai", 0),
        stl=stat.cpu.get("stl", 0)
    )

    dsk_line = "{read:6d} {writ:6d}".format(
        read=stat.dsk.get("read", 0),
        writ=stat.dsk.get("free", 0)
    )

    net_line = "{recv:6d} {send:6d}".format(
        recv=stat.net.get("recv", 0),
        send=stat.net.get("send", 0)
    )

    paging_line = "{_in:4d} {out:4d}".format(
        _in=stat.paging.get("in", 0),
        out=stat.paging.get("out", 0)
    )

    system_line = "{intr:3d} {csw:3d}".format(
        intr=stat.system.get("intr", 0),
        csw=stat.system.get("csw", 0)
    )

    print(header)
    print(line1)
    print(cpu_line + "\t" + dsk_line + "\t" + net_line + "\t" + paging_line + "\t" + system_line)

def process_json_format(stat):
    json_data = json.dumps(stat._asdict(), indent=2)
    print(json_data)

if __name__ == "__main__":
    main()