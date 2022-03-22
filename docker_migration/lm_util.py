#!usr/bin
#encoding: utf-8


import random
import string
import os.path
import logging
import subprocess as sp
import json
import re
import time
base_dir = '/var/lib/docker'
PORT = 10020
MAX_MSG_LENGTH = 1024
INIT = 0
DUMPED = 1
TARED = 2
SENDED = 3
RESTORED = 4
state_ofcontainer = {}
time_record = {}
cgroup_log = '/var/lib/docker/cgroup.log'
mount_log = '/var/lib/docker/mount.log'
crit_bin = '/home/hdq/criu/crit/crit'
cgroup_img = '/cgroup.img'
mount_img = '/mountpoints-12.img'


def setstate_ofcontainer(container_name, value):
    state_ofcontainer[container_name] = value


def getstate_ofcontainer(container_name):
    return state_ofcontainer[container_name]


def settime(taskid):
    stime = time.time()
    if taskid not in time_record.keys():
        time_record[taskid] = []
    time_record[taskid].append(stime)


def gettime():
    return time_record


def rate_translate(rate_str):
    '''read str(KB/S,mbps...). return bps'''
    raw_rate_str = rate_str
    rate_str = rate_str.lower()
    index = rate_str.find('b')
    if index == -1:
        return False
    rate_str = list(rate_str)
    rate_str[index] = raw_rate_str[index]
    rate_str = ''.join(rate_str)
    print(rate_str)
    rate = re.findall(r'[0-9]+|[a-zA-Z/]+', rate_str)
    print(rate)
    if len(rate) != 2:
        return False
    rate_list = {'b/s': 1, 'bps': 1, 'B/s': 8,
                 'kb/s': 1024, 'kbps': 1000, 'kB/s': 1024*8,
                 'mb/s': 1024*1024, 'mbps': 1000000, 'mB/s': 1024*1024*8,
                 'gb/s': 1024*1024*1024, 'gbps': 1000000000, 'gB/s': 1024*1024*1024*8, }
    if rate[1] in rate_list:
        return int(rate[0]) * rate_list[rate[1]]
    else:
        return False


def isBlank(inString):
    if inString and inString.strip():
        return False
    return True


def exec_shell(cmdstr, smsg):
    '''exec cmd in shell'''
    logging.debug('%s: EXEC: %s' % (smsg, cmdstr))
    msg = sp.call(cmdstr, shell=True)
    if msg:
        logging.error('Error: exec failed')
        return False
    logging.debug('%s: EXEC finish' % smsg)
    return True


def check_dir(file_path):
    if os.path.exists(file_path):
        return True
    else:
        return False


def check_file(file):
    if os.path.isfile(file):
        return True
    else:
        return False


def random_str(size=6, chars=string.ascii_lowercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))


def dict_tobytes(sdict):

    jresp = json.dumps(sdict)
    return jresp.encode('utf-8')


def bytes_todict(sbytes):

    jresp = json.loads(sbytes.decode('utf-8'))
    return jresp


def get_dir_real_size(path):
    '''get size of all files in dir'''
    size = 0.0
    for root, dirs, files in os.walk(path):
        size += sum([os.path.getsize(os.path.join(root, file))
                    for file in files])
    return size
