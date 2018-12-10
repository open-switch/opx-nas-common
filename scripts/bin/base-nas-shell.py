#!/usr/bin/python
# Copyright (c) 2018 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.

import cps
import cps_utils
import socket
import select
import sys
import time
import systemd.daemon

def _flush(sock):
    while True:
        rds, wrs, xcepts = select.select([sock], [], [], 0.5)
        if len(rds) > 0:
            junk = sock.recv(2048)
            continue
        break


def _get_data(sock, time, prompt):
    ret = ""
    while True:
        rds, wrs, xcepts = select.select([sock], [], [], time)
        if len(rds) > 0:
            ret += sock.recv(2048)
            if prompt is not None and ret.find(prompt) != -1:
                break
            continue
        break
    return ret


def run_shell_cmd(cmd):
    s = ""
    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect('/var/run/ar.npu.shell')

        prompt = ''
        retry = 5
        sock.sendall('\n')

        while len(prompt) == 0 and retry > 0:
            _flush(sock)
            sock.sendall('::prompt on\n')
            prompt = _get_data(sock, 1, ">")
            retry -= 1

        if len(prompt.split('>')) > 2:
            if prompt.find(">") != -1:
                prompt = prompt[:prompt.find(">") + 1]

        if prompt.rfind('\n') != -1:
            prompt[prompt.rfind('\n'):]

        sock.sendall('\n')
        _flush(sock)

        sock.sendall(cmd + '\n')
        s = _get_data(sock, 10, prompt)

    except Exception as bad_exc:
        print bad_exc
        s = ""
    sock.close()
    return s


def get_cb(methods, params):
    return False


def sigterm_hdlr(signum, frame):
    global shutdown
    shutdown = True

def trans_cb(methods, params):
    if params['operation'] == 'rpc':
        cmd = params['change']['data'][
            'base-switch/diag_shell/input/command'].decode("utf-8")
        resp = run_shell_cmd(cmd).encode("utf-8")
        params['change']['data']['base-switch/diag_shell/output/result'] = resp
    else:
        print "Invalid operation requested."

    return True

if __name__ == '__main__':

    global shutdown
    shutdown = False

    # Install signal handlers.
    import signal
    signal.signal(signal.SIGTERM, sigterm_hdlr)

    if len(sys.argv) > 1:
        print run_shell_cmd(sys.argv[1])
        sys.exit(0)

    handle = cps.obj_init()
    d = {}
    d['get'] = get_cb
    d['transaction'] = trans_cb
    key = cps.key_from_name('target', 'base-switch/diag_shell')
    cps.obj_register(handle, key, d)
 
    # Notify systemd: Daemon is ready
    systemd.daemon.notify("READY=1")

    # wait until a signal is received
    while False == shutdown:
        signal.pause()

    systemd.daemon.notify("STOPPING=1")
    # cleanup code here

    # No need to specifically call sys.exit(0).
    # That's the default behavior in Python.
